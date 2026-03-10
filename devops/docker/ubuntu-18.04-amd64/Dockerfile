FROM mcr.microsoft.com/mirror/docker/library/ubuntu:18.04

ARG CMAKE_VERSION=v3.30.1
ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    bc \
    build-essential \
    curl \
    gcovr \
    git \
    jq \
    libcurl4-openssl-dev \
    libssl-dev \
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

COPY .  /src
WORKDIR /src
RUN ls -al
RUN ls -al external || true
RUN ls -al external/vcpkg || true
RUN cd external/vcpkg && sed -e "s/\"2\\.7\\.4\"/\"1\\.0\\.0\"/g" -i scripts/vcpkg-tools.json && ./bootstrap-vcpkg.sh
RUN cd src && ../external/vcpkg/vcpkg install

