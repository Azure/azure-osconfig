#!/bin/bash

set -euxo pipefail

SCRIPT_DIR=$(dirname "$0")
mkdir -p "$SCRIPT_DIR/../build"
cd "$SCRIPT_DIR/../build"

cmake ../src -DCMAKE_BUILD_TYPE=Debug -Duse_prov_client=ON -Dhsm_type_symm_key=ON -DBUILD_TESTS=ON
sudo cmake --build . --config Debug --target install
