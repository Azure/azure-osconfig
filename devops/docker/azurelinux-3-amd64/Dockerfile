FROM mcr.microsoft.com/azurelinux/base/core:3.0

RUN tdnf install -y azurelinux-repos-extended
RUN tdnf install -y \
    azurelinux-rpm-macros \
    build-essential \
    ca-certificates \
    cmake \
    file \
    git \
    jq \
    perl \
    perl-IPC-Cmd \
    rpm-build \
    shadow-utils \
    tar \
    tmux \
    tree \
    unzip \
    util-linux \
    uuid-devel \
    wget \
    zip

RUN tdnf upgrade -y && \
    tdnf clean all

COPY .  /src
WORKDIR /src
RUN ls -al
RUN ls -al external || true
RUN ls -al external/vcpkg || true
RUN cd external/vcpkg && sed -e "s/\"2\\.7\\.4\"/\"1\\.0\\.0\"/g" -i scripts/vcpkg-tools.json && ./bootstrap-vcpkg.sh
RUN cd src && ../external/vcpkg/vcpkg install

