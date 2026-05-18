# Analysis: `EnsureLockoutForFailedPasswordAttempts` (CIS L1 Server 5.3.2)

Rule identity:

- Title: `Ensure lockout for failed password attempts is configured (CIS: L1 - Server - 5.3.2)`
- ID: `1895704d-a326-90ca-3795-3a1289e7482d`
- Short name: `EnsureLockoutForFailedPasswordAttempts`

This document summarizes how the rule is implemented today, and where it will produce
**false negatives (audit fails on a properly configured system)** or **broken
remediation (remediation makes a working system worse)** on common modern Linux
distributions, both Debian-family and Red Hat-family.

---

## 1. Implementation overview

### 1.0 Scope assumption

OSConfig only runs on AMD64 (x86_64) Linux. Findings in this document are scoped
accordingly — multi-architecture / non-x86_64 concerns (arm64, riscv64, ppc64le, etc.)
are out of scope.

### 1.1 Call graph

`src/common/asb/Asb.c`

- `AuditEnsureLockoutForFailedPasswordAttempts(log)` (line 2080)
  Calls `CheckLockoutForFailedPasswordAttempts(file, pamSo, '#', &reason, log)` for the
  cartesian product of:
  - files: `/etc/pam.d/system-auth`, `/etc/pam.d/password-auth`, `/etc/pam.d/login`
  - PAM modules: `pam_faillock.so`, `pam_tally2.so`, `pam_tally.so`
  using `RETURN_REASON_IF_ZERO` to short-circuit on the first 0 return.
  If none of the 9 calls succeed it **frees the accumulated `reason` and replaces it
  with a hardcoded "modules don't exist" string** before returning (see §2 below for
  why this matters).

- `RemediateEnsureLockoutForFailedPasswordAttempts(value, log)` (line 3736)
  Just calls `SetLockoutForFailedPasswordAttempts(log)`.

`src/common/commonutils/PassUtils.c`

- `CheckLockoutForFailedPasswordAttempts(fileName, pamSo, commentCharacter, reason, log)`
  (line 197). For the given file, reads it line-by-line and considers the rule satisfied if
  it finds **a single uncommented line** containing:
  - the substring `auth`
  - the substring `pamSo`
  - `GetStringOptionFromBuffer(line, "auth", ' ', '#')` returns `"required"`
  - `deny` integer option with `0 <= deny <= 5`
  - `unlock_time` integer option strictly `> 0`

- `SetLockoutForFailedPasswordAttempts(log)` (line 311). For each of
  `/etc/pam.d/login`, `/etc/pam.d/system-auth`, `/etc/pam.d/password-auth`,
  `/etc/pam.d/common-auth` that exists, finds the first available PAM module
  (`pam_faillock.so` then `pam_tally2.so` then `pam_tally.so`+`pam_deny.so`) under a
  hardcoded set of paths (`FindPamModule`, line 20), formats a single line and writes
  it via `ReplaceMarkedLinesInFile(...)` with the bare module name as marker:

  - faillock template:
    `auth required <abs path>/pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root\n`
  - tally2 template:
    `auth required <abs path>/pam_tally2.so file=/var/log/tallylog onerr=fail audit silent deny=5 unlock_time=900 even_deny_root\n`
  - tally + deny template:
    `auth required <abs path>/pam_tally.so onerr=fail deny=3 unlock_time=900\nauth required <abs path>/pam_deny.so\n`

`src/common/commonutils/FileUtils.c`

- `ReplaceMarkedLinesInFile(fileName, marker, newline, '#', preserveAccess, log)` →
  `ReplaceMarkedLinesInFileInternal(...)` (line 960). For each line:
  - if line contains `marker` as a substring and is uncommented and `newline` is
    non-NULL/length > 1: **replace the first match, then delete (skip) every other
    matching line in the file** (lines 1022–1047).
  - if the marker was never found, **append** `newline` at the end of the file
    (lines 1095–1104). Note: prepend mode is never used by this rule.

`FindPamModule` (PassUtils.c line 20) searches the following directories (in order):

```
/usr/lib/x86_64-linux-gnu/security/    # Debian/Ubuntu AMD64 multiarch
/usr/lib/security/                     # historic
/lib/security/                         # historic
/lib64/security/                       # RHEL/CentOS/Mariner (typically /lib64 -> /usr/lib64)
/lib/x86_64-linux-gnu/security/        # older Debian (typically /lib -> /usr/lib)
```

The first match wins. On every supported AMD64 distro (RHEL/CentOS/Rocky/Alma/Fedora,
SUSE, Mariner, Debian, Ubuntu) at least one of these paths resolves to the right
module via the `/lib -> /usr/lib` or `/lib64 -> /usr/lib64` usrmerge symlinks. The
list is missing the direct path `/usr/lib64/security/` — relevant only in unusual
sysroots where the `/lib64` compatibility symlink is absent (minimal containers,
chroots).

---

## 2. How the `reason` string is assembled (and why it matters here)

The reason machinery lives in `src/common/logging/Reasons.h`:

```c
#define InternalOsConfigAddReason(reason, format, ...) {
    char* last = NULL;
    char* temp = FormatAllocateString("%s, also ", *reason);
    FREE_MEMORY(*reason);
    last = FormatAllocateString(format, ##__VA_ARGS__);
    last[0] = tolower(last[0]);
    *reason = ConcatenateStrings(temp, last);
    FREE_MEMORY(temp); FREE_MEMORY(last);
}

#define OsConfigCaptureReason(reason, format, ...) {                                  // FAILURE
    if (NULL != reason) {
        if ((NULL != *reason) && (0 != strncmp(*reason, "PASS", 4))) {
            InternalOsConfigAddReason(reason, format, ##__VA_ARGS__);                 // append
        } else {
            FREE_MEMORY(*reason);
            *reason = FormatAllocateString(format, ##__VA_ARGS__);                    // replace
        }
    }
}

#define OsConfigCaptureSuccessReason(reason, format, ...) {                           // SUCCESS
    if (NULL != reason) {
        if ((NULL != *reason) && (0 == strncmp(*reason, "PASS", 4))) {
            InternalOsConfigAddReason(reason, format, ##__VA_ARGS__);                 // append
        } else {
            FREE_MEMORY(*reason);
            *reason = ConcatenateStrings("PASS", FormatAllocateString(format, ...));  // replace, prefix with "PASS"
        }
    }
}
```

So the `reason` buffer has **two flavours** distinguished by a `"PASS"` prefix on the
success flavour. The behaviour of the four transitions is:

| Current `*reason`     | Call                              | Result                                                       |
|-----------------------|-----------------------------------|--------------------------------------------------------------|
| NULL                  | `OsConfigCaptureReason` (fail)    | `*reason = <fail text>`                                      |
| NULL                  | `OsConfigCaptureSuccessReason`    | `*reason = "PASS" + <pass text>`                             |
| Starts with `"PASS"`  | `OsConfigCaptureReason` (fail)    | **Frees and replaces** with `<fail text>` (success wiped)    |
| Starts with `"PASS"`  | `OsConfigCaptureSuccessReason`    | Appends `", also " + tolower(<pass text>)`                   |
| Failure flavour       | `OsConfigCaptureReason` (fail)    | Appends `", also " + tolower(<fail text>)`                   |
| Failure flavour       | `OsConfigCaptureSuccessReason`    | **Frees and replaces** with `"PASS" + <pass text>` (failures wiped) |

Two important properties follow:

1. **Reasons of the same flavour accumulate** through repeated calls in the same
   audit, separated by `", also "` with the first character of each appended
   fragment lowercased.
2. **Reasons of the opposite flavour wipe the buffer.** A subsequent success
   destroys all previously-recorded failure reasons; a subsequent failure destroys
   all previously-recorded success reasons.

### 2.1 What this produces in `AuditEnsureLockoutForFailedPasswordAttempts`

`reason` starts as `NULL`. Then either:

- **Some file passes**: `CheckLockoutForFailedPasswordAttempts` calls
  `OsConfigCaptureSuccessReason(reason, "'auth required %s' found uncommented with 'deny' set to %d and 'unlock_time' set to %d in '%s'", …)`.
  - If failures had been recorded for earlier files in the chain, they are wiped and
    replaced by `"PASS" + <success text>`. So the final reason describes **only the
    first passing file**, even though up to 8 prior files contributed failure detail
    that has now been thrown away.
  - `RETURN_REASON_IF_ZERO` then `return reason;` — audit reports compliant with the
    success reason.

- **All nine probes fail**: each call appends two `OsConfigCaptureReason` lines
  (one for `deny`, one for `unlock_time`). After 9 failing calls the buffer contains
  18 failure clauses joined by `", also "`. Then:

  ```c
  FREE_MEMORY(reason);
  reason = DuplicateString("Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution. "
      "Manually set lockout for failed password attempts following specific instructions for this distrubution. Automatic remediation is not possible");
  return reason;
  ```

  The 18 detailed clauses describing **exactly which files and modules were inspected**
  are **freed and discarded**. The caller gets a single hardcoded sentence that
  (a) is wrong about its diagnosis — on RHEL 9, Mariner, modern Ubuntu, etc. the PAM
  modules *do* exist; only the inline-options form does not — and (b) contains a typo
  ("distrubution").

### 2.2 Consequences

- An operator looking at the reason on a non-compliant RHEL 9 machine sees
  "Neither pam_faillock.so … exist for this distribution", goes to check, finds the
  module is in fact installed and the lockout policy is in `/etc/security/faillock.conf`,
  and concludes the audit is broken — which it is.
- The same `reason` flow is used by other ASB rules in this file via
  `RETURN_REASON_IF_ZERO`/`RETURN_REASON_IF_NOT_ZERO`. The audit-specific defect
  here is the **explicit `FREE_MEMORY(reason)` followed by hardcoded replacement**
  at the bottom of `AuditEnsureLockoutForFailedPasswordAttempts`, which short-circuits
  the otherwise-correct accumulation. Removing those two lines would already make the
  failure path far more diagnostic.

---

