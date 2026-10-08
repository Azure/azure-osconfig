# Telemetry runtime

The focused C client implements OSConfig event encoding and delivery without the general-purpose 1DS C++ SDK. DNS, TLS and network operations are in the owned `OSConfigTelemetry` worker, not the policy host.

Reusable code is built into one static library, `telemetry`. Source files are separate archive members, so linking the producer does not pull in the worker-only TLS and transport implementations. The production worker and subprocess test fixtures remain separate executables; test-specific source overrides do not modify the production library.

Telemetry is always built. The worker is installed with OSConfig and included in both audit and remediation policy packages; there is no telemetry-off build variant. This does not bypass runtime requirements: sending still requires an initialized invocation, an ingestion key, and a supported TLS runtime. Telemetry failures remain isolated from the audited or remediated operation's result. The live Aria test remains explicit-only.

## System TLS requirement

The worker loads system OpenSSL in preference order **3, 1.1, then 1.0.2**. No OpenSSL headers, downloaded OpenSSL source, or bundled TLS implementation are needed to build this client. Runtime discovery checks both the library version and required APIs; an `openssl` command on PATH alone does not establish support.

The legacy adapter recognizes `libssl.so.1.0.2`, `libssl.so.10` and `libssl.so.1.0.0`, but accepts only the 1.0.2 version family under those names. OpenSSL 1.0.1 and earlier are unsupported. Stock Ubuntu 14.04 and custom/minimal images without a supported runtime need an additional runtime installation to send telemetry. Discovery produces explicit diagnostics if no supported runtime is usable; there is no in-tree or plaintext fallback. Dynamic loading does not automatically add this prerequisite to package-manager dependency metadata.

An unavailable or API-incompatible candidate can be skipped during discovery. Once a provider is selected, initialization, configuration, trust, handshake and I/O failures never cause a retry through another provider. The selected library remains mapped for the worker's lifetime.
