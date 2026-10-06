# mintls

Private, in-tree TLS fallback, not a general TLS library. System OpenSSL remains the preferred telemetry provider. A certificate, handshake, initialization, or I/O failure on that provider never causes a retry through mintls.

On Linux distributions or custom images that lack a supported system OpenSSL provider (OpenSSL 3 or 1.1), mintls provides the TLS client needed for HTTPS telemetry. It is an OSConfig-maintained derivative of Mbed TLS, statically linked into the telemetry executable, so no separate TLS library needs to be installed. It still requires a trusted CA bundle and remains subject to the security-policy restrictions below.

## Distilled implementation

All implementation files and the upstream license are in this directory. There is no nested `core` tree or upstream build. `MinTlsConfig.h` fixes the TLS 1.2 client profile; this is not a configurable replacement for the general Mbed TLS API.

Dependency tracing starts at `MinTls.c` and the core tests. The first distillation removes the unused modular-bignum/block-cipher modules, PSA integration, TLS 1.3-only data, and profile-disabled server, DTLS, alternative cipher/curve, filesystem, threading and self-test branches. Required compiler/architecture branches and cryptographic operations are retained. SHA-1 remains for legacy trust-anchor parsing, not permission to accept SHA-1 peer signatures. The internal `mbedtls_` names are retained to make provenance and security-fix comparison practical.

This is a fixed-profile reduction, not a claim that every remaining general-purpose helper is reachable from a telemetry send. Further function-level pruning must account for callback tables, header inlines, certificate inputs and all supported targets; absence from one binary is insufficient evidence.

## Diagnostics

Every public `MinTls*` operation accepts the caller's borrowed `OsConfigLogHandle`. A stack-owned `MinTlsDiagnostics` context carries that handle explicitly through retained status-returning helpers and the RNG, entropy, socket, cipher, public-key and TLS callbacks. These private interfaces require a valid diagnostic context; they do not retain it after the synchronous call. There is no global/thread-local log handle, separate core log, or allocation for diagnostics. The public MinTls API is unchanged.

Each failure-capable call records only operation/file/line and numeric status. A successful enclosing call discards speculative parser and certificate-candidate failures; a failed outer call reports the originating diagnostic. Explicit error translations retain their cause, while a new direct failure replaces an earlier recovered probe. Positive byte counts remain success; positive partial-certificate counts and TLS alert codes follow their distinct failure contracts. Alert-send failures report separately without replacing the primary error. Readiness continuations and authenticated close-notify are not errors.

The general upstream debug stream stays disabled: key, MPI, certificate and payload dumps are never enabled, even at debug level. Emitting a diagnostic preserves errno. Constant-time RSA unpadding remains uninstrumented internally; its caller diagnoses the returned failure after clearing its sensitive buffer. Ordinary C implementation functions are not `static`; existing header-only inline primitives retain their linkage.

Failure propagation includes checking the Finished-message PRF result and the EC key-pair check's group-copy result rather than discarding them. Healthy-operation behavior and cryptographic calculations are unchanged; these two failure paths now return their actual errors. The expanded diagnostics and callback interfaces require fresh owner build/runtime coverage; earlier live-delivery evidence predates this pass.

## Trust, policy, and process contract

No roots ship with mintls. An explicit `MinTlsCreate` CA file takes precedence, then `SSL_CERT_FILE`, then a recognized OS PEM bundle. A supplied invalid or empty override fails; it never falls back to unrelated trust. A directory-only `SSL_CERT_DIR` override is unsupported. When both variables exist, the file is the complete trust input. Missing, partially unparseable, nonregular, empty, or over-4-MiB bundles fail closed. Custom images need to provision their CA bundle, including approved interception roots when applicable.

mintls is not FIPS validated and cannot interpret arbitrary OpenSSL policy. It refuses explicit `OPENSSL_CONF`, `OPENSSL_CONF_INCLUDE`, `OPENSSL_MODULES`, `OPENSSL_FIPS`, or `OPENSSL_FORCE_FIPS_MODE` overrides, `/etc/system-fips`, a system crypto-policy configuration, and an enabled or unreadable kernel FIPS setting. These checks also apply when tests force mintls at runtime. Sites imposing other cryptographic restrictions must keep `OSCONFIG_TELEMETRY_MINTLS=OFF`; the fallback is not a substitute for their OS provider policy. There is no accept-any-certificate or policy-bypass switch.

The API borrows a connected nonblocking socket and preserves absolute deadlines across retries. Chain, dates, and DNS/IP identity are mandatory; numeric IP identities require an IP SAN and are not sent as SNI. Only a TLS close-notify is EOF. Failure discards the TLS session without closing the caller's socket or replaying application data. The caller must arm its process watchdog: filesystem, entropy, and cryptographic operations cannot all be interrupted safely inside the C library. The telemetry worker keeps its existing 500 ms work allowance and 10-minute lifetime.

## License and provenance

Every C source and header begins with the unchanged two-line Microsoft copyright/MIT notice. Retained derivative files also keep their upstream copyright/license notices, identify their original Mbed TLS 3.6.7 paths, and prominently date OSConfig modifications. The complete upstream license is in [LICENSE](LICENSE). Upstream offers Apache-2.0 OR GPL-2.0-or-later; OSConfig uses the Apache-2.0 option and retains its redistribution obligations. The owned wrapper, diagnostics and integration are MIT-licensed.

The source subset was imported from the official [Mbed TLS 3.6.7 release archive](https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.7/mbedtls-3.6.7.tar.bz2), with SHA-256 `a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`. This records provenance for maintenance; it does not introduce an upstream download or build dependency.

Preserve the upstream notices and license when redistributing this code. Redistribution of binaries containing mintls must also provide recipients with the Apache-2.0 license; source headers or this README alone do not replace that requirement.
