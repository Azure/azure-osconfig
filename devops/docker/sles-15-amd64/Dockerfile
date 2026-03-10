# TODO: Add mcr.io retagged images
FROM registry.suse.com/bci/bci-base:15.5

ARG CMAKE_VERSION=v3.30.1

RUN zypper install -y \
    autoconf \
    curl \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    make \
    libopenssl-devel \
    rpm-build \
    tar \
    unzip \
    zip

WORKDIR /git

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

