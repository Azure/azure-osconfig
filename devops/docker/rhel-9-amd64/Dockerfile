# TODO: Add mcr.io retagged images
FROM registry.access.redhat.com/ubi9/ubi

ARG CMAKE_VERSION=v3.21.7

RUN yum install -y --disableplugin=subscription-manager \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    make \
    openssl-devel \
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

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.12.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
