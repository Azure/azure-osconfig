# OSConfig

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.md)

Azure OSConfig is a modular security configuration stack for Linux Edge devices. OSConfig supports multi-authority device management over Azure, GitOps, as well as local management.

For more information on OSConfig see [OSConfig North Star Architecture](docs/architecture.md) and [OSConfig Management Modules](docs/modules.md).

For our code of conduct and contributing instructions see [CONTRIBUTING](CONTRIBUTING.md). For our approach to security see [SECURITY](SECURITY.md).

For the list of our published binary packages and instructions how to install them see [devops/README.md](devops/README.md).

### C Standard

OSConfig's C/C++ code currently targets compliance with C11.

## Getting started

### Prerequisites

Build environments have many dependencies required, the easiest way to get started is to use our pre-defined container environments in [devops/docker](devops/docker/).

Make sure all dependencies are installed for your distribution. All of our supported distributions are documented in the Dockerfiles under [devops/docker](devops/docker/) (additional packages may be present for CI which are not necessary for building). The following packages are typically required however the package names may vary across distributions.

**Common build dependencies across distributions:**
- git, tar, unzip, zip
- cmake (>= 3.21)
- build-essential (gcc >= 4.4.7, g++, make)
- python3 and the OS `openssl` command for test fixtures when tests are enabled

This branch's distilled telemetry prototype does not fetch or build the 1DS
C++ SDK, curl, OpenSSL, zlib, SQLite, or nlohmann JSON. GoogleTest is still fetched
when tests are enabled. The sender prefers **OS-installed OpenSSL 3 or 1.1**.
When neither supported provider is available, the owned executable can use
the in-tree [mintls fallback](src/common/mintls/README.md), built directly from
a maintained client-only source subset, with no TLS dependency download.
Both paths require trusted certificates; no root bundle is shipped.
Fallback cannot replace a failed OS-provider initialization or handshake.
Fallback validation, size measurement, and the final compiler/distribution
matrix remain pending. CI/download utilities may still use the `curl`
command; that is not a telemetry library dependency.

Refer to the specific Dockerfile for your distribution under [devops/docker/](devops/docker/) for the complete and up-to-date list of dependencies. See the following example. The environments do also contain tools used in our CI which are not required for building (bc, jq, libubsan1, libasan8, clang, clang-tools, file).

#### Example prerequisite install on Ubuntu 24.04
Using the pre-defined [devops/docker/ubuntu-24.04-amd64/Dockerfile](devops/docker/ubuntu-24.04-amd64/Dockerfile) we can simply use the `RUN` commands to install all needed pre-requisites. We also need to run `sudo -i` since many commands require `root` (some uneeded packages only used by CI were omitted).

```bash
sudo apt -y update && sudo apt-get -y install software-properties-common
sudo apt -y update && sudo apt-get -y install build-essential cmake git curl pkg-config tar unzip wget zip
```

Verify that CMake is at least version 3.21 and gcc is at least version 4.4.7.

```bash
cmake --version
gcc --version
```

