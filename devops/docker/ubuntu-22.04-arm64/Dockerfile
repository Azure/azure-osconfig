# TODO: Add mcr.io tag
FROM arm64v8/ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    bc \
    build-essential \
    cmake \
    curl \
    git \
    glibc \
    jq \
    libcurl4-openssl-dev \
    libgmock-dev \
    libgtest-dev \
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

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/rapidjson