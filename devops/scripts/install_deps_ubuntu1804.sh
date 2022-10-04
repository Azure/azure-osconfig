#!/bin/sh
# Install build essentials for Ubuntu 18.04 base Docker Image
apt -y update
apt-get -y install gnupg software-properties-common
apt -y update
apt-get -y install git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev rapidjson-dev ninja-build wget
# Install Dotnet - Needed for e2e tests
# See https://docs.microsoft.com/en-us/dotnet/core/install/linux-ubuntu#1804-
curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 6.0