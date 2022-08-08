# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(cpp)
if (BUILD_RUST)
    corrosion_import_crate(MANIFEST_PATH rust/Cargo.toml)
endif()