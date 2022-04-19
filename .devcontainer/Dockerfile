# Setup development environment for OSConfig (Base: devops/docker/ubuntu20.04-amd64/Dockerfile)
# Creates an image, pre-provisioned with all necessary SDKs and tools for working with OSConfig
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
    gcovr \
    gdb \
    ca-certificates \
    \
    # .NET dependencies
    libc6 \
    libgcc1 \
    libgssapi-krb5-2 \
    libicu66 \
    libssl1.1 \
    libstdc++6 \
    zlib1g
RUN curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 5.0
ENV PATH="${PATH}:/root/.dotnet"

ENV SHELL /bin/zsh

RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v1.1.2/zsh-in-docker.sh)" -- \
    -t https://github.com/denysdovhan/spaceship-prompt