FROM debian:stretch

ARG CMAKE_VERSION=v3.21.7
ARG DEBIAN_FRONTEND=noninteractive

# debian-9 is EOL, update sources.list to use snapshot.debian.org mirrors
RUN cat <<EOF > /etc/apt/sources.list
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian/20220622T000000Z stretch main
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/20220622T000000Z stretch/updates main
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian/20220622T000000Z stretch-updates main
EOF

RUN apt -y update && apt-get -y install \
    bc \
    build-essential \
    ca-certificates \
    cmake \
    gcovr \
    git \
    jq \
    libssl-dev \
    python3 \
    tar

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . && cmake --build . --target install && rm -rf /git/googletest
