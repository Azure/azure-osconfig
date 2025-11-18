# TODO: Add mcr.io retagged images
FROM registry.access.redhat.com/ubi8/ubi

ARG CMAKE_VERSION=v3.21.7

RUN yum install -y --disableplugin=subscription-manager \
    autoconf \
    curl \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
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
