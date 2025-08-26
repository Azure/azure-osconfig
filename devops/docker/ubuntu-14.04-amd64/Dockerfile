FROM ubuntu:trusty

ARG CMAKE_VERSION=v3.31.8
ARG GTEST_VERSION=release-1.10.0
ARG RAPIDJSON_COMMIT_HASH=24b5e7a8b27f42fa16b96fc70aade9106cf7102f
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
    libsqlite3-dev \
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

# Set up static build environment
ENV PREFIX=/opt/curl-static
RUN mkdir -p "$PREFIX"

# Common environment variables for static builds
ENV PATH="$PREFIX/bin:$PATH"
ENV PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
ENV CPPFLAGS="-I$PREFIX/include"
ENV LDFLAGS="-L$PREFIX/lib"
ENV CFLAGS="-O2 -fPIC"
ENV CXXFLAGS="-O2 -fPIC"

# Make pkg-config prefer static libs
ENV PKG_CONFIG="pkg-config --static"

# Build all dependencies for curl from source to allow for static linking
# Build zlib
RUN wget https://zlib.net/zlib-1.3.1.tar.gz \
    && tar xf zlib-1.3.1.tar.gz \
    && cd zlib-1.3.1 \
    && ./configure --static --prefix="$PREFIX" \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf zlib-1.3.1*

# Build OpenSSL
RUN wget https://www.openssl.org/source/openssl-1.1.1w.tar.gz \
    && tar xf openssl-1.1.1w.tar.gz \
    && cd openssl-1.1.1w \
    && ./Configure linux-x86_64 no-shared no-tests no-asm --prefix="$PREFIX" --openssldir="$PREFIX/ssl" -fPIC \
    && make -j$(nproc) \
    && make install_sw \
    && cd .. \
    && rm -rf openssl-1.1.1w*

# Build nghttp2
RUN wget https://github.com/nghttp2/nghttp2/releases/download/v1.61.0/nghttp2-1.61.0.tar.gz \
    && tar xf nghttp2-1.61.0.tar.gz \
    && cd nghttp2-1.61.0 \
    && ./configure --prefix="$PREFIX" --disable-shared --enable-static --without-libxml2 --disable-python-bindings \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf nghttp2-1.61.0*

# Build cURL
RUN wget https://curl.se/download/curl-8.7.1.tar.xz \
    && tar xf curl-8.7.1.tar.xz \
    && cd curl-8.7.1 \
    && ./configure \
        --prefix="$PREFIX" \
        --disable-shared --enable-static \
        --with-ssl="$PREFIX" \
        --with-zlib="$PREFIX" \
        --with-nghttp2="$PREFIX" \
        --disable-ldap \
        --disable-rtsp \
        --without-libidn2 \
        --without-librtmp \
        --without-brotli \
        --enable-threaded-resolver \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf curl-8.7.1*

# Set GCC 4.8 as default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 60 --slave /usr/bin/g++ g++ /usr/bin/g++-4.8

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
