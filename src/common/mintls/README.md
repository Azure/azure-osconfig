# mintls

Private, in-tree TLS fallback for an owned OSConfig process, not a general TLS
library and not a new dependency of the policy SO. System OpenSSL remains the
preferred telemetry provider. A certificate, handshake, initialization, or
I/O failure on that provider never causes a retry through mintls.

## Ownership and scope

The source is ingested and maintained here, not fetched, installed, or selected
by a dependency version pin. `core` contains the required crypto/X.509/TLS
modules and their header dependencies. Its initial provenance is the official
Mbed TLS 3.6.7 release archive, SHA-256
`a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`.
That identifies the imported source for security-fix tracking, not a build
dependency. The original files/notices are retained under the Apache-2.0
choice in [core/LICENSE](core/LICENSE). OSConfig owns the wrapper, profile,
integration, tests, and ongoing maintenance of the imported subset.

Do not hand-rewrite cryptographic primitives to reduce source size. The build
selects client-only TLS 1.2, authenticated ECDHE RSA/ECDSA, AES-GCM, and normal
16 KB records. Server, DTLS, TLS 1.3, PSK, legacy CBC suites, session tickets,
renegotiation, and upstream network/thread/storage implementations are not
enabled. Guarded header declarations for disabled features are not compiled
implementations. SHA-1 parsing support is retained for legacy self-signed CA
roots; the peer-chain verification profile still rejects SHA-1 signatures.
The usual RSA 2048-4096 and P-256/P-384/P-521 certificate families are supported.

Review applicable upstream advisories and patch this subset, including shared
crypto and parser fixes. A source import is not a maintenance exemption or a
claim of independent authorship.

## Trust, policy, and process contract

No roots ship with mintls. An explicit `MinTlsCreate` CA file takes precedence,
then `SSL_CERT_FILE`, then a recognized OS PEM bundle. A supplied invalid or
empty override fails; it never falls back to unrelated trust. A directory-only
`SSL_CERT_DIR` override is unsupported. When both variables exist, the file is
the complete trust input. Missing, partially unparseable, nonregular, empty,
or over-4-MiB bundles fail closed. Custom images need to provision their CA
bundle, including approved interception roots when applicable.

mintls is not FIPS validated and cannot interpret arbitrary OpenSSL policy.
It refuses explicit `OPENSSL_CONF`, `OPENSSL_CONF_INCLUDE`, `OPENSSL_MODULES`,
`OPENSSL_FIPS`, or `OPENSSL_FORCE_FIPS_MODE` overrides, `/etc/system-fips`,
a system crypto-policy configuration, and an
enabled or unreadable kernel FIPS setting. These checks also apply in the
forced test build. Sites imposing other cryptographic restrictions must keep
`OSCONFIG_TELEMETRY_MINTLS=OFF`; the fallback is not a substitute for their OS
provider policy. There is no accept-any-certificate or policy-bypass switch.

The API borrows a connected nonblocking socket and preserves absolute
deadlines across retries. Chain, dates, and DNS/IP identity are mandatory;
numeric IP identities require an IP SAN and are not sent as SNI. Only a TLS
close-notify is EOF. Failure discards the TLS session without closing the
caller's socket or replaying application data. The caller must arm its
process watchdog: filesystem, entropy, and cryptographic operations cannot
all be interrupted safely inside the C library. The telemetry worker keeps
its existing 500 ms work allowance and 10-minute lifetime.

## Owner-run validation

Implementation is not yet build-, integration-, live-, or size-validated.
No universal-distro claim follows from adding the source.

Build normal and forced-fallback trees from the same source/settings. Add
`-DOSCONFIG_TELEMETRY_FORCE_MINTLS=ON -DBUILD_TESTS=ON` only to the test tree.
The switch bypasses OS TLS discovery for the worker and shared tests; it does
not rename, remove, or modify Ubuntu's OpenSSL. Do not distribute the forced
build. Independently, the standalone `telemetryariatest` temporarily always
uses its own forced-mintls transport for the No. 3 verification run.

Build `mintlstests`, `commontests`, and `telemetryariatest`. Run `mintlstests`
and the common `TelemetryTlsDeathTest.*`, `TelemetryTransportDeathTest.*`, and
worker/producer tests in both trees. The forced exchange test checks that
neither libssl nor libcrypto is mapped in the client process. Loopback fixtures
use the OS openssl CLI/Python SSL as the independent server, not as the client.
They exercise RSA/ECDSA certificates, name/IP/date/trust failures, TLS 1.0
rejection, SNI/ALPN, close-notify, abrupt EOF, deadlines, backpressure, proxy
tunneling, collector replies, and no-replay behavior.

The owner confirmed the fixture's negative-day certificate generation on
OpenSSL 3.0.2: `-days -1` produced an expired certificate and verification
returned error 10 on 2026-10-02. This confirms the fixture command, not a
mintls handshake or the full test suite.

Only after offline results, the owner can explicitly run the existing
`telemetryariatest --send-status-trace-10000` from either tree. No extra CMake
option is needed to force mintls for this executable. Its marker is
`*** Distilled 1DS SDK test No. 3 ***`; its startup banner states
`TLS mode: mintls only (OS OpenSSL discovery disabled)`. Provider/trust failure
stops the test rather than retrying with system OpenSSL. Normal production
provider selection remains unchanged unless the global test option is enabled.

For the <=300,000-byte added stripped-worker target, build identical release
trees with `OSCONFIG_TELEMETRY_MINTLS=OFF` and `ON` (not forced). Pass the two
`OSConfigTelemetry` files to `tests/MeasureSize.py`. It strips temporary copies,
reports the exact file-size delta, and fails above the target. Use `--strip`
for a cross-target strip executable. This is a target, not a measurement;
source size, CA roots, debug information, runtime RAM, and package license
text are separate. Inspect the resulting SO/executable symbols and all four
ZIPs to confirm the boundary and inclusion of `mintls-LICENSE`.
