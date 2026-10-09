FROM ubuntu:trusty

ARG CMAKE_VERSION=v3.21.7
ARG GIT_VERSION=2.14.0
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get -y update && apt-get -y install \
    autoconf \
    automake \
    build-essential \
    cmake3 \
    curl \
    file \
    g++-5 \
    gcc-5 \
    gettext \
    git \
    libcurl4-openssl-dev \
    libexpat1-dev \
    libgtest-dev \
    libssl-dev \
    libtool \
    ninja-build \
    pkg-config \
    sqlite3 \
    tar \
    unzip \
    uuid-dev \
    wget \
    zip

WORKDIR /tmp

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 40 --slave /usr/bin/g++ g++ /usr/bin/g++-5

# Build Git from source
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

# Set GCC 4.8 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.8

WORKDIR /tmp

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b $CMAKE_VERSION
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

RUN rm -rf /git
