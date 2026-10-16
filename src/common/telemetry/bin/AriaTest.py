#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

"""Compare the existing SDK worker with replacement test No. 68.

From ~/azure-osconfig/build, after building this branch with the NonProd key:

sudo --preserve-env=https_proxy,HTTPS_PROXY,all_proxy,ALL_PROXY,no_proxy,NO_PROXY \
    python3 ../src/common/telemetry/bin/AriaTest.py --send-status-trace-100 \
    --worker ./adapters/mc/telemetry-external-prefix/src/telemetry-external-build/bin/telemetrybin

Pass the actual worker path if your build layout differs. The Version property
comes from this checkout's generated commonutils/version.h; use a worker built
from the same checkout. The worker uses its COMPILED key, not a runtime key
override. Confirm NonProd configuration before running.

Only synthetic JSONL is created, in a private temporary directory. The existing
worker consumes it with the production caller's 8-second teardown allowance.
The harness waits at most 10 seconds, then kills and reaps that worker. It does
not call the SDK itself, retry events, or modify the worker or its configuration.
The SDK's existing shared cache and possible later replay remain in effect.
Avoid concurrent SDK uploader runs when comparing counts.

Worker exit zero is NOT collector acceptance: the existing executable ignores
ProcessJsonFile's result and does not expose per-event delivery counts. Compare
Power BI/Kusto rows using CorrelationId and distinct LineNumber (1 through 100);
also inspect the worker log at /var/log/osconfig_telemetry.log. SDK batching,
enrichment and persistence differ from the replacement's standalone transport
test, so this compares the existing worker path, not just the two transports.
"""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import uuid


TEST_COUNT = 100
TEST_RESULT = "731001"
TEST_MARKER = "*** Real 1DS SDK test No. 69 (old) ***"
TEARDOWN_SECONDS = 8
WORKER_TIMEOUT_SECONDS = 10
VERSION_HEADER = Path(__file__).resolve().parents[2] / "commonutils" / "version.h"


def read_version():
    text = VERSION_HEADER.read_text(encoding="utf-8")
    match = re.search(r'^\s*#define\s+OSCONFIG_VERSION\s+"([^"]+)"\s*$', text, re.MULTILINE)
    if not match or "@" in match.group(1):
        raise ValueError("Missing generated OSCONFIG_VERSION; configure this checkout first")
    return match.group(1)


def make_event(correlation, version, sequence):
    # Match No. 68's property values, including its synthetic source/function names.
    return {
        "EventName": "StatusTrace",
        "DistroName": "Synthetic Linux test",
        "CorrelationId": correlation,
        "Version": version,
        "Timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S+0000"),
        "FileName": "AriaTest.c",
        "LineNumber": str(sequence),
        "ScenarioName": "DistilledTelemetryTest",
        "FunctionName": "TelemetryAriaTest",
        "RuleCodename": "DistilledTelemetryTest",
        "CallingFunctionName": "TelemetryTransportSend",
        "Microseconds": "0",
        "ResultCode": TEST_RESULT,
        "ResultString": TEST_MARKER,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--send-status-trace-100", action="store_true", required=True,
                        help="Explicitly authorize 100 synthetic events through the existing worker")
    parser.add_argument("--worker", type=Path, required=True,
                        help="Path to the existing SDK telemetrybin/OSConfigTelemetry executable")
    args = parser.parse_args()

    correlation = str(uuid.uuid4())
    generated = 0
    worker_exit = "not-started"
    result = 1
    started = time.monotonic()
    try:
        worker = args.worker.resolve(strict=True)
        if not worker.is_file() or not os.access(str(worker), os.X_OK):
            raise ValueError("Worker path must identify an executable file")
        version = read_version()
        with tempfile.TemporaryDirectory(prefix="osconfig-aria-test-") as directory:
            # NamedTemporaryFile creates a mode-0600 file; close it before invoking the worker.
            with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", prefix="events-",
                                             suffix=".jsonl", dir=directory, delete=False) as events:
                for sequence in range(1, TEST_COUNT + 1):
                    events.write(json.dumps(make_event(correlation, version, sequence)) + "\n")
                    generated += 1
                event_path = events.name

            print("LIVE existing-worker StatusTrace test: requested={} ResultCode={}\n"
                  "ResultString={}\nCorrelationId={}\nVersion={}\nWorker={}\n"
                  "StartedUtc={}\nTeardownSeconds={} WorkerTimeoutSeconds={}\n"
                  "Uses worker's compiled key and existing SDK cache; delivery counts are unavailable."
                  .format(TEST_COUNT, TEST_RESULT, TEST_MARKER, correlation, version, worker,
                          datetime.now(timezone.utc).isoformat(), TEARDOWN_SECONDS,
                          WORKER_TIMEOUT_SECONDS), flush=True)

            command = [str(worker), "-f", event_path, "-t", str(TEARDOWN_SECONDS)]
            with subprocess.Popen(command) as process:
                try:
                    worker_exit = process.wait(timeout=WORKER_TIMEOUT_SECONDS)
                    result = 0 if worker_exit == 0 else 1
                    if worker_exit != 0:
                        print("ERROR: worker exited with status {}".format(worker_exit), file=sys.stderr)
                except subprocess.TimeoutExpired:
                    worker_exit = "timeout"
                    result = 124
                    print("ERROR: worker exceeded the {}-second timeout".format(WORKER_TIMEOUT_SECONDS),
                          file=sys.stderr)
                finally:
                    if process.poll() is None:
                        process.kill()
                    process.wait()
    except (OSError, ValueError) as error:
        result = 1
        print("ERROR: {}".format(error), file=sys.stderr)
    except KeyboardInterrupt:
        result = 130
        print("ERROR: test interrupted", file=sys.stderr)
    finally:
        print("requested={} generated={} worker_exit={} accepted=unknown rejected=unknown "
              "unconfirmed=unknown status={} elapsed_seconds={:.3f}\n"
              "CorrelationId={}\nFinishedUtc={}\n"
              "Worker completion is not proof of collector acceptance or downstream visibility."
              .format(TEST_COUNT, generated, worker_exit, result, time.monotonic() - started,
                      correlation, datetime.now(timezone.utc).isoformat()), flush=True)
    return result


if __name__ == "__main__":
    sys.exit(main())