## 3. Why the audit fails on modern Linux distributions

The current audit requires that **a single line in one of `system-auth`, `password-auth`,
or `login`** contains `auth required <pam_faillock|pam_tally2|pam_tally>.so` with inline
`deny=…` and `unlock_time=…`. That layout is essentially the pre-2020,
pre-`faillock.conf`, pre-`authselect`, pre-`pam-auth-update` style. The following
real-world configurations all fail audit even though lockout is correctly enforced:

### 3.1 RHEL 8/9, Rocky/Alma 8/9, Fedora, CentOS Stream (authselect + faillock.conf)

On these systems `authselect` generates `/etc/pam.d/system-auth` and
`/etc/pam.d/password-auth`. The generated files look like:

```
auth        required      pam_faillock.so preauth silent
auth        sufficient    pam_unix.so ...
auth        [default=die] pam_faillock.so authfail
account     required      pam_faillock.so
```

The numeric policy (`deny`, `unlock_time`, `even_deny_root`, `fail_interval`, …) lives in
`/etc/security/faillock.conf` (a key=value file), **not** on the PAM line. The audit:

- never opens `/etc/security/faillock.conf`,
- requires `deny=` and `unlock_time=` to appear on the same PAM line,
- requires control flag exactly `required`, but the policy-bearing line is usually
  `[default=die]` (i.e. `authfail`),

so it returns ENOENT and the rule is reported as non-compliant on a system that is in
fact perfectly configured. No matches anywhere in the codebase for `faillock.conf`
confirms this gap.

### 3.2 Debian 11/12 and Ubuntu 20.04/22.04/24.04 (pam-auth-update + common-auth)

On Debian-family, the active file is `/etc/pam.d/common-auth`, generated by
`pam-auth-update` from snippets in `/usr/share/pam-configs/`. The audit never reads
`common-auth`, so even if `pam_faillock` (or `pam_tally2` on older releases) is
correctly enabled there the rule fails.

Note the inconsistency with the **remediation**: `SetLockoutForFailedPasswordAttempts`
*does* write to `/etc/pam.d/common-auth` (line 346), but the audit never checks it.
Result: on Debian/Ubuntu, even after a successful remediation the audit will still
report non-compliant.

### 3.3 Modern Ubuntu / Debian using `pam_faillock`

Recent Ubuntu (23.10+) and Debian (12 backports) ship `pam_faillock` and also use
`/etc/security/faillock.conf` for the numeric options. Same failure mode as §3.1.

### 3.4 SUSE / openSUSE

SLES and openSUSE use `pam_tally2` historically and `pam_faillock` more recently,
configured through `pam-config` and again often via `faillock.conf`. Files of interest
include `/etc/pam.d/common-auth` (not checked) and `/etc/pam.d/login` (checked but
typically only has an `include` directive, not the policy line).

### 3.5 Azure Linux / CBL-Mariner

Mariner uses `/etc/pam.d/system-auth` with `pam_faillock` and `faillock.conf`. Same
issue as §3.1.

### 3.6 Old Linux distributions

The audit works on legacy systems where the operator put a literal
`auth required pam_tally2.so ... deny=5 unlock_time=900` line into `system-auth`,
`password-auth` or `login`. This is correct, but it is exactly the configuration style
that has been deprecated for ~5 years.

---

## 4. Specific defects in the audit (`CheckLockoutForFailedPasswordAttempts`)

| # | Location | Issue |
|---|----------|-------|
| A1 | PassUtils.c:253–256 | `deny=0` is treated as compliant (`0 <= deny <= 5`). `deny=0` disables the lockout entirely. Lower bound should be `1`. |
| A2 | PassUtils.c:253–256 | `deny > 5` (e.g. `deny=10`) is reported non-compliant. CIS 5.3.2 only requires `deny <= 5`; some hardened baselines use higher values. The upper bound is debatable but at minimum the failure message ("instead of a value between 0 and 5") contradicts the lower bound. |
| A3 | PassUtils.c:253 | Match is done with `strstr(line, "auth")`, so any line containing words like `authtok`, `pam_authok.so`, or even `default=authok` matches. Combined with A4 this is largely harmless but should anchor on the first token. |
| A4 | PassUtils.c:254 | `GetStringOptionFromBuffer(line, "auth", ' ', '#')` returns the token after the first occurrence of `auth` followed by a space. On a line `password required pam_unix.so authok=…`, it can return the wrong token. PAM lines must be parsed positionally (column 1 = type, column 2 = control, column 3 = module), not as `key separator value`. |
| A5 | PassUtils.c:254 | Only accepts control flag `required`. Real-world policy lines use `requisite`, `[default=die]`, `[success=ok default=die]`, `sufficient` (for `pam_faillock` `authsucc`), etc. The audit will reject all of these. |
| A6 | PassUtils.c:253 | Substring match on `pamSo` means a line with `pam_faillock.so.bak` or `# see pam_tally.so` would match (the latter is filtered by the comment-character early-out, but inline comments after the directive are not). |
| A7 | Asb.c:2086–2094 | Never checks `/etc/pam.d/common-auth` (Debian/Ubuntu), `/etc/security/faillock.conf` (RHEL 8+/Fedora/SUSE/Mariner), or `pam_pwquality.conf`. |
| A8 | Asb.c:2086–2094 | The 9 calls form an OR: pass if any one combination satisfies the inline check. A system with conflicting / contradictory lines across files would still pass. |
| A9 | PassUtils.c:240 | `fgets` reads at most `_SC_LINE_MAX` bytes and treats each as a line. PAM files allow line continuation with `\` at EOL; a continued policy line would be parsed as two unrelated half-lines. |
| A10 | PassUtils.c:248 | The "comment" check is only `line[0] == '#'`. Lines with leading whitespace followed by `#` (perfectly legal in PAM files) are treated as active configuration. |
| A11 | PassUtils.c:248 | Inline comments (`auth required pam_faillock.so preauth # disabled`) are not stripped before parsing. |
| A12 | PassUtils.c:233 | `fopen` failure returns `EACCES` but the function then falls through and still frees `line` etc.; on a non-existent file the earlier `CheckFileExists` returns `ENOENT` so this is mostly dead code, but the error message is misleading. |
| A13 | PassUtils.c:215 | If `CheckFileExists` fails, the function returns `ENOENT` *before* allocating `line` — fine; but it also bypasses the `CaptureReason` calls at the bottom, so the audit reason will simply say nothing for that file. |
| A14 | PassUtils.c:197 | The function only handles the inline `deny=`/`unlock_time=` form. It has no concept of `faillock.conf`, no concept of `pam_faillock` split into `preauth` + `authfail` lines, no concept of `account required pam_faillock.so` (which is mandatory for lockouts to actually be enforced; without the `account` line on RHEL/CentOS the user is never marked as locked). |
| A15 | Asb.c:2095–2097 | The audit explicitly `FREE_MEMORY(reason)`s the accumulated per-file diagnostic reasons and overwrites them with a hardcoded "Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution. … distrubution …" string (note the typo). This wipes all useful diagnostic information *and* misdiagnoses the failure: on RHEL 9/Mariner/modern Ubuntu the modules **do** exist; they are just driven via `faillock.conf`. Simply deleting these two lines would already produce a far more accurate failure reason via the normal `OsConfigCaptureReason` accumulation. |

---

## 5. Specific defects in the remediation (`SetLockoutForFailedPasswordAttempts`)

| # | Location | Issue |
|---|----------|-------|
| R1 | PassUtils.c:339 | The single `preauth silent audit deny=3 unlock_time=900 even_deny_root` line is **insufficient for `pam_faillock` to enforce lockouts.** A correct config requires three lines: `auth required pam_faillock.so preauth …`, `auth [default=die] pam_faillock.so authfail`, and `account required pam_faillock.so`. The current code writes only the first. After remediation, `pam_faillock` counts failures but never denies. |
| R2 | PassUtils.c:363 / FileUtils.c:1022 | The marker passed to `ReplaceMarkedLinesInFile` is the bare module name (e.g. `pam_faillock.so`). `ReplaceMarkedLinesInFileInternal` replaces the *first* uncommented line containing that substring and **deletes all subsequent uncommented lines containing that substring** (lines 1026–1037 set `skipLine = true`). On a correctly configured RHEL 9 system this **collapses three valid `pam_faillock.so` lines (preauth / authfail / account) into one broken `preauth` line**, actively breaking lockout. |
| R3 | PassUtils.c:346 | Writes the same `auth …` template into `/etc/pam.d/login` and `/etc/pam.d/common-auth`. `common-auth` is auto-generated by `pam-auth-update` on Debian/Ubuntu and the changes will be reverted on the next `pam-auth-update --package` (e.g. on package install). The correct approach is to ship a config snippet under `/usr/share/pam-configs/` and call `pam-auth-update`. |
| R4 | PassUtils.c:346 | On RHEL 8+/Fedora, `/etc/pam.d/system-auth` and `/etc/pam.d/password-auth` are managed by `authselect` (they are typically symlinks into `/etc/authselect/`). Direct edits emit a warning from `authselect check` and will be overwritten on the next `authselect apply-changes`. Correct approach: write a custom authselect profile or modify `/etc/security/faillock.conf`. |
| R5 | PassUtils.c:341, 390–402 | The `pam_tally + pam_deny` template is `"auth required %s onerr=fail deny=3 unlock_time=900\nauth required %s\n"` and is passed as a single `newline` value. Because `ReplaceMarkedLinesInFile` only marks lines matching the **marker** `pam_tally.so` (line 392), the `pam_deny.so` half can never be re-located or updated later (its marker is different), and if the file already contains `pam_deny.so` somewhere it will be left as a stale duplicate. |
| R6 | PassUtils.c:386 | No `restorecon` is done after editing PAM files. On Enforcing SELinux systems (RHEL/Mariner/Fedora), `RenameFileWithOwnerAndAccess` leaves the file with the temp file's SELinux context, which can cause the PAM stack to fail to load the configuration. |
| R7 | PassUtils.c:115 | `EnsurePamModulePackagesAreInstalled` blindly tries to install `pam`, `libpam-modules`, `pam_pwquality`, `libpam-pwquality`, `libpam-cracklib`. None of these provide `pam_faillock.so` (on RHEL it comes from `pam`, on Debian from `libpam-modules`). `libpam-cracklib` does not exist on Debian 12+. The call is at best wasted work; at worst it logs spurious package-not-found warnings. |
| R8 | PassUtils.c:339–341 | Hardcoded `deny=3` (faillock/tally+deny) or `deny=5` (tally2). The desired value is not taken from `value` (the JSON payload is discarded by `RemediateEnsureLockoutForFailedPasswordAttempts(value, log) { UNUSED(value); ... }` at Asb.c:3736). So even if a customer wanted `deny=5 unlock_time=1800`, they cannot specify it. |
| R9 | FileUtils.c:1095–1104 | When the marker is not present, `ReplaceMarkedLinesInFile` **appends** the new line at the end of the file. For PAM files this is critical: putting an `auth required pam_faillock.so preauth` line *after* `auth sufficient pam_unix.so` means the failure counter is only incremented if `pam_unix` already failed — and `pam_faillock authfail` is missing entirely. The correct insertion point for the preauth line is **before** the first `auth` module of the stack (only `ReplaceMarkedLinesInFilePrepend` would do that, and it is never used here). |
| R10 | PassUtils.c:347 | `numPamConfigurations` is iterated but each file is treated independently with no cross-file validation; if some files are symlinks to a shared file (RHEL: `password-auth -> /etc/authselect/password-auth`) the remediation processes the same physical file multiple times via different paths, potentially racing temp-file renames. |
| R11 | PassUtils.c:339 | The template contains `even_deny_root`. On systems where root login is via a different path, `even_deny_root` can lock out the only admin account during a brute-force test, with no remote recovery. CIS does not require `even_deny_root`. |

