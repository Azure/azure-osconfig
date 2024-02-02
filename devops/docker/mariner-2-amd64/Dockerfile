FROM mcr.microsoft.com/cbl-mariner/base/core:2.0

RUN tdnf install -y mariner-repos-extended
RUN tdnf install -y \
    build-essential \
    ca-certificates \
    cmake \
    curl-devel \
    file \
    gcovr \
    git \
    gmock-devel \
    gtest-devel \
    jq \
    mariner-rpm-macros \
    ninja-build \
    openssl-devel \
    rapidjson-devel \
    rpm-build \
    tar \
    tmux \
    tree \
    unzip \
    util-linux \
    wget \
    zip && \
    tdnf upgrade -y && \
    tdnf clean all