FROM ubuntu:trusty

ARG GIT_VERSION=2.14.0
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

# Git
RUN wget https://github.com/git/git/archive/refs/tags/v${GIT_VERSION}.tar.gz \
    && tar xf v${GIT_VERSION}.tar.gz \
    && cd git-${GIT_VERSION} \
    && make configure \
    && ./configure --prefix=/usr/local \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf git-${GIT_VERSION}* \
    && hash -r

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap -- -DCMAKE_USE_OPENSSL=OFF && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake