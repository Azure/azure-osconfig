# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(cpp)
corrosion_import_crate(MANIFEST_PATH rust/Cargo.toml)
install(TARGETS rustsample DESTINATION ${MODULES_INSTALL_DIR})