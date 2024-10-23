FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    gcc \
    git \
    cmake \
    build-essential

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake