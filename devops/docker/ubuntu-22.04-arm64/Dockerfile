# TODO: Add mcr.io tag
FROM arm64v8/ubuntu:20.04
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
    libgtest-dev \
    libgmock-dev \
    rapidjson-dev \
    ninja-build \
    python3-jsonschema \
    wget \
    jq \
    bc

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/rapidjson