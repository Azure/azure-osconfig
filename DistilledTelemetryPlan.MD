# Distilled OSConfig Telemetry: Design and Restart Handoff

Created: 2026-09-30
Refreshed in full against the October 5, 2026 owner evidence;
TLS distillation source update: October 6, 2026
Deep diagnostics/interface update: October 6, 2026; owner rebuild pending

Canonical document: `D:\GitHub\azure-osconfig\DistilledTelemetryPlan.MD`

## 1. Current state and evidence

**The C telemetry replacement, producer/worker SEND integration, package wiring,
and MinTls fallback are implemented. Owner builds and execution have progressed
well beyond the old "not yet owner-build-validated" checkpoint.** The owner
reported tests passing earlier on October 5 and demonstrated fresh No. 66
standalone events in Aria later that afternoon. On October 6, the owner supplied
a fresh standalone Aria sample after the first TLS distillation pass and a
passing rerun of all three corrected diagnostics tests.
The subsequent deep-diagnostics/interface changes described in section 3 have
not yet received owner build/runtime results; the earlier evidence is not their
validation.

This is not yet a release-readiness signoff. The latest complete post-edit
regression summary, final 10,000-event counts, real packaged-policy-host results,
and full supported-platform results have not been supplied. Do not confuse these
missing results with an implementation that has never built or sent events.

This document consolidates the former overlapping chronological checkpoints.
Current instructions below supersede earlier DNS-only, in-host TLS, OS-only,
separate forced-provider build, 100-event test, and pre-cutover proposals.
Historical evidence is summarized in section 11 rather than presented as pending
implementation work. TLS distillation started October 6; its first source pass
and remaining work are recorded in section 3.
After that is complete, the owner wants to clean up and consolidate all existing
telemetry event writes, as recorded in section 14.

### Current implementation

| Surface | Implemented state |
| --- | --- |
| Producer | `Telemetry.c/h` preserve the four existing named events, string property contracts, invocation metadata and disabled-build behavior. No JSONL spool or shutdown upload. |
| Event protocol | `TelemetryEvent.c/h` pack bounded memory-only IPC and validate exact named schemas before Common Schema encoding. |
| Encoder | `TelemetryEncoder.c/h` encode one uncompressed Common Schema 3 Bond Compact Binary record, with bounded writes and explicit input failures. |
| Worker | `TelemetryWorker.c` owns lazy child startup, READY/RESOLVE/SEND IPC, budgets, suppression and exact-child cleanup. |
| Executable | `TelemetryWorkerMain.c` implements the packaged `OSConfigTelemetry` C executable, owns DNS/TLS/HTTP and its local log. |
| Transport | `TelemetryTransport.c` implements direct or HTTP CONNECT routing, verified TLS, connection reuse, acknowledgment handling and no replay. |
| TLS | OS OpenSSL 3/1.1 are preferred. MinTls fallback selection is unchanged. The first October 6 reduction has standalone forced-MinTls Aria receipt and 3/3 diagnostics tests passing. The later deep-diagnostics/helper-interface pass awaits owner rebuilding and runtime results. |
| Integration | Existing ASB/NRP and previous-crash producer paths use the C implementation. Actual host compatibility remains a runtime release gate. |
| Packaging | Existing executable name, four policy ZIP names, resource-library names and staging destinations are preserved. The parent CMake target supplies the old artifact-copy contract from the new executable. |
| Removed stack | Legacy SDK/C++ uploader, its external build/test import and telemetry-only curl, bundled OpenSSL, zlib, SQLite and nlohmann JSON build dependencies are removed. |
| Tests | Common encoder/event/HTTP/proxy/resolver/worker/TLS/MinTls tests, producer tests and loopback helpers exist. The optional live sender is not packaged. |

### Latest owner evidence

- October 6, 09:23 -0700: owner supplied a fresh No. 66 Aria sample after
  distillation: `StatusTrace`, sequence/LineNumber 67, SDK `OSConfig-C/0.1`,
  ResultCode string `731001`, version `1.0.5.20261006-g6d550953`.
  Event time was `2026-10-06T16:22:29.6106307Z`; recorded ingestion was
  `2026-10-06T16:22:29.6140000Z`. This demonstrates downstream receipt from
  the standalone sender, whose source forces MinTls. The payload itself does
  not identify the TLS provider; it is not a full-run count or production-worker
  validation.
- October 6, 09:21 -0700: owner reran all three `MinTlsDiagnosticsTest` cases;
  all passed after the test-only RNG fixture correction (section 3).
- October 5, 13:52: owner said "Tests all passed now" after the earlier comparison/
  brace cleanup. No exact command, count or platform matrix accompanied this
  statement. It predates the later worker-main edits and is not their regression
  result.
- October 5: owner started another build. Subsequent test output proves
  execution reached the live-sender loopback tests; four failed because the sender
  marker was No. 66 while both fixtures still expected No. 3.
- That mismatch was identified, and the latest source inspection finds No. 66 in
  all three locations. A complete passing rerun has not been supplied.
- October 5: owner reported "Still working" and supplied a No. 66 Aria
  sample: `StatusTrace`, sequence/LineNumber 152, SDK `OSConfig-C/0.1`,
  ResultCode string `731001`, version `1.0.5.20261005-g406f783d`.
  Event time was `2026-10-05T23:12:01.7240255Z`; recorded ingestion was
  `2026-10-05T23:12:01.7540000Z`.
- This is actual downstream receipt of that event. It is not proof that all
  10,000 records arrived, or that the production worker/OS-provider path was
  exercised. The standalone sender forces MinTls and bypasses worker IPC.

Do not copy raw portal payloads, ingestion tokens, device identifiers or client
addresses into this document. The synthetic marker and test metadata suffice.

## 2. Workspace and operating agreement

- Actual repository: `D:\GitHub\azure-osconfig`. The CLI may start in
  `D:\MSCodeHub\SKATE`; that is not the repository for this work.
- Last completed readiness-review status check: clean source revision
  `77372028`. The owner commits independently. Recheck HEAD/worktree before
  editing; older staged-file counts and branch snapshots are not current state.
- This document was moved from `D:\GitHub\DistilledTelemetryPlan.MD` into the
  repository at the owner's direction. Use only the canonical path above.
- The owner builds, tests and runs live sends. No assistant compilation, WSL,
  installation, runtime testing, live send, publication or commit without further
  authorization. Read-only source/diff checks are not runtime evidence.
