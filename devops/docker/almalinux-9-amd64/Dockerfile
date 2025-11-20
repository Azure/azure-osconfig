# TODO: Add mcr.io retagged images
FROM almalinux:9

ARG CMAKE_VERSION=v3.21.7

RUN dnf install -y \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    openssl-devel \
    make \
    perl \
    perl-IPC-Cmd \
    rpm-build \
    tar \
    unzip \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
