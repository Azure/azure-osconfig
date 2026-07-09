# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.
#
# MOF parser fuzzer. Built only when -DBUILD_FUZZER=ON, which also requires
# clang and enables -fsanitize=fuzzer-no-link,address,undefined globally
# (see src/CMakeLists.txt). Run manually:
#
#   ./mof-parser-fuzzer -max_total_time=60 seed_corpus/

add_executable(mof-parser-fuzzer target.cpp)

# The shared library is built with -fsanitize=fuzzer-no-link (no main); add
# -fsanitize=fuzzer here to pull in libFuzzer's entry point and link it.
target_compile_options(mof-parser-fuzzer PRIVATE -fsanitize=fuzzer)
target_link_options(mof-parser-fuzzer PRIVATE -fsanitize=fuzzer)

target_link_libraries(mof-parser-fuzzer PRIVATE
    compliance-engine-assessor-lib
)
