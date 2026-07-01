#!/usr/bin/env bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# Code-quality hook runner for Kompli.
#
# Usage:
#   devops/scripts/run-hooks.sh --help      Show this help
#   devops/scripts/run-hooks.sh             Check staged files (pre-commit hook mode)
#   devops/scripts/run-hooks.sh --all-files Check all tracked files (CI / manual run)
#   devops/scripts/run-hooks.sh --install   Install as .git/hooks/pre-commit
#
# Adding a hook
# -------------
# Add an executable *.sh script to devops/scripts/hooks-src/.
# Run --install to symlink it into devops/scripts/hooks/.
# The script receives all relevant files as absolute-path arguments.
# It should filter, fix, and return 0 (pass) or non-zero (fail).
# Source devops/scripts/hooks-src/common.sh for shared helpers.

set -euo pipefail

# Resolve symlink so this works when installed as .git/hooks/pre-commit
readonly SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
# Unset GIT_DIR: git sets it when launching hooks, which breaks rev-parse
readonly REPO_ROOT="$(env -u GIT_DIR git -C "$SCRIPT_DIR" rev-parse --show-toplevel)"
readonly HOOKS_DIR="$SCRIPT_DIR/hooks"
readonly HOOKS_SRC_DIR="$SCRIPT_DIR/hooks-src"

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTION]

Options:
  (none)        Check staged files — intended for use as a git pre-commit hook.
  --all-files   Check all tracked files in the repository (CI / manual run).
  --install     Symlink selected hooks from hooks-src/ into hooks/, then install
                this script as .git/hooks/pre-commit.
  --help        Show this help and exit.

Adding a hook:
  1. Add an executable *.sh script to devops/scripts/hooks-src/.
  2. Run --install and opt in to the new hook.
  Each hook receives all relevant files as absolute paths; it should filter,
  fix in place, and return 0 (pass) or non-zero (fail).
  Source devops/scripts/hooks-src/common.sh for shared helpers.
EOF
}

ALL_FILES=false
INSTALL=false

for arg in "$@"; do
    case "$arg" in
        --all-files) ALL_FILES=true ;;
        --install)   INSTALL=true ;;
        --help)      usage; exit 0 ;;
    esac
done

# ---------------------------------------------------------------------------
# Install mode: symlink selected hooks into hooks/, then wire .git/hooks/
# ---------------------------------------------------------------------------
if $INSTALL; then
    echo "Select hooks to activate (Enter = keep current state):"
    for src in "$HOOKS_SRC_DIR"/*.sh; do
        name="$(basename "$src" .sh)"
        if [[ "$name" == "common" ]] ; then
                ln -sf "$(realpath --relative-to="$HOOKS_DIR" "$src")" "$dest"
        fi

        dest="$HOOKS_DIR/$name.sh"
        if [[ -L "$dest" ]]; then
            prompt="Y/n"; currently=true
        else
            prompt="y/N"; currently=false
        fi

        printf '  Enable %-24s [%s] ' "$name" "$prompt"
        read -r answer
        case "${answer:-}" in
            [Nn]*)
                rm -f "$dest"
                echo "    -> disabled"
                ;;
            [Yy]*)
                ln -sf "$(realpath --relative-to="$HOOKS_DIR" "$src")" "$dest"
                echo "    -> enabled"
                ;;
            *)  # keep current state
                $currently && echo "    -> enabled (unchanged)" \
                           || echo "    -> disabled (unchanged)"
                ;;
        esac
    done

    readonly HOOK_PATH="$REPO_ROOT/.git/hooks/pre-commit"
    # Relative path so the symlink survives moves of the working tree.
    readonly SCRIPT_REL="$(realpath --relative-to="$REPO_ROOT/.git/hooks" \
                            "$SCRIPT_DIR/run-hooks.sh")"
    ln -sf "$SCRIPT_REL" "$HOOK_PATH"
    echo "Installed as $HOOK_PATH -> $SCRIPT_REL"
    exit 0
fi

# ---------------------------------------------------------------------------
# Collect files to pass to each hook
# ---------------------------------------------------------------------------
if $ALL_FILES; then
    mapfile -t _RAW < <(git -C "$REPO_ROOT" ls-files)
else
    # Only files staged for this commit
    mapfile -t _RAW < <(git -C "$REPO_ROOT" diff --cached --name-only \
                                                   --diff-filter=ACMR)
fi

FILES=()
for f in "${_RAW[@]+"${_RAW[@]}"}"; do
    [[ -f "$REPO_ROOT/$f" ]] && FILES+=("$REPO_ROOT/$f")
done

# ---------------------------------------------------------------------------
# Run every executable *.sh in hooks/ (common.sh is not executable)
# ---------------------------------------------------------------------------
OVERALL_STATUS=0

for hook in "$HOOKS_DIR"/*.sh; do
    [[ -x "$hook" ]] || continue

    hook_name="$(basename "$hook" .sh)"
    [[ "$hook_name" == "common" ]] && continue

    printf '\n==> %s\n' "$hook_name"

    if "$hook" "${FILES[@]+"${FILES[@]}"}"; then
        printf "${hook_name} OK\n"
    else
        printf "${hook_name}    FAILED\n"
        OVERALL_STATUS=1
    fi
done

if [[ $OVERALL_STATUS -ne 0 ]]; then
    printf '\nOne or more hooks failed.\n'
fi
exit $OVERALL_STATUS
