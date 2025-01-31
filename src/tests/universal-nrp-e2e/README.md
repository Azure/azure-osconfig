# Universal NRP End-to-End Tests Local/GitHub
This directory contains tests for the universal NRP both for local/onprem and GitHub workflows. There is a GitHub workflow which performs these tests but only for modern distros (excluding EOL distributions eg. centos 7-8, ubuntu 18.04-20.04, etc.). We do support EOL testing on our infrastructure, as such, we've developed some tools in order for us to test locally.

The following bash tools have been created to enable testing across any linux-based OS:
 - `StartLocalTest.sh`: Peforms tests locally with a given Azure Policy package. This is meant to be used directly on the machine being tested (See [Testing on VMs](#testing-on-vms)).
 - `StartVMTest.sh`: Uses QEMU with a given cloud based image (cloud-init enabled) and performs the tests on the VM (See [Testing directly on target machine](#testing-directly-on-target-machine)).
 - `StartTests.sh` (Microsoft Internal Only): Uses the _StartVMTest.sh_ above to orchestrate tests on images not being tested by GitrHub workflows. These are internal as they leverage Azure Blob Storage for storing the disk images (See [Testing all supported distributions (Internal-Only)](#testing-all-supported-distributions-internal-only).

# Testing on VMs
To perform tests on VMs the `StartVMTest.sh` allows you to perform tests on specific Azure Policy packages targetting a particular VM disk image. QEMU is used as the virtualization platform and allows us to target linux distributions shared as raw/qcow2 images (See [VM Image Sources](#vm-image-sources) for compatible images). Images are booted using cloud-init in order to provision the user and help orchestrate tests, collect logs and test reports back to the QEMU host.

## Prerequisites
 - jq
 - qemu-system-x86
 - cloud-image-utils

## Example
The following example performs tests on an [Ubuntu Noble cloud image](https://cloud-images.ubuntu.com/noble/current/) on the AzureLinuxBaseline.zip (built in directory tree).
```sh
# Download image https://cloud-images.ubuntu.com/
wget https://cloud-images.ubuntu.com/noble/current/noble-server-cloudimg-amd64.img
# Add 2Gb to filesystem in order to install all necessary dependencies
qemu-img resize noble-server-cloudimg-amd64.img +2G
./StartVMTest.sh -i noble-server-cloudimg-amd64.img -p ../../build/AzureLinuxBaseline.zip
```
Once completed, log archives are created under `_<distro-image>` directory (in this case __focal-server-cloudimg-amd64.img_) which contain the JUnit test report along with the osconfig logs contained under `/var/log/osconfig*`

# Testing directly on target machine
To perform tests directly on a target machine the `StartLocalTest.sh` performs the tests locally.

You need to copy over the following files onto the target machine:
 - UniversalNRP.Tests.ps1
 - StartLocalTest.sh
 - A policy package to apply on the machine - for this example, lets assume `AzureLinuxBaseline.zip`

## Example
```sh
./StartLocalTest.sh -p AzureLinuxBaseline.zip
```

# Testing all supported distributions (Internal-Only)
This Microsoft Internal Only script (`StartTests.sh`) downloads generalized images stored in our internal blob storage and then runs `StartVMTest.sh` on each distribution and policy package. You may also specify the amount of memory used by each vm (default: 512mb) in addition to the number of concurrent jobs/tests to run (default: 5).

## Prerequisites
 - jq
 - az-cli - [How to install the Azure CLI](https://learn.microsoft.com/cli/azure/install-azure-cli)
 - qemu-system-x86
 - cloud-image-utils

## Example
```sh
./StartTest.sh
```
## Usage
```
Usage: ./StartTests.sh [-r run-id] [-m vm-memory-mb] [-j max-concurrent-jobs]
          -r run-id: Specify the run-id of the pipeline run to download the packages from (Default: latest-succeeded-run-id)
          -m vm-memory-mb: Specify the memory in MB to be used for the VMs (Default: 512)
          -j max-concurrent-jobs: Specify the maximum number of concurrent jobs to run the tests (Default: 5)
```

# More Information
## Testing locally/directly on target machine
Although `StartLocalTest.sh` can be used to simply test a policy package, it can also invoke "Stages" which is used by the `StartVMTest.sh` to orchestrate tests on the VM through its ssh session and provide accurate exit codes for error reporting. It also provides the `-g` flag used to [generalize the image](#generalizing-a-vm-image) which is useful when preparing an image that will be reused/shared.
```
Usage: ./StartLocalTest.sh [-s stage-name] [-p policy-package.zip [-r]]
        -s stage-name:         Specify the stage name. Valid options are: dependency_check, run_tests, collect_logs.
                               If no stage is specified, all stages will be executed in this order:
                                    dependency_check, run_tests, collect_logs

            - dependency_check: Checks to ensure dependencies are satisfied.
                                Checks for and installs them if not present:
                                    - Powershell +modules: MachineConfiguration, Pester
                                    - OMI

            - run_tests:        Runs the tests (Powershell Pester Tests).

            - collect_logs:     Creates a tar.gz archive with the osconfig logs and JUnit Test Report

        -p policy-package.zip:  The Azure Policy Package to test

        -r remediate-flag:      When the flag is enabled, performs remediation on the Policy Package (Default: No remediation performed)

        -g generalize-flag:     Generalize the current machine for tests. Performs the following:
                                    - Remove logs and tmp directories
                                    - Clean package management cache
                                    - Clean cloud-init flags to reset cloud-init to initial-state
```

## Testing on VMs
```
Usage: ./StartVMTest.sh [-i image.img -p policypackage.zip [-r]] [-i image.img -g] [-m 512] [-d]
       -i Image Path:            Path to the image (raw or qcow2 format)
       -p Policy Package:        Path to the policy package
       -m VM Memory (Megabytes): Size of VMs RAM (Default: 512)
       -r Remediation:           Perform remediation flag (Default: false)
       -g Generalize Flag:       Generalize the current machine for tests. Performs the following:
                                   - Remove logs and tmp directories
                                   - Clean package management cache
                                   - Clean cloud-init flags to reset cloud-init to initial-state
       -l Log Directory:         Directory used to place output logs
       -d Debug Mode Flag:       VM stays up for debugging (Default: false)
```
### VM Image Sources
It's preferable for images to be "Cloud" images (contain cloud-init) as it makes tooling work out-of-box and does not require creating your own image of the distro and getting all the necessary packages/dependencies installed.

Here are a few locations where cloud images can be obtained:
 - [Ubuntu Cloud Images](https://cloud-images.ubuntu.com/)
 - [Debian Cloud Images](https://cloud.debian.org/images/cloud/)
 - [CentOS Cloud Images](https://cloud.centos.org/centos/)
 - [Rocky Linux Images](https://rockylinux.org/download) - also contains Cloud images
 - [Oracle Cloud Images](https://yum.oracle.com/oracle-linux-templates.html)

## Generalizing a VM Image
Generalizing an image is useful if its going to be reused for re-running tests on it. The tooling provides the ability to provision the VM, install dependencies, cleans logs, bash history and clears the cloud-init boot-flags so it can be reprovisioned appropriately. Once an image is generalized you should copy it somewhere so it can be copied and reused. The debug mode (`-d`) is useful here as it allows you to ssh into the VM once it has been provisioned so you may install any additional dependencies and/or fix any issues with the image.

You typically want to generalize images that do not just work out-of-box in order to keep a "Golden Master" image that just works to avoid having to reprovision VM images when you want to re-run tests on a particular VM image.

## Example - Using CentOS-7 an EOL distribution with errors
This example will generalize an older distro image, where we will hit many issues. This example will demonstrate some common problems can can occur using older distributions where they are no longer being maintained and you must build and install some of the needed dependencies.

This example demonstrates:
 - EOL distribution whose package mirrors are out of date and must be updated
 - Install dependencies that exist in the package-mirrors
 - Install dependencies that require building/installing sources

Using the _CentOS-7_ distribution below.
```sh
# Download the image
wget https://cloud.centos.org/centos/7/images/CentOS-7-x86_64-GenericCloud-2211.qcow2
# Add some additional space just to ensure satisfactory space for installing tools/dependencies
qemu-img resize CentOS-7-x86_64-GenericCloud-2211.qcow2 +2G
# Start VM Image with extra memory (4096mb) generalization and debug flag
./StartVMTest.sh -i CentOS-7-x86_64-GenericCloud-2211.qcow2 -m 4096 -g -d
```
The VM will boot and show the console, so any errors can be resolved afterwards, the utility will provide ssh login information in order for you to go and resolve any issues.

As we can see below, there was an issue `pwsh: /lib64/libstdc++.so.6: version 'GLIBCXX_3.4.21' not found (required by pwsh)`. But our ssh login and credentials are available for us to go and resolve the issue.
```
Image path: CentOS-7-x86_64-GenericCloud-2211.qcow2.
VM memory: 8096 mb.
Generalization Mode: enabled.
Debug Mode: enabled.
sudo is available. Attempting to sudo for better VM performance...
[sudo] password for ahmed:
done!
Starting QEMU using qcow2 format SSH port (22) forwarded to 22902 with 8096 mb RAM ...
QEMU process started with PID: 5448
Waiting for VM provisioning...........done!
sudo is available. Attempting to sudo...
done!
Checking dependencies...

Powershell not found. Installing Powershell...
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0
100 68.3M  100 68.3M    0     0  5553k      0  0:00:12  0:00:12 --:--:-- 6122k
ln: failed to create symbolic link '/usr/bin/pwsh': File exists
pwsh: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.20' not found (required by pwsh)
pwsh: /lib64/libstdc++.so.6: version `GLIBCXX_3.4.21' not found (required by pwsh)
Generalizing machine...
Failed to set locale, defaulting to C
Loaded plugins: fastestmirror
Cleaning repos: base extras updates
done!
######################################################################
Debug mode enabled. To connect via SSH:
  ssh -i /home/ahmed/git/azure-osconfig/src/tests/universal-nrp-e2e/_CentOS-7-x86_64-GenericCloud-2211/id_rsa user1@localhost -p 26654
When done with VM, terminate the process to free up VM resources.
  sudo kill 2293
######################################################################
✅ Generalization complete!
```

As seen above, our ssh command is present for us to go fix the issue, in this case it seems `GLIBCXX_3.4.21` is missing for the Powershell runtime.
GLIBCXX_3.4.21 is included in gcc-9.5, so we will have to build it, meaning we will have to install a C++ toolchain and other needed dependencies.

```
ssh -i /home/ahmed/git/azure-osconfig/src/tests/universal-nrp-e2e/_CentOS-7-x86_64-GenericCloud-2211/id_rsa user1@localhost -p 26654
[user1@osconfige2etest ~]$ sudo yum group install "Development Tools"
Failed to set locale, defaulting to C
Loaded plugins: fastestmirror
Determining fastest mirrors
Could not retrieve mirrorlist http://mirrorlist.centos.org/?release=7&arch=x86_64&repo=os&infra=genclo error was
14: curl#6 - "Could not resolve host: mirrorlist.centos.org; Unknown error"
```
It seems there are more errors, being a EOL image, the package manager will not work as the package manager mirrors are out-of-date. Most distributions have alternative mirrors you can find. In this case we were able to fix it.
```sh
sudo vi /etc/yum.repos.d/CentOS-Base.repo
```
```
[base]
name=CentOS-$releasever - Base
baseurl=http://vault.centos.org/7.9.2009/os/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[updates]
name=CentOS-$releasever - Updates
baseurl=http://vault.centos.org/7.9.2009/updates/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[extras]
name=CentOS-$releasever - Extras
baseurl=http://vault.centos.org/7.9.2009/extras/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7
EOF
```
Now we can try installing again and it will resolve the mirrors. We can also install the needed dependencies along with building and install gcc which will resolve our glibcxx dependencies.
```
[user1@osconfige2etest ~]$ sudo yum group install "Development Tools"
...
[user1@osconfige2etest ~]$ sudo yum install gmp-devel mpfr-devel libmpc-devel wget libicu
[user1@osconfige2etest ~]$ wget https://ftp.gnu.org/gnu/gcc/gcc-9.5.0/gcc-9.5.0.tar.gz
[user1@osconfige2etest ~]$ tar -zxvf gcc-9.5.0.tar.gz
[user1@osconfige2etest ~]$ cd gcc-9.5.0
[user1@osconfige2etest ~]$ ./configure --prefix=/usr --enable-languages=c,c++ --disable-multilib
[user1@osconfige2etest ~]$ make -j$(nproc)
[user1@osconfige2etest ~]$ sudo make install
```
Now let's try PowerShell again
```
[user1@osconfige2etest ~]$ pwsh
PowerShell 7.4.6
PS /home/user1>
```
It worked, we can now exit powershell, cleanup everything, regeneralize and shutdown vm
```
PS /home/user1> exit
[user1@osconfige2etest ~]$ cd ~
[user1@osconfige2etest ~]$ ./StartLocalTests.sh -g
[user1@osconfige2etest ~]$ rm -rf *
[user1@osconfige2etest ~]$ sudo shutdown now
Connection to localhost closed by remote host.
Connection to localhost closed.
```
We now have a generalized image you can share and reuse in tests. Let's prepare the final image for sharing in a azure blob storage for easy reuse.
Let's compress the images images before placing them into blob storage.
```sh
# Compress image
qemu-img convert -c -O qcow2 CentOS-7-x86_64-GenericCloud-2211.qcow2 CentOS-7-x86_64-GenericCloud-2211-Generalized.qcow2
```
Let's try the image and ensure it works before widely sharing...
```
./StartVMTest.sh -i CentOS-7-x86_64-GenericCloud-2211.qcow2 -p /mnt/c/Users/ahbenmes/Downloads/AzureLinuxBaseline.zip
Image path: CentOS-7-x86_64-GenericCloud-2211.qcow2.
Policy package: /mnt/c/Users/ahbenmes/Downloads/AzureLinuxBaseline.zip.
...
...
Tests Passed: 4, Failed: 0, Skipped: 0, Inconclusive: 0, NotRun: 0
#######################################################################

Authorized access only!

If you are not authorized to access or use this system, disconnect now!

#######################################################################
sudo is available. Attempting to sudo...
done!
Collecting logs/reports...
osconfig-logs/
osconfig-logs/osconfig_nrp.bak
osconfig-logs/osconfig_nrp.log
osconfig-logs/testResults.xml
#######################################################################

Authorized access only!

If you are not authorized to access or use this system, disconnect now!

#######################################################################
Log archive created: _CentOS-7-x86_64-GenericCloud-2211/osconfig-logs-20250118_175800.tar.gz
✅ Tests passed!
```
Lets place the image into blob storage.
```sh
# Upload into Azure blob storage
az login
az storage blob upload \
  --account-name osconfigstorage \
  --container-name diskimages \
  --name centos-7.qcow2 \
  --file CentOS-7-x86_64-GenericCloud-2211-Generalized.qcow2 \
  --content-md5 $(md5sum CentOS-7-x86_64-GenericCloud-2211-Generalized.qcow2 | awk '{ print $1 }') \
  --auth-mode login
```
