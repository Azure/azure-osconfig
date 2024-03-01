# TODO: Add MSFT mirror
FROM registry.access.redhat.com/ubi8/ubi

RUN yum install -y --disableplugin=subscription-manager \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libcurl-devel \
    libtool \
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
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . && cmake --build . --parallel --target install && rm -rf /git/rapidjson