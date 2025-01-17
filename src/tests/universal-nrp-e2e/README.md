# Universal NRP End-to-End Tests
This directory contains tests for the universal NRP. There is a GitHub workflow which performs these tests but only for modern distros (excluding EOL distributions eg. centos 7-8, ubuntu 18.04-20.04, etc.). We do support EOL

The following bash tools have been created to enable testing across any linux-based OS:
 - `StartLocalTest.sh`: Peforms tests locally with a given Azure Policy package.
 - `StartVMTest.sh`: Uses QEMU with a given cloud based image (cloud-init enabled) and performs the tests on the VM.

 ## Testing on VMs
To perform tests on VMs the `StartVMTest.sh` allows you to perform tests on specific Azure Policy packages targetting a particular VM disk image. QEMU is used as the virtualization platform and allows us to target linux distributions shared as raw/qcow2 images. Images are booted using cloud-init in order to provision the user and help orchestrate tests, collect logs and test reports back to the QEMU host.

### Image Sources
Its preferable for images to be "Cloud" images (contain cloud-init) as it makes tooling work out-of-box and does not require creating your own image of the distro and getting all the necessary packages/dependencies installed.

Here are a few locations where cloud images can be obtained:
 - [Ubuntu Cloud Images](https://cloud-images.ubuntu.com/)
 - [Debian Cloud Images](https://cloud.debian.org/images/cloud/)
 - [CentOS Cloud Images](https://cloud.centos.org/centos/)
 - [Rocky Linux Images](https://rockylinux.org/download) - also contains Cloud images

### Usage
```
Usage: ./StartVMTest.sh [-i /path/to/image.img -p /path/to/policypackage.zip -c resource-count [-g]] [-m 512] [-r] [-d]
       -i Image Path:            Path to the image qcow2 format
       -p Policy Package:        Path to the policy package
       -c Resource Count:        The number of resources to validate, tests will fail if this doesn't match (Default: 0)
       -m VM Memory (Megabytes): Size of VMs RAM (Default: 512)
       -r Remediation:           Perform remediation flag (Default: false)
       -g Generalize Flag:       Generalize the current machine for tests. Performs the following:
                                   - Remove logs and tmp directories
                                   - Clean package management cache
                                   - Clean cloud-init flags to reset cloud-init to initial-state
       -d Debug Mode:            VM stays up for debugging (Default: false)
```
### Example
The following example performs tests on an [Ubuntu Noble cloud image](https://cloud-images.ubuntu.com/noble/current/) on the AzureLinuxBaseline.zip (built in directory tree) which has 168 resources defined.
```sh
# Download image https://cloud-images.ubuntu.com/
wget https://cloud-images.ubuntu.com/noble/current/noble-server-cloudimg-amd64.img
# Add 2Gb to filesystem in order to install all necessary dependencies
qemu-img resize noble-server-cloudimg-amd64.img +2G
./StartVMTest.sh -i noble-server-cloudimg-amd64.img -p ../../build/AzureLinuxBaseline.zip -c 168
```
Once completed, log archives are created under `_<distro-image>` directory (in this case __focal-server-cloudimg-amd64.img_) which contain the JUnit test report along with the osconfig logs contained under `/var/log/osconfig*`

### Generalizing a VM Image
Generalizing an image is useful if its going to be reused for re-running tests on it. The tooling provides the ability to provision the VM, install dependencies, cleans logs, bash history and clears the cloud-init boot-flags so it can be reprovisioned appropriately. Once an image is generalized you should copy it somewhere so it can be copied and reused. The debug mode (`-d`) is useful here as it allows you to ssh into the VM once it has been provisioned so you may install any additional dependencies and/or fix any issues with the image.

You typically want to generalize images that do not just work out-of-box in order to keep a "Golden Master" image that just works to avoid having to reprovision VM images when you want to re-run tests on a particular VM image.

#### Example - Using CentOS-7 an EOL distribution with Errors
This example will generalize an older distro image, where we will hit some errors. This example will demonstrate some common problems can can occur using older distributions where they are no longer being maintained.

Using the _CentOS-7_ distribution below.
```sh
# Download the image
wget https://cloud.centos.org/centos/7/images/CentOS-7-x86_64-GenericCloud-2211.qcow2
# Add some additional space just to ensure satisfactory space for installing tools/dependencies
qemu-img resize CentOS-7-x86_64-GenericCloud-2211.qcow2 +2G
# Start VM Image with generalization and debug flag
./StartVMTest.sh -i CentOS-7-x86_64-GenericCloud-2211.qcow2 -g -d
```
The VM will boot and show the console, so any errors can be resolved afterwards, the utility will provide ssh login information in order for you to go and resolve any issues.

As we can see below, there was an issue `StartLocalTest.sh: line 112: wget: command not found`. But our ssh login and credentials are available for us to go and resolve the issue.
```
...
StartLocalTest.sh: line 112: wget: command not found
error: open of /tmp/omi.rpm failed: No such file or directory
done!
sudo is available. Attempting to sudo...
done!
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

As seen above, our ssh command is present for us to go fix the issue, in this case it seems `wget` is missing, let's go and install it.

```
ssh -i /home/ahmed/git/azure-osconfig/src/tests/universal-nrp-e2e/_CentOS-7-x86_64-GenericCloud-2211/id_rsa user1@localhost -p 26654
[user1@osconfige2etest ~]$ sudo yum install wget
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
Now we can try installing again
```
[user1@osconfige2etest ~]$ sudo yum install wget
Failed to set locale, defaulting to C
Loaded plugins: fastestmirror
Loading mirror speeds from cached hostfile
Resolving Dependencies
--> Running transaction check
---> Package wget.x86_64 0:1.14-18.el7_6.1 will be installed
--> Finished Dependency Resolution

Dependencies Resolved

==============================================================================================================================================================================
 Package                               Arch                                    Version                                            Repository                             Size
==============================================================================================================================================================================
Installing:
 wget                                  x86_64                                  1.14-18.el7_6.1                                    base                                  547 k

Transaction Summary
==============================================================================================================================================================================
Install  1 Package

Total download size: 547 k
Installed size: 2.0 M
Is this ok [y/d/N]: y
Downloading packages:
wget-1.14-18.el7_6.1.x86_64.rpm                                                                                                                        | 547 kB  00:00:01
Running transaction check
Running transaction test
Transaction test succeeded
Running transaction
  Installing : wget-1.14-18.el7_6.1.x86_64                                                                                                                                1/1
  Verifying  : wget-1.14-18.el7_6.1.x86_64                                                                                                                                1/1

Installed:
  wget.x86_64 0:1.14-18.el7_6.1

Complete!
```
Now let's try to regeneralize the image again. Let's shutdown the VM:
```
[user1@osconfige2etest ~]$ sudo shutdown now
Connection to localhost closed by remote host.
Connection to localhost closed.
```
and we can restart the generalization:
```sh
./StartVMTest.sh -i CentOS-7-x86_64-GenericCloud-2211.qcow2 -g -d
...
Configuring OMI service ...
Created symlink from /etc/systemd/system/multi-user.target.wants/omid.service to /usr/lib/systemd/system/omid.service.
Trying to start omi with systemctl
omi is started.
done!
sudo is available. Attempting to sudo...
done!
Generalizing machine...
Failed to set locale, defaulting to C
Loaded plugins: fastestmirror
Cleaning repos: base extras updates
Cleaning up list of fastest mirrors
done!
######################################################################
Debug mode enabled. To connect via SSH:
  ssh -i /home/ahmed/git/azure-osconfig/src/tests/universal-nrp-e2e/_CentOS-7-x86_64-GenericCloud-2211/id_rsa user1@localhost -p 50720
When done with VM, terminate the process to free up VM resources.
  sudo kill 4164
######################################################################
✅ Generalization complete!
```
There are no errors this time, but the vm is still up and running: let's shutdown the vm

```sh
# Shutdown VM
ssh -i /home/ahmed/git/azure-osconfig/src/tests/universal-nrp-e2e/_CentOS-7-x86_64-GenericCloud-2211/id_rsa user1@localhost -p 50720 "sudo shutdown now"
```
We now have a generalized image you can share and reuse in tests.

## Testing directly on target machine
To perform tests directly on a target machine the `StartLocalTest.sh` performs the tests locally.

You need to copy over the following files onto the target machine:
 - UniversalNRP.Tests.ps1
 - StartVMTest.sh
 - A policy package to apply on the machine - for this example, lets assume `AzureLinuxBaseline.zip`

### Usage
```
Usage: ./StartLocalTest.sh [-s stage-name] [-p policy-package.zip -c resource-count [-r]]
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

        -c resource-count:      The number of resources to validate, tests will fail if this doesn't match (Default: 0)

        -r remediate-flag:      When the flag is enabled, performs remediation on the Policy Package (Default: No remediation performed)

        -g generalize-flag:     Generalize the current machine for tests. Performs the following:
                                    - Remove logs and tmp directories
                                    - Clean package management cache
                                    - Clean cloud-init flags to reset cloud-init to initial-state
```
### Example
```sh
./StartLocalTest.sh -p AzureLinuxBaseline.zip -c 168
```