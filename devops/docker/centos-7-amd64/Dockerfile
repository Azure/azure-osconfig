# TODO: Add mcr.io retagged images
FROM centos:7

ARG CMAKE_VERSION=v3.21.7
ARG GIT_VERSION=2.14.0

RUN rm /etc/yum.repos.d/*.repo
# Docker context is at the root of the project
COPY ./centos.repo /etc/yum.repos.d/centos.repo

# Install dependencies needed for Git compilation and general build
RUN yum install -y \
    autoconf \
    curl \
    devtoolset-7-gcc \
    devtoolset-7-gcc-c++ \
    gettext-devel \
    jq \
    libtool \
    make \
    openssl-devel \
    perl \
    perl-core \
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

# Git
RUN curl -L https://github.com/git/git/archive/v${GIT_VERSION}.tar.gz | tar xz
RUN cd git-${GIT_VERSION} && \
    make configure && \
    ./configure && \
    make all -j$(nproc) && \
    make install && \
    hash -r && \
    rm -rf /git/git-${GIT_VERSION}

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
