#!/usr/bin/env bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Hook: clang-format
# Applies clang-format in-place on relevant C/C++ sources and fails if any
# file was reformatted (so the developer re-stages the corrected files).
#
# Files checked:
#   src/modules/complianceengine/**/*.{h,hpp,c,cpp}
#   src/compliance-engine-assessor/**/*.{h,hpp,c,cpp}
#   src/common/telemetry/**/*.{h,hpp,c,cpp}
# Excluded:
#   src/modules/complianceengine/src/lib/ProcedureMap.{h,cpp}  (generated)

set -euo pipefail

readonly HOOK_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
readonly REPO_ROOT="$(env -u GIT_DIR git -C "$HOOK_DIR" rev-parse --show-toplevel)"
# shellcheck source=common.sh
source "$HOOK_DIR/common.sh"

readonly PATTERN='^(src/modules/complianceengine|src/compliance-engine-assessor|src/common/telemetry)/.*\.(h|hpp|c|cpp)$'
readonly EXCLUDE='^src/modules/complianceengine/src/lib/ProcedureMap\.(h|cpp)$'

mapfile -t _MATCHED < <(filter_files "$PATTERN" "$@")

FILES=()
for f in "${_MATCHED[@]+"${_MATCHED[@]}"}"; do
    rel="${f#"$REPO_ROOT/"}"
    [[ "$rel" =~ $EXCLUDE ]] || FILES+=("$f")
done

[[ ${#FILES[@]} -eq 0 ]] && exit 0

printf '    %d file(s) to check\n' "${#FILES[@]}"
snapshot_files "${FILES[@]}"
clang-format -i -style=file "${FILES[@]}"

if ! check_unmodified "${FILES[@]}"; then
    printf '    Re-stage the files above after running clang-format locally.\n'
    exit 1
fi
