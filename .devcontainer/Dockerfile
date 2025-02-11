# When building use build context from child directory - needed for vcpkg jsons
FROM mcr.microsoft.com/devcontainers/cpp:1-debian-11

ARG VCPKG_REF=master

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive && apt-get -y install --no-install-recommends \
    libgmock-dev \
    libgtest-dev \
    rapidjson-dev \
    uuid-dev \
    wget

# Install openssl 3.4.0
# RUN wget https://github.com/openssl/openssl/releases/download/openssl-3.4.0/openssl-3.4.0.tar.gz \
#     && tar -xvf openssl-3.4.0.tar.gz \
#     && cd openssl-3.4.0 \
#     && ./config \
#     && make -j$(nproc) \
#     && make install \
#     && cd .. \
#     && rm -rf openssl-3.4.0 openssl-3.4.0.tar.gz

    # TODO: Missing this
    # export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

# Install vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git --branch ${VCPKG_REF} --single-branch /vcpkg
COPY ./vcpkg.json /vcpkg/
COPY ./vcpkg-configuration.json /vcpkg/
WORKDIR /vcpkg
RUN ./bootstrap-vcpkg.sh && ./vcpkg install && ./vcpkg integrate install
ENV VCPKG_ROOT /vcpkg