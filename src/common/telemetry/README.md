# Telemetry runtime

The focused C client implements OSConfig event encoding and delivery without the general-purpose 1DS C++ SDK. DNS, TLS and network operations are in the owned `OSConfigTelemetry` worker, not the policy host.

Reusable code is built into one static library, `telemetry`. Source files are separate archive members, so linking the producer does not pull in the worker-only TLS and transport implementations. The production worker and subprocess test fixtures remain separate executables; test-specific source overrides do not modify the production library.

Telemetry is always built. The worker is installed with OSConfig and included in both audit and remediation policy packages; there is no telemetry-off build variant. This does not bypass runtime requirements: sending still requires an initialized invocation, an ingestion key, and a supported TLS runtime. Telemetry failures remain isolated from the audited or remediated operation's result. The live Aria test remains explicit-only.

## System TLS requirement

The worker loads system OpenSSL in preference order **3, 1.1, 1.0.2, then 1.0.1**. No OpenSSL headers, downloaded OpenSSL source, or bundled TLS implementation are needed to build this client. Runtime discovery checks both the library version and required APIs; an `openssl` command on PATH alone does not establish support.

OpenSSL 1.0.1 is an exceptional last-resort path for supported older installations, including Ubuntu 14.04 and unupdated CentOS/RHEL/Oracle Linux 7. Every 1.0.2 candidate is considered before accepting 1.0.1, including when different versions coexist under `libssl.so.10` and `libssl.so.1.0.0`. The explicit `libssl.so.1.0.1` name is also recognized. Versions older than 1.0.1 remain unsupported.

The 1.0.1 path requires TLS 1.2 and the system trust store. OpenSSL still verifies the certificate chain, purpose and validity dates. Because upstream 1.0.1 lacks hostname-verification APIs, the adapter uses public certificate accessors to verify DNS SANs or exact binary IP SANs before enabling application I/O. DNS SANs take precedence over the common name; CN fallback requires a single CN and is never used for IP literals. Only complete leftmost-label wildcards with at least two suffix labels are accepted, and they cannot match multiple labels or an IDNA A-label. Malformed or duplicate SAN extensions and embedded NUL identities are rejected. This identity compatibility path is not used on newer providers.

ALPN is omitted only on 1.0.1; delivery remains HTTP/1.1. Newer providers retain their existing ALPN and built-in identity verification. Compatibility does not imply that an old upstream runtime is security-maintained or guarantee acceptance by the collector's current cipher and trust requirements; operating-system servicing remains necessary. Discovery produces explicit diagnostics if no supported runtime is usable; there is no in-tree or plaintext fallback. Dynamic loading does not automatically add this prerequisite to package-manager dependency metadata.

An unavailable or API-incompatible candidate can be skipped during discovery. Once a provider is selected, initialization, configuration, trust, handshake and I/O failures never cause a retry through another provider. The selected library remains mapped for the worker's lifetime.

Provider tests cover discovery and legacy identity decisions; the real TLS peer also supports 1.0.1, including post-handshake identity rejection before any application bytes. Running those tests on a newer-only host does not establish 1.0.1 interoperability: the real-peer suite must also run with the target distribution's actual 1.0.1 runtime and trust configuration.
