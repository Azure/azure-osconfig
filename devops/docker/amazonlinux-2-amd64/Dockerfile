# TODO: Add mcr.io retagged images
FROM amazonlinux:2

ARG CMAKE_VERSION=v3.21.7

RUN yum install -y \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    make \
    perl \
    perl-core \
    perl-IPC-Cmd \
    rpm-build \
    tar \
    unzip \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
