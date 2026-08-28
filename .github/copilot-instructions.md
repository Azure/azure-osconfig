# Copilot Coding Agent Instructions for azure-osconfig

> Trust these instructions first. Only search the repo if information here is incomplete or found to be incorrect.

## Project Overview

Azure OSConfig is a **modular security configuration stack for Linux Edge devices**. It provides multi-authority device management over Azure (IoT Hub, Azure Policy), GitOps, and local management (RC/DC files). Written in **C11/C++11** using **CMake** and **vcpkg** for dependency management. Targets Linux only.

The architecture has three layers: **Adapters** (IoT Hub PnP agent, RC/DC watcher, GitOps watcher) → **Management Platform** (REST API over Unix Domain Sockets) → **Management Modules** (`.so` shared libraries loaded at runtime). Modules are the primary extension point.

## Repository Layout

```
src/                          # All source code
  CMakeLists.txt              # Root CMake file (project config, vcpkg integration, build options)
  vcpkg.json                  # Dependencies: openssl, curl, sqlite3, nlohmann-json, gtest
  vcpkg-configuration.json    # vcpkg registry baseline
  adapters/                   # Agent adapters (PnP IoT Hub client, MC machine config)
    pnp/                      # PnP agent (main binary: /usr/bin/osconfig)
      azure-iot-sdk-c/        # Git submodule - Azure IoT C SDK
    mc/                       # Machine configuration adapters (ASB, SSH)
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
- Test binaries are in: `build/common/commonutils/`, `build/platform/tests/`, `build/modules/commandrunner/tests/`, `build/modules/configuration/tests/`.
- Total test time: ~10 seconds.

### Formatting and Linting (required before PR)

Always run pre-commit before submitting changes:
```bash
python3 -m pre_commit run --all-files
```

This runs: trailing whitespace fix, end-of-file fix, LF line endings, clang-format (v14, on telemetry files only), and clang-tidy (on telemetry C++ files only).

**Important**: clang-format and clang-tidy in pre-commit only apply to files in `src/common/telemetry/`. Other C/C++ files are not auto-formatted but must follow the style in `docs/style.md`.

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

**Only 4 modules are actively built:** commandrunner, securitybaseline, configuration, deviceinfo. Other module directories (adhs, firewall, hostname, networking, pmc, tpm, ztsi) exist but are not included in the default build.
