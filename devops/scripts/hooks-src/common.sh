# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Shared helpers for hook scripts.  Source this file at the top of each hook:
#
#   HOOK_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
#   REPO_ROOT="$(env -u GIT_DIR git -C "$HOOK_DIR" rev-parse --show-toplevel)"
#   source "$HOOK_DIR/common.sh"
#
# Requires REPO_ROOT to be set before sourcing.

[[ -n "${REPO_ROOT:-}" ]] || { printf 'common.sh: REPO_ROOT must be set before sourcing\n' >&2; exit 1; }

# filter_files <extended-regex> [file ...]
# Prints the absolute paths of files whose repo-relative path matches <regex>.
function filter_files() {
    if [[ $# -lt 1 ]]; then
        printf 'filter_files: expected at least 1 argument (pattern)\n' >&2
        return 1
    fi
    local pattern="$1"; shift
    local rel f
    for f in "${@+"$@"}"; do
        rel="${f#"$REPO_ROOT/"}"
        [[ "$rel" =~ $pattern ]] && printf '%s\n' "$f"
    done
}

# snapshot_files [file ...]
# Records a content hash for each file into _FILE_HASHES[].
# Call before running a tool, then call check_unmodified after.
declare -A _FILE_HASHES
function snapshot_files() {
    _FILE_HASHES=()
    for f in "${@+"$@"}"; do
        _FILE_HASHES["$f"]="$(sha256sum < "$f")"
    done
}

# check_unmodified [file ...]
# Returns 0 if no file content changed since the last snapshot_files call.
# Prints "modified: <path>" for each changed file.
function check_unmodified() {
    local failed=0 after rel
    for f in "${@+"$@"}"; do
        after="$(sha256sum < "$f")"
        if [[ "${_FILE_HASHES["$f"]:-}" != "$after" ]]; then
            rel="${f#"$REPO_ROOT/"}"
            printf '    modified: %s\n' "$rel"
            failed=1
        fi
    done
    return $failed
}