- Stage source changes only as requested; no assistant commits. The recent
  worker-main fixes were intentionally left unstaged at completion and later
  incorporated by the owner's workflow.
- Preserve unrelated owner changes. In particular, do not inspect or modify the
  owner's unrelated `src\adapters\agent\Agent.c` fix.
- The October 6 owner request authorizes TLS distillation and logging implementation.
  Event-write consolidation remains the subsequent phase, not concurrent work.
- Keep this document current with meaningful results and restart instructions;
  do not create separate planning documents.

### Coding direction

Owned C/C++ files have undergone manual function-by-function cleanup. Use braces
and multiline control bodies, blank lines between blocks, explicit grouping of
logical operands, constants on the left of equality comparisons, and initialized
locals at function entry when this preserves behavior and resource lifetimes.

Combine adjacent status calls/checks when equivalent. Do not move unconditional
budget charging behind a short-circuit success guard. Preserve errno captures,
deadline creation points, per-iteration state resets and C++ RAII lifetimes.
Do not wrap a signature merely to put one final argument on a new line.
Avoid unnecessary early-return/goto rewrites when they obscure cleanup.

Keep logging format templates on one source line. The latest owner direction
uses plain printf specifiers with matching argument types/casts, not PRI macro
concatenation, and `label: value` rather than `label=value`. This was applied to
TelemetryWorkerMain.c; do not claim every older file has already been converted.
Pass the available log handle, save errno before logging, and identify the actual
failing operation. No caller-side NULL-log guards merely to suppress diagnostics.
Do not recursively generate telemetry from telemetry's own diagnostics.

The former imported TLS core was excluded from earlier formatting passes.
Retained derivative files now identify their original upstream paths and local
modifications. Preserve these notices and the retained cryptographic operations.

## 3. Active work: distill the retained TLS implementation

**Implementation authorized October 6:** remove code we do not need, flatten the
retained implementation into the mintls root, and add OSConfig logging inside
the retained implementation. Make it an
OSConfig-maintained derivative while preserving its upstream license and
attribution. Do not replace this task with removing the fallback, switching to
OS-only TLS, or inventing cryptographic primitives from scratch.

The earlier deferred start is superseded. The first dependency/profile reduction,
flat layout and logging implementation are now in the worktree, uncommitted.
The owner's October 6 build reached test execution. Three new diagnostics tests
failed during shared setup because the fixture omitted the required RNG callback
(`MBEDTLS_ERR_SSL_NO_RNG`, -29696). A test-only callback now satisfies setup and
fails the test if randomness is actually requested; these are logging-only tests.
On October 6 at 09:21 -0700, the owner supplied the focused rerun:
`sudo ctest -R '^MinTlsDiagnosticsTest\.' --output-on-failure`.
All three diagnostics tests passed, with zero failures. This confirms the fixture
correction and the three logging contracts, not the complete TLS/telemetry suite
or the supported-platform matrix. Current work must preserve the owner's
independent changes.
The public MinTls functions already accept OsConfigLogHandle. The owner selected
propagating logging through internal contexts/callbacks rather than adding a log
argument to every retained core function.

The owner also requested avoiding static internal functions for crash-stack
diagnostics. Ordinary C implementation functions now have external linkage;
colliding SHA signal-handler names and general wrapper-helper names were made
distinct. Static data and existing header-only inline primitives were not changed.
No new linker export/optimization policy was introduced; removing static alone
is not proof of symbol visibility in a stripped packaged crash stack.

### Existing ownership and build boundary

All retained code and LICENSE now live directly in `src\common\mintls`; the
`core` subtree is gone. CMake directly compiles the derivative modules and our
`MinTls.c`/`MinTlsLog.c` into the private static library. Includes, common tests
and the normal license-install path were updated. No MC ZIP changes were made.
There is no upstream clone, download, ExternalProject, installed Mbed TLS package
or upstream build invocation.

The initial inventory verified 40 core C modules and 124 core headers.
The first pass removed three disabled C modules and 62 unused headers.
After the first pass, the root had 39 C files (37 derivative modules plus two owned
integration/logging files) and 64 headers (62 derivative headers plus
MinTls.h/MinTlsConfig.h). The later diagnostics pass adds MinTlsLog.h, making
65 headers and 104 C/header files.
C/header source decreased from 5,061,019 to 2,414,724 bytes at this checkpoint,
approximately 52%. These are source bytes, not a measured executable reduction.

### October 6 source changes and validation boundary

- Traced the adapter/core-test include roots and explicit configuration-derived
  dependencies. Removed disabled modular-bignum and block-cipher modules, PSA
  headers/glue, unused TLS 1.3 data and headers, and profile-inactive branches for
  server/DTLS, other ciphers/curves, filesystem APIs, threading and self-tests.
- Conditional reduction used an explicit reviewed feature set, leaving unknown
  compiler/platform/arithmetic conditions intact. Retained cryptographic
  statements were not translated. A source comparison confirmed matching retained
  tokens in 93 files before subsequent linkage/targeted edits. Multiline directive
  comment tails exposed during reduction were found and repaired.
- Public MinTls APIs retained their log arguments. The first pass's per-context
  configuration callback routed 76 existing internal return-code sites to the
  caller's log, with additional seed/configuration, socket and certificate-stage
  diagnostics. Handles refresh on synchronous operations and are never owned or
  closed by MinTls. Core callbacks preserve errno.
- Readiness/success/close-notify do not produce core errors. Generic message,
  payload, key/MPI and certificate dumps stay disabled even at debug level.
- Six additional tests cover the four supported suites, three required NIST
  curves, retained hash/profile policy, caller-routed core errors/errno, suppression
  of readiness and sensitive dumps, and public API failure diagnostics.
- Source checks confirm CMake membership, flat include paths, unchanged license
  text, absence of the old subtree, no ordinary static C function definitions or
  cross-file helper collisions, and clean changed-file whitespace. These checks
  do not prove compilation, linking, runtime TLS correctness or constant-time
  behavior.
- No compiler/CMake was available through this session's PATH. No tools were
  installed, no Linux build/tests/live sends ran, and no commit was created.
  These statements concern assistant execution. The owner's subsequent build/test
  run and diagnostics-fixture correction are recorded above; a complete passing
  result is still needed before deeper pruning.

### October 6 deep diagnostics and ownership notices

