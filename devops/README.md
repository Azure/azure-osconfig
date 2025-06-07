[![CI Tests](https://github.com/Azure/azure-osconfig/actions/workflows/ci.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/ci.yml)
[![Integration - NRP Tests](https://github.com/Azure/azure-osconfig/actions/workflows/universalnrp-test.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/universalnrp-test.yml)
[![insiders-fast](https://github.com/Azure/azure-osconfig/actions/workflows/insiders-fast.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/insiders-fast.yml)
[![prod](https://github.com/Azure/azure-osconfig/actions/workflows/prod.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/prod.yml)

# Package Installation

Our packages are distributed via the [Microsoft repository](https://packages.microsoft.com/). The following instructions will guide you through the installation of the `osconfig` package on our supported Linux distributions. Additionally, you can find our MSDN documentation on package installation at [docs.microsoft.com](https://learn.microsoft.com/azure/osconfig/howto-install?tabs=package).

Channels | Description
-----|-----
`insiders-fast` | The latest and greatest features and bug fixes, represents current work-in-progress so things might not work as expected. This channel is updated frequently.
`prod` | The stable channel. This channel is updated less frequently and is recommended for production use.

*If you see any missing platforms or architectures that would help, please contact osconfigcore@microsoft.com.

| Platform | Architecture | Package Repository
-----|-----|-----
AlmaLinux 9 | x86_64 | [insiders-fast](#almalinux9-insiders-fast)
Amazon Linux 2 | x86_64 | [insiders-fast](#amazonlinux2-insiders-fast)
Azure Linux 2 (mariner-2) | x86_64 | [insiders-fast](#azurelinux2-insiders-fast)
CentOS 7 | x86_64 | [insiders-fast](#centos7-insiders-fast)
CentOS 8 | x86_64 | [insiders-fast](#centos8-insiders-fast)
Debian 10 | x86_64 / arm64 | [insiders-fast](#debian10-insiders-fast) / [prod](#debian10-prod)
Debian 11 | x86_64 / arm64 | [insiders-fast](#debian11-insiders-fast) / [prod](#debian11-prod)
Debian 12 | x86_64 / arm64 | [insiders-fast](#debian12-insiders-fast) / [prod](#debian12-prod)
Red Hat Enterprise Linux 7 | x86_64 | [insiders-fast](#rhel7-insiders-fast)
Red Hat Enterprise Linux 8 | x86_64 | [insiders-fast](#rhel8-insiders-fast)
Red Hat Enterprise Linux 9 | x86_64 | [insiders-fast](#rhel9-insiders-fast)
Rocky Linux 9 | x86_64 | [insiders-fast](#rockylinux9-insiders-fast)
SUSE Linux Enterprise Server 15 | x86_64 | [insiders-fast](#sles15-insiders-fast)
Ubuntu 20.04 | x86_64 / arm64 | [insiders-fast](#ubuntu2004-insiders-fast) / [prod](#ubuntu2004-prod)
Ubuntu 22.04 | x86_64 / arm64 | [insiders-fast](#ubuntu2204-insiders-fast) / [prod](#ubuntu2204-prod)

## AlmaLinux 9
### Install
#### <a name="almalinux9-insiders-fast">insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/config/alma/9/insiders-fast.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## Amazon Linux 9
### Install
#### <a name="amazonlinux2-insiders-fast"></a>insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-amazonlinux2-insiders-fast-prod/config.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## Azure Linux 2 (cbl-mariner2, mariner2)
### Install
#### <a name="azurelinux2-insiders-fast"></a>insiders-fast / preview
```bash
tdnf install dnf dnf-plugins-core
dnf config-manager --add-repo https://packages.microsoft.com/yumrepos/cbl-mariner-2.0-preview-Microsoft-x86_64/config.repo
tdnf install osconfig
```
### Remove
```bash
tdnf erase osconfig
```

## CentOS 7
### Install
#### <a name="centos7-insiders-fast"></a>insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-centos7-insiders-fast-prod/config.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## CentOS 8
### Install
#### <a name="centos8-insiders-fast">insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-centos8-insiders-fast-prod/config.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## Debian 10
### Install
#### <a name="debian10-insiders-fast">insiders-fast
```bash
wget https://packages.microsoft.com/config/debian/10/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
```
#### <a name="debian10-prod">prod
```bash
wget https://packages.microsoft.com/config/debian/10/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
```
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
apt update
apt install osconfig
```
### Remove
```bash
apt remove osconfig
```

## Debian 11
### Install
#### <a name="debian11-insiders-fast">insiders-fast
```bash
wget https://packages.microsoft.com/config/debian/11/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
```
#### <a name="debian11-prod">prod
```bash
wget https://packages.microsoft.com/config/debian/11/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
```
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
apt install osconfig
```
### Remove
```bash
apt remove osconfig
```

## Debian 12
### Install
#### <a name="debian12-insiders-fast">insiders-fast
```bash
wget https://packages.microsoft.com/config/debian/12/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
```
#### <a name="debian12-prod">prod
```bash
wget https://packages.microsoft.com/config/debian/12/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
```
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor -o /etc/apt/trusted.gpg.d/packages-microsoft-com_key.asc
ln -s /etc/apt/trusted.gpg.d/packages-microsoft-com_key.asc /usr/share/keyrings/microsoft-prod.gpg
apt install osconfig
```
### Remove
```bash
apt remove osconfig
```

## Red Hat Enterprise Linux 7
### Install
#### <a name="rhel7-insiders-fast">insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-rhel7.4-insiders-fast-prod/config.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## Red Hat Enterprise Linux 8
### Install
#### <a name="rhel8-insiders-fast">insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-rhel8.0-insiders-fast-prod/config.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## Red Hat Enterprise Linux 9
### Install
#### <a name="rhel9-insiders-fast">insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-rhel9.0-insiders-fast-prod/config.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## Rocky Linux 9
### Install
#### <a name="rockylinux9-insiders-fast">insiders-fast
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/config/rocky/9/insiders-fast.repo
yum install osconfig
```
### Remove
```bash
yum remove osconfig
```

## SUSE Linux Enterprise Server 15
### Install
#### <a name="sles15-insiders-fast">insiders-fast
```bash
zypper install yum-utils
zypper ar -f https://packages.microsoft.com/yumrepos/microsoft-sles15-insiders-fast-prod/config.repo
zypper install osconfig
```
### Remove
```bash
zypper remove osconfig
```

## Ubuntu 20.04
### Install
#### <a name="ubuntu2004-insiders-fast">insiders-fast
```bash
wget https://packages.microsoft.com/config/ubuntu/20.04/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
```
#### <a name="ubuntu2004-prod">prod
```bash
wget https://packages.microsoft.com/config/ubuntu/20.04/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
```
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
apt update
apt install osconfig
```
### Remove
```bash
apt remove osconfig
```

## Ubuntu 22.04
#### <a name="ubuntu2204-insiders-fast">insiders-fast
```bash
wget https://packages.microsoft.com/config/ubuntu/22.04/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
```
#### <a name="ubuntu2204-prod">prod
```bash
wget https://packages.microsoft.com/config/ubuntu/22.04/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
```
### Install
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
apt update
apt install osconfig
```
### Remove
```bash
apt remove osconfig
```

# DevOps Folder
This folder contains the artifacts and scripts used by our engineering systems (build, test, package).

- `debian` - Debian packaging scripts
- `docker/<os>-<arch>` - Build containers for supported operating systems and architectures
  - `amd64`
  - `arm` (armv7)
  - `arm64` (aarch64)
- `e2e`
  - `cloudtest` - backup of 1ES managed CloudTest image definitions
  - `terraform` - terraform modules for provisioning Azure resources for E2E tests
- `rpm` - RPM packaging scripts
