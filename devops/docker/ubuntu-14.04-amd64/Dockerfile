FROM ubuntu:trusty

ARG CMAKE_VERSION=v3.31.8
ARG GTEST_VERSION=release-1.10.0
ARG RAPIDJSON_COMMIT_HASH=24b5e7a8b27f42fa16b96fc70aade9106cf7102f
ARG GIT_VERSION=2.14.0
ARG DEBIAN_FRONTEND=noninteractive
ARG VCPKG_REF=master

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
RUN git clone https://github.com/Kitware/CMake --recursive -b $CMAKE_VERSION
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b $GTEST_VERSION
RUN cd googletest && cmake . && cmake --build . --target install

# rapidjson
RUN git clone --no-checkout https://github.com/Tencent/rapidjson
RUN cd rapidjson && git checkout $RAPIDJSON_COMMIT_HASH && git submodule update --init --recursive
RUN cd rapidjson && cmake . -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF && cmake --build . --target install

RUN rm -rf /git