---

## 6. Concrete examples of how the rule misbehaves today

### 6.1 RHEL 9 / Rocky 9 / Alma 9 (default authselect "sssd" or "minimal")

Default `/etc/pam.d/system-auth` (excerpt):

```
auth        required      pam_faillock.so preauth silent
auth        sufficient    pam_unix.so nullok
auth        [default=die] pam_faillock.so authfail
auth        required      pam_deny.so
account     required      pam_faillock.so
```

Plus `/etc/security/faillock.conf`:

```
deny = 5
unlock_time = 900
```

- Audit: **FAIL.** No PAM line has inline `deny=`/`unlock_time=`. The faillock control
  flag is `required` (preauth) or `[default=die]` (authfail) — only the preauth line
  matches `required`, but it has no `deny`/`unlock_time`. The reason returned to the
  caller is the hardcoded "modules don't exist" sentence from Asb.c:2095–2097, not
  the (more accurate) per-file failure clauses that were accumulated and then thrown
  away (see §2.1).
- Remediation: **breaks lockout.** Marker `pam_faillock.so` is found, the `preauth` line
  is replaced with `auth required …/pam_faillock.so preauth silent audit deny=3 unlock_time=900 even_deny_root`, then the `authfail` and `account` lines are
  **deleted** (skipped) by ReplaceMarkedLinesInFile. Result: `pam_faillock` is invoked
  in preauth only, never increments fail counter on failure, never denies access in
  `account` phase. Lockout is effectively disabled.
- Side effect: authselect will mark the file as modified; next `authselect
  apply-changes` (e.g. after a `dnf update` of `authselect-libs`) silently restores the
  vendor file, undoing osconfig's changes.

### 6.2 Ubuntu 22.04 (default install)

Default `/etc/pam.d/common-auth`:

```
auth    [success=1 default=ignore]  pam_unix.so nullok
auth    requisite                   pam_deny.so
auth    required                    pam_permit.so
```

`pam_faillock` not configured; `pam_tally2` not installed by default.

- Audit: reads `/etc/pam.d/system-auth` (does not exist on Ubuntu),
  `/etc/pam.d/password-auth` (does not exist), `/etc/pam.d/login`. None match.
  `common-auth` is never checked. Audit returns the hardcoded "Neither pam_faillock.so,
  pam_tally2.so or pam_tally.so PAM modules exist for this distribution …" message
  (despite the module being installed at
  `/usr/lib/x86_64-linux-gnu/security/pam_faillock.so`).
- Remediation: `FindPamModule("pam_faillock.so")` finds the module in
  `/usr/lib/x86_64-linux-gnu/security/`. `common-auth` exists, so a line is
  **appended** (marker not present) at the **end** of common-auth. Because it appears
  after the `pam_unix.so` `[success=1 default=ignore]` branch, the `pam_faillock.so
  preauth` is only reached on failure, and there is no `authfail` or `account` line.
  Lockout still does not work, and the audit still fails because audit never reads
  `common-auth`.

### 6.3 Debian 11 / 12, old policy with `pam_tally2`

If an operator had hand-edited `common-auth` to add `auth required pam_tally2.so
onerr=fail deny=5 unlock_time=900`, the audit still says non-compliant because
`common-auth` is never inspected.

### 6.4 Legacy CentOS 6 / Ubuntu 16.04

The single-inline-line layout is present, the modules are in
`/usr/lib/x86_64-linux-gnu/security/` or `/lib/security/`, and both audit and
remediation work as designed. This is essentially the only modern environment in which
the current code is correct.

---

## 7. Suggested direction for a robust implementation

(High-level only — for a future PR.)

1. **Detect the active mechanism by environment, not by enumerating module files.**

   Order of preference:

   1. If `/etc/security/faillock.conf` exists → policy lives there.
      Audit: parse `deny`, `unlock_time`, `even_deny_root`, `fail_interval`
      from the conf file (use the existing `GetIntegerOptionFromFileLessOrEqualWith`
      helper). Additionally confirm `pam_faillock.so` is referenced (uncommented)
      in `system-auth`/`password-auth` (RHEL/Mariner/SUSE) or `common-auth`
      (Debian/Ubuntu).
      Remediation: write to `/etc/security/faillock.conf` (idempotent key=value
      file, no PAM stack ordering issues). On RHEL, also run `authselect
      apply-changes` (or use `authselect enable-feature with-faillock`). On
      Debian, ship `/usr/share/pam-configs/osconfig-faillock` and run
      `pam-auth-update --package`.

   2. Else if `pam_faillock.so` is present but no `faillock.conf` → legacy inline
      style: require `preauth`, `authfail`, **and** `account` lines with `deny<=N`
      and `unlock_time>=M` on the PAM lines.

   3. Else if `pam_tally2.so` is present → legacy `pam_tally2` style (single line
      with inline options), check in `system-auth`/`password-auth`/`common-auth`/`login`.

   4. Else if `pam_tally.so` + `pam_deny.so` are present → very legacy.

   5. Otherwise → N/A on this distro, return a clear "skipped" reason
      (today the audit returns a misleading non-compliant message).

2. **Fix the failure reason path.** Delete the
   `FREE_MEMORY(reason); reason = DuplicateString("Neither pam_faillock.so …")`
   block at the bottom of `AuditEnsureLockoutForFailedPasswordAttempts` so that the
   genuinely-accumulated per-file failure reasons (assembled by the
   `OsConfigCaptureReason` "PASS"/", also " machinery — see §2) survive to the
   operator. Fix the "distrubution" typo while you're there.

3. **Fix `CheckLockoutForFailedPasswordAttempts`**:
   - parse PAM lines positionally (token 1 = type, token 2 = control, token 3 = module),
     accepting `required`, `requisite`, `[default=die]`, `[success=… default=die]` as
     enforcement-capable controls;
   - accept inline comments and leading whitespace;
   - support line continuation;
   - require `deny >= 1` (not `>= 0`);
   - read `deny`/`unlock_time` from `faillock.conf` when present;
   - check `/etc/pam.d/common-auth` on Debian-family.

4. **Fix `SetLockoutForFailedPasswordAttempts`**:
   - **never** use bare `pam_faillock.so` as a `ReplaceMarkedLinesInFile` marker;
     use distinct markers per role (`pam_faillock.so preauth`, `pam_faillock.so authfail`,
     `pam_faillock.so` in `account` section). Or, better, do not edit PAM files at all
     for `pam_faillock` — drive everything through `faillock.conf`;
   - emit all three required lines (preauth/authfail/account), inserted at the
     correct positions (preauth/authfail before `pam_unix.so`, `account` in the
     account section). For ordering, `ReplaceMarkedLinesInFilePrepend` already exists
     but is not used here.
   - respect the `authselect`/`pam-auth-update` workflow on systems that use them
     (call `authselect apply-changes` / `pam-auth-update --package` after editing the
     underlying source files);
   - honour a `value` payload to allow operators to set `deny`/`unlock_time` rather
     than hardcoding 3/5 and 900;
   - drop `even_deny_root` from the default template (or make it opt-in);
   - restore SELinux context (`selinux_restorecon`) on edited PAM files, since
     `RenameFileWithOwnerAndAccess` produces files with the temp file's context.

5. **`ReplaceMarkedLinesInFile` semantics** are dangerous for PAM. Either:
   - introduce a variant that replaces *only the first* matching line and **does not**
     delete subsequent matches, or
   - require the caller to pass a more specific marker (full normalized line prefix).

6. **Documentation / telemetry**: when no supported mechanism is present (e.g. Alpine,
   musl, containers without PAM), emit a clear "rule not applicable on this system"
   reason instead of a "manual remediation required" message.

---

## 8. Files referenced

- `src/common/asb/Asb.c` — rule registration (line 727), audit (line 2080), remediation
  dispatch (line 3736), file path constants (lines 490–492), `RETURN_REASON_IF_ZERO`
  macro (line 22).
- `src/common/commonutils/PassUtils.c` — `FindPamModule` (line 20),
  `EnsurePamModulePackagesAreInstalled` (line 113),
  `CheckLockoutForFailedPasswordAttempts` (line 197),
  `SetLockoutForFailedPasswordAttempts` (line 311).
