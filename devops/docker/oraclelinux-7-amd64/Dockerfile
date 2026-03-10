# TODO: Add mcr.io retagged images
FROM oraclelinux:7

ARG CMAKE_VERSION=v3.30.1
ARG GIT_VERSION=2.14.0

# Install dev repos
RUN yum install -y oracle-softwarecollection-release-el7

RUN yum install -y \
    autoconf \
    devtoolset-7-gcc \
    devtoolset-7-gcc-c++ \
    gettext \
    jq \
    libcurl-devel \
    libtool \
    make \
    openssl-devel \
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
    ./configure --with-curl --with-openssl && \
    make all -j$(nproc) && \
    make install && \
    hash -r && \
    rm -rf /git/git-${GIT_VERSION}

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

COPY .  /src
WORKDIR /src
RUN ls -al
RUN ls -al external || true
RUN ls -al external/vcpkg || true
RUN cd external/vcpkg && sed -e "s/\"2\\.7\\.4\"/\"1\\.0\\.0\"/g" -i scripts/vcpkg-tools.json && ./bootstrap-vcpkg.sh
RUN cd src && ../external/vcpkg/vcpkg install

