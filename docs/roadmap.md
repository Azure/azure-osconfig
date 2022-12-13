Azure Device OS Configuration (OSConfig) - Roadmap
==================================================
Author: [MariusNi](https://github.com/MariusNi)

# 1. Introduction

Azure Device OS Configuration (OSConfig) is a modular services stack running on a Linux IoT Edge device that facilitates remote device management over Azure as well from local management authorities.

Development of OSConfig happens in stages. This document describes the development roadmap of the OSConfig project.

During any of these stages OSConfig can be extended by adding new [OSConfig Management Modules](modules.md).

For more information on OSConfig and the various component name and other terminology used in this document see the [OSConfig North Star Architecture](architecture.md) document.
 
# 1. Monolithic Agent (codename Iron)

Main target scenarios: apply policy settings and execute reboot and other commands remotely via the IoT Hub.

- In its first release codename Iron OSConfig is a monolithic (all-in-one) PnP Agent including Settings (with basic Device Health Telemetry and few Delivery Optimization (DO) policies configuration) and CommandRunner (including Shutdown, Reboot, and running custom commands/scripts). 
- This agent process runs as a daemon (background service) under root authority to allow its embedded management modules to perform their administrative configuration functions. 
- OSConfig runs under a device identity with a manual IoT Hub connection string read from a local file. 
- There are embedded versions of CommandRunner and Settings hard coded into the agent. 
- The PnP Agent writes telemetry events and traces to its own and to system logs.
- The remote operator uses Azure IoT Explorer and/or Azure Portal or Azure Command Line INterface (CLI) to configure the device.

<img src="assets/1_monolith.png" alt="Iron" width=60%/>
 
# 2. Detached Modules (codename Cobalt)

The modules become detached Shared Object (.so) libraries, partners can develop modules on their own, two new modules Networking and Firewall are added. 

- In its second version codename Cobalt, OSConfig gains detached Management Modules running as Dynamically Loaded Shared Object libraries (.so). 
- Although OSConfig continues to still run all in one process, modules can be dynamically discovered, loaded, and unloaded at run time (and, most importantly, can be developed by other teams). 
- With the Management Modules specification shared with the partners to guide Module development and newly introduced PnP/DTDL-agnostic Module Interface Model (MIM), new modules are decoupled from PnP and easier designed.
- The PnP Agent runs under a module identity dynamically obtained from Azure Identity Service (AIS) and with this it can share the device channel with other PnP agents (ADU, Defender, Edge Runtime, etc.)
- The CommandRunner and Settings modules detach into their own separate module SOs.
- New modules for Networking and Firewall configuration are introduced. 
- New reusable logging library that all OSConfig components can use.
- The Modules Manager is introduced as a static library linked into the agent process.

<img src="assets/2_modules.png" alt="Cobalt" width=60%/>
 
# 3. Detached Agent & Local Management Authorities (codename Nickel)

OSConfig continues to run as a monolithic process and can accept requests from agentless management authorities like OOBE. More module scenarios are enabled (TPM, etc.)

- The Watcher is introduced. OSConfig can accept requests from Local Management Authorities (like OOBE). 
- The Orchestrator is introduced to orchestrate input from multiple Agents/Authorities.
- Modules continue to work unchanged, same as in previous release, as Dynamically Loaded Shared Object libraries exporting the MMI C API. 
- The Modules Manager is introduced and loads the module libraries in-proc.
- Tpm and other new Management Modules can appear (not shown in diagram).
- The agent becomes completely detached from PnP interfaces, MIMs and modules.

<img src="assets/3_watcher.png" alt="Nickel" width=70%/>
 
# 4. Detached Platform (codename Copper)

The Management Platform runs in its own process, separate of the PnP Agent. OSConfig can accept request from both local and remote management authorities.

- The OSConfig separates the Agent and the Management Platform into two separate daemon processes.
- The IPC REST API over Unix Domain Sockets (UDS) and HTTP for MPI is introduced.
- The Management Platform can accept MPI requests from other Agent Authorities (like ADU).
- Modules continue to work unchanged, same as in previous releases, as Dynamically Loaded Shared Object libraries exporting the MMI C API. The Platform loads these libraries in-proc.
- Other Management Modules appear (not shown in diagram).

<img src="assets/4_platform.png" alt="Copper" width=70%/>
  
# 5. Isolated Modules - the current North Star (codename Zinc)

Main target scenario: OSConfig Management Modules run isolated in their own processes, Azure Policy and GitHub

- The IPC REST API over UDS for Management Modules Interface (MMI) is introduced.
- The Module Host is introduced. The Modules Manager instead of loading in-proc the module libraries, it forks a Module Host process to load each module library out-of-proc of the platform.
- Modules continue to work unchanged, same as in previous releases, as Dynamically Loaded Shared Object libraries exporting the MMI C API. The Module Host provides the MMI REST API on top.
- A new OSConfig NRP is added for MC and Azure Policy.
- The local management RC/DC are complemented with a remote Git DC.
- New Storage and Downloader utility libraries are introduced for Platform and Modules to use.
- Other new Management Modules appear (not shown in diagram).

<img src="assets/osconfig.png" alt="Zinc" width=70%/>