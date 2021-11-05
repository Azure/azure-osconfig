FROM ubuntu:18.04
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
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install