The owner explicitly selected originating diagnostics inside low-level helpers,
with context propagated through their interfaces. The new source pass replaces
the earlier configuration debug bridge with an explicit, stack-owned
`MinTlsDiagnostics` argument throughout failure-capable helper and callback
interfaces. Public MinTls signatures, TLS selection, trust policy and transport
deadlines remain unchanged. Core callers and tests were updated together.

- A zero-initialized frame per status-returning call keeps operation/file/line
  and numeric error metadata, not buffers or cryptographic values. Success
  discards speculative failures. A failed outer operation emits through the
  caller's borrowed log; no global/thread-local logging state or diagnostic
  allocation was introduced.
- Direct failures, propagated failures and explicit error translations are
  distinguished so a recovered parser probe does not become a later failure's
  explanation. Partial PEM parsing preserves its first certificate failure and
  original positive count. TLS alert-code returns are failures, while positive
  byte counts are not. Alert-send failures use a separate scope so they cannot
  replace the primary diagnostic.
- WANT_READ/WANT_WRITE, success and authenticated close-notify remain quiet.
  Message/key/MPI/certificate/payload dumps remain disabled. Diagnostic output
  preserves errno. Constant-time RSA unpadding is deliberately left untouched;
  its RSA caller reports failures after sensitive-buffer cleanup.
- The audit found two discarded results directly coupled to failure handling:
  the Finished-message PRF result and the EC key-pair check's group-copy result.
  Both now propagate their errors. No successful cryptographic calculation was
  changed. Added regressions exercise these paths, nested ASN.1 causes, recovered
  probes, error-code translation, entropy callback context, positive alert
  results, first-PEM-error preservation, secondary-alert isolation and
  sensitive-data suppression.
- All 104 C/header files start with the owner's exact, unchanged two Microsoft
  copyright/MIT lines. All 99 retained derivative files also retain upstream
  copyright/SPDX and original-file provenance, with a prominent
  `Modified by Microsoft for OSConfig on 2026-10-06` notice. The complete upstream
  LICENSE is unchanged. No governance exemption is asserted.
- Source-only checks cover named-function/callback wiring, scoped returns,
  error-return macros, exact leading notices and preservation of retained
  source tokens apart from diagnostic plumbing and the two reviewed
  failure-propagation fixes. These are not compilation or runtime evidence.
  No compiler/CMake is available on this session's PATH; the owner remains
  responsible for builds, tests and authorized live sends.

**Resume/validation boundary:** rebuild after these private interface changes and
run the MinTls core/diagnostics and TLS/telemetry regression coverage. Repeat the
owner's forced-MinTls live-delivery check after the new build. The 09:21
diagnostics result and 09:23 Aria sample validate the earlier pass only.
Further source pruning and event-write consolidation remain pending.

This is the first fixed-profile distillation, not a claim that every remaining
general-purpose function is required by a telemetry send. A further function-level
trace must include callback tables, inline dependencies and certificate-driven
paths before removing remaining unused functions. General internal identifiers,
some disabled configuration guards, and upstream helper documentation still
remain. Do not report the full distillation or release validation as complete.

Provenance: official Mbed TLS 3.6.7 release archive,
SHA-256 `a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`.
This identifies the imported source, not a fetch/build dependency. The core
retains Apache-2.0 OR GPL-2.0-or-later licensing; OSConfig uses Apache-2.0.
Our adapter/integration is MIT-licensed.

The owner wants to avoid repeating the substantial exception work associated
with Parson's `src\common\parson\cgmanifest.json`. That manifest registers a
production Git component with upstream URL and commit. Local builds and an owned
fork alone do not establish a different governance classification. No applicable
internal classification criteria have been provided. Preserve provenance and do
not promise an exemption based on trimming, renaming, or omitting registration.

### Ordered work

1. Recheck source revision, owner edits and instructions. Read the adapter,
   profile, core build list and tests before choosing deletions.
2. Inventory source/header membership and transitive includes from all compiled
   modules and test consumers. Include configuration-adjustment headers, compiler
   and architecture branches. Headers absent from CMake can still be required.
3. Identify wholly unreferenced files first. Distinguish unreferenced code from
   disabled feature branches and supported platform variants. Do not delete based
   solely on names, a single host's preprocessor output or binary symbol absence.
4. Continue with small, manually reviewed function/block removals after the
   first profile reduction is owner-validated. Preserve configuration consistency
   and compile-time checks. Do not blindly strip preprocessor branches or
   translate cryptographic algorithms.
5. For modified upstream files, retain applicable copyright/license notices and
   add prominent notices stating that OSConfig changed them. Record meaningful
   local departures in existing component documentation for future maintenance.
6. Preserve and refine the implemented per-context logging. Identify failures at
   useful boundaries, use the available log handle, and avoid hot-loop traces,
   unnecessary duplicate diagnostics, secret material and payload contents.
7. Review each batch for behavioral, lifetime, platform and configuration changes.
   Run authorized source checks; have the owner build and run the affected
   existing suites. Record precisely what was removed and what was actually
   validated. Do not claim source equivalence from delimiter/include checks alone.

### Required invariants and pruning cautions

- Preserve TLS 1.2 client functionality, authenticated ECDHE-RSA/ECDSA,
  AES-GCM, the currently required curves/hashes, secure entropy and key clearing.
- Preserve chain/date/DNS/IP verification, usage checks, SNI rules and HTTP/1.1
  ALPN. Numeric identities require IP SANs, not numeric DNS SAN/CN substitutions.
- Preserve system trust selection, explicit-override failure behavior and
  FIPS/crypto-policy refusal. No bundled roots or accept-any-certificate mode.
- Keep the current normal TLS record capacities; certificate chains are not
  controlled by this client. Do not reduce buffers merely to hit a footprint goal.
- SHA-1 parsing supports certain legacy trust anchors even though SHA-1 peer-chain
  signatures are rejected. Its name is not sufficient evidence for deletion.
- Do not remove runtime/compiler/architecture variants needed by the supported
  Linux matrix. Ubuntu 14/GCC 4.8 and EL7-era libc constraints remain relevant.
- Preserve operation deadlines, partial I/O continuation, authenticated EOF,
  non-replay semantics, borrowed socket ownership and worker-only TLS lifetime.
- Do not change production provider-selection policy or weaken failure checks.
  Production uses `forceMinTls=false`; current tests and live sender use `true`.
- Do not add fetched dependencies or another configuration framework to simplify
  the distillation. Prefer a smaller implementation with the existing interface.

