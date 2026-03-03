# Copilot Coding Agent Instructions for azure-osconfig

> Trust these instructions first. Only search the repo if information here is incomplete or found to be incorrect.

## Project Overview

Azure OSConfig is a **modular security configuration stack for Linux Edge devices**. It provides multi-authority device management over Azure (IoT Hub, Azure Policy), GitOps, and local management (RC/DC files). Written in **C11/C++11** using **CMake** and **vcpkg** for dependency management. Targets Linux only.

The architecture has three layers: **Adapters** (IoT Hub PnP agent, RC/DC watcher, GitOps watcher) → **Management Platform** (REST API over Unix Domain Sockets) → **Management Modules** (`.so` shared libraries loaded at runtime). Modules are the primary extension point.

## Repository Layout

```
src/                          # All source code
  CMakeLists.txt              # Root CMake file (project config, vcpkg integration, build options)
  vcpkg.json                  # Dependencies: openssl, curl, lua, sqlite3, nlohmann-json, gtest
  vcpkg-configuration.json    # vcpkg registry baseline
  adapters/                   # Agent adapters (PnP IoT Hub client, MC machine config)
    pnp/                      # PnP agent (main binary: /usr/bin/osconfig)
      azure-iot-sdk-c/        # Git submodule - Azure IoT C SDK
    mc/                       # Machine configuration adapters (ASB, SSH, compliance engine)
  platform/                   # Management Platform daemon (/usr/bin/osconfig-platform)
    inc/Mpi.h                 # MPI interface header
    Main.c, MpiServer.c, ModulesManager.c, MmiClient.c
    tests/                    # Platform unit tests
  common/                     # Shared libraries
    commonutils/              # OS utility functions
    logging/                  # Circular file logging
    mpiclient/                # MPI REST API client
    parson/                   # JSON parser (vendored)
    asb/                      # Azure Security Baseline implementation
    telemetry/                # 1DS telemetry
    tests/                    # Common library unit tests
  modules/                    # Management Modules (each builds a .so)
    inc/                      # Module interface headers (Mmi.h)
    schema/mim.schema.json    # MIM JSON schema for validation
    mim/                      # Module Interface Model definitions (*.json)
    commandrunner/            # CommandRunner module (runs shell commands)
    securitybaseline/         # SecurityBaseline module
    configuration/            # Configuration module
    deviceinfo/               # DeviceInfo module
    complianceengine/         # ComplianceEngine module (Lua-based evaluator)
    test/                     # Module test harness (moduletest tool)
      recipes/                # Test recipe JSON files
    samples/                  # Sample module (C++)
  tests/                      # Fuzzer tests, e2e tests, clang-tidy scripts
external/vcpkg/               # Git submodule - vcpkg package manager
devops/                       # CI/CD scripts, Docker environments, packaging
  docker/                     # Dockerfiles for each supported distro
  scripts/                    # Build and test scripts
  debian/, rpm/               # Packaging scripts (postinst, prerm, etc.)
docs/                         # Architecture docs, module spec, coding style
dtmi/                         # DTDL model definitions
```

**Key config files at repo root:** `.clang-format` (Microsoft-based, Allman braces, indent 4), `.clang-tidy` (CamelCase functions, cppcoreguidelines checks), `.pre-commit-config.yaml` (formatting + clang-tidy + clang-format).

## Build Instructions

### Prerequisites

- CMake >= 3.21, GCC >= 4.4.7 (project uses C11/C++11)
- Git submodules must be initialized: `git submodule update --init --recursive`
- The `VCPKG_ROOT` environment variable must **NOT** be set (the project uses its own vcpkg submodule at `external/vcpkg/`)

### Clean Build (always use these exact steps)

```bash
# From repo root:
mkdir build && cd build
cmake ../src -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build . --config Release --parallel $(nproc)
```

**Notes:**
- vcpkg bootstrap and dependency installation happens automatically during cmake configure (~30-40 seconds on first run).
- The flags `-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Duse_default_uuid=ON` appear in CI but are unused by current CMake and produce warnings. They can be omitted.
- Use `-DBUILD_TELEMETRY=OFF` to skip telemetry build (avoids needing `OsConfigTelemetryApiKey` env var).
- Build outputs: module `.so` files copied to `build/modules/bin/`, platform binary at `build/platform/osconfig-platform`.
- The build installs files to system paths (`/usr/bin/`, `/usr/lib/osconfig/`, `/etc/osconfig/`). Use `--target install` only if you intend to install system-wide.

