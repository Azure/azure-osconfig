FROM ubuntu:trusty

ARG ONEDS_SDK_VERSION=v3.9.205.1
ARG CMAKE_VERSION=v3.31.8
ARG GTEST_VERSION=release-1.10.0
ARG RAPIDJSON_COMMIT_HASH=24b5e7a8b27f42fa16b96fc70aade9106cf7102f
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get -y update && apt-get -y install \
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
    libsqlite3-dev \
    libssl-dev \
    sqlite3 \
    tar \
    unzip \
    wget \
    zip
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/g++ g++ /usr/bin/g++-5

WORKDIR /git

# 1DS C++ SDK
RUN git clone --no-checkout https://github.com/microsoft/cpp_client_telemetry.git
RUN cd cpp_client_telemetry && git checkout $ONEDS_SDK_VERSION
RUN ./cpp_client_telemetry/tools/setup-buildtools.sh
RUN cd cpp_client_telemetry && cmake . -DBUILD_FUNC_TESTS=OFF -DBUILD_UNIT_TESTS=OFF && cmake --build . --target install

# GTest
RUN git clone https://github.com/google/googletest --recursive -b $GTEST_VERSION
RUN cd googletest && cmake . && cmake --build . --target install

# rapidjson
RUN git clone --no-checkout https://github.com/Tencent/rapidjson
RUN cd rapidjson && git checkout $RAPIDJSON_COMMIT_HASH && git submodule update --init --recursive
RUN cd rapidjson && cmake . -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF && cmake --build . --target install

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b $CMAKE_VERSION
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

RUN rm -rf /git