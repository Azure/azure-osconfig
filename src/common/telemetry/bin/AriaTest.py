#!/usr/bin/env python3
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

r"""Explicit NonProd test of successive processes using the existing 1DS SDK worker.

From the Linux build directory, after building main with the OSConfig NonProd key:

sudo --preserve-env=https_proxy,HTTPS_PROXY,all_proxy,ALL_PROXY,no_proxy,NO_PROXY \
    python3 ../src/common/telemetry/bin/AriaTest.py --send-status-trace-sessions \
    --worker ./adapters/mc/telemetry-external-prefix/src/telemetry-external-build/bin/telemetrybin \
    --runs 8 --output-dir ./aria-session-evidence

Python 3.5+; standard library only. The generated commonutils/version.h must come
from the same checkout/build as the worker. The worker uses its COMPILED key;
setting OsConfigTelemetryApiKey at runtime does not redirect it to NonProd.
The explicit send flag acknowledges that the worker and existing cache are
NonProd-only. Use a dedicated lab without concurrent OSConfig uploaders.

This simulates the SO's event production, not the SO itself: 100 StatusTrace
records are appended and flushed over 2, 4, 20, then 40 seconds, repeating.
One private spool is reused, as in production: records the worker does not
remove remain for the next session, which appends its next 100 records.
Session starts use a monotonic 60-second grid. Each producer session is followed
by a fresh SDK worker process, with production's 8-second teardown allowance
and a 10-second outer timeout. Processes never overlap; overruns skip missed
minute boundaries instead of starting catch-up sessions. No sleep is inserted
inside the SDK worker and no extra flush/retry is added.

The harness NEVER creates, clears, or repairs /var/lib/osconfig/telemetry.
Its persistent cache is deliberately reused, including any pending events.
Thus a missing-cache experiment stays missing unless the worker creates it.
Use separate, deliberately prepared lab states for present/absent comparisons.

The fresh output directory retains original inputs, per-session stdout/stderr,
application-log snapshots, cache-file metadata, PIDs, timings, and exit status.
Interrupted/failed runs remain recorded. ResultString ends, for example, with
"(event 4 of 100 for 2nd run))". Each session has its own CorrelationId; unique
ScenarioName values include the experiment ID, run number, and event number.
Exit zero, input deletion, and an empty cache are NOT delivery acknowledgments.
Verify distinct expected ScenarioName values in Aria, including delayed arrivals
from earlier sessions. The final process exits without an additional drain run.

Offline harness checks from the build directory (no SDK or network calls):
python3 -B -m unittest discover -s ../src/common/telemetry/bin/tests -p test_AriaTest.py
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import uuid


EVENTS_PER_SESSION = 100
SESSION_INTERVAL_SECONDS = 60
PRODUCER_DURATIONS = (2, 4, 20, 40)
TEARDOWN_SECONDS = 8
WORKER_TIMEOUT_SECONDS = 10
TEST_MARKER = "OSConfig NonProd SDK session test"
VERSION_HEADER = Path(__file__).resolve().parents[2] / "commonutils" / "version.h"
CACHE_DIRECTORY = Path("/var/lib/osconfig/telemetry")
APPLICATION_LOGS = (
    Path("/var/log/osconfig_telemetry.log"),
    Path("/var/log/osconfig_telemetry.bak"),
)


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def write_json(path, value):
    temporary = path.with_suffix(".json.tmp")
    with temporary.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")
    os.replace(str(temporary), str(path))


def read_version():
    text = VERSION_HEADER.read_text(encoding="utf-8")
    match = re.search(r'^\s*#define\s+OSCONFIG_VERSION\s+"([^"]+)"\s*$', text, re.MULTILINE)
    if not match or "@" in match.group(1):
        raise ValueError("Missing generated OSCONFIG_VERSION; configure this checkout first")
    return match.group(1)


def file_state(path):
    try:
        stat = path.stat()
    except FileNotFoundError:
        return {"exists": False}
    return {"exists": True, "bytes": stat.st_size, "modified_time": stat.st_mtime}


def cache_state():
    return {
        str(path): file_state(path)
        for path in (
            CACHE_DIRECTORY,
            CACHE_DIRECTORY / "cache.db",
            CACHE_DIRECTORY / "cache.db-wal",
            CACHE_DIRECTORY / "cache.db-shm",
        )
    }


def ordinal(number):
    suffix = "th" if 10 <= number % 100 <= 20 else {1: "st", 2: "nd", 3: "rd"}.get(number % 10, "th")
    return "{}{}".format(number, suffix)


def make_event(experiment, correlation, version, run, sequence):
    return {
        "EventName": "StatusTrace",
        "DistroName": "Synthetic Linux session test",
        "CorrelationId": correlation,
        "Version": version,
        "Timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S+0000"),
        "FileName": "AriaTest.py",
        "LineNumber": str(sequence),
        "ScenarioName": "{}-run{:04d}-event{:04d}".format(experiment, run, sequence),
        "FunctionName": "SimulatedOsConfigSession",
        "RuleCodename": "NonProdSdkSession{:04d}".format(run),
        "CallingFunctionName": "AriaTest",
        "Microseconds": "0",
        "ResultCode": "731001",
        "ResultString": "{} (event {} of {} for {} run))".format(
            TEST_MARKER, sequence, EVENTS_PER_SESSION, ordinal(run)),
    }


def sleep_until(deadline):
    remaining = deadline - time.monotonic()
    while remaining > 0:
        time.sleep(remaining)
        remaining = deadline - time.monotonic()


def produce_events(path, original, result, experiment, version):
    start = time.monotonic()
    with path.open("a", encoding="utf-8", newline="\n") as stream:
        for sequence in range(1, EVENTS_PER_SESSION + 1):
            sleep_until(start + result["producer_seconds"] * (sequence - 1) / (EVENTS_PER_SESSION - 1))
            event = make_event(experiment, result["correlation_id"], version, result["run"], sequence)
            stream.write(json.dumps(event, separators=(",", ":")) + "\n")
            stream.flush()
            result["generated_events"] = sequence
    payload = path.read_bytes()
    original.write_bytes(payload)
    result["submitted_records"] = len(payload.splitlines())
    result["input_sha256"] = hashlib.sha256(payload).hexdigest()


def invoke_worker(worker, events, directory, result):
    command = [str(worker), "-f", str(events), "-t", str(TEARDOWN_SECONDS)]
    result["worker_command"] = command
    result["worker_started_utc"] = utc_now()
    result["timed_out"] = False
    with (directory / "stdout.log").open("xb") as stdout, (directory / "stderr.log").open("xb") as stderr:
        with subprocess.Popen(command, stdout=stdout, stderr=stderr) as process:
            result["worker_pid"] = process.pid
            try:
                process.wait(timeout=WORKER_TIMEOUT_SECONDS)
            except subprocess.TimeoutExpired:
                result["timed_out"] = True
                print("ERROR: run {} worker exceeded {} seconds".format(
                    result["run"], WORKER_TIMEOUT_SECONDS), file=sys.stderr, flush=True)
            finally:
                if process.poll() is None:
                    process.kill()
                result["worker_exit"] = process.wait()
                result["worker_finished_utc"] = utc_now()
    result["status"] = 124 if result["timed_out"] else (0 if result["worker_exit"] == 0 else 1)
    if result["status"] != 0 and not result["timed_out"]:
        print("ERROR: run {} worker exited with {}".format(
            result["run"], result["worker_exit"]), file=sys.stderr, flush=True)


def run_sessions(worker, runs, output, version, summary):
    epoch = time.monotonic()
    deadline = epoch
    for run in range(1, runs + 1):
        skipped = 0
        while deadline < time.monotonic() and run > 1:
            deadline += SESSION_INTERVAL_SECONDS
            skipped += 1
        sleep_until(deadline)
        directory = output / "run-{:04d}".format(run)
        directory.mkdir(mode=0o700)
        events = output / "events.jsonl"
        result = {
            "run": run,
            "correlation_id": str(uuid.uuid4()),
            "scheduled_start_offset_seconds": deadline - epoch,
            "actual_start_offset_seconds": time.monotonic() - epoch,
            "skipped_minute_boundaries": skipped,
            "started_utc": utc_now(),
            "producer_seconds": PRODUCER_DURATIONS[(run - 1) % len(PRODUCER_DURATIONS)],
            "requested_events": EVENTS_PER_SESSION,
            "generated_events": 0,
            "status": "in-progress",
            "worker_exit": None,
            "accepted": None,
            "rejected": None,
        }
        summary["sessions"].append(result)
        write_json(output / "summary.json", summary)
        print("Run {}: CorrelationId={} producer={}s scheduled_offset={}s".format(
            run, result["correlation_id"], result["producer_seconds"],
            result["scheduled_start_offset_seconds"]), flush=True)
        try:
            result["cache_before"] = cache_state()
            result["spool_before"] = file_state(events)
            produce_events(events, directory / "events.original.jsonl",
                           result, summary["experiment_id"], version)
            invoke_worker(worker, events, directory, result)
        except (OSError, ValueError, KeyboardInterrupt) as error:
            result["status"] = "interrupted" if isinstance(error, KeyboardInterrupt) else "harness-error"
            result["error"] = "{}: {}".format(type(error).__name__, error)
            raise
        finally:
            result["finished_utc"] = utc_now()
            result["input_remains"] = events.exists()
            write_json(directory / "session.json", result)
            write_json(output / "summary.json", summary)
        result["cache_after"] = cache_state()
        for log in APPLICATION_LOGS:
            if log.exists():
                shutil.copy2(str(log), str(directory / log.name))
        write_json(directory / "session.json", result)
        write_json(output / "summary.json", summary)
        print("Run {}: generated={} worker_exit={} status={} accepted=unknown".format(
            run, result["generated_events"], result["worker_exit"], result["status"]), flush=True)
        deadline += SESSION_INTERVAL_SECONDS


def positive_count(value):
    count = int(value)
    if count <= 0:
        raise argparse.ArgumentTypeError("Run count must be positive")
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--send-status-trace-sessions", action="store_true", required=True,
                        help="Authorize live sending with a NonProd-built worker and NonProd-only lab cache")
    parser.add_argument("--worker", type=Path, required=True)
    parser.add_argument("--runs", type=positive_count, required=True)
    parser.add_argument("--output-dir", type=Path, required=True,
                        help="New evidence directory; existing directories are never overwritten")
    args = parser.parse_args()
    status = 1
    output = None
    summary = None
    try:
        if not sys.platform.startswith("linux"):
            raise ValueError("Live sending requires a dedicated Linux NonProd lab")
        worker = args.worker.resolve()
        if not worker.is_file() or not os.access(str(worker), os.X_OK):
            raise ValueError("Worker must be an existing executable built with the NonProd key")
        version = read_version()
        destination = Path(os.path.realpath(str(args.output_dir)))
        cache_root = Path(os.path.realpath(str(CACHE_DIRECTORY)))
        if destination == cache_root or cache_root in destination.parents:
            raise ValueError("Evidence directory must not create or modify the SDK cache directory")
        destination.mkdir(mode=0o700, parents=True)
        output = destination
        summary = {
            "experiment_id": "OSConfigNonProdSDKSession-" + str(uuid.uuid4()),
            "started_utc": utc_now(),
            "worker": str(worker),
            "worker_sha256": hashlib.sha256(worker.read_bytes()).hexdigest(),
            "version": version,
            "requested_sessions": args.runs,
            "events_per_session": EVENTS_PER_SESSION,
            "interval_seconds": SESSION_INTERVAL_SECONDS,
            "producer_durations_seconds": PRODUCER_DURATIONS,
            "teardown_seconds": TEARDOWN_SECONDS,
            "worker_timeout_seconds": WORKER_TIMEOUT_SECONDS,
            "destination": "Caller-confirmed compiled NonProd key; no runtime override or automatic key verification",
            "delivery_counts": "Unknown; verify distinct ScenarioName values in Aria",
            "sessions": [],
        }
        print("LIVE NonProd SDK session test: {}. Existing cache is NOT modified by the harness.".format(
            summary["experiment_id"]), flush=True)
        run_sessions(worker, args.runs, output, version, summary)
        status = 0 if all(run["status"] == 0 for run in summary["sessions"]) else 1
    except (OSError, ValueError) as error:
        if summary is not None:
            summary["error"] = "{}: {}".format(type(error).__name__, error)
        print("ERROR: {}".format(error), file=sys.stderr, flush=True)
    except KeyboardInterrupt:
        status = 130
        if summary is not None:
            summary["error"] = "KeyboardInterrupt"
        print("ERROR: interrupted; active child stopped and evidence retained", file=sys.stderr, flush=True)
    finally:
        if summary is not None:
            summary["finished_utc"] = utc_now()
            summary["harness_exit"] = status
            write_json(output / "summary.json", summary)
            print("Evidence: {}. Process completion is not proof of delivery.".format(output), flush=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
