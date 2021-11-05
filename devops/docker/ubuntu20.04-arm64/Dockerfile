FROM multiarch/qemu-user-static:x86_64-aarch64 as qemu
FROM arm64v8/ubuntu:20.04
COPY --from=qemu /usr/bin/qemu-aarch64-static /usr/bin
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
    liblttng-ust-dev \
    rapidjson-dev \
    ninja-build\
    wget 

WORKDIR /git
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.19.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
WORKDIR /git
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/rapidjson