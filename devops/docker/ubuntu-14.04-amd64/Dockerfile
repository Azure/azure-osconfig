FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    git \
    cmake \
    build-essential \
    curl \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev \
    ninja-build \
    python3-jsonschema \
    wget \
    gcovr\
    jq \
    bc \
    file

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive -b v1.1.0
RUN cd rapidjson && cmake . && cmake --build . --parallel --target install && rm -rf /git/rapidjson