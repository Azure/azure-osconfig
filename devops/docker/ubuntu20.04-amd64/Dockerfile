FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    git \
    cmake \
    build-essential \
    curl \
    libcurl4-openssl-dev \
    libssl-dev \
    uuid-dev \
    libgtest-dev \
    libgmock-dev \
    liblttng-ust-dev \
    rapidjson-dev \
    ninja-build\
    wget \
    gcovr
RUN wget https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
RUN dpkg -i packages-microsoft-prod.deb && rm packages-microsoft-prod.deb
RUN apt-get update && apt-get install -y dotnet-sdk-5.0