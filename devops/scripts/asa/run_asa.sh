#!/bin/bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set -e

SUPPORTED_RUNTIMES=("docker" "podman")

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Loop through the supported runtimes and select the first one that is present
for RUNTIME in "${SUPPORTED_RUNTIMES[@]}"; do
    if command_exists "${RUNTIME}"; then
        CONTAINER_RUNTIME="${RUNTIME}"
        break
    fi
done

# Check if a container runtime was found
if [ -z "$CONTAINER_RUNTIME" ]; then
    echo "No supported container runtime found. One of the following is required: " "${SUPPORTED_RUNTIMES[@]}"
    exit 1
else
    echo "Using container runtime: $CONTAINER_RUNTIME"
fi

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
