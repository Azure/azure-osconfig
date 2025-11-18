ARG CMAKE_VERSION=v3.21.7
ARG GIT_VERSION=2.14.0

# ubi7 images are missing RPM devtools, copy from centos7
FROM centos:centos7 AS centos
RUN rm /etc/yum.repos.d/*.repo
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
    libtool \
    libcurl-devel \
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

# Git
RUN curl -L https://github.com/git/git/archive/v${GIT_VERSION}.tar.gz | tar xz
RUN cd git-${GIT_VERSION} && \
    make configure && \
    ./configure --with-curl --with-openssl && \
    make all -j$(nproc) && \
    make install && \
    hash -r && \
    rm -rf /git/git-${GIT_VERSION}

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# jq
RUN git clone https://github.com/stedolan/jq.git --recursive
RUN cd jq && autoreconf -i && ./configure && make -j$(nproc) && make install && rm -rf /git/jq

# elfutils
RUN yum install -y /elfutils.rpm && rm -f /elfutils.rpm

# rpm-build
RUN yum install -y /rpm-build.rpm && rm -f /rpm-build.rpm
