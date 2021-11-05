#!/bin/sh
# Install build essentials for Ubuntu 18.04 base Docker Image
apt -y update
apt-get -y install gnupg software-properties-common
apt-add-repository ppa:lttng/stable-2.10 -y
apt -y update
apt-get -y install git cmake build-essential curl libcurl4-openssl-dev libssl-dev uuid-dev liblttng-ust-dev rapidjson-dev ninja-build wget