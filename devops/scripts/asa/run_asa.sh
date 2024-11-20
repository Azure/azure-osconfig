#!/bin/bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set -e

# Tested and supported runtimes: podman, docker
CONTAINER_RUNTIME="podman"

DISTROS=("ubuntu22" "centos8")

function echo_header {
  echo ""
  echo "============================================================="
  echo ${1}
  echo "============================================================="
}

if ! dpkg --list ${CONTAINER_RUNTIME} > /dev/null && ! rpm -q ${CONTAINER_RUNTIME} > /dev/null; then
    echo "Install prerequisites first: ${CONTAINER_RUNTIME}"
    exit 1
fi

for DISTRO in ${DISTROS[@]}; do
    echo_header "RUNNING ${DISTRO} CONTAINER IMAGE BUILD"

    ${CONTAINER_RUNTIME} build -f ./Dockerfile-${DISTRO} -t osconfig-asa-${DISTRO} .

    echo_header "RUNNING ASA SCAN IN ${DISTRO} CONTAINER"
    mkdir -p ./report/${DISTRO}

    ${CONTAINER_RUNTIME} run -it \
        -v ./scan.sh:/azure/azure-osconfig/devops/scripts/asa/scan.sh \
        -v ./report/${DISTRO}:/azure/azure-osconfig/devops/scripts/asa/report \
        osconfig-asa-${DISTRO} "$@"
    echo_header "Finished for ${DISTRO}"
done

echo_header "No ASA problems detected. Scanned distros: ${DISTROS[@]}"
