FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install \ 
    gcc \
    git \
    build-essential \
    file

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap -- -DCMAKE_USE_OPENSSL=OFF && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake