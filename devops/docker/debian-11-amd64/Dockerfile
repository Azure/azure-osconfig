FROM mcr.microsoft.com/mirror/docker/library/debian:bullseye
ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    bc \
    build-essential \
    cmake \
    curl \
    file \
    gcovr \
    git \
    glibc \
    jq \
    libcurl4-openssl-dev \
    libssl-dev \
    ninja-build \
    python3-jsonschema \
    rapidjson-dev \
    uuid-dev \
    wget

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/googletest

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/rapidjson