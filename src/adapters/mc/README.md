# OSConfig Universal Native Resource Provider (NRP) for Azure Automanage Machine Configuration (MC)

## 1. About Machine Configuration

Read about Azure Automanage Machine Configuration (formerly called Azure Policy Guest Configuration) at [learn.microsoft.com/machine-configuration](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/).

## 2. Regenerating the NRP code for a changed resource class

To regenerate code, see [codegen.cmd](codegen.cmd).

 > **Warning**
 > Regenerating code will overwrite all customizations and additions specific to OSConfig.

## 3. Building the OSConfig Universal NRP

The OSConfig Universal NRP binary (libOsConfigResource.so) is built with rest of OSConfig.

### 3.1. Building the universal NRP binary on Ubuntu 14

The OSConfig Universal NRP binary is built on Ubuntu 14 with gcc 4.8 in order for this same binary to run as-is on all newer Linux distros.

Install Ubuntu 14.04 LTS from one of the archived locations such as at [Ubuntu](https://www.releases.ubuntu.com/14.04/), or [Ubuntu MATE](https://releases.ubuntu-mate.org/archived/14.04/amd64/)).

Install the minimal set of dependencies necessary:

```bash
sudo apt-get install -y gcc git cmake build-essential
```

Verify that gcc is 4.8:

```bash
gcc --version
```
Clone OSConfig locally and build from the main branch using the following simplified commands:

```bash
$ cmake ../src -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config Release  --target all
```

This builds the libOsConfigResource.so binary and the ZIP artifacts package, for example:

```bash
$ cmake ../src -DCMAKE_BUILD_TYPE=Release
-- The C compiler identification is GNU 4.8.4
...
-- Distro: Linux Ubuntu 14.04 trusty
-- osconfig v1.0.5.20231208
...

$ sudo cmake --build . --config Release  --target all
Scanning dependencies of target logging
[  4%] Building C object common/logging/CMakeFiles/logging.dir/Logging.c.o
[  9%] Linking C static library liblogging.a
[  9%] Built target logging
Scanning dependencies of target parsonlib
[ 13%] Building C object common/parson/CMakeFiles/parsonlib.dir/parson.c.o
[ 18%] Linking C static library libparsonlib.a
[ 18%] Built target parsonlib
Scanning dependencies of target commonutils
[ 22%] Building C object common/commonutils/CMakeFiles/commonutils.dir/CommandUtils.c.o
[ 27%] Building C object common/commonutils/CMakeFiles/commonutils.dir/ConfigUtils.c.o
[ 31%] Building C object common/commonutils/CMakeFiles/commonutils.dir/DaemonUtils.c.o
[ 36%] Building C object common/commonutils/CMakeFiles/commonutils.dir/DeviceInfoUtils.c.o
[ 40%] Building C object common/commonutils/CMakeFiles/commonutils.dir/FileUtils.c.o
[ 45%] Building C object common/commonutils/CMakeFiles/commonutils.dir/OtherUtils.c.o
[ 50%] Building C object common/commonutils/CMakeFiles/commonutils.dir/ProxyUtils.c.o
[ 54%] Building C object common/commonutils/CMakeFiles/commonutils.dir/SocketUtils.c.o
[ 59%] Building C object common/commonutils/CMakeFiles/commonutils.dir/SshUtils.c.o
[ 63%] Building C object common/commonutils/CMakeFiles/commonutils.dir/UrlUtils.c.o
[ 68%] Building C object common/commonutils/CMakeFiles/commonutils.dir/UserUtils.c.o
[ 72%] Linking C static library libcommonutils.a
[ 72%] Built target commonutils
Scanning dependencies of target mpiclient
[ 77%] Building C object common/mpiclient/CMakeFiles/mpiclient.dir/MpiClient.c.o
[ 81%] Linking C static library libmpiclient.a
[ 81%] Built target mpiclient
Scanning dependencies of target OsConfigResource
[ 86%] Building C object adapters/mc/CMakeFiles/OsConfigResource.dir/module.c.o
[ 90%] Building C object adapters/mc/CMakeFiles/OsConfigResource.dir/schema.c.o
[ 95%] Building C object adapters/mc/CMakeFiles/OsConfigResource.dir/OsConfigResource.c.o
[100%] Linking C shared library libOsConfigResource.so
[100%] Built target OsConfigResource
Scanning dependencies of target stage_create_zip
[100%] Built target stage_create_zip
Scanning dependencies of target create_zip
.
./OsConfigPolicy.mof
./Modules
./Modules/DscNativeResources
./Modules/DscNativeResources/OsConfigResource
./Modules/DscNativeResources/OsConfigResource/libOsConfigResource.so
./OsConfigPolicy.metaconfig.json
[100%] Built target create_zip
```

## 4. Validating the OSConfig Universal NRP locally with PowerShell and the MC Agent

Follow the instructions at [How to set up a machine configuration authoring environment](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/machine-configuration-create-setup).

### 4.1. Installing PowerShell

For example, on Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y wget apt-transport-https software-properties-common
wget -q "https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb"
sudo dpkg -i packages-microsoft-prod.deb
sudo apt-get update
sudo apt-get install -y powershell
```

### 4.2. Installing the MC Agent

```bash
sudo pwsh
Install-Module -Name GuestConfiguration
Get-Command -Module 'GuestConfiguration'
```

### 4.3. Running the validation

Copy the build generated artifacts ZIP package OsConfigPolicy.zip to a new folder or invoke it directly from the build location.

```bash
sudo pwsh
Start-GuestConfigurationPackageRemediation -path <path to the ZIP>/OsConfigPolicy.zip -Verbose
Get-GuestConfigurationPackageComplianceStatus -path <path to the ZIP>/OsConfigPolicy.zip -Verbose
```
To view the resource class parameters returned by the Get function:

```bash
sudo pwsh
$x = Get-GuestConfigurationPackageComplianceStatus -path <path to the ZIP>/OsConfigPolicy.zip -Verbose
$x.resources[0].properties
```
In addition to the MC traces written to the the PowerShell console, you can also see the NRP's own log at `/var/log/osconfig_mc_nrp.log`.

## 5. Onboarding a device to Arc

1. Go to Azure Portal | Azure Arc | Add your infrastructure for free | Add your existing server | Generate script.
2. Specify the desired Azure subscription, resource group and OS (Linux) for the device.
3. Copy and run the generated instructions in the local shell on the device to be onboarded:

```bash
export subscriptionId="...";
export resourceGroup="...";
export tenantId="...";
export location="...";
export authType="...";
export correlationId="...";
export cloud="AzureCloud";

output=$(wget https://aka.ms/azcmagent -O ~/install_linux_azcmagent.sh 2>&1);
if [ $? != 0 ]; then wget -qO- --method=PUT --body-data="{\"subscriptionId\":\"$subscriptionId\",\"resourceGroup\":\"$resourceGroup\",\"tenantId\":\"$tenantId\",\"location\":\"$location\",\"correlationId\":\"$correlationId\",\"authType\":\"$authType\",\"messageType\":\"DownloadScriptFailed\",\"message\":\"$output\"}" "https://gbl.his.arc.azure.com/log" &> /dev/null || true; fi;
echo "$output";

bash ~/install_linux_azcmagent.sh;

sudo azcmagent connect --resource-group "$resourceGroup" --tenant-id "$tenantId" --location "$location" --subscription-id "$subscriptionId" --cloud "$cloud" --correlation-id "$correlationId";
```

## 6. Uploading the ZIP artifacts package

The generated artifacts ZIP package OsConfigPolicy.zip needs to be uploaded to Azure.

Go to Azure Portal and create a Storage Account. In there, go to Containers and create a new container. Then inside of that container upload the artifacts ZIP package OsConfigPolicy.zip.

> **Important**
> After each update of the ZIP package the policy definition will need to be updated.

Once uploaded, generate a SAS token for the package. Two strings will be generated, the first being the token. Copy the second string, which is the URL of the package. This URL string will start with the following:

`https://<storage account>.blob.core.windows.net/<container name>/OsConfigPolicy.zip...`

Validate that the URL is correct by pasting that URL string into a Web browser and see if the ZIP package gets downloaded.

## 7. Creating a new or changing an existing Azure Policy

### 7.1. [If the policy does not already exist] Generating the policy definition

Read about [creating an Azure Policy definition](https://learn.microsoft.com/en-us/azure/governance/machine-configuration/machine-configuration-create-definition#create-an-azure-policy-definition).

#### 7.1.1. Generating the policy definition

Customize the following command with the package SAS token URL and a new GUID for this policy and to optionally allow certain parameters to be editable at policy assignment time, as in this example:

```bash
sudo pwsh
$PolicyParameterInfo = @(
    @{
        Name = 'AllowUsers'
        DisplayName = 'Allowed users for SSH'
        Description = "List of users to be allowed to connect with SSH. Default is all authenticated users ('*@*')"
        ResourceType = "OsConfigResource"
        ResourceId = 'Ensure allowed users for SSH access are configured'
        ResourcePropertyName = "DesiredObjectValue"
        DefaultValue = '*@*'
    },
    @{
        Name = 'DenyUsers'
        DisplayName = 'Denied users for SSH'
        Description = "List of users to be denied to connect with SSH. Default is root"
        ResourceType = "OsConfigResource"
        ResourceId = 'Ensure denied users for SSH are configured'
        ResourcePropertyName = "DesiredObjectValue"
        DefaultValue = 'root'
    },
    @{
        Name = 'AllowGroups'
        DisplayName = 'Allowed groups for SSH'
        Description = "List of user groups to be allowed to connect with SSH. Default is all groups ('*')"
        ResourceType = "OsConfigResource"
        ResourceId = 'Ensure allowed groups for SSH are configured'
        ResourcePropertyName = "DesiredObjectValue"
        DefaultValue = '*'
    },
    @{
        Name = 'DenyGroups'
        DisplayName = 'Denied groups for SSH'
        Description = "List of user groups to be denied to connect with SSH. Default is root"
        ResourceType = "OsConfigResource"
        ResourceId = 'Ensure denied groups for SSH are configured'
        ResourcePropertyName = "DesiredObjectValue"
        DefaultValue = 'root'
    }
)
New-GuestConfigurationPolicy `
    -ContentUri '<SAS token URL for the artifacts package>' `
    -DisplayName 'Ensure that the SSH Server is securely configured on the Linux device' `
    -Description 'This policy ensures that the SSH Server is securely configured on the Linux device' `
    -Path .\policies\ `
    -Platform Linux `
    -Verbose -PolicyId '<GUID for this policy>' -PolicyVersion 1.0.0.0 -Parameter $PolicyParameterInfo -Mode ApplyAndAutoCorrect
```

The last argument (`-Mode ApplyAndAutoCorrect`) is for remediation, without this the policy will be audit-only (default).

Run this command on the Arc device in PowerShell. This will produce a JSON holding the policy definition, copy that JSON, it will be needed for creating the new policy in Azure Portal.

> **Important**
> In the generated policy definition JSON, before creating a new policy with it, manually search and remove all instances of prefixes with the resource type wrapped in square brackets (for example: `[OsConfigResource]`).

Save a copy of the generated policy definition JSON in case the policy will need to be updated later (for example, because of an updated artifacts ZIP package).

An example of a generated policy definition JSON (with the SAS token and file hash removed) for the above example is available here: [LinuxSshServerSecurityBaseline_DeployIfNotExists.json](ssh/LinuxSshServerSecurityBaseline_DeployIfNotExists.json)

#### 7.1.2. Creating the new policy

Next, go to Azure Portal | Policy | Definitions, select the subscription, then create a new Policy Definition and fill out:

- Definition location: select here the Azure subscription.
- Name: enter the name for the policy
- Description: enter here the description for the policy
- Category: Select `Use Existing` and `Guest Configuration`
- Policy rule: paste the contents of the JSON file containing the policy definition

Then save.

#### 7.1.3. Assigning the new policy

Next, the new policy needs to be assigned. Go to Azure Portal | Policy | Definitions and assign the policy:
1. Select the subscription and resource group where the policy will be targeted at (to all Arc devices in that group).
1. Select `Include Arc connected machines` and uncheck the uncheck the `Only show parameters` box.
1. Enter the desired values for the policy parameters: `ComponentName`, `ReportedObjectName`, `DesiredObjectName`, `DesiredObjectValue`, etc.

#### 7.1.4. Creating a remediation task

One more step is necessary for so called brownfield devices, which are created (onboarded to Arc) before the policy is created (greenfield devices are created after the policy).

For these brownfield devices, go to Azure Portal | Policy | Compliance, select the policy and then create a remediation task.

#### 7.1.5. Deleting a policy

To delete a policy, delete it first from Azure Portal | Policy | Assignments, then from Azure Portal | Policy | Definitions.

### 7.2. [If the policy already exists] Editing the existing policy definition

If there is a new artifacts ZIP package, the policy definition can be manually updated for it. We need the existing policy definition, the new package URL and the new file hash of the package.

On the device, obtain the hash with:

```bash
sudo pwsh
get-filehash <local path to package>.zip
```

Then, keeping as-is the existing policy assignment, change the policy definition as following:
1. Go to the Azure Portal | Policy | Definitions, select the policy and edit.
1. Use the policy definition JSON that was used to generate the policy with or if that is no longer available, copy the current policy definition JSON from the Azure Portal.
1. There will be several instances of the URL (`"contentUri":`) and the hash (`"contentHash":`) in the policy defition JSON. Carefully replace all with the new token URL and hash.
1. If the policy definition JSON was obtained from current policy definition in Azure Portal, manually remove the instances of `"createdBy":`, `"createdOn":`, `"updatedBy":`, `"updatedOn":`, plus the root instances of `"type":`, `"name":` and the whole block of `"systemData":`.
1. Once the policy definition JSON is updated, copy its contents and paste that as the new  policy definition in Azure Portal, and save.
1. If necessary, create a new remediation task. Then wait for the updated policy to be applied to all Arc devices in the designated group.

### 7.3. Monitoring the policy activity from the device side

On the device, the MC Agent will check on the policy every 15 minutes. The MC Agent logs will record this activity. The logs are at `/var/lib/GuestConfig`.

```bash
sudo su
cd /var/lib/GuestConfig/arc_policy_logs
```
View last with:

```bash
tail gc_agent.log -f
```
View the full last MC Agent log with:

```bash
cat gc_agent.log
```
We can also copy the full MC logs to a separate folder and view them from there in full. For example:

```bash
mkdir $HOME/GuestConfig
sudo cp /var/lib/GuestConfig $HOME -r
```

View the current status of the MC Agent via the Azure Arc-enabled Servers Agent with:

```bash
sudo azcmagent show
```

## 8. Registering the Arc device with a different name

Disconnect the device from Azure and restart:

```bash
sudo azcmagent disconnect
sudo service gcad stop
sudo rm -rf /var/lib/GuestConfig/
sudo service gcad start
```

Execute following export commands copied from the Arc onboarding script (filled in with the right data) and register the device with the new name:

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