### Running Tests

```bash
cd build
ctest --test-dir . --output-on-failure -j$(nproc)
```

- ~960 unit tests using Google Test. Some tests requiring root or special filesystem permissions will be **skipped** (not failed) when run as non-root. ~10 tests involving file access may fail without root.
- Tests are registered per-module and per-library in their respective `tests/` subdirectories.
- Test binaries are in: `build/common/commonutils/`, `build/platform/tests/`, `build/modules/commandrunner/tests/`, `build/modules/complianceengine/tests/`, `build/modules/configuration/tests/`.
- Total test time: ~10 seconds.

### Formatting and Linting (required before PR)

Always run pre-commit before submitting changes:
```bash
python3 -m pre_commit run --all-files
```

This runs: trailing whitespace fix, end-of-file fix, LF line endings, clang-format (v14, on complianceengine + telemetry files only), clang-tidy (on complianceengine + telemetry C++ files only), and compliance engine interface generation.

**Important**: clang-format and clang-tidy in pre-commit only apply to files in `src/modules/complianceengine/`, `src/compliance-engine-assessor/`, and `src/common/telemetry/`. Other C/C++ files are not auto-formatted but must follow the style in `docs/style.md`.

## CI Checks on Pull Requests

PRs trigger these GitHub Actions workflows (all must pass):

| Workflow | What it checks |
|----------|----------------|
| `ci.yml` | Builds and runs `ctest` across 21 Linux distros in Docker containers |
| `formatting.yml` | Runs `pre-commit run --all-files` |
| `static-analysis.yml` | Runs `clang-static-analyzer` (analyze-build-14) |
| `mim.yml` | Validates `src/modules/mim/*.json` against `src/modules/schema/mim.schema.json` |
| `ci-sanitizers.yml` | Builds and tests with ASan + UBSan |
| `codeql.yml` | CodeQL security analysis |

## Coding Conventions

See [`docs/style.md`](../docs/style.md) for coding style. ALWAYS load the style guide before suggesting code. The style guide is the ultimate authority on code formatting and conventions.

## Module Development

Each module is a shared library (`.so`) implementing the MMI API (`MmiOpen`, `MmiClose`, `MmiGet`, `MmiSet`, `MmiGetInfo`, `MmiFree`). Module interface is defined by a MIM JSON file in `src/modules/mim/`. When adding a new module:
1. Create MIM definition in `src/modules/mim/<name>.json` (validated against `src/modules/schema/mim.schema.json`)
2. Create module source in `src/modules/<name>/`
3. Register in `src/modules/CMakeLists.txt` using the `add_module()` function
4. Add test recipes in `src/modules/test/recipes/`

**Only 5 modules are actively built:** commandrunner, securitybaseline, configuration, deviceinfo, complianceengine. Other module directories (adhs, firewall, hostname, networking, pmc, tpm, ztsi) exist but are not included in the default build.

## ComplianceEngine Module (Important Module)

The ComplianceEngine is the most complex and important module. It evaluates security compliance rules defined as **rule payloads**, using a combination of **logical combinators** (`allOf`, `anyOf`, `not`), **built-in C++ procedures**, and **Lua scripts**. It supports both **audit** (check compliance) and **remediation** (fix non-compliance) actions.

**Upstream producer**: Rule payloads are generated by the **Compliance Augmentation Engine** (`azcorelinux-Compliance-AugmentationEngine` repo), which transforms CIS XCCDF benchmarks into these JSON structures and base64-encodes them into MOF files. The same concept is referred to as "JSON conditionals", "calling convention", or "mofJson" in that repo. The payload structure (`{audit, remediate, parameters}`) is identical—produced upstream, consumed here.

### Architecture

