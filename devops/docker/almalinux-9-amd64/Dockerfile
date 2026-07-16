# TODO: Add mcr.io retagged images
FROM almalinux:9@sha256:d2515c769e7b73f95c4fde38c0a505336ff38f14990c0b7253b77060a049a743

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
    python3 \
    rpm-build \
    tar \
    unzip \
    zip

RUN python3 -m venv /opt/kompli-venv
ENV PATH="/opt/kompli-venv/bin:${PATH}"

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake


# Run vcpkg to install build depends
COPY .  /src
WORKDIR /src
RUN (cd external/vcpkg && ./bootstrap-vcpkg.sh) && (cd src && ../external/vcpkg/vcpkg install) && rm -rf /src
