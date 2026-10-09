FROM mcr.microsoft.com/mirror/docker/library/ubuntu:22.04

ARG CMAKE_VERSION=v3.21.7
ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install \
    apt-transport-https \
    bc \
    build-essential \
    ca-certificates \
    clang \
    clang-tools \
    cmake \
    file \
    gcovr \
    git \
    jq \
    libasan8 \
    libgmock-dev \
    libgtest-dev \
    libssl-dev \
    libubsan1 \
    python3 \
    tar

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