### Validation for distillation

The owner should exercise the existing MinTls known-answer/profile cases,
TLS positive/negative certificate cases, timeouts/backpressure/truncation,
HTTP CONNECT and credential separation, worker SEND and producer tests.
Confirm retained behavior on supported architectures/toolchains, not only the
local modern host. Validate both included and excluded fallback production
builds; test builds currently require fallback inclusion.

Memory-safety/sanitizer results, protocol interoperability and review of imported
security fixes are required assurance work, not proof supplied by successful
formatting. Any new live run remains explicit and owner-run. Keep the full
10,000-event offline case disabled in normal runs unless the owner changes that.

## 4. Architecture, lifetime and failure contracts

```text
Policy producer
  -> invocation metadata and bounded named-event properties
  -> memory-only IPC to the invocation's OSConfigTelemetry C worker
  -> worker encodes Common Schema, resolves, connects, verifies TLS and POSTs
  -> worker returns operation/disposition information
  -> parent accounts elapsed telemetry work and continues the policy
```

One serialized context per invocation, no worker threads, queues, durable event
storage, replay, background keepalive, or shutdown upload. The host may have other
threads; our single-caller API is not a claim that gc_worker is single-threaded.
Do not depend on the long-lived policy host exiting or pin TLS libraries into it.

Initialization allocates parent state and metadata. Worker creation and network
connection are lazy, on the first operation that needs them, not an eager network
exchange in a loader callback. Reuse the worker/connection during the invocation.
Provider handles remain private to the exec'd worker until its exit.

The private same-host protocol has a fixed native-endian frame with magic,
version, operation, sequence, status, size and monotonic deadline. READY is
mandatory; RESOLVE and SEND are implemented. New SO/worker artifacts must match.
The old file-input CLI is not retained. Never treat a process exit or socket write
as confirmed event acceptance.

The worker owns system resolution, including the OS NSS/hosts configuration.
There is no custom DNS protocol, hard-coded collector address or resolver thread.
Worker timers and parent IPC deadlines cover blocked resolver/provider work
subject to kernel scheduling and uninterruptible-I/O limitations.

The parent uses no-signal socket writes and does not alter host signal policy.
The exec'd child controls its own SIGALRM/SIGPIPE. Startup closes unrelated
inherited descriptors through `/proc/self/fd`, preserving IPC, redirected standard
streams and the log descriptor. Readable procfs is a runtime requirement.
The current parent requires default, waitable SIGCHLD and ownership of its exact
PID; it does not reap unrelated children.

### Implemented timing

| Limit | Current behavior |
| --- | --- |
| Event operation | Up to 500 ms, bounded further by remaining invocation allowance/lifetime. |
| Invocation telemetry allowance | 500 ms cumulative telemetry execution, including preparation and failure paths. Audit work between calls does not consume it. |
| Worker lifetime | 10 minutes, including idle audit time. |
| Current-event replay | None. |
| Standalone live sender | 15 seconds per operation, no aggregate live-run cap. Not production timing. |

The previously discussed one-second first event and 250 ms separate connection
target are not implemented approvals. Do not increase limits simply to pass a
test. The 500 ms allowance is not an unconditional wall-clock guarantee:
blocking child reaping after SIGKILL remains a release follow-up.

Ordinary transport failure drops the current event; a later event may reconnect.
An ambiguous/malformed SEND IPC result suppresses the rest of the invocation
because lost collector controls cannot be reconstructed. Budget/lifetime and
suppression do not reset when a worker restarts.

HTTP 429/503 and retry/kill controls conservatively suppress subsequent sends for
the invocation, including controls captured before acknowledgment parse failure.
The earlier missing failure-path suppression latch has been fixed. This is not
token-scoped, duration-accurate or cross-invocation enforcement. Clock guidance is
preserved, but local UTC remains the timestamp source; no clock correction exists.

## 5. TLS, routing and trust

### OS provider and MinTls selection

`TelemetryInitialize(bool forceMinTls, OsConfigLogHandle log)` carries provider
choice through worker creation/startup, transport creation and reconnects.
Production callers pass false. True adds the private `--force-mintls` argument
and bypasses OS discovery, not trust/policy/deadline checks.

`OSCONFIG_TELEMETRY_MINTLS` controls fallback inclusion and defaults ON.
`OSCONFIG_TELEMETRY_FORCE_MINTLS` and duplicate forced-provider libraries were
removed. Do not suggest those old build instructions. Tests use the same
TLS/transport libraries and pass the runtime Boolean.

The OS adapter resolves the supported public OpenSSL 3/1.1 ABI at runtime,
loads OS configuration/trust, requires chain and endpoint verification, enforces
at least TLS 1.2 without lowering stricter OS policy, disables compression and
offers only HTTP/1.1. No retry through MinTls follows initialization/trust/
handshake/I/O failures.

MinTls uses the selected in-tree Mbed TLS subset and TLS 1.2 client profile.
No roots ship with it. Trust precedence is explicit API CA file, SSL_CERT_FILE,
then a recognized OS PEM bundle. Invalid explicit input does not select unrelated
trust. Directory-only SSL_CERT_DIR is unsupported; with both variables the file
is the complete trust input. Empty, nonregular, partially parsed or over-4-MiB
bundles fail closed.

MinTls is not FIPS validated. Explicit OpenSSL policy environment overrides,
recognized system FIPS/crypto-policy markers, and enabled/unreadable kernel FIPS
settings prevent its use. Arbitrary site policy cannot be inferred. Sites imposing
other restrictions must exclude the fallback rather than treating it as a policy
substitute. OS-provided library maintenance remains the deployment's concern;
we own fixes to the bundled derivative.

NSS/GnuTLS were investigated but are not implemented fallback providers.
Do not revive an NSS integration project merely because the historical research
listed it as a candidate.

### Routing

Direct HTTPS and plain HTTP CONNECT proxies are implemented. Environment
precedence is first nonempty https_proxy/HTTPS_PROXY, then all_proxy/ALL_PROXY.
no_proxy/NO_PROXY matching is evaluated first for the fixed collector hostname.
Configured unsupported routing fails explicitly, never silently goes direct.

HTTP proxy parsing supports ASCII hostname/IPv4, bracketed IPv6, ports and
bounded percent-decoded Basic credentials. Default proxy port 1080 follows the
inspected curl behavior. Basic credentials to a plain HTTP proxy are not encrypted
on that proxy hop; the collector token/event travel only inside verified TLS.
HTTPS/SOCKS proxies, PAC, NTLM/Kerberos and general curl URL compatibility are not
implemented. Determine required deployment coverage rather than assuming all
networks are supported.

