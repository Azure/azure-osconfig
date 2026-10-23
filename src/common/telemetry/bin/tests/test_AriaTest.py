# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SPEC = importlib.util.spec_from_file_location("AriaTest", str(Path(__file__).resolve().parents[1] / "AriaTest.py"))
aria = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(aria)
REAL_POPEN = subprocess.Popen


class Clock:
    def __init__(self):
        self.now = 0

    def monotonic(self):
        return self.now

    def sleep(self, seconds):
        self.now += seconds


class AriaSessionTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.output = self.root / "evidence"
        self.output.mkdir()
        self.clock = Clock()
        self.children = []
        self.commands = []
        self.cache = self.root / "cache"
        self.patches = [
            mock.patch.object(aria, "time", self.clock),
            mock.patch.object(aria, "CACHE_DIRECTORY", self.cache),
            mock.patch.object(aria, "APPLICATION_LOGS", ()),
        ]
        for patch in self.patches:
            patch.start()
            self.addCleanup(patch.stop)
        self.addCleanup(self.temporary.cleanup)
        self.addCleanup(self.reap_children)

    def reap_children(self):
        for child in self.children:
            if child.poll() is None:
                child.kill()
            child.wait()

    def worker_factory(self, code, elapsed=0):
        def launch(command, **kwargs):
            self.commands.append(command)
            child = REAL_POPEN([sys.executable, "-c", code, command[2]], **kwargs)
            self.children.append(child)
            self.clock.now += elapsed
            return child
        return launch

    def run_sessions(self, count, factory):
        summary = {"experiment_id": "offline-test", "sessions": []}
        with mock.patch.object(aria.subprocess, "Popen", side_effect=factory):
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
                aria.run_sessions(Path("SDK-worker"), count, self.output, "test-version", summary)
        return summary

    def test_sessions_generate_valid_numbered_events_on_minute_grid_and_preserve_cache(self):
        self.cache.mkdir()
        cached = self.cache / "cache.db"
        cached.write_bytes(b"previous session pending data")
        summary = self.run_sessions(4, self.worker_factory("import os, sys; os.unlink(sys.argv[1])"))
        sessions = summary["sessions"]
        self.assertEqual([0, 60, 120, 180], [run["actual_start_offset_seconds"] for run in sessions])
        self.assertEqual([2, 4, 20, 40], [run["producer_seconds"] for run in sessions])
        self.assertEqual(4, len(self.children))
        self.assertTrue(all(child.poll() == 0 for child in self.children))
        self.assertEqual(b"previous session pending data", cached.read_bytes())
        schema = (Path(__file__).resolve().parents[2] / "lib" / "ParameterSets.hpp").read_text()
        required = {"EventName"}
        for symbol in ("COMMON_REQUIRED_PARAMS", "STATUS_TRACE_REQUIRED_PARAMS"):
            body = re.search(r"\b" + symbol + r"\s*=\s*\{(.*?)\};", schema, re.S).group(1)
            required.update(re.findall(r'"([^"]+)"', body))
        all_names = set()
        for number, run in enumerate(sessions, 1):
            directory = self.output / "run-{:04d}".format(number)
            lines = (directory / "events.original.jsonl").read_text().splitlines()
            self.assertEqual(100, len(lines))
            self.assertEqual(100, run["generated_events"])
            self.assertEqual(100, run["submitted_records"])
            self.assertIsNone(run["accepted"])
            self.assertEqual(0, run["status"])
            for sequence, line in enumerate(lines, 1):
                record = json.loads(line)
                self.assertEqual(required, set(record))
                self.assertEqual("StatusTrace", record["EventName"])
                self.assertEqual(run["correlation_id"], record["CorrelationId"])
                self.assertTrue(record["ResultString"].endswith(
                    "(event {} of 100 for {} run))".format(sequence, aria.ordinal(number))))
                all_names.add(record["ScenarioName"])
            self.assertEqual(["-f", str(self.output / "events.jsonl"), "-t", "8"],
                             self.commands[number - 1][1:])
        self.assertEqual(400, len(all_names))
        self.assertEqual(4, len({run["correlation_id"] for run in sessions}))
        self.assertEqual(220, self.clock.now)
        for run, suffix in ((11, "11th"), (12, "12th"), (13, "13th"), (21, "21st")):
            self.assertTrue(aria.make_event("experiment", "correlation", "version", run, 1)["ResultString"]
                            .endswith("for {} run))".format(suffix)))

    def test_missing_cache_stays_missing_and_overruns_skip_boundaries(self):
        summary = self.run_sessions(2, self.worker_factory(
            "import os, sys; os.unlink(sys.argv[1])", elapsed=70))
        self.assertFalse(self.cache.exists())
        self.assertEqual([0, 120], [run["scheduled_start_offset_seconds"] for run in summary["sessions"]])
        self.assertEqual([0, 1], [run["skipped_minute_boundaries"] for run in summary["sessions"]])
        self.assertFalse(summary["sessions"][0]["cache_before"][str(self.cache)]["exists"])

    def test_nonzero_exit_is_recorded_and_next_session_still_uses_a_new_process(self):
        summary = self.run_sessions(2, self.worker_factory("import sys; sys.exit(7)"))
        self.assertEqual([7, 7], [run["worker_exit"] for run in summary["sessions"]])
        self.assertEqual([1, 1], [run["status"] for run in summary["sessions"]])
        self.assertEqual([100, 200], [run["submitted_records"] for run in summary["sessions"]])
        self.assertEqual(2, len(self.children))
        self.assertEqual(200, len((self.output / "events.jsonl").read_text().splitlines()))
        persisted = json.loads((self.output / "summary.json").read_text())
        self.assertEqual(7, persisted["sessions"][1]["worker_exit"])

    def test_timeout_kills_and_reaps_worker_and_records_failure(self):
        with mock.patch.object(aria, "WORKER_TIMEOUT_SECONDS", 0.15):
            summary = self.run_sessions(1, self.worker_factory("import time; time.sleep(60)"))
        result = summary["sessions"][0]
        self.assertTrue(result["timed_out"])
        self.assertEqual(124, result["status"])
        self.assertIsNotNone(self.children[0].poll())
        self.assertNotEqual(0, result["worker_exit"])
        self.assertEqual(result["worker_exit"],
                         json.loads((self.output / "run-0001" / "session.json").read_text())["worker_exit"])

    def test_interruption_stops_worker_and_preserves_partial_result(self):
        factory = self.worker_factory("import time; time.sleep(60)")

        def interrupt(command, **kwargs):
            process = factory(command, **kwargs)
            wait = process.wait
            interrupted = [False]

            def wait_once(*args, **kwargs):
                if not interrupted[0]:
                    interrupted[0] = True
                    raise KeyboardInterrupt()
                return wait(*args, **kwargs)

            process.wait = wait_once
            return process

        with self.assertRaises(KeyboardInterrupt):
            self.run_sessions(1, interrupt)
        result = json.loads((self.output / "run-0001" / "session.json").read_text())
        self.assertEqual("interrupted", result["status"])
        self.assertEqual(100, result["generated_events"])
        self.assertIsNotNone(self.children[0].poll())

if __name__ == "__main__":
    unittest.main()
