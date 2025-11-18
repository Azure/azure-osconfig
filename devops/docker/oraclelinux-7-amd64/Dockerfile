# TODO: Add mcr.io retagged images
FROM oraclelinux:7

ARG CMAKE_VERSION=v3.21.7
ARG GIT_VERSION=2.14.0

# Install dev repos
RUN yum install -y oracle-softwarecollection-release-el7

RUN yum install -y \
    autoconf \
    devtoolset-7-gcc \
    devtoolset-7-gcc-c++ \
    gettext \
    jq \
    libtool \
    make \
    perl \
    perl-core \
    perl-IPC-Cmd \
    rpm-build

# Enable devtoolset-7 by default
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