Addresses are tried sequentially. A stalled first address can consume the full
deadline before a reachable alternative is tried. There is no Happy Eyeballs
racing or per-address budget policy yet.

## 6. Event and collector contracts

The cutover preserves `BaselineRun`, `RuleComplete`, `StatusTrace` and
`CrashDetected`, including their existing custom string properties. The worker
validates exact named schemas; `TelemetryEvent.c` is authoritative for field lists.
Missing optional custom values use the existing N/A representation. Oversized
events are rejected, not truncated. Common metadata includes distro, invocation
correlation, version and reporting timestamp.

The earlier proposal to remove RuleComplete or rename StatusTrace is not an
implemented or approved cutover change. Error-volume limits, summary budget
reservation and downstream event cleanup are separate decisions. Early events can
exhaust the allowance before the final BaselineRun. A time budget alone is not an
event-count cap.

SDK-generated device/OS/network/app enrichment is not reproduced wholesale.
The random per-run epoch is not a stable device identifier. Verify required
identity/privacy mappings and reporting usefulness rather than assuming a
successful synthetic POST proves adoption-query compatibility.

### Wire contract and parser invariants

- Fixed Aria endpoint: `https://mobile.events.data.microsoft.com/OneCollector/1.0/`.
  Kusto direct ingestion was explored as an alternative but is not implemented.
- One uncompressed Common Schema 3 Bond Compact Binary record per HTTP/1.1 POST.
  No gzip/DEFLATE, chunked request, Expect wait, redirect following or HTTP/2.
- Full configured token goes in APIKey. Record iKey is `o:` plus its tenant prefix.
  The synthetic marker does not select PROD/NONPROD.
- SDK identity is `OSConfig-C/0.1`; preserve honest owned-client identification.
  Record time is 100-ns ticks since year 1; Upload-Time uses Unix milliseconds.
  The epoch offset is 621355968000000000. Producer Timestamp is a separate field.
- The encoder supports bounded typed values and explicit little-endian wire
  encoding; producer schemas retain strings. Do not round 64-bit values through
  JSON doubles or assume C struct padding is a wire format.
- Incremental response framing supports content length, chunks and authenticated
  close-delimited EOF, with bounded interim responses and trailers. Reject
  ambiguous lengths, invalid encodings, excess bytes and truncation.
- Completion and acceptance are separate. Sparse `{"acc":1}` is accepted;
  omitted zero counts are allowed, present invalid types/contradictions are not.
  Empty HTTP 200 is unconfirmed. Explicit rejection/efi/token failures cannot
  become acceptance. No outcome causes replay.
- JSON envelope/depth/UTF-8 checks surround Parson's parsing limitations. Never
  relax validation merely because a generic HTTP 200 was received.

Current source bounds include 32 properties, 16 KiB encoded event, 8 KiB HTTP
headers/body, 2 KiB lines, 32 KiB total response wire, 16 stored controls,
64 headers, four informational responses and JSON depth 16. Consult headers and
parser source before changing them. Older 64-KiB response and 256-KiB working
memory proposals were measurement targets, not implemented guarantees.

### Crash reporting

CrashDetected remains mandatory as a next-startup observation of existing local
crash evidence, not a signal-time network send or replay queue. Existing NRP
initialization establishes telemetry before crash reporting; CheckForPreviousCrash
borrows an active invocation and does not reset its budget. Standalone callers
retain their own init/cleanup scope.

The owner intends to install crash detection in the telemetry executable, writing
to its log. At the latest source inspection no InstallCrashHandler call was yet
present in TelemetryWorkerMain.c. Recheck owner changes before implementing.
Do not add CheckForPreviousCrash there blindly: it can initialize another worker.
Keep crash diagnostics local and preserve SIGALRM watchdog behavior.
Validate extraction bounds, stale evidence and rotation behavior in the real host.

## 7. Logging and recent corrections

Worker log: `/var/log/osconfig_telemetry.log`, backup
`/var/log/osconfig_telemetry.bak`. The worker now opens its local handle in main
before InitializeTelemetry and passes it explicitly. The earlier proposed global
g_telemetryLog was not adopted in the owner's final structure.

The owner explicitly removed worker JSON configuration loading and fatal
log-open checks. They also explicitly required leaving console logging enabled.
Do not restore these under an old initialization plan. The production parent
redirects worker stdout/stderr to `/dev/null`; standalone test logging behavior is
separate. The live sender still disables console logging and requires its log.

The shared logging library has machine-wide configuration inputs, but each
process holds its own loaded values. The current worker does not read that JSON
file. CommonUtils logging-setting tests remain last in their source file because
they modify settings; source ordering is not isolation under shuffling or parallel
CTest, and process isolation does not isolate shared configuration-file writes.

Recent reviewed fixes in worker main include valid signatures/call arguments,
zero-length NULL transfers, correct tenant-prefix and hostname-length checks,
correct printf argument types, saved errno, operation-specific initialization
diagnostics, separate startup/lifetime parsing and restored descriptor cleanup.
Local diagnostics never serialize credentials or full event/response bodies.

The manual formatting pass covered 42 owned C/C++ files; 30 changed files were
staged at that milestone, without an assistant commit. Those historical counts
are not a current worktree inventory. Imported core was not reformatted.

## 8. Build, packaging, licenses and ownership

The shipped identity remains OSConfigTelemetry. The CMake target is
`telemetryworker`; `telemetrybin` remains a compatibility build target.
The build output is `common/telemetry/OSConfigTelemetry`.

The parent MC CMake file copies that target into the historical
`${BINARY_DIR}/bin/telemetrybin` artifact slot. The original ASB/SSH child staging
rules consume it at `Modules/DscNativeResources/OsConfigResource/OSConfigTelemetry`
in all four ZIPs. They do not use the earlier candidate `distilled` directory.
They were restored to their original rules; no new staging cleanup is present.

**Do not add licenses or other new files to these intermediate policy ZIPs.**
The owner explained that the MC pipeline unpacks them, signs binaries and builds
the final policy packages. Extra files could break processing or fail to reach
customers. The earlier proposed ZIP license fix was withdrawn.

