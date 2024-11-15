#!/bin/bash
set -e

# Tested and supported runtimes: podman, docker
CONTAINER_RUNTIME="podman"

if ! dpkg --list ${CONTAINER_RUNTIME} > /dev/null && ! rpm -q ${CONTAINER_RUNTIME} > /dev/null; then
    echo "Install prerequisites first: ${CONTAINER_RUNTIME}"
    exit 1
fi

${CONTAINER_RUNTIME} build -f ./Dockerfile-ubuntu22 -t osconfig-asa-ubuntu22 .

mkdir -p ./report

${CONTAINER_RUNTIME} run -it \
    -v ./scan.sh:/azure/azure-osconfig/devops/scripts/asa/scan.sh \
    -v ./report:/azure/azure-osconfig/devops/scripts/asa/report \
    osconfig-asa-ubuntu22 "$@"