- `src/common/commonutils/FileUtils.c` — `ReplaceMarkedLinesInFileInternal`
  (line 960), `GetStringOptionFromBuffer` (line 1745),
  `GetIntegerOptionFromBuffer` (line 1812).
- `src/common/logging/Reasons.h` — `OsConfigCaptureReason`,
  `OsConfigCaptureSuccessReason`, `InternalOsConfigAddReason`, `OsConfigResetReason`,
  `SECURITY_AUDIT_PASS` ("PASS" prefix).
- `src/common/commonutils/CommonUtils.h` — public declarations for the above.
- `src/common/tests/CommonUtilsUT.cpp` — existing unit tests for
  `CheckLockoutForFailedPasswordAttempts` (only cover the legacy inline-line case).

---

## 9. TL;DR

The rule as implemented today only works on legacy AMD64 systems whose PAM stack uses
a single `auth required pam_{faillock,tally2,tally}.so … deny=… unlock_time=…` line.
That style is obsolete: RHEL 8+/Fedora/Mariner/SUSE use `pam_faillock` with
`/etc/security/faillock.conf` and `authselect`, and Debian 11+/Ubuntu 20.04+ use
`pam-auth-update` with `/etc/pam.d/common-auth`. The audit does not understand either,
so on all current mainstream distributions it incorrectly reports non-compliant, and
the remediation actively damages a correctly configured RHEL/Fedora/Mariner PAM stack
(collapsing the three required `pam_faillock` lines into one broken line).
On top of that, the audit accumulates fairly detailed per-file failure reasons through
the `OsConfigCaptureReason` "PASS"/", also " machinery — but the bottom of
`AuditEnsureLockoutForFailedPasswordAttempts` then explicitly frees those reasons and
replaces them with a hardcoded "Neither pam_faillock.so … exist for this distribution
… distrubution …" string, which is both diagnostically misleading and contains a typo.

## 10. BUG ON RHEL 8

Below are scoping questions anwered by the user during case creation:
[{"Question":"When did the problem start?","ControlId":"problem_start_time","OrderId":"2","Answer":"2026-05-01T19:59:41+00:00","AnswerValue":"2026-05-01T19:59:41+00:00"},{"Question":"Description","ControlId":"problem_description","OrderId":"1","Answer":"Full description of Azure Linux Baseline rule:\nEnsure lockout for failed password attempts is configured (CIS: L1 - Server - 5.3.2)\n\nFull reason for Azure flagging this VM: \nNeither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution. Manually set lockout for failed password attempts following specific instructions for this distrubution. Automatic remediation is not possible\n\nMy Investigation:\n- PAM module is confirmed installed at the VM\n[root@diih-DB-DEV-01 ~]# ls -l /usr/lib64/security/{pam_faillock.so,pam_tally2.so,pam_tally.so}\nls: cannot access '/usr/lib64/security/pam_tally2.so': No such file or directory\nls: cannot access '/usr/lib64/security/pam_tally.so': No such file or directory\n-rwxr-xr-x 1 root root 24720 Nov 14 04:39 /usr/lib64/security/pam_faillock.so\n\n[root@diih-DB-DEV-01 ~]# grep -RinE 'pam_faillock\\.so|pam_tally2\\.so|pam_tally\\.so' /etc/pam.d /etc/authselect\n/etc/pam.d/password-auth:6:auth required pam_faillock.so preauth silent\n/etc/pam.d/password-auth:12:auth required pam_faillock.so authfail\n/etc/pam.d/password-auth:15:account required pam_faillock.so\n/etc/pam.d/system-auth:6:auth required pam_faillock.so preauth silent\n/etc/pam.d/system-auth:12:auth required pam_faillock.so authfail\n/etc/pam.d/system-auth:15:account required pam_faillock.so\n/etc/authselect/system-auth:6:auth required pam_faillock.so preauth silent\n/etc/authselect/system-auth:12:auth required pam_faillock.so authfail\n/etc/authselect/system-auth:15:account required pam_faillock.so\n/etc/authselect/password-auth:6:auth required pam_faillock.so preauth silent\n/etc/authselect/password-auth:12:auth required pam_faillock.so authfail\n/etc/authselect/password-auth:15:account required pam_faillock.so\n\nQuestion:\nWhy is the VM still getting flagged with the reason \"Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution\"?\n\n\n\n\n","AnswerValue":"Full description of Azure Linux Baseline rule:\nEnsure lockout for failed password attempts is configured (CIS: L1 - Server - 5.3.2)\n\nFull reason for Azure flagging this VM: \nNeither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution. Manually set lockout for failed password attempts following specific instructions for this distrubution. Automatic remediation is not possible\n\nMy Investigation:\n- PAM module is confirmed installed at the VM\n[root@diih-DB-DEV-01 ~]# ls -l /usr/lib64/security/{pam_faillock.so,pam_tally2.so,pam_tally.so}\nls: cannot access '/usr/lib64/security/pam_tally2.so': No such file or directory\nls: cannot access '/usr/lib64/security/pam_tally.so': No such file or directory\n-rwxr-xr-x 1 root root 24720 Nov 14 04:39 /usr/lib64/security/pam_faillock.so\n\n[root@diih-DB-DEV-01 ~]# grep -RinE 'pam_faillock\\.so|pam_tally2\\.so|pam_tally\\.so' /etc/pam.d /etc/authselect\n/etc/pam.d/password-auth:6:auth required pam_faillock.so preauth silent\n/etc/pam.d/password-auth:12:auth required pam_faillock.so authfail\n/etc/pam.d/password-auth:15:account required pam_faillock.so\n/etc/pam.d/system-auth:6:auth required pam_faillock.so preauth silent\n/etc/pam.d/system-auth:12:auth required pam_faillock.so authfail\n/etc/pam.d/system-auth:15:account required pam_faillock.so\n/etc/authselect/system-auth:6:auth required pam_faillock.so preauth silent\n/etc/authselect/system-auth:12:auth required pam_faillock.so authfail\n/etc/authselect/system-auth:15:account required pam_faillock.so\n/etc/authselect/password-auth:6:auth required pam_faillock.so preauth silent\n/etc/authselect/password-auth:12:auth required pam_faillock.so authfail\n/etc/authselect/password-auth:15:account required pam_faillock.so\n\nQuestion:\nWhy is the VM still getting flagged with the reason \"Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution\"?\n\n\n\n\n"}]
Austin - I was able to reproduce the issue by creating an azure VM (RedHat/RHEL/810-gen2).  I then attempted to set values I thought would resolve the issue and it did not
/etc/security/faillock.conf
deny = 5
fail_interval = 900
unlock_time = 900
even_deny_root
root_unlock_time = 900
[austin448xpr@xyRHEL8-10 ~]$ grep -E 'deny|fail_interval|unlock_time|even_deny_root|root_unlock_time' /etc/security/faillock.conf
deny = 5
fail_interval = 900
unlock_time = 900
even_deny_root
# This option implies the `even_deny_root` option.
# the value is the same as of the `unlock_time` option.
root_unlock_time = 900
# the root account (the options `even_deny_root>` and
# `root_unlock_time` will apply to them.
[austin448xpr@xyRHEL8-10 ~]$
[austin448xpr@xyRHEL8-10 ~]$ authselect list
- minimal        Local users only for minimal installations
- nis            Enable NIS for system authentication
- sssd           Enable SSSD for system authentication (also for local users only)
- winbind        Enable winbind for system authentication
[austin448xpr@xyRHEL8-10 ~]$ authselect current
Profile ID: sssd
Enabled features:
- with-faillock
I then today (before 11am Eastern) I tried the following as suggested on https://oneuptime.com/blog/post/2026-03-04-lock-user-accounts-failed-login-attempts-rhel-9/view
audit
silent
deny = 5
fail_interval = 900
unlock_time = 600
even_deny_root
root_unlock_time = 900
https://learn.microsoft.com/en-us/azure/governance/policy/samples/guest-configuration-baseline-linux
Name (CCEID) New Name
Details Remediation check
Ensure lockout for failed password attempts is configured. (5.3.2)
Ensure lockout for failed password attempts is configured (CIS: L1 - Server - 5.3.2)
Description: Locking out user IDs after n unsuccessful consecutive login attempts mitigates brute force password attacks against your systems.
for Ubuntu and Debian, add the pam_tally and pam_deny modules as appropriate. For all other distros, refer to your distro's documentation

## 11. ASK:

Prepare a fix for the two audit and remediate functions in Asb.c and the two functions in PassUtils.c. Also prepare unit-tests for the updated PassUtil.c functions to add to D:\GitHub\azure-osconfig\src\common\tests\CommonUtilsUT.cpp
You can do changes in this current branch here.
Be careful to have correct: 1. informational and error logging, 2. proper reasons for all pass and fail cases, and 3. telemetry (see at the existing cases for telemetry for error, for example)

## 12. Copilot's questions before starting

### Root cause summary (to confirm we agree)

On the customer's RHEL 8 box, `pam_faillock.so` is correctly configured the "modern" way:

- The PAM stack files (`/etc/pam.d/system-auth`, `/etc/pam.d/password-auth`, and the
  authselect source under `/etc/authselect/`) contain three uncommented references to
  the module — `preauth`, `authfail`, `account` — but **none of them** carries
  `deny=` or `unlock_time=` inline.
- The numeric policy (`deny`, `unlock_time`, `even_deny_root`, etc.) lives in
  `/etc/security/faillock.conf` instead.

Today `CheckLockoutForFailedPasswordAttempts` requires `deny=<0..5>` **and**
`unlock_time>0` to appear **on the same PAM line as** `auth required pam_faillock.so`.
With the modern split-line + `faillock.conf` layout, that condition is never
satisfied, so all 9 audit probes fail. `AuditEnsureLockoutForFailedPasswordAttempts`
then frees the accumulated per-file reasons and overwrites them with the hardcoded
"Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this
distribution. … distrubution …" sentence, which is what reaches the customer in
Portal.

[Marius] Keep that hard-coded default there, it is meant to catch if none of previous calls succeeded.

