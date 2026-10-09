FROM mcr.microsoft.com/mirror/docker/library/debian:bullseye

ARG CMAKE_VERSION=v3.21.7
ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    bc \
    build-essential \
    cmake \
    curl \
    file \
    gcovr \
    git \
    jq \
    libssl-dev \
    pkg-config \
    python3-jsonschema \
    tar \
    unzip \
    uuid-dev \
    wget \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GoogleTest and GoogleMock
RUN git clone --depth 1 --branch release-1.12.0 https://github.com/google/googletest.git
RUN cmake -S googletest -B googletest/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF && \
    cmake --build googletest/build --parallel --target install && rm -rf /git/googletest
