FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    gcc \
    git \
    cmake \
    build-essential \
    file

WORKDIR /git