The bug is therefore "the audit doesn't understand the modern `pam_faillock` +
`/etc/security/faillock.conf` configuration layout", not "wrong PAM modules
detected". The fix has to:

1. **Audit**: also recognize the modern layout. Where the inline-options style is
   used (legacy), keep accepting it exactly as today (no regression on the >2M
   already-shipping fleet).
2. **Audit reason**: stop overwriting accumulated reasons with the misleading
   hardcoded sentence. The Portal-visible string must accurately describe what was
   inspected and why it failed (US English, no typo), or describe the success.
3. **Remediation**: when the modern layout is in use, the existing PAM-line
   rewriting is harmful (it collapses the three `pam_faillock.so` lines into one
   broken line — see §6.1). The remediation should write `deny`/`unlock_time` into
   `/etc/security/faillock.conf` instead, when that file exists, and leave the PAM
   stack alone in that case. The existing PAM-line rewriting must continue to work
   on systems that don't have `/etc/security/faillock.conf` (the legacy path).

[Marius] Not more than one line per call is replaced -- and if the line does not exists, it is appended. So what you described (collapses the three `pam_faillock.so` lines into one broken line) does not happen. Be careful with that function and continue to use it properly.

Existing helpers that already do what we need:

- `CheckLineFoundNotCommentedOut(file, '#', "pam_faillock.so", &reason, log)` —
  verifies that a substring appears uncommented in a file (correct reason capture
  for both pass and fail).
- `GetIntegerOptionFromFile(file, "deny", '=', '#', 10, log)` and
  `CheckIntegerOptionFromFileLessOrEqualWith(...)` — already parse
  `key = value` lines and produce reasons.
- `ReplaceMarkedLinesInFile` and `SetEtcConfValue` for writing back.

[Marius] Correct. Ideally for this fix we shall keep using same commonutils utils. However, if necessary, we can add more (and unit-tests).

These cover the customer's case without new low-level utilities, which is
attractive given the regression risk.

### Q12.1 — Acceptance criteria for the modern `faillock.conf` path

For the modern layout, when should the audit return PASS? (`deny` / `unlock_time`
values follow CIS L1 5.3.2 → `deny <= 5` and `unlock_time > 0`, matching the
inline path that's already shipping.) Pick one:

- **A (lenient)**: `/etc/security/faillock.conf` exists **and** `pam_faillock.so`
  appears uncommented in at least one of `/etc/pam.d/system-auth`,
  `/etc/pam.d/password-auth`, `/etc/pam.d/login`. Numeric values not required to
  be uncommented — rely on `pam_faillock`'s own compliant defaults
  (`deny=3`, `unlock_time=600`).
- **B (strict)**: same files + `pam_faillock.so` reference, **and**
  `/etc/security/faillock.conf` has `deny` set, uncommented, with `0 < deny <= 5`,
  **and** `unlock_time` set, uncommented, with `unlock_time > 0`.
- **C (mixed, my recommendation)**: same files + `pam_faillock.so` reference,
  **and** *if* `deny` is uncommented in `faillock.conf` it must satisfy
  `0 < deny <= 5` (otherwise treat the unset default of `3` as compliant), **and**
  *if* `unlock_time` is uncommented it must satisfy `unlock_time > 0` (otherwise
  treat the unset default of `600` as compliant).

  C exactly mirrors what `pam_faillock` actually does at runtime and matches the
  customer's "I set `deny = 5` / `unlock_time = 900` and the rule still fails"
  scenario, while also passing a default RHEL 8 box that has the file present but
  fully commented out.

  *Note: the current inline path already accepts `deny=0`, which is actually a
  disable. Option C is consistent with the existing leniency, so we don't change
  the threshold elsewhere and avoid a behavioural regression. If you want me to
  also tighten to `deny >= 1` here I can — say so.*

[Marius] We need to detect old vs new without hard-coding for exact distro versions. This is done for other cases (for example package management utils detect differences by installed package manager). Then we need to fork and use either existing or the new methodology for both audit and remediation. Feel free to proposae criteria for differentiation.

### Q12.2 — Which PAM files count as "configured"?

The current audit inspects `system-auth`, `password-auth`, `login`. The current
remediation also writes to `common-auth`. Should the audit's modern path inspect:

- **A**: just `system-auth`, `password-auth`, `login` (preserve today's audit scope).
- **B**: `system-auth`, `password-auth`, `login` **and** `/etc/pam.d/common-auth`
  (Debian/Ubuntu) — fixes the existing "remediation writes there, audit doesn't
  read there" asymmetry called out in §4 A7 / §3.2.

  My recommendation: B. It's a one-line addition (same call against one more
  path), it doesn't affect RHEL 8 (where `common-auth` doesn't exist), and it
  removes a long-standing latent failure on Debian/Ubuntu.

[Marius] Agree, let's B -- add `/etc/pam.d/common-auth` -- but in exact same style as existing code and using the existing constant.

### Q12.3 — Where does the modern remediation write?

When the modern layout is in use (i.e., `/etc/security/faillock.conf` exists),
should the remediation:

- **A**: write `deny=3` and `unlock_time=900` (the current hardcoded constants
  used by the inline templates) into `/etc/security/faillock.conf` and **skip**
  the PAM-line rewriting for that distro. PAM-line rewriting still runs on
  distros that don't have `faillock.conf`.
- **B**: write to `/etc/security/faillock.conf` **and** also do the existing
  PAM-line rewriting.
- **C**: don't touch `/etc/security/faillock.conf` at all — only fix the audit,
  leave the remediation alone.

  My recommendation: A. Touching the PAM stack on a system whose vendor manages
  it via `authselect` (RHEL 8/Mariner) is the actively harmful part of today's
  remediation — see §6.1. Writing to `faillock.conf` is the supported, vendor-
  documented way and is what the customer already did manually. C means a
  RHEL 8 box with no operator action will still flag and will still get a
  destructive remediation if the customer enables remediation.

  [Marius] See my earlier comment. We need to detect new vs old here on generic ways, without precise distro detection (we have common APIs for that but we do NOT want to hard-code per distro or group). Let me know what criteria you propose before we proceed.

  I would also propose a guarded form: when option A applies, only `deny` and
  `unlock_time` keys are touched in `faillock.conf` — using the existing
  `ReplaceMarkedLinesInFile` helper with the key as marker so existing user
  customisations to other keys (`fail_interval`, `even_deny_root`,
  `root_unlock_time`, etc.) are preserved untouched. Confirm this surgical
  approach is OK.

### Q12.4 — Replacement for the hardcoded "modules don't exist" sentence

`AuditEnsureLockoutForFailedPasswordAttempts` lines 2095–2097 currently do:

```c
FREE_MEMORY(reason);
reason = DuplicateString("Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules exist for this distribution. "
    "Manually set lockout for failed password attempts following specific instructions for this distrubution. Automatic remediation is not possible");
```

What should happen on the all-fail path? Pick one:

