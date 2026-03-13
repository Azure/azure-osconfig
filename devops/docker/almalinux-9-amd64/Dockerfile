# TODO: Add mcr.io retagged images
FROM almalinux:9

ARG CMAKE_VERSION=v3.30.1

RUN dnf install -y \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    openssl-devel \
    make \
    perl \
    perl-IPC-Cmd \
    rpm-build \
    tar \
    unzip \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake


# Run vcpkg to install build depends
COPY .  /src
WORKDIR /src
RUN (cd external/vcpkg && ./bootstrap-vcpkg.sh) && (cd src && ../external/vcpkg/vcpkg install) && rm -rf /src
