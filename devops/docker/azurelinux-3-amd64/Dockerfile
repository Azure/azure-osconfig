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

WORKDIR /git

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.12.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
