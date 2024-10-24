FROM ubuntu:trusty

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && apt-get -y install \ 
    gcc \
    git \
    cmake3 \
    build-essential \
    file

WORKDIR /git