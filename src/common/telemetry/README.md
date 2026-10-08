# Telemetry runtime

The focused C client implements OSConfig event encoding and delivery without the general-purpose 1DS C++ SDK. DNS, TLS and network operations stay in the owned `OSConfigTelemetry` worker, not the policy host. Public producer calls and event schemas are unchanged.

Reusable code is built into one static library, `telemetry`. Source files remain separate archive members, so linking the producer does not pull in the worker-only TLS and transport implementations. Worker and test entry points remain separate executables; test-specific source overrides do not modify the production library.

Telemetry is always built. The worker is installed with OSConfig and included in both audit and remediation policy packages; there is no telemetry-off build variant. This does not bypass runtime requirements: sending still requires an initialized invocation, an ingestion key, and a supported TLS runtime. Telemetry failures remain isolated from the audited or remediated operation's result. The live Aria test remains explicit-only.

## Failure-only logging

The existing `osconfig_telemetry.log` and `osconfig_telemetry.bak` filenames are retained, including when these files contain entries from the previous implementation. Code in this directory emits no informational or debug entries for routine startup, successful events, connection progress or normal idle shutdown, even when debug logging is enabled. Shared utilities retain their existing logging behavior, so their calls during initialization can still produce routine entries.

Failures that block preparation, delivery or cleanup are logged at error level with the actual function, failed operation, numeric error and diagnostic text. Capture `errno` before logging; use resolver/OpenSSL diagnostics for their respective error domains rather than treating those codes as `errno`. Helpers without a log handle propagate failures to the owning caller. An idle worker's parent disconnect is normal shutdown; an incomplete event transfer is an error.

Repeated producer initialization is a silent no-op until cleanup: it neither resets invocation limits nor retries an earlier failed initialization. Rejected OpenSSL candidates and unsuccessful address attempts are normal discovery steps; they produce error diagnostics only if discovery or connection ultimately fails. Genuine cleanup failures are still reported.

Diagnostics must not include ingestion keys, proxy credentials, event payloads or collector control values. The explicit test/probe utilities retain their console results; they do not add routine runtime log entries.

## System TLS requirement

The worker loads system OpenSSL in preference order **3, 1.1, then 1.0.2**. No OpenSSL headers, downloaded OpenSSL source, or bundled TLS implementation are needed to build this client. Runtime discovery checks both the library version and required APIs; an `openssl` command on PATH alone does not establish support.

The legacy adapter recognizes `libssl.so.1.0.2`, `libssl.so.10` and `libssl.so.1.0.0`, but accepts only the 1.0.2 version family under those names. OpenSSL 1.0.1 and earlier are unsupported. Stock Ubuntu 14.04 and custom/minimal images without a supported runtime need an additional runtime installation to send telemetry. Discovery produces explicit diagnostics if no supported runtime is usable; there is no in-tree or plaintext fallback. Dynamic loading does not automatically add this prerequisite to package-manager dependency metadata.

An unavailable or API-incompatible candidate can be skipped during discovery. Once a provider is selected, initialization, configuration, trust, handshake and I/O failures never cause a retry through another provider. The selected library remains mapped for the worker's lifetime.

## Verification and policy

OpenSSL 1.0.2 uses its TLS 1.2-only client method and built-in hostname/IP verification APIs. Newer providers require at least TLS 1.2 while retaining any higher configured minimum. All paths require certificate-chain, validity and DNS/IP verification using the provider's trust store; numeric identities require an IP SAN and do not send SNI. Compression is disabled, ALPN offers HTTP/1.1, and an unauthenticated socket EOF is an error. 

System OpenSSL performs cryptographic operations and processes its configuration. The 1.0.2 adapter loads configuration through the status-returning module API rather than the void `OPENSSL_config` helper, so configuration errors fail the operation. A missing implicit default configuration file is allowed; a missing explicit `OPENSSL_CONF` file is not. The worker is serialized and single-threaded, so it does not install legacy OpenSSL threading callbacks. 

Accepting an ABI does not establish that its installation is patched, supported or FIPS validated. Deployments must use an appropriately serviced distribution runtime; FIPS requirements depend on the actual validated module and its configuration, not simply the OpenSSL version. 

## Coverage

Loopback tests use a separate C++ peer loading real system OpenSSL server and certificate-generation APIs dynamically. No Python, Bash, `openssl` command, development headers or CI package-installation step is required by this fixture. Fresh RSA/ECDSA keys and certificates are generated in memory; only the public trust certificate is written to the test's private temporary directory. The expiry fixture verifies a valid leaf first, then changes its dates and requires the exact leaf-expiration error, preserving the positive and negative controls. The peer is not installed and listens only on the loopback interface. Production code is unchanged.

`telemetrytlsprovidertests` exercises the real adapter with substituted loader entry points: provider ordering, legacy API differences, version/API rejection, configuration failures and refusal to downgrade after provider failures. `commontests` exercises real system OpenSSL against loopback TLS/HTTP peers. Run both on the target images; mocked ABI coverage is not a substitute for real 1.0.2, 1.1 and 3 runtime coverage. 

The offline `telemetrytlsprobe` only inventories candidate libraries and sampled symbols; its successful exit is not proof of TLS readiness. `telemetryariatest` uses normal system-provider discovery and sends live traffic only when explicitly invoked with its send argument.
