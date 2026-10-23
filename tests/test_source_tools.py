"""Check false-success prevention and the documented performance acceptance boundaries."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import runpy
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


class CatchReportTests(unittest.TestCase):
    def run_report(self, content, returncode=0, stale=False):
        spec = importlib.util.spec_from_file_location("catch_runner", ROOT / "overlay/tests/RunCatch2.py")
        module = importlib.util.module_from_spec(spec)
        with patch.object(sys, "dont_write_bytecode", True):
            spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "result.xml"
            if stale:
                report.write_text('<testsuites><testsuite tests="1" errors="0" failures="0"/></testsuites>')

            def launch(*args, **kwargs):
                if content is not None:
                    report.write_text(content)
                return SimpleNamespace(returncode=returncode)

            with patch.object(sys, "argv", ["runner", "fake.exe", str(report)]), patch.object(module.subprocess, "run", side_effect=launch), contextlib.redirect_stdout(io.StringIO()):
                module.main()

    def test_real_success(self):
        self.run_report('<testsuites><testsuite tests="4" errors="0" failures="0"/></testsuites>')

    def test_failed_assertion_with_zero_exit_fails(self):
        with self.assertRaises(SystemExit):
            self.run_report('<testsuites><testsuite tests="4" errors="0" failures="1"/></testsuites>')

    def test_stale_success_cannot_hide_renderer_startup_failure(self):
        with self.assertRaises(SystemExit):
            self.run_report(None, stale=True)

    def test_empty_suite_fails(self):
        with self.assertRaises(SystemExit):
            self.run_report('<testsuites><testsuite tests="0" errors="0" failures="0"/></testsuites>')

    def test_native_failure_code_is_retained(self):
        with self.assertRaises(SystemExit) as failure:
            self.run_report(None, returncode=7)
        self.assertEqual(failure.exception.code, 7)


class PerformanceGateTests(unittest.TestCase):
    def gate(self, old, new):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            before, after = [], []
            for label, value, paths in (("before", old, before), ("after", new, after)):
                for i in range(3):
                    path = folder / f"{label}-{i}.json"
                    path.write_text(json.dumps([{"vertical": 0, "mode": "test", "mean_ms": value, "p99_ms": value, "over_8_333ms": 0}]))
                    paths.append(str(path))
            output = folder / "result.json"
            argv = ["gate", "--baseline", *before, "--candidate", *after, "--output", str(output)]
            with patch.object(sys, "argv", argv), contextlib.redirect_stdout(io.StringIO()):
                try:
                    runpy.run_path(str(ROOT / "overlay/scripts/CompareFramePerformance.py"), run_name="__main__")
                except SystemExit as failure:
                    self.assertEqual(failure.code, 1)
            return json.loads(output.read_text())["status"]

    def test_above_five_percent_but_below_point_one_ms_passes(self):
        self.assertEqual(self.gate(1, 1.09), "PASS")

    def test_above_point_one_ms_but_below_five_percent_passes(self):
        self.assertEqual(self.gate(10, 10.4), "PASS")

    def test_exact_five_percent_is_not_an_excess(self):
        self.assertEqual(self.gate(10, 10.5), "PASS")

    def test_exact_point_one_ms_is_not_an_excess(self):
        self.assertEqual(self.gate(1, 1.1), "PASS")

    def test_exceeding_both_limits_fails(self):
        self.assertEqual(self.gate(10, 10.6), "FAIL")


if __name__ == "__main__":
    unittest.main()
