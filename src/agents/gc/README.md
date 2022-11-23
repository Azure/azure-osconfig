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

### Running the local validation

Copy the build generated artifacts ZIP package OSConfig_Proto_Policy.zip to a new folder or invoke it directly from the build location.

```bash
sudo pwsh
Start-GuestConfigurationPackageRemediation -path <path to the ZIP>/OSConfig_Proto_Policy.zip -Verbose
Get-GuestConfigurationPackageComplianceStatus -path <path to the ZIP>/OSConfig_Proto_Policy.zip -Verbose
```
To view the resource class parameters returned by the Get function:

```bash
sudo pwsh
$x = Get-GuestConfigurationPackageComplianceStatus -path <path to the ZIP>/OSConfig_Proto_Policy.zip -Verbose
$x.resources[0].properties
```
In addition to the MC traces written to the the PowerShell console, you can also see the NRP's own log at `/var/log/osconfig_gc_nrp.log`.

## Onboarding a device to Arc

Go to Azure Portal | Azure Arc | Add your infrastructure for free | Add your existing server | Generate script. Specify the desired Azure subscription, resource group and OS (Linux) for the device. Copy and run the generated instructions in the local shell on the device to be onboarded.

## Uploading the ZIP artifacts package

The generated artifacts ZIP package OSConfig_Proto_Policy.zip needs to be uploaded to Azure.

Go to Azure Portal and create a Storage Account. In there, go to Containers and create a new container. Then inside of that container upload the artifacts ZIP package OSConfig_Proto_Policy.zip.

Important: after each update of the ZIP package the policy definition will need to be updated.

Once uploaded, generate a SAS token for the package. Two strings will be generated, the first being the token. Copy the second string, which is the URL of the package. This URL string will start with the following:

`https://<storage account>.blob.core.windows.net/<container name>/OSConfig_Proto_Policy.zip...`

Validate that the URL is correct by pasting that URL string into a Web browser and see if the ZIP package gets downloaded.

## Creating a new or changing an existing Azure Policy

Read about [creating custom machine configuration policy definitions](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/machine-configuration-create-definition).

### [If the policy does not already exist] Generating the policy definition and creating the policy from scratch

Read about [creating an Azure Policy definition](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/machine-configuration-create-definition#create-an-azure-policy-definition).

Customize the following commands script with the new package URL. Names in the example below are for the NRP prototype which is configuring the host name of the device:

```bash
sudo pwsh
$PolicyParameterInfo = @(
    @{
        Name = 'HostName'                                               # Policy parameter name (mandatory)
        DisplayName = 'Host Name.'                                      # Policy parameter display name (mandatory)
        Description = "Host Name."                                      # Policy parameter description (optional)
        ResourceType = "OSConfig_PrototypeResource"                     # dsc configuration resource type (mandatory)
        ResourceId = 'Ensure the host name of the device is configured' # dsc configuration resource property name (mandatory)
        ResourcePropertyName = "DesiredString"                          # dsc configuration resource property name (mandatory)
        DefaultValue = 'GCPOCAZ'                                        # Policy parameter default value (optional)
    }
)

New-GuestConfigurationPolicy `<insert here the SAS token URL>' `
    -DisplayName 'Guest Configuration policy to change host name.' `
    -Description 'Guest Configuration policy to change host name.' `
    -Path .\policies\ `
    -Platform Linux `
    -Verbose -PolicyId '<GUID for this policy>' -PolicyVersion 1.0.0.0 -Parameter $PolicyParameterInfo -Mode ApplyAndAutoCorrect
```

The last argument (`-Mode ApplyAndAutoCorrect`) is for remediation, without this the policy will be audit-only (default).

Run these script commands on the Arc device in PowerShell. This will produce a JSON holding the policy definition, copy that JSON, it will be needed for creating the new policy in Azure Portal.

Next, go to Azure Portal | Policy | Definitions, select the subscription, then create a new Policy Definition and fill out:  

- Definition location: the subsription.
- Name: any name (e.g. for the prototype this can be "Ensure the device host name is configured")
- Description: any description"
- Category: Select 'Use Existing' and 'Guest Configuration'.'
- Policy rule: paste the contents of the JSON file containing the policy definition.

Then save.

Next, the new policy needs to be asigned. Go to Azure Portal | Policy | Definitions and asign the policy. Select the subscription and resource group where the policy will be targeted at (to all Arc devices in that group). Select 'Include Arc connected machines' and uncheck the uncheck the 'only show parameters' box. Enter the desired value for the policy parameter, in this case 'DesiredString', containing the desired host name for the device.

To delete a policy, delete it first from Azure Portal | Policy | Assignments, then from Azure Portal | Policy | Definitions.

One more step is necessary for so called brownfield devices, which are created (onboarded to Arc) before the policy is created (greenfield devices are created after the policy). For these brownfield devices, go to Azure Portal | Policy | Compliance, select the policy and then create a remediation task.

### [If the policy already exists] Editing the existing policy definition

If there is a new artifacts ZIP package, the policy definition can be manually updated for it. We need the package URL and the file hash of the package. 

On the device, obtain the hash with:

```bash
sudo pwsh
get-filehash <local path to package>.zip
```

Keep the existing policy assignment and change its definition. Go to the Azure Portal | Policy | Definitions, select the policy and edit. There will be several instances of the URL (`"contentUri":`) and the hash (`"contentHash":`) in the policy defition JSON. Carefully replace all with the new token URL and hash. Paste in the new  policy definition contents and save.

If necessary, create a new remediation task. Then wait for the updated policy to be applied to all Arc devices in the designated group.

### Monitoring the policy activity from the device side

On the device, the GC Agent will check on the policy every 15 minutes. The GC Agent logs will record this activity. The logs are at `/var/lib/GuestConfig`.

View last with:

```bash
sudo cd /var/lib/GuestConfig/arc_policy_logs\
tail gc_agent.log -f
```

## Registering the Arc device with a different name

### Disconnecting the device from Azure 

```bash
sudo azcmagent disconnect
```

### Cleaning up the device

```bash
sudo service gcad stop
sudo rm -rf /var/lib/GuestConfig/
```

### Restarting the agent

```bash
sudo service gcad start
```

### Re-registering the device into Azure with a different name

Execute following export commands copied from the Arc onboarding script and register the device with the new name: 

```bash
export subscriptionId="...";
export resourceGroup="...";
export tenantId="...";
export location="...";
export authType="token";
export correlationId="...";
export cloud="AzureCloud";

sudo azcmagent connect --resource-group "$resourceGroup" --tenant-id "$tenantId" --location "$location" --subscription-id "$subscriptionId" --cloud "$cloud" --correlation-id "$correlationId" --resource-name "<new name>";
```