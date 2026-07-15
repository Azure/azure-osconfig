# TODO: Add mcr.io retagged images
FROM registry.suse.com/bci/bci-base:15.7@sha256:c1ec34664d6735fdec897dabefed021386e1bf36a5aef1138bf1d41ab614efb9

ARG CMAKE_VERSION=v3.30.1

RUN zypper install -y \
    autoconf \
    curl \
    gcc \
    gcc-c++ \
    git \
    jq \
    libtool \
    make \
    libopenssl-devel \
    python3 \
    rpm-build \
    tar \
    unzip \
    zip

RUN python3 -m venv /opt/kompli-venv
ENV PATH="/opt/kompli-venv/bin:${PATH}"
RUN printf '%s\n' \
    'dataclasses==0.8 \' \
    '    --hash=sha256:0201d89fa866f68c8ebd9d08ee6ff50c0b255f8ec63a71c16fda7af82bb887bf \' \
    '    --hash=sha256:8479067f342acf957dc82ec415d355ab5edb7e7646b90dc6e2fd1d96ad084c97' \
    > /tmp/python-requirements.txt && \
    pip install --require-hashes -r /tmp/python-requirements.txt && \
    rm -f /tmp/python-requirements.txt

WORKDIR /git

# CMake
RUN git clone https://github.com/Kitware/CMake --recursive -b ${CMAKE_VERSION}
RUN cd CMake && ./bootstrap && make -j$(nproc) && make install && hash -r && rm -rf /git/CMake

COPY .  /src
WORKDIR /src
RUN (cd external/vcpkg && ./bootstrap-vcpkg.sh) && (cd src && ../external/vcpkg/vcpkg install) && rm -rf /src
