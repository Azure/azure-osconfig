# TODO: Add mcr.io retagged images
FROM centos:7

RUN rm /etc/yum.repos.d/*.repo
# Docker context is at the root of the project
COPY ./devops/docker/centos-7-amd64/centos.repo /etc/yum.repos.d/centos.repo

# Install dependencies needed for Git compilation and general build
RUN yum install -y \
    autoconf \
    curl \
    devtoolset-7-gcc \
    devtoolset-7-gcc-c++ \
    gettext-devel \
    jq \
    libcurl-devel \
    libtool \
    make \
    openssl-devel \
    perl \
    perl-CPAN \
    perl-devel \
    perl-IPC-Cmd \
    rpm-build \
    tar \
    unzip \
    zip \
    zlib-devel

# Add devtoolset-7 bin to PATH (GCC 7)
ENV PATH=/opt/rh/devtoolset-7/root/usr/bin:$PATH

WORKDIR /git

# Install Git 2.14.0 from source (required for vcpkg)
RUN curl -L https://github.com/git/git/archive/v2.14.0.tar.gz | tar xz
RUN cd git-2.14.0 && \
    make configure && \
    ./configure && \
    make all -j$(nproc) && \
    make install && \
    hash -r && \
    rm -rf /git/git-2.14.0

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# rapidjson
RUN git clone --no-checkout https://github.com/Tencent/rapidjson
RUN cd rapidjson && git checkout 24b5e7a8b27f42fa16b96fc70aade9106cf7102f && git submodule update --init --recursive
RUN cd rapidjson && cmake . -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF && cmake --build . --parallel --target install && rm -rf /git/rapidjson

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
