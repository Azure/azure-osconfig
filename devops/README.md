# Package Installation
We use the [Microsoft repository](https://packages.microsoft.com/) to distribute our packages. Below are the instructions to install the `osconfig` package on various operating systems.

Channels | Description
-----|-----  
[![insiders-fast](https://github.com/Azure/azure-osconfig/actions/workflows/insiders-fast.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/insiders-fast.yml) | The latest and greatest features and bug fixes. This channel is updated frequently.
[![prod](https://github.com/Azure/azure-osconfig/actions/workflows/prod.yml/badge.svg)](https://github.com/Azure/azure-osconfig/actions/workflows/prod.yml) | The stable channel. This channel is updated less frequently and is recommended for production use.

*If you see any missing platforms or architectures that would help, please contact osconfigcore@microsoft.com.

| Platform | Architecture | Package Repository | Package Removal
-----|-----|-----|-----
Amazon Linux 2 | x86_64 | [insiders-fast](#amazon-linux-9---insiders-fast-amd64) | [link](#amazon-linux-9---package-removal)
Azure Linux 2 (mariner-2) | x86_64 | [insiders-fast](#azure-linux-2-cbl-mariner2-mariner2---insiders-fast-amd64) | [link](#azure-linux-2---package-removal)
CentOS 7 | x86_64 | [insiders-fast](#centos-7---insiders-fast-amd64) | [link](#centos---package-removal)
CentOS 8 | x86_64 | [insiders-fast](#centos-8---insiders-fast-amd64) | [link](#centos---package-removal)
Debian 10 | x86_64 | [insiders-fast](#debian-10---insiders-fast-amd64) / [prod](#debian-10---prod-amd64) | [link](#debian---package-removal)
Debian 11 | x86_64 | [insiders-fast](#debian-11---insiders-fast-amd64) / [prod](#debian-11---prod-amd64) | [link](#debian---package-removal)
Debian 12 | x86_64 | [insiders-fast](#debian-12---insiders-fast-amd64) | [link](#debian---package-removal)
Red Hat Enterprise Linux 7 | x86_64 | [insiders-fast](#red-hat-enterprise-linux-7---insiders-fast-amd64) | [link](#red-hat-enterprise-linux---package-removal)
Red Hat Enterprise Linux 8 | x86_64 | [insiders-fast](#red-hat-enterprise-linux-8---insiders-fast-amd64) | [link](#red-hat-enterprise-linux---package-removal)
Red Hat Enterprise Linux 9 | x86_64 | [insiders-fast](#red-hat-enterprise-linux-9---insiders-fast-amd64) | [link](#red-hat-enterprise-linux---package-removal)
Rocky Linux 9 | x86_64 | [insiders-fast](#rocky-linux-9---insiders-fast-amd64) | [link](#rocky-linux---package-removal)
SUSE Linux Enterprise Server 15 | x86_64 | [insiders-fast](#suse-linux-enterprise-server-15---insiders-fast-amd64) | [link](#suse-linux-enterprise-server---package-removal)
Ubuntu 20.04 | x86_64 | [insiders-fast](#ubuntu-2004---insiders-fast-amd64) / [prod](#ubuntu-2004---prod-amd64) | [link](#ubuntu---package-removal)
Ubuntu 22.04 | x86_64 | [insiders-fast](#ubuntu-2204---insiders-fast-amd64) / [prod](#ubuntu-2204---prod-amd64) | [link](#ubuntu---package-removal)

## Amazon Linux 9 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-amazonlinux2-insiders-fast-prod/config.repo
yum install osconfig
```

## Amazon Linux 9 - Package Removal
```bash
yum remove osconfig
```

## Azure Linux 2 (cbl-mariner2, mariner2) - insiders-fast (AMD64)
```bash
tdnf install dnf-plugins-core
dnf config-manager --add-repo https://packages.microsoft.com/yumrepos/cbl-mariner-2.0-preview-Microsoft-x86_64/config.repo
tdnf install osconfig
```

## Azure Linux 2 - Package Removal
```bash
tdnf erase osconfig
```

## CentOS 7 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-centos7-insiders-fast-prod/config.repo
yum install osconfig
```

## CentOS 8 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-centos8-insiders-fast-prod/config.repo
yum install osconfig
```

## CentOS - Package Removal
```bash
yum remove osconfig
```

## Debian 10 - insiders-fast (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/debian/10/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
apt install osconfig
```

## Debian 10 - prod (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/debian/10/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
apt update
apt install osconfig
```

## Debian 11 - insiders-fast (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor -o /etc/apt/trusted.gpg.d/packages-microsoft-com_key.asc
wget https://packages.microsoft.com/config/debian/11/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
apt install osconfig
```

## Debian 11 - prod (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/debian/11/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
apt update
apt install osconfig
```

## Debian 12 - insiders-fast (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor -o /etc/apt/trusted.gpg.d/packages-microsoft-com_key.asc
wget https://packages.microsoft.com/config/debian/12/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
apt install osconfig
```

## Debian - Package Removal
```bash
apt remove osconfig
```

## Red Hat Enterprise Linux 7 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-rhel7.4-insiders-fast-prod/config.repo
yum install osconfig
```

## Red Hat Enterprise Linux 8 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-rhel8.0-insiders-fast-prod/
yum install osconfig
```

## Red Hat Enterprise Linux 9 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-rhel9.0-insiders-fast-prod/config.repo
yum install osconfig
```

## Red Hat Enterprise Linux - Package Removal
```bash
yum remove osconfig
```

## Rocky Linux 9 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/config/rocky/9/insiders-fast.repo
yum install osconfig
```

## Rocky Linux - Package Removal
```bash
yum remove osconfig
```

## SUSE Linux Enterprise Server 15 - insiders-fast (AMD64)
```bash
zypper install yum-utils
zypper ar -f https://packages.microsoft.com/yumrepos/microsoft-sles15-insiders-fast-prod/config.repo
zypper install osconfig
```

## SUSE Linux Enterprise Server - Package Removal
```bash
zypper remove osconfig
```

## Ubuntu 20.04 - insiders-fast (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/ubuntu/20.04/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
apt update
apt install osconfig
```

## Ubuntu 20.04 - prod (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/ubuntu/20.04/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
apt update
apt install osconfig
```

## Ubuntu 22.04 - insiders-fast (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/ubuntu/22.04/insiders-fast.list -O /etc/apt/sources.list.d/packages-microsoft-com_insiders-fast.list
apt update
apt install osconfig
```

## Ubuntu 22.04 - prod (AMD64)
```bash
wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add -
wget https://packages.microsoft.com/config/ubuntu/22.04/prod.list -O /etc/apt/sources.list.d/packages-microsoft-com_prod.list
apt update
apt install osconfig
```

## Ubuntu - Package Removal
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