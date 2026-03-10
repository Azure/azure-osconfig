FROM mcr.microsoft.com/cbl-mariner/base/core:2.0

RUN tdnf install -y mariner-repos-extended
RUN tdnf install -y \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    file \
    gcovr \
    git \
    jq \
    mariner-rpm-macros \
    openssl-devel \
    rpm-build \
    tar \
    tmux \
    tree \
    unzip \
    util-linux \
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

