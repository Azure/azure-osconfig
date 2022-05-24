# OSConfig

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.md)

Azure Device OS Configuration (OSConfig) is a modular services stack running on a Linux IoT Edge device that facilitates remote device management over Azure as well from local management authorities.

OSConfig contains an Agent, a Management Platform (Modules Manager) and several Management Modules.

For more information on OSConfig see [OSConfig North Star Architecture](docs/architecture.md), [OSConfig Roadmap](docs/roadmap.md) and [OSConfig Management Modules](docs/modules.md).

For our code of conduct and contributing instructions see [CONTRIBUTING](CONTRIBUTING.md). For our approach to security see [SECURITY](SECURITY.md).

### C Standard

OSConfig's C/C++ code currently targets compliance with C11.

## Getting started

### Prerequisites

Make sure all dependencies are installed. For example, on Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev libgtest-dev libgmock-dev rapidjson-dev
```

Verify that CMake is at least version 3.2.0 and gcc is at least version 4.4.7.

```bash
cmake --version
gcc --version
```

Add the dependencies necessary to build *TraceLogging* as described at [github.com/microsoft/tracelogging/](https://github.com/microsoft/tracelogging/blob/master/README.md#dependencies).

Install and configure the *Azure IoT Identity Service (AIS)* package as described at [azure.github.io/iot-identity-service/](https://azure.github.io/iot-identity-service/).

### Build

Create a folder build folder under the repo root /build

```bash
mkdir build && cd build
```

Build with the following commands issued from under the build subfolder:

```bash
cmake ../src -DCMAKE_BUILD_TYPE=Release|Debug -Duse_prov_client=ON -Dhsm_type_symm_key=ON -DBUILD_TESTS=ON|OFF
cmake --build . --config Release|Debug  --target install
```
The following OSConfig files are binplaced at build time:

Source | Destination | Description
-----|-----|-----
[src/agents/pnp/](src/agents/pnp/) | /usr/bin/osconfig | The OSConfig Agent and the main control binary for OSConfig
[src/platform/](src/platform/) | /usr/bin/osconfig-platform | The OSConfig Platform binary
[src/agents/pnp/daemon/osconfig.conn](src/agents/pnp/daemon/osconfig.conn) | /etc/osconfig/osconfig.conn | Holds manual IoT Hub device connection id string (optional)
[src/agents/pnp/daemon/osconfig.json](src/agents/pnp/daemon/osconfig.json) | /etc/osconfig/osconfig.json | The main configuration file for OSConfig
[src/modules/commandrunner/assets/osconfig_commandrunner.cache](src/modules/commandrunner/assets/osconfig_commandrunner.cache) | /etc/osconfig/osconfig_commandrunner.cache | Persistent cache for the CommandRunner module
[src/agents/pnp/daemon/osconfig.service](src/agents/pnp/daemon/osconfig.service) | /etc/systemd/system/osconfig.service | The service unit for the OSConfig Agent
[src/platform/daemon/osconfig-platform.service](src/platform/daemon/osconfig-platform.service) | /etc/systemd/system/osconfig-platform.service | The service unit for the OSConfig Platform
[src/agents/pnp/daemon/osconfig.toml](src/agents/pnp/daemon/osconfig.toml) | /etc/aziot/identityd/config.d/osconfig.toml | The OSConfig Module configuration for AIS
[src/agents/pnp/telemetryevents/OsConfigAgentTelemetry.conf](src/agents/pnp/telemetryevents/OsConfigAgentTelemetry.conf) | /etc/azure-device-telemetryd/OsConfigAgentTelemetry.conf | Device health telemetry events (not active on Ubuntu and Debian)
[src/modules/commandrunner/](src/modules/commandrunner/) | /usr/lib/osconfig/commandrunner.so | The CommandRunner module binary
[src/modules/firewall/](src/modules/firewall/) | /usr/lib/osconfig/firewall.so | The Firewall module binary
[src/modules/networking/](src/modules/networking/) | /usr/lib/osconfig/networking.so | The Networking module binary
[src/modules/settings/](src/modules/settings/) | /usr/lib/osconfig/settings.so | The Settings module binary
[src/modules/tpm/](src/modules/tpm/) | /usr/lib/osconfig/tpm.so | The TPM module binary
[src/modules/hostname/](src/modules/hostname/) | /usr/lib/osconfig/hostname.so | The HostName module binary

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
sudo cat /var/log/osconfig_pnp_agent.log
sudo cat /var/log/osconfig_platform.log
sudo cat /var/log/osconfig_commandrunner.log
sudo cat /var/log/osconfig_networking.log
sudo cat /var/log/osconfig_firewall.log
sudo cat /var/log/osconfig_tpm.log
...
```

Each of these log files when it reaches maximum size (128 KB) gets rolled over to a file with the same name and a .bak extension (osconfig_agent.bak, for example).

When OSConfig exists prematurely (crashes) the Agent's log (osconfig_pnp_agent.log) at the very end may contain an indication of that. For example:

```
[ERROR] OSConfig crash due to segmentation fault (SIGSEGV) during MpiGet to Firewall.FirewallRules
```
## Local Management

OSConfig uses two local files as local digital twins in MIM JSON format:

`/etc/osconfig/osconfig_desired.json` contains desired configuration (to be applied to the device)

