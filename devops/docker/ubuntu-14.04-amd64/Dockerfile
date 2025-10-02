FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install \
    autoconf \
    build-essential \
    curl \
    file \
    gcc \
    gettext \
    git \
    libcurl4-gnutls-dev \
    libssl-dev \
    tar \
    unzip \
    zip \
    zlib1g-dev

WORKDIR /git

# Install Git 2.14.0 from source (required for vcpkg)
RUN curl -L https://github.com/git/git/archive/v2.14.0.tar.gz | tar xz
RUN cd git-2.14.0 && \
    make configure && \
    ./configure --with-curl --with-openssl && \
    make all -j$(nproc) && \
    make install && \
    hash -r && \
    rm -rf /git/git-2.14.0

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap -- -DCMAKE_USE_OPENSSL=OFF && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