Keep root MIT and component-local notices in source. The normal CMake installation
path installs the root license and the conditional mintls license.
The owner previously said DEB/RPM are not currently shipped. Their install rules
do not establish final MC distribution notice handling, which belongs to downstream
release review.

Keep attribution, provenance and modification-notice requirements in the existing
component documentation; avoid unrelated root documentation or packaging changes.

Use fresh generated staging areas when inspecting artifacts: restored staging
rules do not clean stale worker/notice files left by another configuration.
BUILD_TELEMETRY=OFF must be checked on actual clean outputs, not inferred solely
from conditional CMake copies.

No old spool/database is imported, replayed or automatically deleted. Do not
remove unrelated curl command-line tools, test dependencies or container packages
merely because the telemetry runtime stopped using libcurl.

Apache-2.0 permits modifications and commercial derivatives without upstream
contribution or publication of modifications. Preserve applicable notices, mark
changed files and provide required license/NOTICE material through the actual
distribution mechanism. This is distinct from internal component classification;
this document is not a claim of governance/legal signoff.

## 9. Owner-run commands and test interpretation

These are Linux commands for the owner's existing configured build directory,
not authorization for assistant execution. Use the owner's established privilege
and build configuration; do not install tools or change routing/trust to force a
passing result.

### Focused offline build and tests

```bash
sudo --preserve-env=OsConfigTelemetryApiKey cmake --build . --target commontests telemetryproducertests telemetryariatest
sudo ctest -R '^(MinTls.*|Telemetry.*)\.' --output-on-failure
```

The full 10,000-event loopback case has the GoogleTest DISABLED_ prefix and stays
out of the normal pass. Its explicit filter and `--gtest_also_run_disabled_tests`
are required to run it. Four one-event live-sender failure fixtures remain enabled.
MinTlsUT.cpp is part of commontests; the old mintlstests target and MeasureSize.py
were removed. SDK serializer parity tests were also removed with the SDK;
their earlier passing results remain historical evidence, not runnable targets.

Python TelemetryTlsPeer.py is still used by offline TLS/transport/worker tests.
It simulates the server, CONNECT proxy and collector controls using temporary
certificates and fake tokens. It is not a production dependency or the server
used by a direct live Aria send. Certificate expiry uses explicit year-2000 dates
via openssl ca, not negative x509 -days.

### Explicit live Aria sender

```bash
sudo --preserve-env=OsConfigTelemetryApiKey cmake --build . --target telemetryariatest
sudo --preserve-env=https_proxy,HTTPS_PROXY,all_proxy,ALL_PROXY,no_proxy,NO_PROXY \
  ./common/telemetry/telemetryariatest --send-status-trace-10000
```

The key is compiled from the existing configure-time variable unless explicitly
runtime-overridden. Preserving an unset variable does not supply a key.
If deliberately using a runtime override, preserve that variable through sudo
too; never print it or put it in command arguments.

Current marker: `*** Distilled 1DS SDK test No. 66 ***`.
Keep it synchronized in TelemetryAriaTest.c, TelemetryTlsPeer.py and
TelemetryTlsUT.cpp. A mismatch prevents the offline peer from producing its
intended response and causes EPROTO before throttle/rejection behavior is tested.

The live executable prints a correlation ID and final attempted/accepted/rejected/
unconfirmed/unsent counts, not one success line per event. It stops on failure,
rejection, unconfirmed delivery or suppression. Do not automatically rerun an
ambiguous or throttled batch. A sampled portal cannot establish complete counts.
The source contains the owner's requested "How to run this" top comment.

This test calls transport directly and forces MinTls. It does not validate
production worker IPC, automatic OS-provider selection or the 500 ms total budget.
Do not restore the obsolete --send-status-trace-100 command or the removed global
forced-MinTls CMake option.

## 10. Release readiness: remaining work, not cosmetic cleanup

The implementation is a release candidate needing targeted hardening and evidence,
not "production ready except formatting." Distillation is the next approved
engineering task; the following remain separate release concerns.

| Area | Finding or missing evidence |
| --- | --- |
| Host latency | ReleaseChild performs blocking waitpid after SIGKILL. Uninterruptible child I/O can exceed the intended host allowance. Preserve ownership while resolving the cleanup/latency tradeoff; do not simply leak zombies. |
| Address fallback | The first stalled TCP address can consume the entire deadline. Evaluate bounded alternatives without introducing event replay or resetting budgets. |
| Production TLS path | Current tests and live sender force MinTls. Re-establish coverage for OS OpenSSL 3/1.1, automatic absence-based fallback and failures that must not fall back. Earlier OS-adapter tests are useful historical evidence, not current automatic-selection coverage. |
| Real policy host | Validate packaged ASB/SSH Get/Set invocations, executable discovery/permissions, SIGCHLD ownership, constructor/destructor scope and module reuse. No separate external gc_worker implementation project is requested. |
| Budget versus delivery | Measure cold-start metadata/spawn/DNS/TCP/TLS/sends/cleanup on real routes and slow hardware. The 15-second standalone limit and a fast loopback producer cannot establish production delivery rates inside 500 ms. |
| Final regression | Obtain the complete post-worker-edit, post-marker-fix run, supported distro/architecture builds and sanitizer results for the intended release revision. |
| Legacy compatibility | Ubuntu 14/GCC 4.8 is both a build and runtime requirement; EL7 glibc 2.17 can be older than Ubuntu 14's glibc 2.19. Inspect ELF symbols, CPU baseline and actual runtime closure. |
| Trust/policy coverage | Verify bundle rollover, missing/invalid trust, required enterprise roots, FIPS/crypto-policy behavior and fallback exclusions on actual images. |
| Collector contract | Cross-invocation retry/kill duration enforcement and clock correction remain unresolved. Conservative within-invocation suppression is implemented. |
| Network population | Confirm the supported proxy/authentication subset is sufficient; do not claim curl parity. |
| Data usefulness | Confirm event fields, stable identity/privacy requirements, query compatibility, event-volume limits and whether final-summary starvation is acceptable. |
| Crypto maintenance | Track advisories and fixes affecting retained core code, interoperability and independent assurance. A source hash and successful handshake do not establish ongoing maintenance. |
| Distribution | Preserve MC intermediate artifact contracts; resolve final-distribution notices through the release pipeline rather than adding ZIP files. |
| Rollout | Define release cohort, monitoring and artifact-level rollback. No legacy SDK fallback is present in the source. |

