#!/usr/bin/env python3

import sys, os
import subprocess
import platform

def main():
    if platform.system().lower() == "windows":
        print("This script is not supported on Windows.")
        sys.exit(0)

    post_args = [
        "-xc++",
        "-std=c++11",
        "-Isrc/common/logging/",
        "-Isrc/common/parson/",
        "-Isrc/modules/inc",
        "-Isrc/common/commonutils",
        "-Isrc/modules/compliance/src/lib",
    ]


    prefix = os.environ.get('CLANG_PREFIX') or ''
    if prefix and prefix[-1] != os.sep:
        prefix = prefix + os.sep
    command = [prefix  + "clang-tidy", "--fix"] + sys.argv[1:] + ["--"] + post_args
    print(' '.join(c for c in command))
    subprocess.run(command)

if __name__ == '__main__':
    main()
