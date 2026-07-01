#!/usr/bin/env bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Hook: line-endings
# Enforces LF line endings on source/doc files.
# Fails if any file was modified (so the developer re-stages).
#
# Files checked: *.c *.h *.cpp *.hpp *.cs *.md *.cmake *.toml *.json *.txt

set -euo pipefail

readonly HOOK_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
readonly REPO_ROOT="$(env -u GIT_DIR git -C "$HOOK_DIR" rev-parse --show-toplevel)"
# shellcheck source=common.sh
source "$HOOK_DIR/common.sh"

readonly PATTERN='\.(c|h|cpp|hpp|cs|md|cmake|toml|json|txt)$'

mapfile -t FILES < <(filter_files "$PATTERN" "$@")
[[ ${#FILES[@]} -eq 0 ]] && exit 0

printf '    %d file(s) to check\n' "${#FILES[@]}"
snapshot_files "${FILES[@]}"

for f in "${FILES[@]}"; do
    # Convert CRLF -> LF; leave lone CR untouched (binary safety)
    sed -i 's/\r$//' "$f"
done

if ! check_unmodified "${FILES[@]}"; then
    printf '    Re-stage the files above.\n'
    exit 1
fi