The original 4-8-week production estimate is no longer an actionable schedule.
Do not derive a new delivery date from source size or the successful live sample.
No deployment, destination change or release approval is implied by this document.

### Routing investigation requiring revalidation

An October 2 read-only investigation of the separate internal pipeline repository
found that telemetry routing is selected by the configured key, not CMake
Release/Debug. Its shared build template selected ONEDS_PROD for sourceBranch main,
otherwise ONEDS_NONPROD. Both entrypoints appeared to omit forwarding sourceBranch,
allowing the template's main default even for another checkout branch.

That was an observed pipeline finding, not fixed here. Recheck the actual release
pipeline before relying on it; do not infer current destination from a branch name.
The earlier OSConfig-NonPROD project was established by owner portal evidence,
not merely equal generated Keys.h fingerprints or collector acceptance.

## 11. Historical evidence and footprint

These results overlap and come from different revisions. Do not sum them into a
single final-suite result or reinterpret elapsed suite time as per-event latency.

| Milestone | Owner-supplied evidence |
| --- | --- |
| September 30 encoder | 21 cases passed. |
| September 30 pinned SDK serializer comparison | 13 cases passed on Ubuntu 22.04, for the implemented serializer subset. The SDK-dependent target is now removed. |
| September 30 resolver | 10 cases passed, 2.02 seconds. |
| October 1 worker/resolver | 31 cases passed, 10.42 seconds, including worker log startup, self-timeout and child ownership. |
| October 1 HTTP protocol | 29 cases passed, 0.14 seconds. |
| October 1 OS TLS/worker | 36 cases passed, 12.71 seconds. This predates forced-MinTls test selection. |
| October 2 proxy discovery | 18 cases passed, 0.09 seconds. |
| October 2 proxy/transport/TLS/worker | 86 cases passed, 22.06 seconds. |
| October 2 key alignment | Five live-executable loopback regressions passed. |
| October 2 sparse acknowledgment | Real collector omitted zero rej in {"acc":1}; parser corrected without relaxing invalid-present-value checks. The separate failure-path control-latching gap was subsequently fixed during SEND integration. |
| October 2 full live100 | 100 requested/attempted/accepted, zero rejected/unconfirmed/unsent, status 0. |
| October 2 downstream samples | Initial 10,000-event run seen in OSConfig-NonPROD at sequence 9665; later No. 2 and forced-MinTls No. 3 samples also seen. Final 10,000-run totals were not supplied. |
| October 5 earlier regression | Owner said all tests passed; later edits require their own result. |
| October 5 No. 66 | Current standalone sender event visible in Aria; details and limitations in section 1. |

The old missing-events investigation is resolved for the demonstrated samples.
Do not reopen it as if no event has arrived. The portal was a live sampled
inspector; absence there did not prove loss, bad epoch arithmetic or wrong routing.
An unrelated legacy crash sample was identified by the owner as an old intentional
crash test, not a current production crash. Do not expand this task into repairing it.

### Measured artifact reports

Owner-reported October 5 smaller build:

| Artifact | Bytes |
| --- | ---: |
| OSConfigTelemetry | 630,168 |
| AzureLinuxBaseline.zip | 665,569 |
| LinuxSshServerSecurityBaseline.zip | 655,309 |
| SetAzureLinuxBaseline.zip | 665,581 |
| SetLinuxSshServerSecurityBaseline.zip | 655,321 |

The comparison ZIPs were 4,956,797; 4,946,537; 4,956,809; and 4,946,549 bytes,
respectively. Each saved exactly 4,291,228 bytes: approximately 86.57% for ASB
and 86.75% for SSH. The repeated executable listing was the same smaller binary,
not an old executable measurement.

Earlier 15-MB executable/10-MB package estimates may include vcpkg removal and
are not a controlled comparison attributable solely to this cutover. The original
300,000-byte incremental stripped-MinTls target has not been measured; the
630,168-byte total executable is not that incremental measurement.

## 12. Retained research and design rationale

### Why a small owned worker

The prior SDK path wrote JSONL, launched a C++ helper at shutdown, and used general
SDK queue/storage/HTTP/TLS machinery. The replacement removes that architecture,
not only its package manager. Existing local crash diagnostics remain allowed;
they are not a telemetry spool.

The inspected SDK/curl path included blocking setup before later socket waits,
future/destructor waits, callback-list locking, unbounded cancellation/drain
surfaces and permissive acceptance handling. Those were source hazards, not
proof of a reproduced production deadlock. Avoid copying their architecture.
Likewise, copying TLS handshake entrypoints alone cannot preserve cryptographic
or certificate-validation guarantees.

In-host TLS was rejected because we cannot depend on gc_worker exit, unload
provider code safely beneath other users, or perform global TLS cleanup.
The approved owned child supplies a private TLS lifetime. Only there may loaded
provider references remain until process exit. The earlier DNS-only executable
remains an offline utility, not another packaged production process.

### Wire evidence worth preserving

The inspected SDK source was microsoft/cpp_client_telemetry v3.9.309.1.
It selected HAVE_CS3, not CS4. BondSplicer concatenated records without an outer
length package. JsonFormatter existence did not establish a JSON upload route.

The encoder was distilled from its CompactBinary writer and generated record
schema: checked varints, unsigned ZigZag arithmetic, length-prefixed UTF-8 bytes,
explicit little-endian values, ordered map properties and struct terminators.
Do not replace that evidence with a second hand-written test oracle sharing the
same assumptions. Historical SDK comparison excluded its undefined signed-shift
reference cases; owned fixtures cover negative/extreme integers independently.

The SDK's sparse-count handling informed the real {"acc":1} fix, but its permissive
HTTP-200/body-failure behavior was not copied. Parsing successful HTTP framing,
explicit collector acceptance and downstream sample visibility are three distinct
observations.

The pinned curl investigation used curl-8_16_0. SDK/curl/bundled-OpenSSL/zlib/
SQLite/nlohmann references describe historical investigated dependencies, not
the current telemetry build closure. Uncompressed encoding is the current design.

### Source anchors for future investigation

Use the investigated SDK version as the reference for these paths:

