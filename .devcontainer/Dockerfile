# When building use build context from child directory - needed for vcpkg jsons
FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

ARG VCPKG_REF=master

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && apt-get -y install --no-install-recommends \
    libgmock-dev \
    libgtest-dev \
    rapidjson-dev \
    uuid-dev \
    wget

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.31.8"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git --branch ${VCPKG_REF} --single-branch /vcpkg
COPY ./vcpkg.json /vcpkg/
COPY ./vcpkg-configuration.json /vcpkg/
WORKDIR /vcpkg
RUN ./bootstrap-vcpkg.sh && ./vcpkg install && ./vcpkg integrate install
ENV VCPKG_ROOT /vcpkg

# Base container pre-defines VCPKG_ROOT in /etc/bash.bashrc
sed -i '/VCPKG_ROOT/d' /etc/bash.bashrc
