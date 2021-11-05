FROM debian:stretch
ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    git \
    cmake \
    build-essential \
    curl \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev \
    liblttng-ust-dev \
    rapidjson-dev \
    ninja-build\
    wget \
    gcovr

WORKDIR /git
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.19.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake
WORKDIR /git
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/googletest
WORKDIR /git
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/rapidjson