| Source | Relevant contract |
| --- | --- |
| `lib\include\mat\config-default.h`; `CsProtocol_types.hpp` | CS3 selection and schema. |
| `lib\api\Logger.cpp`; `EventPropertiesDecorator.hpp` | Property mapping, metadata, flags and time. |
| `lib\bond\CompactBinaryProtocolWriter.hpp`; generated `CsProtocol_writers.hpp` | Binary primitives and record/Value/Data encoding. |
| `lib\packager\BondSplicer.cpp` and its unit tests | Record concatenation/framing. |
| `lib\http\HttpRequestEncoder.cpp` | Headers, token and upload-time contract. |
| `lib\http\HttpResponseDecoder.cpp` | Sparse counts, efi and incomplete acceptance behavior. |
| `lib\pal\PAL.cpp`; `lib\utils\Utils.hpp` | Epoch conversion and tenant prefix. |
| `lib\pal\posix\sysinfo_sources.cpp`; `ContextFieldsProvider.cpp` | Legacy stable identity/enrichment; do not silently equate it with random epoch. |
| `lib\system\ClockSkewDelta.h`; `lib\offline\ClockSkewManager.hpp` | Distinct clock-negotiation paths; not an implemented owned-client feature. |
| `lib\offline\KillSwitchManager.hpp` | Historical control semantics requiring a deliberate stateless contract. |
| `lib\http\HttpClient_Curl.hpp`; `HttpClientManager.cpp`; `TelemetrySystem.cpp` | Connection, callback, cancellation and shutdown hazards. |

For curl-8_16_0: `lib\connect.c` covers deadline accounting; `hostip.c` and
`asyn-thrdd.c` explain blocking/threaded DNS; `http.c` and `http_chunks.c` cover
framing; `url.c`, `noproxy.c` and `cf-h1-proxy.c` cover routing/CONNECT; and
`vtls\openssl.c` covers verification, ALPN, readiness and provider lifecycle.
These are research sources, not a direction to import another general library.

OpenSSL reference contracts:

- https://docs.openssl.org/1.1.1/man3/OPENSSL_init_crypto/
- https://docs.openssl.org/1.1.1/man3/SSL_get_error/
- https://docs.openssl.org/1.1.1/man3/SSL_set1_host/
- https://docs.openssl.org/1.1.1/man3/SSL_CTX_load_verify_locations/
- https://docs.openssl.org/1.1.1/man3/SSL_shutdown/

Legacy distribution research found differing OpenSSL ABIs and non-universal NSS/
GnuTLS availability. A package index or successful symbol probe is not proof of
deployment availability, maintained security, usable trust or bounded operation.
The optional telemetrytlsprobe inventories libraries/symbols/trust-path presence;
exit zero means inventory completed, not verified TLS or Aria delivery.

## 13. Restart checklist

1. Read sections 1-3 before changing code; confirm any new owner results/commits.
2. Obtain owner build/regression results for the October 6 flat-profile/logging
   changes, then continue function-level TLS distillation. Do not resume old SDK
   cutover, missing-event debugging, NSS integration or license ZIP staging.
3. Preserve current event schema, worker identity, package contract and runtime
   forceMinTls behavior. Keep No. 66 fixture expectations synchronized.
4. Record removed source and changed upstream files, provenance and validation
   evidence as work progresses. Never substitute source-only checks for the
   owner's build/runtime results.
5. Keep release follow-ups distinct from implemented features. Reassess the
   section 10 gates against new evidence before declaring readiness.
6. After TLS distillation is complete, proceed to event-write cleanup and
   consolidation (section 14); do not mix it into the TLS changes.

## 14. Following phase: clean up and consolidate event writes

Owner direction, October 5: once TLS distillation is complete, clean up and
consolidate all existing telemetry event writes. This is the next phase after
distillation, not work to start during the documentation refresh or TLS pruning.

Begin by inventorying production event-emission call sites, macros and shared
producer helpers across the repository. Trace their event shapes and intended
purpose, identify redundant or inconsistent emission logic, and consolidate
through existing shared helpers where appropriate.

Preserve event names, property types, intended reporting coverage, invocation
ownership and budget/error behavior unless the owner approves behavioral changes.
In particular, this direction does not by itself approve the earlier proposed
RuleComplete removal, event renaming, filtering or volume policy. Distinguish
implementation duplication from intentionally separate event observations before
removing writes. Include crash-reporting callers and update directly affected
fixtures/documentation as the agreed cleanup proceeds.

### Estimate hourly volume and stay below Aria limits

As part of this phase, calculate estimated events per hour from the inventoried
writes and evaluate how to remain below Aria's applicable limit. Establish the
actual configured quota, its scope (for example project/token versus device),
enforcement window and any separate burst/request/byte limits from authoritative
service information. Do not assume a universal numeric Aria limit or infer one
from successful synthetic sends.

Model each policy/event category using active device count, assignments per
device, invocations per hour and events per invocation. Separate typical,
high-error and worst-case scenarios, including repeated rule/status writes,
crash reports, overlapping policies and rollout/startup bursts. Aggregate all
producers sharing the quota, including other clients and diagnostic traffic.
Record assumptions and distinguish attempted sends from accepted events.

Start with the total eligible device population, not just devices visible in
recent telemetry. Establish the observed active population and full-rollout
population separately, with the source and observation window for each count.
Break these into cohorts by policy assignments, execution cadence and relevant
event behavior; account for devices running more than one policy without
double-counting the devices or omitting their additional event writes.

Deliver a per-event table for BaselineRun, RuleComplete, StatusTrace and
CrashDetected showing device count, expected events per device per hour and
fleet-wide events per hour, with typical and peak estimates. Derive frequency
from actual call sites, rules evaluated, execution schedules and observed
error/crash frequency rather than assuming one event of each type per run.
For each event type:

```text
fleet events/hour = sum across cohorts(
    devices in cohort * events of that type per device per hour)
```

Sum the event-type totals for the overall hourly load, then include any other
traffic sharing the quota. Show current and full-rollout estimates, uncertainty,
and each event type's contribution so consolidation targets the largest
contributors without obscuring essential diagnostics.

Compare both hourly totals and shorter-window peaks with the confirmed limits,
allowing explicit safety headroom for growth and uncertainty. The 500 ms
invocation allowance is not a rate limit: fast connections and many devices can
still produce substantial volume. Current invocation-local suppression likewise
does not coordinate the fleet or enforce a cross-invocation hourly cap.

Use the estimate to propose suitable controls, such as eliminating redundant
writes, aggregation, sampling, per-event caps or scheduling spread, while
preserving essential error/crash visibility. Agree on behavior and headroom with
the owner before implementation; do not silently drop events or add persistent
state. Validate the resulting estimate against measured production-shaped
emission counts and recheck the quota before rollout.