```
src/modules/complianceengine/
  src/
    lib/                          # Core library (complianceenginelib)
      Engine.h/.cpp               # Top-level MMI handler, manages rule database
      Evaluator.h/.cpp            # Recursive rule evaluator (allOf/anyOf/not/Lua/builtin dispatch)
      Procedure.h/.cpp            # Stores a single rule's audit/remediate JSON + parameters
      ProcedureMap.h/.cpp         # AUTO-GENERATED - maps procedure names → function pointers
      Bindings.h                  # Template framework connecting params structs → string args
      BindingParsers.h/.cpp       # Type parsers (string, int, bool, mode_t, regex, Pattern)
      GenInterface.py             # Code generator: parses .h files → generates ProcedureMap.h/.cpp
      Indicators.h/.cpp           # Tree of compliance/non-compliance status messages
      ContextInterface.h/.cpp     # Abstract interface for system access (commands, files, logging)
      LuaEvaluator.h/.cpp         # Lua script execution engine
      payload.schema.json         # JSON Schema for rule payloads (allOf/anyOf/not/Lua/procedures)
      procedures/                 # Built-in procedure implementations (one triad per procedure)
    so/                           # Module .so entry point (ComplianceEngineModule.c)
    assessor/                     # CLI assessor tool
    lua-evaluator/                # Lua evaluator subdirectory
  tests/                          # Unit tests
    procedures/                   # Per-procedure test files
```

### How Rule Evaluation Works

A **rule payload** is a JSON object with `audit`, optional `remediate`, and optional `parameters` fields. This is the exact structure produced by the Compliance Augmentation Engine (where it is called "JSON conditionals" or "calling convention") and base64-encoded into the MOF `ProcedureObjectValue` field. The `audit` and `remediate` fields contain a recursive expression tree:

```json
{
  "audit": {
    "allOf": [
      { "EnsureFilePermissions": { "filename": "/etc/passwd", "permissions": "0644" } },
      { "anyOf": [
          { "PackageInstalled": { "packageName": "openssh-server" } },
          { "not": { "EnsureFileExists": { "filename": "/etc/ssh/sshd_config" } } }
      ]},
      { "Lua": { "script": "return Compliant('ok')" } }
    ]
  },
  "parameters": { "myParam": "defaultValue" }
}
```

The `Evaluator` recursively processes each node in the expression tree (`Evaluator.cpp`):
- **`allOf`**: Array of sub-expressions. Returns `NonCompliant` on first failure (short-circuit AND).
- **`anyOf`**: Array of sub-expressions. Returns `Compliant` on first success (short-circuit OR).
- **`not`**: Inverts the result. Audit-only (no remediation through `not`).
- **`Lua`**: Runs an inline Lua script via `LuaEvaluator`.
- **Any other key**: Looked up as a built-in procedure name in `Evaluator::mProcedureMap`.

Procedure arguments support **parameter substitution**: a value starting with `$` (e.g., `"$myParam"`) is replaced with the value from the rule's `parameters` map. The Compliance Augmentation Engine produces these `$paramName` placeholders with default values in the `parameters` dict; user overrides arrive via `DesiredObjectValue` in the MOF (`key=value` pairs) and are applied by `Procedure::UpdateUserParameters()`.

### Built-in Procedures

Built-in procedure source files live in `src/modules/complianceengine/src/lib/procedures/`. A single file can contain multiple related procedures — for example, `EnsureFilePermissions.h/.cpp` defines both `EnsureFilePermissions` and `EnsureFilePermissionsCollection`. The file naming reflects the logical grouping, not a 1:1 mapping to procedure names.

Each source file consists of up to three parts:

| File | Purpose |
|------|---------|
| `.h` | Params struct(s) + audit/remediate function declarations for one or more procedures |
| `.cpp` | Implementation(s) |
| `.schema.json` | JSON Schema fragment(s) with `definitions.audit` and `definitions.remediation` sections for each procedure in the file |

Each procedure implements one or both of:
- `Result<Status> Audit<Name>(const <Name>Params& params, IndicatorsTree& indicators, ContextInterface& context)`
- `Result<Status> Remediate<Name>(const <Name>Params& params, IndicatorsTree& indicators, ContextInterface& context)`

Functions return `Status::Compliant` or `Status::NonCompliant` via `indicators.Compliant("msg")` / `indicators.NonCompliant("msg")`, or `Error(...)` on failure.

### How to Add a New Procedure

**Important:** A single file can contain multiple related procedures (e.g., `EnsureFilePermissions.h` defines both `EnsureFilePermissions` and `EnsureFilePermissionsCollection`). The filename does not need to match any individual procedure name — group related procedures together when it makes sense. When adding a new procedure, consider whether it logically belongs in an existing file before creating a new one.

