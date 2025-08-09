FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && apt-get -y install --no-install-recommends \
    libgmock-dev \
    libgtest-dev \
    libsqlite3-dev \
    rapidjson-dev \
    uuid-dev \
    wget
