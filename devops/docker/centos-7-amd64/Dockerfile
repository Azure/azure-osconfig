# TODO: Add mcr.io retagged images
FROM centos:7

RUN rm /etc/yum.repos.d/*.repo
COPY centos.repo /etc/yum.repos.d/centos.repo

RUN yum install -y \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    jq \
    libcurl-devel \
    libtool \
    make \
    openssl-devel \
    rpm-build 

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive -b v1.1.0
RUN cd rapidjson && cmake -DRAPIDJSON_BUILD_TESTS=0 . && cmake --build . --parallel --target install && rm -rf /git/rapidjson

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest
