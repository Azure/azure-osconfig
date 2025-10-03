FROM mcr.microsoft.com/mirror/docker/library/ubuntu:24.04

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
    gcovr\
    git \
    jq \
    libasan8 \
    libcurl4-openssl-dev \
    libssl-dev \
    libubsan1 \
    ninja-build \
    pkg-config \
    python3-jsonschema \
    rapidjson-dev \
    tar \
    unzip \
    uuid-dev \
    wget \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.12.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
