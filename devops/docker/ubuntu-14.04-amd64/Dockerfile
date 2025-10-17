FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install \
    gcc \
    git \
    build-essential \
    file \
    curl \
    tar \
    unzip \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap -- -DCMAKE_USE_OPENSSL=OFF && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

RUN apt-get install -y ninja-build

RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/googletest
