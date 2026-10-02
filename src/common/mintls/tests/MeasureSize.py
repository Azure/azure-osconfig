# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

"""Compare two owner-built workers without changing the original executables."""

import argparse
import os
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("system_only", help="Worker built with OSCONFIG_TELEMETRY_MINTLS=OFF")
    parser.add_argument("with_mintls", help="Otherwise identical worker built with fallback ON")
    parser.add_argument("--strip", default="strip", help="Target architecture's strip executable")
    args = parser.parse_args()
    sizes = []
    with tempfile.TemporaryDirectory(prefix="osconfig-mintls-size-") as directory:
        for index, source in enumerate((args.system_only, args.with_mintls)):
            target = os.path.join(directory, str(index))
            subprocess.check_call([args.strip, "--strip-all", "-o", target, source])
            sizes.append(os.path.getsize(target))
    delta = sizes[1] - sizes[0]
    print("System-only stripped worker: {} bytes".format(sizes[0]))
    print("With mintls stripped worker: {} bytes".format(sizes[1]))
    print("Added stripped file size: {} bytes (target <= 300000)".format(delta))
    if delta <= 0:
        raise SystemExit("Invalid comparison: fallback did not increase the stripped worker size")
    if delta > 300000:
        raise SystemExit("mintls exceeds the 300 KB added stripped-file target")


if __name__ == "__main__":
    main()
