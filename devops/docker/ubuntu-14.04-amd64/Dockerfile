FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install 
    gcc \
    git \
    cmake \
    build-essential \
    file

WORKDIR /git