# TODO: Add mcr.io retagged images
FROM registry.suse.com/bci/bci-base:15.5

ARG CMAKE_VERSION=v3.21.7

RUN zypper install -y \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    make \
    libopenssl-devel \
    rpm-build \
    tar

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
