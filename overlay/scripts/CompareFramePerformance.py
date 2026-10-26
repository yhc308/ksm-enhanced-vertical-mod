"""Compare three real-clock benchmark repetitions using the agreed regression gate."""
import argparse
import json
import math
from pathlib import Path
from statistics import median

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--baseline", type=Path, nargs=3, required=True)
parser.add_argument("--candidate", type=Path, nargs=3, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()

# Below measurement precision; keeps an exact written threshold from failing on float subtraction.
ROUNDING_TOLERANCE_MS = 1e-9


def load(paths):
    runs = []
    for path in paths:
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        cases = (data["cases"] if "cases" in data else [data]) if isinstance(data, dict) else data
        by_key = {}
        for case in cases:
            key = (f'{case["width"]}x{case["height"]}' if "width" in case else str(case["vertical"]), str(case["mode"]))
            if key in by_key:
                raise SystemExit("Duplicate benchmark case: " + str(key))
            for metric in ("mean_ms", "p99_ms"):
                value = float(case[metric])
                if not math.isfinite(value) or value <= 0:
                    raise SystemExit("Invalid benchmark metric: " + str(path) + " " + metric)
            by_key[key] = case
        if not by_key:
            raise SystemExit("No benchmark cases: " + str(path))
        runs.append(by_key)
    if any(set(run) != set(runs[0]) for run in runs):
        raise SystemExit("Incomplete benchmark repetition")
    return runs


before, after = load(args.baseline), load(args.candidate)
if set(before[0]) != set(after[0]):
    raise SystemExit("Baseline and candidate cases differ")
rows = []
for key in sorted(before[0]):
    row = {"case": list(key)}
    for metric in ("mean_ms", "p99_ms"):
        old = median(float(run[key][metric]) for run in before)
        new = median(float(run[key][metric]) for run in after)
        delta = new - old
        row[metric] = {"baselineMedian": old, "candidateMedian": new, "deltaMs": delta,
                       "deltaPercent": 100 * delta / old, "regressed": delta > .1 + ROUNDING_TOLERANCE_MS and new > old * 1.05 + ROUNDING_TOLERANCE_MS}
    row["longFramesBaselineMedian"] = median(int(run[key]["over_8_333ms"]) for run in before)
    row["longFramesCandidateMedian"] = median(int(run[key]["over_8_333ms"]) for run in after)
    rows.append(row)
failed = any(row[m]["regressed"] for row in rows for m in ("mean_ms", "p99_ms"))
report = {"status": "FAIL" if failed else "PASS", "repetitions": 3,
          "aggregation": "Per-case median of three real-clock measurements",
          "gate": "Regress only when the increase exceeds both 5 percent and 0.1 ms",
          "cases": rows}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(report["status"], len(rows), "performance comparisons")
if failed:
    raise SystemExit(1)
