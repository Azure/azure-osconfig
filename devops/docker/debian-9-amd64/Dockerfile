FROM debian:stretch
ARG DEBIAN_FRONTEND=noninteractive

# debian-9 is EOL, update sources.list to use snapshot.debian.org mirrors
RUN cat <<EOF > /etc/apt/sources.list
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian/20220622T000000Z stretch main
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian-security/20220622T000000Z stretch/updates main
deb [check-valid-until=no] http://snapshot.debian.org/archive/debian/20220622T000000Z stretch-updates main
EOF

RUN apt -y update && apt-get -y install software-properties-common
RUN apt -y update && apt-get -y install \
    bc \
    build-essential \
    cmake \
    curl \
    gcovr \
    git \
    jq \
    libcurl4-openssl-dev \
    libssl-dev \
    ninja-build \
    pkg-config \
    python3-jsonschema \
    rapidjson-dev \
    tar \
    unzip \
    uuid-dev \
    wget \
    zip

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b v3.21.7
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

# GTest
RUN git clone https://github.com/google/googletest --recursive -b release-1.10.0
RUN cd googletest && cmake . -G Ninja && cmake --build . --target install && rm -rf /git/googletest

# rapidjson
RUN git clone --no-checkout https://github.com/Tencent/rapidjson
RUN cd rapidjson && git checkout 24b5e7a8b27f42fa16b96fc70aade9106cf7102f && git submodule update --init --recursive
RUN cd rapidjson && cmake . -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF && cmake --build . --parallel --target install && rm -rf /git/rapidjson