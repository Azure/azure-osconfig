#!/usr/bin/env python3

import sys
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

    command = ["clang-tidy", "--fix"] + sys.argv[1:] + ["--"] + post_args
    subprocess.run(command)

if __name__ == '__main__':
    main()
