#!/bin/sh
# Install build essentials for Debian 9 base Docker Image - (based on Ubuntu 18.04 build requirements)
apt -y update
apt-get -y install software-properties-common gpg
apt-add-repository ppa:lttng/stable-2.10 -y
apt -y update
apt-get -y install git cmake build-essential curl libcurl4-openssl-dev libssl1.0-dev uuid-dev libgtest-dev google-mock liblttng-ust-dev rapidjson-dev ninja-build wget

# Outdated CMake on Debian 9
cd ~
git clone https://github.com/Kitware/CMake --recursive -b v3.19.7
cd CMake
./bootstrap && make -j$(nproc) && make install
hash -r

# libgtest-dev only installs GTest 1.7.0 Use latest release (April 15 2021 - v1.10.x)
# https://github.com/google/googletest/releases/tag/v1.10.x
cd ~
wget https://github.com/google/googletest/archive/refs/tags/v1.10.x.tar.gz
tar xvf v1.10.x.tar.gz
cd googletest-1.10.x
cmake . -G Ninja
cmake --build . --target install

cd ~
git clone https://github.com/Tencent/rapidjson --recursive
cd rapidjson
cmake . -G Ninja
cmake --build . --target install

# cleanup
cd ~ && rm -rf googletest CMake rapidjson

# Install Dotnet - Needed for e2e tests
# See https://docs.microsoft.com/en-us/dotnet/core/install/linux-ubuntu#1804-
curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 6.0