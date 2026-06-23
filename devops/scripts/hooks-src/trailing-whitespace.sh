#!/usr/bin/env bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Hook: trailing-whitespace
# Strips trailing whitespace from each line and ensures every file ends with
# a single newline.  Fails if any file was modified (so the developer re-stages).
#
# Files checked: *.c *.h *.cpp *.hpp *.cs *.md *.cmake *.toml *.json *.txt

set -euo pipefail

readonly HOOK_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
readonly REPO_ROOT="$(env -u GIT_DIR git -C "$HOOK_DIR" rev-parse --show-toplevel)"
# shellcheck source=common.sh
source "$HOOK_DIR/common.sh"

readonly PATTERN='\.(c|h|cpp|hpp|cs|md|cmake|toml|json|txt)$'

mapfile -t FILES < <(filter_files "$PATTERN" "$@")
[[ ${#FILES[@]} -eq 0 ]] && exit 0

printf '    %d file(s) to check\n' "${#FILES[@]}"
snapshot_files "${FILES[@]}"

for f in "${FILES[@]}"; do
    # Strip trailing whitespace on every line
    sed -i 's/[[:space:]]*$//' "$f"
    # Ensure the file ends with exactly one newline
    if [[ -s "$f" ]]; then
        # Collapse any trailing newlines to a single newline; add one if missing
        perl -i -0pe 's/\n*\z/\n/' "$f"
    fi
done

if ! check_unmodified "${FILES[@]}"; then
    printf '    Re-stage the files above.\n'
    exit 1
fi
