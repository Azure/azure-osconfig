# When building use build context from child directory - needed for vcpkg jsons
FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

ARG VCPKG_REF=master

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && apt-get -y install --no-install-recommends \
    libgmock-dev \
    libgtest-dev \
    rapidjson-dev \
    uuid-dev \
    wget

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git --branch ${VCPKG_REF} --single-branch /vcpkg
COPY ./vcpkg.json /vcpkg/
COPY ./vcpkg-configuration.json /vcpkg/
WORKDIR /vcpkg
RUN ./bootstrap-vcpkg.sh && ./vcpkg install && ./vcpkg integrate install
ENV VCPKG_ROOT /vcpkg

# Base container pre-defines VCPKG_ROOT in /etc/bash.bashrc
RUN sed -i '/VCPKG_ROOT/d' /etc/bash.bashrc