For contributing to the project, also install the following prerequisites for [pre-commit](https://pre-commit.com/):

```bash
sudo apt-get install python3
pip3 install pre_commit
python3 -m pre_commit install
```

### Build

Create a folder build folder under the repo root /build

```bash
mkdir build && cd build
```

Build with the following commands issued from under the build subfolder:

```bash
cmake ../src -DCMAKE_BUILD_TYPE=Release|Debug -DBUILD_TESTS=ON|OFF
cmake --build . --config Release|Debug  --target install
```
The following OSConfig files are binplaced at build time:

Source | Destination | Description
-----|-----|-----
[src/adapters/agent/](src/adapters/agent/) | /usr/bin/osconfig | The OSConfig Agent and the main control binary for OSConfig
[src/platform/](src/platform/) | /usr/bin/osconfig-platform | The OSConfig Platform binary
[src/adapters/agent/daemon/osconfig.json](src/adapters/agent/daemon/osconfig.json) | /etc/osconfig/osconfig.json | The main configuration file for OSConfig
[src/adapters/agent/daemon/osconfig.service](src/adapters/agent/daemon/osconfig.service) | /etc/systemd/system/osconfig.service | The service unit for the OSConfig Agent
[src/platform/daemon/osconfig-platform.service](src/platform/daemon/osconfig-platform.service) | /etc/systemd/system/osconfig-platform.service | The service unit for the OSConfig Platform
[src/modules/deviceinfo/](src/modules/deviceinfo/) | /usr/lib/osconfig/deviceinfo.so | The DeviceInfo module binary
[src/modules/configuration/](src/modules/configuration/) | /usr/lib/osconfig/configuration.so | The Configuration module binary
[src/modules/securitybaseline/](src/modules/securitybaseline/) | /usr/lib/osconfig/securitybaseline.so | The SecurityBaseline module binary
[src/common/telemetry/](src/common/telemetry/) | /usr/bin/OSConfigTelemetry and /usr/lib/osconfig/OSConfigTelemetry | Owned C telemetry worker, installed beside executable/module callers when telemetry is enabled

### Distilled telemetry prototype

The four ASB/SSH policy ZIPs package the same C worker beside
`libOsConfigResource.so`, under the existing `OSConfigTelemetry` filename.
`telemetrybin` remains a build target for that executable. Producer calls use
synchronous, bounded memory-only IPC; there is no JSONL spool, SDK queue,
database, shutdown upload, or event replay. The old file-input CLI is removed.
Existing crash files and SDK caches are not read or deleted by this migration.
Previous-crash evidence in the component log still generates `CrashDetected`
at the next startup.

The prototype preserves `StatusTrace`, `BaselineRun`, `RuleComplete`, and
`CrashDetected`, with their existing string-valued custom properties.
The parent uses a 500 ms cumulative telemetry-work budget and 500 ms per
operation, with a 10-minute worker lifetime. Audit time between calls does not
consume the work budget. Telemetry failure drops the event without changing
the policy result. Collector suppression lasts for the invocation; full
cross-invocation control handling remains a production follow-up.

Configure `OsConfigTelemetryApiKey` before CMake to embed the ingestion key in
the worker. A present runtime environment variable overrides it; an empty or
missing effective key fails explicitly without sending. No Debug/Release
setting selects PROD/NONPROD: the key determines the project.
The separate `telemetryariatest --send-status-trace-10000` remains an explicit
live diagnostic, not installed, packaged, or run by CTest. Its demonstration
of NONPROD delivery does not validate the newly wired policy path.

### Enable and start OSConfig for the first time

Enable and start OSConfig for the first time by enabling and starting the OSConfig Agent Daemon (`osconfig`):

```bash
sudo systemctl daemon-reload
sudo systemctl enable osconfig
sudo systemctl start osconfig
```

The OSConfig Agent service is configured to be allowed to be restarted (automatically by systemd or manually by user) for a maximum number of 3 times at 5 minutes intervals. There is a total delay of 16 minutes before the OSConfig Agent service could be restarted again by the user unless the user reboots the device.

The OSCOnfig Management Platform Daemon (`osconfig-platform`) is automatically started and stopped by the OSConfig Agent service (`osconfig`) but also can be manually started and stopped separately by itself.

Other daemon control operations:

```bash
sudo systemctl status osconfig | osconfig-platform
sudo systemctl disable osconfig | osconfig-platform
sudo systemctl stop osconfig  | osconfig-platform
```
To replace a service binary while OSConfig is running: stop the Agent daemon, rebuild, start the Agent daemon.
To replace a service unit while the daemon is running: stop the Agent daemon, disable the Agent amnd Platform daemons, rebuild, reload daemons, start and enable the Agent daemon.

## Logs

OSConfig logs to its own logs at `/var/log/osconfig*.log*`:

```bash
sudo cat /var/log/osconfig_agent.log
sudo cat /var/log/osconfig_platform.log
sudo cat /var/log/osconfig_commandrunner.log
sudo cat /var/log/osconfig_networking.log
sudo cat /var/log/osconfig_firewall.log
sudo cat /var/log/osconfig_tpm.log
...
```

Each of these log files when it reaches maximum size (128 KB) gets rolled over to a file with the same name and a .bak extension (osconfig_agent.bak, for example).

When OSConfig exists prematurely (crashes) the Agent's log (osconfig_agent.log) at the very end may contain an indication of that. For example:

```
[ERROR] OSConfig crash due to segmentation fault (SIGSEGV) during MpiGet to Firewall.FirewallRules
```
Only the root user can view these log files.

## Configuration

OSConfig can be configured via `/etc/osconfig/osconfig.json`. After changing this configuration file, restart OSConfig to apply the configuration changes. Only the root user can view or edit this configuration file.

### Adjusting the reporting interval

OSConfig periodically reports device data at a default time period of 30 seconds. This interval period can be adjusted between 1 second and 86,400 seconds (24 hours) via the OSConfig general configuration file at `/etc/osconfig/osconfig.json`. Edit there the integer value named "ReportingIntervalSeconds" to a value between 1 and 86400:

```json
{
    "ReportingIntervalSeconds": 30
}
```

This interval is used for RC/DC, GitOps DC, and IoT Hub processing.

### Enabling debug logging

Debug logging means that OSConfig will log all input and output from and to all management authority channels, as well as all input and output from system commands executed by Agent, Platform and Modules.

Generally it is not recommended to run OSConfig with debug logging enabled.

To enable debug logging, edit the OSConfig general configuration file `/etc/osconfig/osconfig.json` and set there (or add if needed) an integer value named "LoggingLevel" to a value 7:

```json
{
    "LoggingLevel": 7
}
```

To disable debug logging, set "LoggingLevel" to 6 (informational logging, default).

## Local Management over RC/DC

OSConfig uses two local files as local digital twins in MIM JSON payload format:

`/etc/osconfig/osconfig_desired.json` contains desired configuration (to be applied to the device)

`/etc/osconfig/osconfig_reported.json` contains reported configuration (to be reported from the device)

This pair of files are called Reported Configuration (RC) and Desired Configuration (DC) or RC/DC.

Once created, only the root user can view these files or change the DC file.

By default, the reported configuration is locally saved to the DC file at `/etc/osconfig/osconfig_reported.json` (local reporting is enabled) and desired configuration is picked-up from the DC file at `/etc/osconfig/osconfig_desired.json`.

To disable local management, edit the OSConfig general configuration file `/etc/osconfig/osconfig.json` and set there (or add if needed) an integer value named "LocalManagement" to a zero value:

```json
{
    "LocalManagement": 0
}
```
To enable local management, set "LocalManagement" to 1.

### Desired Configuration (DC) management over GitOps

OSConfig can apply to the device desired configuration in MIM JSON payload format (same as for RC/DC) read from a Git repository and branch. The DC file must be named `osconfig_desired.json` and be placed in the root of the repository.

By default, desired configuration (DC) over GitOps is disabled and there are no configured Git repository or branch.

To enable GitOps DC management, edit the OSConfig general configuration file `/etc/osconfig/osconfig.json` and there:

1. Set (or add if needed) a string value named "GitRepositoryUrl" to a string value containing the string that can be used to clone a Git repository, for example (this example uses OSConfig's own repository but can be anything):

```json
{
    "GitRepositoryUrl": "https://github.com/Azure/azure-osconfig"
}
```

For HTTPS cloning of a private Git repository, add necessary credentials to the "GitRepositoryUrl" such as, for example: `https://<username>:<password>@github.com/path/to/repo`. For SSH cloning, configure authetication separately on the device so OSConfig can use it.

2. Set (or add if needed) a string value named "GitBranch" to a string value containing the Git branch name where the DC file is located, for example:

```json
{
    "GitBranch": "name/branch"
}
```

Set (or add if needed) an integer value named "GitManagement" to a non-zero value to enable GitOps DC management:

```json
{
    "GitManagement": 1
}
```
To disable GitOps DC management, set "GitManagement" to 0.

OSConfig clones locally the configured Git DC file and branch to `/etc/osconfig/gitops/osconfig_desired.json`. This Git clone is automatically deleted when the OSConfig Agent (Watcher) terminates. While active, the cloned DC file is protected for root user access only.

---

Microsoft may collect performance and usage information which may be used to provide and improve Microsoft products and services and enhance users experience. To learn more, review the [privacy statement](https://go.microsoft.com/fwlink/?LinkId=521839&clcid=0x409).
