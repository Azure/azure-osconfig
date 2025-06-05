# TODO: Add mcr.io retagged images
FROM registry.access.redhat.com/ubi9/ubi

RUN yum install -y --disableplugin=subscription-manager \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    libcurl-devel \
    make \
    ninja-build \
    openssl-devel \
    openssl-devel \
    rpm-build

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b v1.12.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest

# rapidjson
RUN git clone --no-checkout https://github.com/Tencent/rapidjson
RUN cd rapidjson && git checkout 24b5e7a8b27f42fa16b96fc70aade9106cf7102f && git submodule update --init --recursive
RUN cd rapidjson && cmake . -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF && cmake --build . --parallel --target install && rm -rf /git/rapidjson