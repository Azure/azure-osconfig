FROM mcr.microsoft.com/mirror/docker/library/debian:bullseye

ARG CMAKE_VERSION=v3.21.7
ARG DEBIAN_FRONTEND=noninteractive

# Bullseye security packages have moved off the live mirror.
RUN cat <<EOF >/etc/apt/sources.list
deb http://deb.debian.org/debian/ bullseye main
deb http://deb.debian.org/debian/ bullseye-updates main
deb http://archive.debian.org/debian-security bullseye-security main
EOF

RUN apt-get -o APT::Update::Error-Mode=any update && apt-get -y install \
    bc \
    build-essential \
    ca-certificates \
    cmake \
    file \
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
