FROM mcr.microsoft.com/mirror/docker/library/ubuntu:22.04

ARG CMAKE_VERSION=v3.21.7
ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    bc \
    build-essential \
    clang \
    clang-tools \
    cmake \
    curl \
    file \
    gcovr \
    git \
    jq \
    libasan8 \
    libcurl4-openssl-dev \
    libssl-dev \
    libubsan1 \
    pkg-config \
    python3-jsonschema \
    tar \
    unzip \
    uuid-dev \
    wget \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake