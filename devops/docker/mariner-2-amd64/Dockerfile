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