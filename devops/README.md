# Package Installation
We use the [Microsoft repository](https://packages.microsoft.com/) to distribute our packages. Below are the instructions to install the `osconfig` package on various operating systems.

## Amazon Linux 9 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/yumrepos/microsoft-amazonlinux2-insiders-fast-prod/config.repo
yum install osconfig
```

## Azure Linux 2 (cbl-mariner2, mariner2) - insiders-fast (AMD64)
```bash
tdnf install dnf-plugins-core
dnf config-manager --add-repo https://packages.microsoft.com/yumrepos/cbl-mariner-2.0-preview-Microsoft-x86_64/config.repo
tdnf install osconfig
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

## Rocky Linux 9 - insiders-fast (AMD64)
```bash
yum install yum-utils
yum-config-manager --add-repo https://packages.microsoft.com/config/rocky/9/insiders-fast.repo
yum install osconfig
```

## SUSE Linux Enterprise Server 15 - insiders-fast (AMD64)
```bash
zypper install yum-utils
zypper ar -f https://packages.microsoft.com/yumrepos/microsoft-sles15-insiders-fast-prod/config.repo
zypper install osconfig
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