1. **Add the procedure to a header** in `procedures/` (new or existing `.h` file): Define a params struct and declare audit/remediate functions.
   - Struct fields use types: `std::string`, `int`, `bool`, `mode_t`, `regex`, `Pattern`, `Optional<T>`, `Separated<T, char>`, or enum types.
   - Document each field with `///` comments (used by GenInterface.py). Add `/// pattern: <regex>` for validation.

   ```cpp
   struct Audit<Name>Params
   {
       /// Description of the parameter
       std::string requiredParam;

       /// Optional parameter description
       Optional<int> optionalParam;
   };

   Result<Status> Audit<Name>(const Audit<Name>Params& params, IndicatorsTree& indicators, ContextInterface& context);
   ```

2. **Add the implementation** to a `.cpp` file in `procedures/` (matching the header file, not necessarily the procedure name).

3. **Add or update the schema** in a `.schema.json` file in `procedures/` with `definitions.audit` and `definitions.remediation` sections for the new procedure.

4. **Run `GenInterface.py`** (or `pre-commit`): The script parses all procedure headers and **auto-generates** `ProcedureMap.h` and `ProcedureMap.cpp`. These files:
   - Include all procedure headers.
   - Define `Bindings<Params>` specializations (field name arrays + member pointer tuples).
   - Define `MapEnum<E>()` specializations for any custom enums.
   - Populate `Evaluator::mProcedureMap` with `{name, {MakeHandler(Audit...), MakeHandler(Remediate...)}}`.

   **NEVER edit `ProcedureMap.h` or `ProcedureMap.cpp` manually** — they are regenerated by `GenInterface.py`.

5. **Register in CMakeLists.txt**: Add the `.cpp` to the `PROCEDURES` list and the `.schema.json` to the `SCHEMAS` list in `src/modules/complianceengine/src/lib/CMakeLists.txt`. The build enforces that every `.cpp` in `procedures/` is listed and every procedure has a matching schema.

6. **Add unit tests** in `tests/procedures/<Name>Test.cpp`. Use `MockContext` from `tests/MockContext.h` to mock file/command access.

7. **Run pre-commit** to regenerate the ProcedureMap files and validate formatting:
   ```bash
   python3 -m pre_commit run --all-files
   ```

### Key Types for Procedure Development

- `IndicatorsTree`: Call `indicators.Compliant("message")` or `indicators.NonCompliant("message")` to record results.
- `ContextInterface`: Use `context.ExecuteCommand(cmd)` and `context.GetFileContents(path)` for system access. Never call system functions directly — this enables unit testing with `MockContext`.
- `Result<T>`: Either holds a value (`HasValue()`) or an `Error`. Return `Error("msg", errno_code)` on failure.
- `Optional<T>`: For optional procedure parameters. `HasValue()` checks if set.
- `Separated<T, delimiter>`: For pipe/comma-separated list parameters (e.g., `Separated<Pattern, '|'>`).

### Enum Parameters

Enum types allow procedure parameters to accept a fixed set of string labels from JSON. To add an enum parameter:

1. **Define the enum** in the procedure header file, placing it **before** the params struct that uses it. Each enum value must have a `/// label: <json_label>` comment specifying the string used in JSON rule payloads:

   ```cpp
   enum class PackageManagerType
   {
       /// label: autodetect
       Autodetect,

       /// label: rpm
       RPM,

       /// label: dpkg
       DPKG,
   };
   ```

2. **Use the enum type** in the params struct:

   ```cpp
   struct PackageInstalledParams
   {
       /// Package name
       std::string packageName;

       /// Package manager, autodetected by default
       Optional<PackageManagerType> packageManager;
   };
   ```

3. **Run `GenInterface.py`** (or `pre-commit`): It parses the `/// label:` comments and auto-generates a `MapEnum<PackageManagerType>()` specialization in `ProcedureMap.h` that maps JSON string labels to C++ enum values. **Do not write this mapping manually.**

In rule payloads, the enum value is specified by its label string: `{ "PackageInstalled": { "packageName": "nftables", "packageManager": "rpm" } }`.

### Testing ComplianceEngine

Unit tests are in `src/modules/complianceengine/tests/` and `tests/procedures/`. Integration tests use recipe JSON files in `src/modules/test/recipes/complianceengine/`. Run:

```bash
cd build && ctest -R complianceengine --output-on-failure -j$(nproc)
```
