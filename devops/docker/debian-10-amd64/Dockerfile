FROM mcr.microsoft.com/mirror/docker/library/debian:buster

ARG CMAKE_VERSION=v3.21.7
ARG DEBIAN_FRONTEND=noninteractive

RUN cat <<EOF >/etc/apt/sources.list
deb http://archive.debian.org/debian/ buster main contrib non-free
deb http://archive.debian.org/debian/ buster-proposed-updates main contrib non-free
deb http://archive.debian.org/debian-security buster/updates main contrib non-free
EOF

RUN apt -y update && apt-get -y install \
    bc \
    build-essential \
    ca-certificates \
    cmake \
    gcovr \
    git \
    jq \
    libssl-dev \
    python3 \
    tar

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --target install && rm -rf /git/googletest
