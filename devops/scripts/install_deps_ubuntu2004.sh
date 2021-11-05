#!/bin/sh
# Install build essentials for Ubuntu 20.04 base Docker image
apt -y update
apt-get -y install software-properties-common
apt-add-repository ppa:lttng/stable-2.10 -y
apt -y update
apt-get -y install git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev libgtest-dev libgmock-dev liblttng-ust-dev rapidjson-dev ninja-build wget