- **A**: delete those two lines entirely. The `reason` buffer already contains
  the per-file `OsConfigCaptureReason` clauses ("'deny' not found in
  '/etc/pam.d/system-auth' for 'pam_faillock.so', also …") joined by ", also ".
  Diagnostic but can be long.
- **B**: replace with a single, accurate, US-English summary that points the
  operator at the actual fix on modern systems, e.g.:
  `"No compliant lockout configuration was found. On systems using pam_faillock,
  ensure '/etc/security/faillock.conf' contains 'deny <= 5' and 'unlock_time >
  0'. On legacy systems, ensure 'auth required pam_{faillock,tally2,tally}.so'
  with 'deny' and 'unlock_time' set is present in one of '/etc/pam.d/system-auth',
  '/etc/pam.d/password-auth', '/etc/pam.d/login', '/etc/pam.d/common-auth'."`
  (No PASS prefix.)
- **C**: B, but only when truly nothing is found; if some clauses were
  accumulated, keep the accumulated detail in addition.

  My recommendation: B. It's bounded length, gives the operator a clear action,
  no typo, and removes the misleading "modules don't exist" diagnosis.

[Marius] Keep it. Its purpose is if none of above checks pass. If you believe that won't be necesasary anymore, tell me why, do not proceed directly to remove.

### Q12.5 — Logging level for the new code paths

The new `faillock.conf` code paths will produce log lines such as:

- "CheckLockoutForFailedPasswordAttempts: '/etc/security/faillock.conf' found, checking 'deny' and 'unlock_time'" → **Info**.
- "CheckLockoutForFailedPasswordAttempts: 'deny' from '/etc/security/faillock.conf' is N (<= 5), compliant" → **Info**.
- "CheckLockoutForFailedPasswordAttempts: 'deny' from '/etc/security/faillock.conf' is N (> 5), non-compliant" → **Info**.
- Out-of-memory / EINVAL / fopen failures → **Error**, **plus**
  `OSConfigTelemetryStatusTrace("<symbol>", <errno>)` matching the existing
  conventions ("fileName"+EINVAL, "malloc"+ENOMEM, "FormatAllocateString"+ENOMEM,
  etc.).

[Marius] Reason strings must be JSON compliant (shall not break JSON). You can notice we JSON encode paths and such. Resist fancy characters, stick with ANSI. Also keep using same style that is in the code for BOTH reasons and logs AND coding style (no TABs, but spaces, 4 spaces per TAB, no trailing spaces, etc)

Confirm this matches your expectation, or tell me if there's a level I should
use differently. (I don't see a Debug helper in the codebase, only Info / Error.)

### Q12.6 — Telemetry for "no compliant config found"

When the all-fail path triggers (i.e., the audit returns non-compliant), is that
considered:

- **A**: a normal audit result (no `OSConfigTelemetryStatusTrace` call) — only
  internal failures (malloc, EINVAL, EACCES) emit telemetry.
- **B**: telemetry-worthy in its own right (emit a status trace tagged with the
  rule id when no compliant config is found, so we can see in telemetry how
  often this fires).

  My recommendation: A, consistent with how other rules treat "audit returned
  non-compliant" as the normal outcome rather than a failure.

[Marius] No. Do not add any new telemetry here. Only use the Telemetry macro for errors that are treated as errors in code by logging. Note these errors are NOT ALL errors -- errors encountered in normal audit operations are not treated as errors, only fatal ones are (invalid argument, out of memory, etc). Later is only telemetry we need to add, using existing telemetry macro and existring code as model.

### Q12.7 — Unit tests scope

For `CommonUtilsUT.cpp` I'll add tests that cover:

- `CheckLockoutForFailedPasswordAttempts` with the existing inline file
  formats (regression — same `goodTestFileContents` and `badTestFileContents`
  arrays must keep passing).
- `CheckLockoutForFailedPasswordAttempts` with the modern split-line PAM file
  + a `faillock.conf` companion: success and failure variants
  (`deny=5 unlock_time=900` PASS; `deny=10 unlock_time=900` FAIL;
  `deny=5 unlock_time=0` FAIL; both keys commented FAIL or PASS depending on
  Q12.1; etc.).
- `CheckLockoutForFailedPasswordAttempts` with `pam_faillock.so` referenced but
  `faillock.conf` missing → must still pass on the legacy inline path if the
  inline values are present in the PAM line.
- Reason-string assertions (we explicitly check that the buffer starts with
  `SECURITY_AUDIT_PASS` on pass, does **not** on fail, contains `, also ` on
  multi-clause cases, and is NULL-safe when called with `nullptr`).
- `SetLockoutForFailedPasswordAttempts` is harder to unit-test (it does real
  file I/O on absolute system paths). I'd prefer not to refactor those paths
  to be configurable inside this change — they're already test-overridable in
  the file for some constants via `#ifdef TEST_CODE` but not the PAM file
  list. Options: (i) leave `SetLockoutForFailedPasswordAttempts` covered only
  indirectly via the audit tests on synthetic fixtures; (ii) add a small
  helper (e.g. `SetFaillockConfValues(faillockConfPath, deny, unlockTime,
  log)`) that takes the path as a parameter, and unit-test that helper. I'd
  recommend (ii).

Confirm the scope, and let me know if you want anything else covered.

[Marius] We do not currently test for reasons. Skip that part. Question: how will you test for those file contents? We will need test file contents to test on, no attempt to reach system files should be done (unit-tests). Feel free to code and then I will review and tell you if we need changes.

### Q12.8 — Scope boundary check

The following pre-existing defects called out in §4–§5 are **out of scope** for
this fix unless you tell me otherwise:

- A1 — `deny=0` accepted (would tighten existing behaviour, possible regression).
- A2 — "between 0 and 5" wording (cosmetic).
- A3/A4/A6 — strstr-based PAM line matching (not triggering on the customer's
  case; refactoring is risky).
- A5 — only "required" control flag accepted (the customer's lines are
  `required`; broadening this could mask misconfigurations).
- A9 — line continuation (PAM-style continuations are extremely rare).
- A10/A11 — leading whitespace / inline `#` (PAM files rarely use these).
- A12/A13 — error-path cosmetics.
- R1 — `pam_faillock` needs preauth+authfail+account (we're not touching the
  PAM-stack write path for modern systems — see Q12.3).
- R2 — bare-marker line collapse (avoided because Q12.3.A means we won't
  rewrite PAM files on systems with `faillock.conf`).
- R5/R10 — authselect / cross-symlink races (we won't be writing PAM files on
  those systems anymore — see Q12.3).
- R6/R7/R8/R11 — SELinux, package install, even_deny_root, etc. (orthogonal).

Confirm this scoping is acceptable. If any of these should also be addressed
in the same change, list which ones.

[Marius] Agree. Speaking of this, add source code comments where necessary, in same style comments are now.

### Q12.9 — Branch / commit policy

You said "You can do changes in this current branch here." I will:

- not rewrite git history;
- group the change into logical commits (e.g. "PassUtils: recognize
  faillock.conf-based lockout configuration", "PassUtils: write faillock.conf
  values in remediation when present", "Asb: stop overwriting accumulated
  audit reason; remove typo'd string", "Tests: cover faillock.conf-based
  lockout audit"); 
- include the configured `Co-authored-by: Copilot` trailer on each commit;
- not push or open a PR — leave that to you.

Confirm or amend.

[Marius] Onlt change files, do not git add or commmit -- I will do that. But before proceeding let's discuss and agree how we will do the detection between old and new.



---

## 13. Copilot's response to Marius's feedback -- proposed approach

### 13.1 -- Detection criterion (the key open item)

I propose a single, environment-based criterion: **the presence of the file
`/etc/security/faillock.conf` on disk**.

- If `/etc/security/faillock.conf` exists -> "modern" path (faillock.conf-based).
- Otherwise -> "legacy" path (today's inline-options behaviour, unchanged).

Why this works generically, without naming any distro:

- `pam_faillock` was introduced upstream in Linux-PAM 1.4.0 (June 2020) together
  with `/etc/security/faillock.conf` as the canonical place to set `deny`,
  `unlock_time`, `even_deny_root`, `fail_interval`, `root_unlock_time`, etc.
  Every distro that ships `pam_faillock` today ships this file: RHEL 8/9,
  Rocky/Alma 8/9, Fedora 28+, CentOS Stream 8+, CBL-Mariner / Azure Linux,
  recent SUSE, Debian 11+ via `libpam-modules`, Ubuntu 22.04+ via
  `libpam-modules`. The file is shipped by the same package that ships the
  module, so "file exists" == "this distro uses the faillock.conf model".
- Older distros that never had `pam_faillock` (CentOS 6/7, Ubuntu 16.04/18.04,
  Debian 9/10, SLES 12, ...) never ship `/etc/security/faillock.conf`. They use
  the pre-1.4 inline-options style, exactly what today's code already
  handles correctly.
- This is purely a `CheckFileExists(...)` test -- no `uname`, no `/etc/os-release`
  parsing, no shell-out, no distro name hard-coding. Same spirit as how
  `PackageUtils.c` picks between `dpkg`/`apt` and `rpm`/`yum`/`dnf`/`tdnf` by
  probing which binary is installed, rather than by inspecting the distro
  name.

Pseudocode for the fork in `AuditEnsureLockoutForFailedPasswordAttempts`:

```c
if (0 == CheckFileExists(g_etcSecurityFaillockConf, NULL, log))
{
    // Modern path: pam_faillock.so + /etc/security/faillock.conf
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttemptsViaFaillockConf(
        g_etcPamdSystemAuth,    g_etcSecurityFaillockConf, &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttemptsViaFaillockConf(
        g_etcPamdPasswordAuth,  g_etcSecurityFaillockConf, &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttemptsViaFaillockConf(
        g_etcPamdLogin,         g_etcSecurityFaillockConf, &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttemptsViaFaillockConf(
        g_etcPamdCommonAuth,    g_etcSecurityFaillockConf, &reason, log));
}
else
{
    // Legacy path: existing inline checks (unchanged), plus common-auth added
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdSystemAuth,    pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdPasswordAuth,  pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdLogin,         pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdCommonAuth,    pamFailLockSo, '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdSystemAuth,    pamTally2So,   '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdPasswordAuth,  pamTally2So,   '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdLogin,         pamTally2So,   '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdCommonAuth,    pamTally2So,   '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdSystemAuth,    pamTallySo,    '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdPasswordAuth,  pamTallySo,    '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdLogin,         pamTallySo,    '#', &reason, log));
    RETURN_REASON_IF_ZERO(CheckLockoutForFailedPasswordAttempts(g_etcPamdCommonAuth,    pamTallySo,    '#', &reason, log));
}

// Catch-all (kept, typo fixed):
FREE_MEMORY(reason);
reason = DuplicateString("Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules are configured for failed-login lockout on this system. "
    "Manually set lockout for failed password attempts following specific instructions for this distribution. Automatic remediation is not possible");
return reason;
```

[Marius] Agree just that: do not do vertical alignments! Ony one space after comma as any other operatior, always. Please strictly respect style of existing code.

The same fork applies to `SetLockoutForFailedPasswordAttempts`:

```c
if (0 == CheckFileExists(g_etcSecurityFaillockConf, NULL, log))
{
    // Modern: write deny/unlock_time into faillock.conf (preserve other keys untouched).
    // Do NOT edit the PAM stack files in this branch.
    status = SetLockoutForFailedPasswordAttemptsViaFaillockConf(g_etcSecurityFaillockConf, log);
}
else
{
    // Legacy: existing PAM-file edits, unchanged (with /etc/pam.d/common-auth already in the list today).
    ... (today's loop over the 4 PAM files) ...
}
```

[Marius] Agree.

**Regression analysis on the >2M deployed fleet**:

- Distros that today produce **PASS** with the legacy path: they have inline
  `deny=`/`unlock_time=` on a `pam_{faillock,tally2,tally}.so` line in one of
  the 3 PAM files. On those distros, `/etc/security/faillock.conf` either does
  not exist (true legacy -> fork picks legacy, behaviour unchanged) or *does*
  exist but the operator has gone out of their way to also keep inline values.
  The latter is rare; in that case the modern path would also pass (because
  `pam_faillock.so` is referenced uncommented and `faillock.conf` defaults are
  compliant). So no observed PASS -> FAIL flip.
- Distros that today produce **FAIL** with the legacy path despite being
  correctly configured the modern way (this is the customer's bug): they
  flip from FAIL -> PASS. Intended.
- Distros where neither configuration is in place (lockout truly absent):
  catch-all sentence is returned, same as today.

The only edge case I can construct where the new code returns a different
verdict than the old code on a single VM is: `/etc/security/faillock.conf`
exists, `pam_faillock.so` is *not* referenced in any of the 4 PAM files, but
some operator has put a *literal* inline `deny=.../unlock_time=...` line on a
`pam_tally2.so` or `pam_tally.so` rule. Old: PASS via the tally2 inline line.
New: FAIL because the modern fork doesn't look at tally2/tally. I think this
combination doesn't exist in the wild (a system modern enough to ship
faillock.conf has long since deprecated `pam_tally2`/`pam_tally`), but
**please confirm** you accept this edge case before I implement.

**Alternative if you want zero-risk-of-flip**: instead of an `if/else` fork,
do "modern first, fall back to legacy if modern says no", i.e. *try both*.
That fully eliminates the edge case above at the cost of running more I/O on
modern systems and producing two-layer reason strings on failure. I do not
recommend it because it muddles the semantics, but mentioning the option.

### 13.2 -- Reply to your inline notes

> **[Marius @ root-cause] Keep that hard-coded default there, it is meant to
> catch if none of previous calls succeeded.**

Agreed, kept. It is still reachable in both forks when nothing compliant is
found (see 13.1 pseudocode). I would like to make two minimal text edits to
the hardcoded sentence -- please confirm:

1. Fix the typo: `"distrubution"` -> `"distribution"`.
2. Replace `"exist"` with `"are configured for failed-login lockout"`.
   Reason: on a modern RHEL 8 box that hits the catch-all (e.g.
   `pam_faillock.so` is not referenced at all in any PAM file), saying the
   module "doesn't exist" is factually wrong -- the module is installed, it
   is just not in the stack. The new wording is true on both legacy and
   modern systems. Final string:

   `"Neither pam_faillock.so, pam_tally2.so or pam_tally.so PAM modules are configured for failed-login lockout on this system. Manually set lockout for failed password attempts following specific instructions for this distribution. Automatic remediation is not possible"`

   [Marius] Your proposal looks perfect to me. I agree.

   If you would rather keep "exist", just say so.

> **[Marius @ "ReplaceMarkedLinesInFile"] Not more than one line per call is
> replaced -- and if the line does not exists, it is appended. So what you
> described (collapses the three pam_faillock.so lines into one broken line)
> does not happen. Be careful with that function and continue to use it
> properly.**

You are right that **at most one line is *replaced*** by
`ReplaceMarkedLinesInFile` (it sets `replacedLine = true` after the first
uncommented match and won't replace again -- FileUtils.c:1024-1037). I want
to flag one subtle behaviour I want us to be aligned on before I use it:

When a file already has **multiple** uncommented lines containing the same
marker, the first match is replaced and **subsequent matching lines are
skipped (dropped)** -- see FileUtils.c:1026-1030 where `skipLine = true` is
set on the second and later matches inside the `else` branch of
`replacedLine`. The net effect on a file with N>1 uncommented lines
containing the marker is **1 replaced + (N-1) deleted = 1 line remaining**.

On the customer's `/etc/pam.d/system-auth`:

```
auth    required    pam_faillock.so preauth silent
...
auth    required    pam_faillock.so authfail
...
account required    pam_faillock.so
```

Calling `ReplaceMarkedLinesInFile(systemAuth, "pam_faillock.so", "<one line template>", '#', true, log)`
would leave only one line. That is why I am proposing in 12.3 to **not call
this on the PAM stack at all** when we are on the modern path -- only call
it on `/etc/security/faillock.conf`, where each key (`deny`,
`unlock_time`) appears on its own one-time line, so the "drop later
matches" path is never taken. We agree?

If your reading of the function is that subsequent matching lines are *not*
dropped (e.g. the comment I am reading at line 1028 ("Already replaced this
line once") plus `skipLine = true` is doing something else), please walk me
through it so I correct my understanding before I write code that relies on
it.

[Marius] I understand what you are describing and this is a limitation of current code. Let's proceed carefully and limit to this use. Do we need a new function to replace lines? If we do, we can make a new one, simpler, just for this case.

> **[Marius @ helpers] Correct. Ideally for this fix we shall keep using same
> commonutils utils. However, if necessary, we can add more (and unit-tests).**

Agreed. My current plan uses only existing utils:

- `CheckFileExists` (for the fork decision)
- `CheckLineFoundNotCommentedOut` (for the `pam_faillock.so` reference check)
- `GetIntegerOptionFromFile` (for reading faillock.conf values)
- `CheckIntegerOptionFromFileLessOrEqualWith` (for `deny`)
- `ReplaceMarkedLinesInFile` (for writing faillock.conf)
- `OsConfigCaptureSuccessReason` / `OsConfigCaptureReason` (for reasons)
- `OsConfigLogInfo` / `OsConfigLogError` (for logs)
- `OSConfigTelemetryStatusTrace` (only on EINVAL/ENOMEM/EACCES, same conventions as everywhere else)

No new low-level helpers needed. New higher-level functions I am adding to
`PassUtils.c` (and exposing in `CommonUtils.h`):

- `int CheckLockoutForFailedPasswordAttemptsViaFaillockConf(const char* pamFile, const char* faillockConf, char** reason, OsConfigLogHandle log);`
- `int SetLockoutForFailedPasswordAttemptsViaFaillockConf(const char* faillockConf, int deny, int unlockTime, OsConfigLogHandle log);`

Both follow the existing signature pattern of
`CheckLockoutForFailedPasswordAttempts` and
`SetLockoutForFailedPasswordAttempts` so they can be unit-tested with
synthetic file paths via the existing `CreateTestFile`/`m_path` fixture.

[Marius] OK

### 13.3 -- Acceptance criteria (re 12.1)

Per your "fork" comment in 12.1, my proposal for the **modern** path is what
I previously labelled **C (mixed)**: for the modern path to PASS we require

1. `/etc/security/faillock.conf` exists, AND
2. `pam_faillock.so` is referenced (uncommented, comment-character `#`) in
   the PAM file under inspection, AND
3. If `deny` is uncommented in `faillock.conf`, it satisfies
   `0 <= deny <= 5` (same threshold as the existing inline path -- 12.8
   says do not tighten); if `deny` is not uncommented, treat the
   `pam_faillock` default of 3 as compliant.
4. If `unlock_time` is uncommented in `faillock.conf`, it satisfies
   `unlock_time > 0`; if not uncommented, treat the default 600 as
   compliant.

The legacy path's behaviour is unchanged. Confirm?

[Marius] Confirm.

### 13.4 -- Common-auth (re 12.2)

Confirmed B. I will add a single new constant alongside the existing trio:

```c
static const char* g_etcPamdCommonAuth = "/etc/pam.d/common-auth";
```

placed in `Asb.c` right below `g_etcPamdLogin` (Asb.c:492), and replace
the string literal `"/etc/pam.d/common-auth"` in `PassUtils.c`'s
`SetLockoutForFailedPasswordAttempts` array with this constant for
consistency (so remediation and audit use the same symbol).

[Marius] Add to Abc.c but that array in PassUtils.c, either use constant variables there (vs text constants) for all or none.

Add it to both forks' file-iteration list in the audit. On RHEL/Mariner
where the file does not exist, `CheckFileExists` will return `ENOENT` and
the helper will produce a non-fatal "file not found" reason fragment -- same
behaviour as today for `password-auth` on Debian/Ubuntu. No behavioural
impact on RHEL.

[Marius] There is no Mariner name anymore. It's 'Azure Linux'. 

### 13.5 -- Remediation values (re 12.3)

When the modern path applies, the remediation writes only:

- `deny = 3`
- `unlock_time = 900`

into `/etc/security/faillock.conf` using `ReplaceMarkedLinesInFile` with
`"deny"` and `"unlock_time"` as the markers (each is a single uncommented
line, so the multi-match-drop concern does not apply). All other keys
(`fail_interval`, `even_deny_root`, `root_unlock_time`, `silent`,
`audit`, ...) are left untouched. The values `3` and `900` match what
the legacy template already writes inline (faillock template) and what the
audit considers compliant.

The legacy remediation path (when `faillock.conf` does not exist) is
unchanged.

I am **not** touching `RemediateEnsureLockoutForFailedPasswordAttempts` in
`Asb.c` -- it remains a one-line
`return SetLockoutForFailedPasswordAttempts(log);`. The fork lives
entirely inside `SetLockoutForFailedPasswordAttempts`.

[Marius] Perfect.

### 13.6 -- Coding style (re 12.5)

Confirmed:

- 4-space indentation, no tabs.
- No trailing whitespace.
- Reason and log strings restricted to printable ASCII (no Unicode/UTF-8,
  no control chars, no `"` or `\` inside reason fragments where possible
  -- if a path contains a `\` or `"` it goes through `'%s'` and parson
  handles the JSON escape, but we do not introduce any such characters
  ourselves).
- Comment style matches existing comments (multi-line block-style preamble
  for new functions explaining what config layout they handle, single-line
  `//` for in-line notes).
- Existing macro / helper / constant naming conventions
  (`g_etcSecurityFaillockConf`,
  `CheckLockoutForFailedPasswordAttemptsViaFaillockConf`, etc.).

### 13.7 -- Telemetry (re 12.6)

Confirmed A. No new `OSConfigTelemetryStatusTrace` calls except in the
same fatal-error contexts the existing code uses
(`OSConfigTelemetryStatusTrace("fileName", EINVAL)` on NULL args,
`OSConfigTelemetryStatusTrace("malloc", ENOMEM)` on allocation failure,
`OSConfigTelemetryStatusTrace("FormatAllocateString", ENOMEM)` on
format-string allocation failure, etc.). Normal audit-returned-non-compliant
does not emit telemetry.

### 13.8 -- Unit test plan (re 12.7)

Skipping reason-string assertions per your note. Tests will only assert
integer return values, matching the style of the existing
`CheckLockoutForFailedPasswordAttempts` test.

Test file content is fully synthetic, created by
`CreateTestFile(path, data)` and removed by `Cleanup(path)`. The
`CommonUtilsTest` fixture already provides `m_path`, `m_path2`,
`m_path3`, `m_path4` at `/tmp/~test.test{,2,3,4}` -- I will use
`m_path` for the synthetic PAM file and `m_path2` for the synthetic
`/etc/security/faillock.conf` content. No system file is touched.

Concretely, two new tests added to `CommonUtilsUT.cpp`:

1. `TEST_F(CommonUtilsTest, CheckLockoutForFailedPasswordAttemptsViaFaillockConf)`
   - EINVAL on NULL args.
   - ENOENT when `pamFile` does not exist.
   - ENOENT when `faillockConf` does not exist.
   - PASS cases (return 0): combinations of compliant `faillock.conf`
     contents (set, default-by-commented-out) crossed with PAM files that
     reference `pam_faillock.so` uncommented (preauth-only,
     preauth+authfail+account, with extra whitespace, etc., mirroring the
     customer's RHEL 8 layout).
   - FAIL cases (return non-zero): `deny=6`, `deny=10`,
     `unlock_time=0`, `unlock_time=-1`, `pam_faillock.so` commented
     out in the PAM file, `pam_faillock.so` not referenced at all, etc.

2. `TEST_F(CommonUtilsTest, SetLockoutForFailedPasswordAttemptsViaFaillockConf)`
   - EINVAL on NULL args.
   - ENOENT when target file does not exist.
   - PASS case: synthetic `faillock.conf` with all lines commented out,
     after the call, the file contains `deny = 3` and `unlock_time = 900`
     uncommented, and any other content present before is preserved.
   - PASS case: synthetic `faillock.conf` with `deny = 99` and
     `unlock_time = 1` already uncommented, after the call those values
     are replaced with `3` and `900`, and any unrelated keys
     (`fail_interval = 60`, `even_deny_root`) are preserved.
   - A round-trip: after
     `SetLockoutForFailedPasswordAttemptsViaFaillockConf` succeeds,
     `CheckLockoutForFailedPasswordAttemptsViaFaillockConf` against the
     resulting file returns 0.

The existing `CheckLockoutForFailedPasswordAttempts` test is **not**
modified -- same `goodTestFileContents` / `badTestFileContents`, same
assertions. Pure regression guarantee.

### 13.9 -- Commit policy (re 12.9)

Confirmed. I will edit files only, no `git add`, no `git commit`, no
push. You own staging and commit.

---

### Open items I need your sign-off on before I start coding

1. **13.1**: detection criterion = "`/etc/security/faillock.conf` exists" -- OK?
2. **13.1**: strict if/else fork (recommended) vs. "try modern then fall
   back to legacy" -- OK with strict fork?
3. **13.1 / 13.2 first sub-point**: minor wording edit to the catch-all
   sentence (fix typo + "exist" -> "are configured for failed-login lockout")
   -- OK?
4. **13.2 second sub-point**: my reading of `ReplaceMarkedLinesInFile`
   (later uncommented marker matches are dropped) -- confirm whether you
   agree, so we share the same mental model before I write code that relies
   on it.
5. **13.4**: introducing `g_etcPamdCommonAuth` constant and using it from
   both `Asb.c` and `PassUtils.c` -- OK?
6. **13.5**: the two hardcoded values written to `faillock.conf` will be
   `deny = 3` and `unlock_time = 900`. OK?

Once you confirm 1-6, I will start the implementation. Estimate is ~150 LoC
across PassUtils.c, CommonUtils.h, Asb.c and ~200 LoC of new tests in
CommonUtilsUT.cpp, plus the 1-line constant in Asb.c.

[Marius] All OK. Go!


## 14. We have unit test failures

[ RUN      ] CommonUtilsTest.CheckLockoutForFailedPasswordAttemptsViaFaillockConf
[2026-05-18 14:41:07-0700][ERROR][PassUtils.c:337] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: invalid arguments
[2026-05-18 14:41:07-0700][ERROR][PassUtils.c:337] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: invalid arguments
[2026-05-18 14:41:07-0700][ERROR][PassUtils.c:337] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: invalid arguments
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:513] CheckFileExists: file '~file_that_does_not_exist' is not found
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:344] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: '~file_that_does_not_exist' does not exist
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:351] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is not referenced uncommented in '~file_that_does_not_exist'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '5' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '5' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '900' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '900' in '/tmp/~test.test2' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:393] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is referenced uncommented in '/tmp/~test.test' and '/tmp/~test.test2' has 'deny' set to 5 (less than or equal to 5) and 'unlock_time' set to 900 (greater than 0)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1799] GetStringOptionFromBuffer: 'deny' is found but commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1799] GetStringOptionFromBuffer: 'deny' is found but commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1881] GetIntegerOptionFromFile: 'deny' not found in '/tmp/~test.test2'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:370] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'deny' not set in '/tmp/~test.test2', the pam_faillock default of 3 applies
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1799] GetStringOptionFromBuffer: 'unlock_time' is found but commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1881] GetIntegerOptionFromFile: 'unlock_time' not found in '/tmp/~test.test2'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:388] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'unlock_time' not set in '/tmp/~test.test2', the pam_faillock default of 600 applies
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:393] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is referenced uncommented in '/tmp/~test.test' and '/tmp/~test.test2' has 'deny' set to 3 (less than or equal to 5) and 'unlock_time' set to 600 (greater than 0)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '5' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '5' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '1' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '1' in '/tmp/~test.test2' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:393] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is referenced uncommented in '/tmp/~test.test' and '/tmp/~test.test2' has 'deny' set to 5 (less than or equal to 5) and 'unlock_time' set to 1 (greater than 0)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '6' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '6' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:361] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'deny' from '/tmp/~test.test2' is 6, expected a value less than or equal to 5
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '3' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '3' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '0' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '0' in '/tmp/~test.test2' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:379] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'unlock_time' from '/tmp/~test.test2' is 0, expected a positive value
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '3' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '3' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '-1' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '-1' in '/tmp/~test.test2' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:379] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'unlock_time' from '/tmp/~test.test2' is -1, expected a positive value
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:351] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is not referenced uncommented in '/tmp/~test.test'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1577] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 but is commented out with '#'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:351] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is not referenced uncommented in '/tmp/~test.test'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '100' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '100' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:361] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'deny' from '/tmp/~test.test2' is 100, expected a value less than or equal to 5
/home/mn/azure-osconfig/src/common/tests/CommonUtilsUT.cpp:2194: Failure
Expected equality of these values:
  0
  CheckLockoutForFailedPasswordAttemptsViaFaillockConf(m_path, m_path2, nullptr, nullptr)
    Which is: 2
