# ubi7 images are missing RPM devtools, copy from centos7
FROM centos:centos7 as centos
RUN yum update -y && yumdownloader rpm-build && yumdownloader elfutils

# TODO: Add MSFT mirror
FROM registry.access.redhat.com/ubi7/ubi
COPY --from=centos /rpm-build*.rpm /rpm-build.rpm
COPY --from=centos /elfutils*.rpm /elfutils.rpm

RUN yum-config-manager --enable ubi-7-server-devtools-rpms
RUN yum install -y --disableplugin=subscription-manager \
    autoconf \
    gcc \
    gcc-c++ \
    git \
    libtool \
    make \
    openssl-devel

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# jq
RUN git clone https://github.com/stedolan/jq.git --recursive
RUN cd jq && autoreconf -i && ./configure && make -j$(nproc) && make install && rm -rf /git/jq

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive -b v1.1.0
RUN cd rapidjson && cmake . && cmake --build . --parallel --target install && rm -rf /git/rapidjson

# elfutils
RUN yum install -y /elfutils.rpm && rm -f /elfutils.rpm

# rpm-build
RUN yum install -y /rpm-build.rpm && rm -f /rpm-build.rpm