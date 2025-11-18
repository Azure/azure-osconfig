# ubi7 images are missing RPM devtools, copy from centos7
FROM centos:centos7 AS centos
RUN rm /etc/yum.repos.d/*.repo
# Docker context is at the root of the project
COPY ./centos.repo /etc/yum.repos.d/centos.repo

RUN yum update -y
RUN rm /etc/yum.repos.d/*.repo
COPY ./centos.repo /etc/yum.repos.d/centos.repo

RUN yumdownloader rpm-build elfutils

# Download devtoolset-7 packages
RUN yumdownloader --resolve --destdir=/devtoolset devtoolset-7-gcc devtoolset-7-gcc-c++ devtoolset-7-runtime devtoolset-7-libstdc++-devel devtoolset-7-binutils

# TODO: Add mcr.io retagged images
FROM registry.access.redhat.com/ubi7/ubi
COPY --from=centos /rpm-build*.rpm /rpm-build.rpm
COPY --from=centos /elfutils*.rpm /elfutils.rpm
COPY --from=centos /devtoolset/ /devtoolset/

RUN yum-config-manager --enable ubi-7-server-devtools-rpms
RUN yum install -y --disableplugin=subscription-manager \
    autoconf \
    curl \
    gcc \
    gcc-c++ \
    gettext \
    libcurl-devel \
    libtool \
    make \
    openssl-devel \
    perl \
    perl-core \
    perl-IPC-Cmd \
    tar \
    unzip \
    zip

# Install and enable devtoolset-7
RUN yum install -y /devtoolset/*.rpm && rm -rf /devtoolset
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

# jq
RUN git clone https://github.com/stedolan/jq.git --recursive
RUN cd jq && autoreconf -i && ./configure && make -j$(nproc) && make install && rm -rf /git/jq

# rapidjson
RUN git clone --no-checkout https://github.com/Tencent/rapidjson
RUN cd rapidjson && git checkout 24b5e7a8b27f42fa16b96fc70aade9106cf7102f && git submodule update --init --recursive
RUN cd rapidjson && cmake . -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF && cmake --build . --parallel --target install && rm -rf /git/rapidjson

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest

# elfutils
RUN yum install -y /elfutils.rpm && rm -f /elfutils.rpm

# rpm-build
RUN yum install -y /rpm-build.rpm && rm -f /rpm-build.rpm