[  FAILED  ] CommonUtilsTest.CheckLockoutForFailedPasswordAttemptsViaFaillockConf (1 ms)
[ RUN      ] CommonUtilsTest.SetLockoutForFailedPasswordAttemptsViaFaillockConf
[2026-05-18 14:41:07-0700][ERROR][PassUtils.c:575] SetLockoutForFailedPasswordAttemptsViaFaillockConf: invalid argument
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:437] SetFaillockConfOption: '~file_that_does_not_exist' does not exist
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:437] SetFaillockConfOption: '~file_that_does_not_exist' does not exist
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:587] SetLockoutForFailedPasswordAttemptsViaFaillockConf('~file_that_does_not_exist', 3, 900) returning 2
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'deny = 3' written to '/tmp/~test.test2' (in place)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'unlock_time = 900' written to '/tmp/~test.test2' (in place)
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:587] SetLockoutForFailedPasswordAttemptsViaFaillockConf('/tmp/~test.test2', 3, 900) returning 0
/home/mn/azure-osconfig/src/common/tests/CommonUtilsUT.cpp:2227: Failure
Expected equality of these values:
  nullptr
    Which is: NULL
  strstr(contents, "unlock_time = 60\n")
    Which is: 0x5d8d08688f46
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'deny = 5' written to '/tmp/~test.test2' (appended)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'unlock_time = 1200' written to '/tmp/~test.test2' (appended)
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:587] SetLockoutForFailedPasswordAttemptsViaFaillockConf('/tmp/~test.test2', 5, 1200) returning 0
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'deny = 3' written to '/tmp/~test.test2' (appended)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'unlock_time = 900' written to '/tmp/~test.test2' (appended)
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:587] SetLockoutForFailedPasswordAttemptsViaFaillockConf('/tmp/~test.test2', 3, 900) returning 0
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'deny = 3' written to '/tmp/~test.test2' (in place)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'unlock_time = 900' written to '/tmp/~test.test2' (in place)
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:587] SetLockoutForFailedPasswordAttemptsViaFaillockConf('/tmp/~test.test2', 3, 900) returning 0
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'deny = 5' written to '/tmp/~test.test2' (appended)
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:232] InternalSecureSaveToFile: directory '/tmp' exists, is owned by user (0, 0) and has access mode 01777
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:674] SetAccess: desired '/tmp/~test.test2' ownership (owner 0, group 0 with access 00644) already set
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:551] SetFaillockConfOption: 'unlock_time = 900' written to '/tmp/~test.test2' (appended)
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:587] SetLockoutForFailedPasswordAttemptsViaFaillockConf('/tmp/~test.test2', 5, 900) returning 0
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 26 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 128 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1583] IsLineNotFoundOrCommentedOut: 'pam_faillock.so' found in '/tmp/~test.test' at position 179 and it's not commented out with '#'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '5' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '5' in '/tmp/~test.test2' for 'deny'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:508] CheckFileExists: file '/tmp/~test.test2' exists
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1787] GetStringOptionFromBuffer: found '900' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][FileUtils.c:1876] GetIntegerOptionFromFile: found '900' in '/tmp/~test.test2' for 'unlock_time'
[2026-05-18 14:41:07-0700][INFO][PassUtils.c:393] CheckLockoutForFailedPasswordAttemptsViaFaillockConf: 'pam_faillock.so' is referenced uncommented in '/tmp/~test.test' and '/tmp/~test.test2' has 'deny' set to 5 (less than or equal to 5) and 'unlock_time' set to 900 (greater than 0)
[  FAILED  ] CommonUtilsTest.SetLockoutForFailedPasswordAttemptsViaFaillockConf (2 ms)

## 15. We have one missed bug

Line 250 in D:\GitHub\azure-osconfig\src\common\commonutils\PassUtils.c:

```c
            if ((commentCharacter == line[0]) || (EOL == line[0]))
            {
                status = 0;
                continue;
            }

```

If all lines found here are commented out, the function with end with status 0 but no reason. 
