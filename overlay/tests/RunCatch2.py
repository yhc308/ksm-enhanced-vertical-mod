"""Run the Siv3D Catch2 entry point and fail on missing or failed JUnit results."""
import argparse
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("report", type=Path)
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    report = args.report.resolve()
    report.parent.mkdir(parents=True, exist_ok=True)
    report.unlink(missing_ok=True)
    env = dict(os.environ, KSM_TEST_REPORT=str(report))
    result = subprocess.run([str(args.executable.resolve())], env=env, timeout=args.timeout)
    if result.returncode:
        raise SystemExit(result.returncode)
    if not report.is_file():
        raise SystemExit("Missing Catch2 JUnit report: " + str(report))
    tree = ET.parse(report)
    suites = list(tree.iter("testsuite"))
    if not suites or sum(int(s.get("tests", "0")) for s in suites) == 0:
        raise SystemExit("Catch2 reported no tests")
    if any(int(s.get("failures", "0")) or int(s.get("errors", "0")) for s in suites):
        raise SystemExit("Catch2 reported failures: " + str(report))
    print("Catch2 PASS: " + str(report))


if __name__ == "__main__":
    main()
