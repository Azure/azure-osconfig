# When building use build context from child directory - needed for vcpkg jsons
FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

ARG VCPKG_REF=master

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && apt-get -y install --no-install-recommends \
    libgmock-dev \
    libgtest-dev \
    rapidjson-dev \
    uuid-dev \
    wget
