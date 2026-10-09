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

# GoogleTest and GoogleMock
RUN git clone --depth 1 --branch release-1.12.0 https://github.com/google/googletest.git
RUN cmake -S googletest -B googletest/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF && \
    cmake --build googletest/build --parallel --target install && rm -rf /git/googletest
