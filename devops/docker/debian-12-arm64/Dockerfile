FROM multiarch/qemu-user-static:x86_64-aarch64 as qemu
FROM arm64v8/debian:bookworm

COPY --from=qemu /usr/bin/qemu-aarch64-static /usr/bin

ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    git \
    cmake \
    build-essential \
    curl \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev \
    rapidjson-dev \
    ninja-build \
    python3-jsonschema \
    wget \
    gcovr \
    jq \
    bc \
    file

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.12.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/googletest
