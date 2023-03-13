FROM multiarch/qemu-user-static:x86_64-arm as qemu
FROM arm32v7/ubuntu:20.04
COPY --from=qemu /usr/bin/qemu-arm-static /usr/bin
ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    git \
    cmake \
    build-essential \
    curl \
    openssl \
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

ENV OPENSSL_ROOT_DIR=/usr/lib/arm-linux-gnueabihf/

WORKDIR /git
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.22.0-rc1
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
WORKDIR /git
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/rapidjson