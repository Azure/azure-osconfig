#!/bin/bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set -e

DISTROS=("ubuntu22" "centos8")
SUPPORTED_RUNTIMES=("docker" "podman")

function command_exists() {
    command -v "$1" >/dev/null 2>&1
}

function echo_header {
  echo ""
  echo "============================================================="
  echo "${1}"
  echo "============================================================="
}

# Loop through the supported runtimes and select the first one that is present
for RUNTIME in "${SUPPORTED_RUNTIMES[@]}"; do
    if command_exists "${RUNTIME}"; then
        CONTAINER_RUNTIME="${RUNTIME}"
        break
    fi
done

# Check if a container runtime was found
if [ -z "${CONTAINER_RUNTIME}" ]; then
    echo "No supported container runtime found. One of the following is required: " "${SUPPORTED_RUNTIMES[@]}"
    exit 1
else
    echo "Using container runtime: ${CONTAINER_RUNTIME}"
fi

if ! dpkg --list "${CONTAINER_RUNTIME}" > /dev/null && ! rpm -q "${CONTAINER_RUNTIME}" > /dev/null; then
    echo "Install prerequisites first: ${CONTAINER_RUNTIME}"
    exit 1
fi

for DISTRO in "${DISTROS[@]}"; do
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

echo_header "No ASA problems detected. Scanned distros: " "${DISTROS[@]}"
