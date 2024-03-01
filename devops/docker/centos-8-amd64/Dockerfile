# TODO: Add mcr.io retagged images
FROM centos:latest

# CentOS Linux 8 had reached the End Of Life (EOL) on December 31st, 2021. 
# It means that CentOS 8 will no longer receive development resources from
# the official CentOS project. After Dec 31st, 2021, if you need to update 
# your CentOS, you need to change the mirrors to vault.centos.org where they
# will be archived permanently
RUN cd /etc/yum.repos.d/
RUN sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN yum update -y && yum install -y \
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

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --parallel --target install && rm -rf /git/googletest

# rapidjson
RUN git clone https://github.com/Tencent/rapidjson --recursive
RUN cd rapidjson && cmake . && cmake --build . --parallel --target install && rm -rf /git/rapidjson