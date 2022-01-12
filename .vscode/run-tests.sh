#!/bin/bash

set -euxo pipefail

SCRIPT_DIR=$(dirname "$0")
mkdir -p "$SCRIPT_DIR/../build"
cd "$SCRIPT_DIR/../build"

cmake --build . --config Debug  --target test