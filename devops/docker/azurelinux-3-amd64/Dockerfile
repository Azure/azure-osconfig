FROM mcr.microsoft.com/azurelinux/base/core:3.0

RUN tdnf install -y azurelinux-repos-extended
RUN tdnf install -y \
    azurelinux-rpm-macros \
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