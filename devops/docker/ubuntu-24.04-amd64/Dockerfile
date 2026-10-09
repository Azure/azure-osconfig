FROM mcr.microsoft.com/mirror/docker/library/ubuntu:24.04

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
    libssl-dev \
    libubsan1 \
    python3 \
    tar

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.12.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
