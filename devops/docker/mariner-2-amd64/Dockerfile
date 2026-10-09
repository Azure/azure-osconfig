FROM mcr.microsoft.com/cbl-mariner/base/core:2.0

RUN tdnf install -y mariner-repos-extended
RUN tdnf install -y \
    build-essential \
    ca-certificates \
    cmake \
    file \
    gcovr \
    git \
    gmock-devel \
    gtest-devel \
    jq \
    mariner-rpm-macros \
    openssl \
    rpm-build \
    tar \
    tmux \
    tree \
    util-linux

RUN tdnf upgrade -y && \
    tdnf clean all
