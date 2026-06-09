#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
"""Pre-commit hook: verify Microsoft copyright header.

Supports two comment styles:
  --style=slash   (default)  // Copyright ...  for C/C++/H files
  --style=hash               # Copyright ...   for shell/CMake/Python files
"""

import sys

COPYRIGHT_TEMPLATE = [
    "{prefix} Copyright (c) Microsoft Corporation.",
    "{prefix} Licensed under the MIT License.",
]
MAX_LINES = 100


def copyright_lines(prefix):
    return [t.format(prefix=prefix) for t in COPYRIGHT_TEMPLATE]


def check_file(path, prefix):
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            head = [fh.readline().rstrip("\n") for _ in range(MAX_LINES)]
    except OSError as exc:
        print(f"ERROR: cannot read {path}: {exc}", file=sys.stderr)
        return False

    for required in copyright_lines(prefix):
        if not any(required in line for line in head):
            print(f"{path}: missing copyright header line: {required!r}")
            return False
    return True


def main():
    args = sys.argv[1:]
    prefix = "//"
    if args and args[0] == "--style=hash":
        prefix = "#"
        args = args[1:]
    elif args and args[0] == "--style=slash":
        args = args[1:]

    failures = [f for f in args if not check_file(f, prefix)]
    if failures:
        print(
            f"\n{len(failures)} file(s) are missing the Microsoft copyright header.",
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
