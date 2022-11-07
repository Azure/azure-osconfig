# Prototype of an OSConfig Native Resource Provider (NRP) for Azure Automanage Machine Configuration (MC) 

## About Machine Configuration

Read about Azure Automanage Machine Configuration (formerly called Azure Policy Guest Configuration) at [learn.microsoft.com/machine-configuration](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/).

## Regenerating the NRP code for a changed resource class:

To regenerate code, see [codegen.cmd](codegen.cmd). Warning: regenerating code will overwrite all customizations and additions specific to OSConfig. 

## Building the prototype NRP

The prototype NRP binary (libOSConfig_PrototypeResource.so) is built with rest of OSConfig.

## Validating the prototype NRP with PowerShell and the MC Agent

Follow the instructions at [How to set up a machine configuration authoring environment](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/machine-configuration-create-setup). 

### Installing PowerShell

For example, on Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y wget apt-transport-https software-properties-common
wget -q "https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb"
sudo dpkg -i packages-microsoft-prod.deb
sudo apt-get update
sudo apt-get install -y powershell
```

### Installing the MC Agent from PowerShell

```bash
sudo pwsh
Install-Module -Name GuestConfiguration
Get-Command -Module 'GuestConfiguration'
```

### Running the validation

Copy the build generated artifacts ZIP package OSConfig_Proto_Policy.zip to a new folder or invoke it directly from the build location.

```bash
sudo pwsh
Get-GuestConfigurationPackageComplianceStatus -path <path to the ZIP>/OSConfig_Proto_Policy.zip -Verbose
Start-GuestConfigurationPackageRemediation -path <path to the ZIP>/OSConfig_Proto_Policy.zip -Verbose
```
To view the resource class parameters returned by the Get function:

```bash
sudo pwsh
$x = Get-GuestConfigurationPackageComplianceStatus -path <path to the ZIP>/OSConfig_Proto_Policy.zip -Verbose
$x.resources[0].properties
```
In addition to the MC traces written to the the PowerShell console, you can also see the NRP's own log at `/var/log/osconfig_gc_nrp.log`.