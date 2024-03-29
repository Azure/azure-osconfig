FROM mcr.microsoft.com/mirror/docker/library/ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    bc \
    build-essential \
    cmake \
    curl \
    gcovr\
    git \
    glibc \
    jq \
    libcurl4-openssl-dev \
    libgmock-dev \
    libgtest-dev \
    libssl-dev \
    ninja-build \
    python3-jsonschema \
    rapidjson-dev \
    uuid-dev \
    wget

RUN wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
RUN dpkg -i packages-microsoft-prod.deb && rm packages-microsoft-prod.deb
RUN apt-get update && apt-get install -y dotnet-sdk-5.0

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake