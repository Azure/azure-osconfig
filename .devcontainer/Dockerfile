# Setup development environment for OSConfig (Base: devops/docker/ubuntu20.04-amd64/Dockerfile)
# Creates an image, pre-provisioned with all necessary SDKs and tools for working with OSConfig
FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    apt-transport-https \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    gcovr \
    gdb \
    git \
    jq \
    libcurl4-openssl-dev \
    libgmock-dev \
    libgtest-dev \
    liblttng-ust-dev \
    libssl-dev \
    ninja-build\
    rapidjson-dev \
    uuid-dev \
    wget

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# .NET Install
RUN curl https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > microsoft.gpg && \
    cp ./microsoft.gpg /etc/apt/trusted.gpg.d/ && \
    curl https://packages.microsoft.com/config/ubuntu/20.04/prod.list > ./microsoft-prod.list && \
    cp ./microsoft-prod.list /etc/apt/sources.list.d/ && \
    apt update -y && apt install -y dotnet-sdk-6.0

# ZSH
ENV SHELL /bin/zsh
RUN sh -c "$(wget -O- https://github.com/deluan/zsh-in-docker/releases/download/v1.1.2/zsh-in-docker.sh)" -- \
    -t https://github.com/denysdovhan/spaceship-prompt

# Terraform
RUN curl -fsSL https://apt.releases.hashicorp.com/gpg | apt-key add - && \
    apt-add-repository "deb [arch=$(dpkg --print-architecture)] https://apt.releases.hashicorp.com $(lsb_release -cs) main" && \
    apt-get -y install terraform

# Installs and executes the Rustup-init executable (https://forge.rust-lang.org/infra/other-installation-methods.html)
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"