`/etc/osconfig/osconfig_reported.json` contains reported configuration (to be reported from the device)

## Configuration

OSConfig has a general configuration file at `/etc/osconfig/osconfig.json` that can be used to configure how it runs. After changing this configuration file, to make OSConfig apply the change, restart or refresh OSConfig with the command:

```bash
sudo systemctl kill -s SIGHUP osconfig.service
```

### Adjusting the reporting interval

OSConfig periodically reports device data at a default time period of 30 seconds. This interval period can be adjusted between 1 second and 86,400 seconds (24 hours) via the OSConfig general configuration file at `/etc/osconfig/osconfig.json`. Edit there the integer value named "ReportingIntervalSeconds" to a value between 1 and 86400:

```json
{
    "ReportingIntervalSeconds": 30
}
```

### Enabling logging of system commands executed by OSConfig for debugging purposes

Command logging means that OSConfig will log all input and output from internally executed system commands.

Generally it is not recommended to run OSConfig with command logging enabled.

To enable command logging for debugging purposes, edit the OSConfig general configuration file `/etc/osconfig/osconfig.json` and set there (or add if needed) a integer value named "CommandLogging" to a non zero value:

```json
{
    "CommandLogging": 1
}
```

To disable command logging, set "CommandLogging" to 0.

### Enabling full logging for debugging purposes

Full logging means that OSConfig will log all input and output from and to IoT Hub, AIS and local configuration.

Generally it is not recommended to run OSConfig with full logging enabled.

To enable full logging for debugging purposes, edit the OSConfig general configuration file `/etc/osconfig/osconfig.json` and set there (or add if needed) a integer value named "FullLogging" to a non zero value:

```json
{
    "FullLogging": 1
}
```

To disable full logging, set "FullLogging" to 0.

### Enabling local management

By default the reported configuration is not saved locally to `/etc/osconfig/osconfig_reported.json` (local reporting is disabled) and desired configuration is not picked-up from `/etc/osconfig/osconfig_desired.json`.

To enable local management, edit the OSConfig general configuration file `/etc/osconfig/osconfig.json` and set there (or add if needed) a integer value named "LocalManagement" to a non zero value:

```json
{
    "LocalManagement": 1
}
```
To disable local management, set "LocalManagement" to 0.

### Changing the protocol OSConfig uses to connect to the IoT Hub

The networking protocol that OSConfig uses to connect to the IoT Hub is configured in the OSConfig general configuration file `/etc/osconfig/osconfig.json`:

```json
{
    "Protocol": 2
}
```

OSConfig currently supports the following protocol values:

Value | Description
-----|-----
0 | Decided by OSConfig (currently this is MQTT)
1 | MQTT
2 | MQTT over Web Socket

## HTTP proxy configuration

When the configured protocol is set to value 2 (MQTT over Web Socket) OSConfig attempts to use the HTTP proxy information configured in one of the following environment variables, the first such variable that is locally present:

1. `http_proxy`
1. `https_proxy`
1. `HTTP_PROXY`
1. `HTTPS_PROXY`

OSConfig supports the HTTP proxy configuration to be in one of the following formats:

- `http://server:port`
- `http://username:password@server:port`
- `http://domain\username:password@server:port`

Where the prefix is either lowercase `http` or uppercase `HTTP` and the username and password can contain `@` characters escaped as `\@`.

For example: `http://username\@mail.foo:p\@ssw\@rd@www.foo.org:100` where username is `username@mail.foo`, password is `p@ssw@rd`, the proxy server is `www.foo.org` and the port is 100.

The environment variable needs to be set for the root user account. For example, for a fictive proxy server, user and password, the environment variable `http_proxy` can be set for the root user manually via console with:

```
sudo -E su
export http_proxy=http://user:password@wwww.foo.org:100//
```

The environment variable can also be set in the OSConfig service unit file by uncommenting and editing the following line in [src/agents/pnp/daemon/osconfig.service](src/agents/pnp/daemon/osconfig.service):

```
# Uncomment and edit next line to configure OSConfig with a proxy to connect to the IoT Hub
# Environment="http_proxy=http://user:password@wwww.foo.org:100//"
```

After editing the service unit file, stop and disable osconfig.service, rebuild OSConfig, then enable and start osconfig.service:

```bash
sudo systemctl stop osconfig.service
sudo systemctl disable osconfig.service
cd build
cmake ../src -DCMAKE_BUILD_TYPE=Release|Debug -Duse_prov_client=ON -Dhsm_type_symm_key=ON -DBUILD_TESTS=ON|OFF
cmake --build . --config Release|Debug  --target install
sudo systemctl enable osconfig.service
sudo systemctl start osconfig.service
```

## Health Telemetry

OSConfig emits device health telemetry via TraceLogging calls for the events configured in [OsConfigAgentTelemetry.conf](src/agents/pnp/telemetryevents/OsConfigAgentTelemetry.conf). This telemetry can be configured via OSConfig's [Settings module](src/modules/settings/). The service responsible of collecting these local TraceLogging calls (azure-device-telemetryd.service) is not currently available for Ubuntu and Debian and there is no emitted telemetry in that case.

---

Microsoft may collect performance and usage information which may be used to provide and improve Microsoft products and services and enhance users experience. To learn more, review the [privacy statement](https://go.microsoft.com/fwlink/?LinkId=521839&clcid=0x409).