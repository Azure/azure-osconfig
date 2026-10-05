# mintls

Private, in-tree TLS fallback, not a general TLS library. System OpenSSL remains the preferred telemetry provider. A certificate, handshake, initialization, or I/O failure on that provider never causes a retry through mintls.

On Linux distributions or custom images that lack a supported system OpenSSL provider (OpenSSL 3 or 1.1), mintls provides the TLS client needed for HTTPS telemetry. It is a minimal, in-tree subset of Mbed TLS, statically linked into the telemetry executable, so no separate TLS library needs to be installed. It still requires a trusted CA bundle and remains subject to the security-policy restrictions below.

## Trust, policy, and process contract

No roots ship with mintls. An explicit `MinTlsCreate` CA file takes precedence, then `SSL_CERT_FILE`, then a recognized OS PEM bundle. A supplied invalid or empty override fails; it never falls back to unrelated trust. A directory-only `SSL_CERT_DIR` override is unsupported. When both variables exist, the file is the complete trust input. Missing, partially unparseable, nonregular, empty, or over-4-MiB bundles fail closed. Custom images need to provision their CA bundle, including approved interception roots when applicable.

mintls is not FIPS validated and cannot interpret arbitrary OpenSSL policy. It refuses explicit `OPENSSL_CONF`, `OPENSSL_CONF_INCLUDE`, `OPENSSL_MODULES`, `OPENSSL_FIPS`, or `OPENSSL_FORCE_FIPS_MODE` overrides, `/etc/system-fips`, a system crypto-policy configuration, and an enabled or unreadable kernel FIPS setting. These checks also apply in the forced test build. Sites imposing other cryptographic restrictions must keep `OSCONFIG_TELEMETRY_MINTLS=OFF`; the fallback is not a substitute for their OS provider policy. There is no accept-any-certificate or policy-bypass switch.

The API borrows a connected nonblocking socket and preserves absolute deadlines across retries. Chain, dates, and DNS/IP identity are mandatory; numeric IP identities require an IP SAN and are not sent as SNI. Only a TLS close-notify is EOF. Failure discards the TLS session without closing the caller's socket or replaying application data. The caller must arm its process watchdog: filesystem, entropy, and cryptographic operations cannot all be interrupted safely inside the C library. The telemetry worker keeps its existing 500 ms work allowance and 10-minute lifetime.

## License and provenance

The imported Mbed TLS code in `core` retains its upstream copyright notices and the complete license text in [core/LICENSE](core/LICENSE). Upstream offers Apache-2.0 OR GPL-2.0-or-later; OSConfig uses the Apache-2.0 option. The OSConfig wrapper and integration remain MIT-licensed.

The source subset was imported from the official [Mbed TLS 3.6.7 release archive](https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.7/mbedtls-3.6.7.tar.bz2), with SHA-256 `a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`. This records provenance for maintenance; it does not introduce an upstream download or build dependency.

Preserve the upstream notices and license when redistributing this code. Redistribution of binaries containing mintls must also provide recipients with the Apache-2.0 license; source headers or this README alone do not replace that requirement.
