"""Compare fixed-input timing-setting captures and replay traces against a saved baseline."""
import argparse
import csv
import json
from pathlib import Path

import numpy as np
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("baseline", type=Path, help="Saved judge-setting-tests.json")
parser.add_argument("candidate", type=Path, help="Candidate judge-setting-tests.json")
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
old = Path(json.loads(args.baseline.read_text(encoding="utf-8-sig"))["reference"])
new = Path(json.loads(args.candidate.read_text(encoding="utf-8-sig"))["reference"])
results = []
failed = []
for path in sorted(old.glob("*.png")):
    other = new / path.name
    if not other.exists():
        failed.append("Missing " + path.name)
        continue
    a = np.asarray(Image.open(path).convert("RGB")).astype(np.int16)
    b = np.asarray(Image.open(other).convert("RGB")).astype(np.int16)
    if a.shape != b.shape:
        failed.append("Dimensions changed: " + path.name)
        continue
    height, width = a.shape[:2]
    delta = np.abs(a - b).max(2)
    # This exact draw-only FPS rectangle comes from JudgeSettingOverlay::draw.
    # It is the only excluded area; text, geometry, effects and all other pixels must match.
    unit = width / 720 if height > width else height / 480
    x0, x1 = int(12 * unit), int(np.ceil(152 * unit))
    y0, y1 = int(height - 38 * unit), int(np.ceil(height - 18 * unit))
    delta[y0:y1, x0:x1] = 0
    count = int(np.count_nonzero(delta))
    results.append({"file": path.name, "changedPixelsOutsideFps": count})
    if count:
        failed.append("Rendered pixels changed: " + path.name)
traces = []
for path in sorted(old.glob("*-frames.csv")):
    other = new / path.name
    if not other.exists():
        failed.append("Missing " + path.name)
        continue
    with path.open(encoding="utf-8-sig", newline="") as stream:
        before = list(csv.DictReader(stream))
    with other.open(encoding="utf-8-sig", newline="") as stream:
        after = list(csv.DictReader(stream))
    equal = before == after
    traces.append({"file": path.name, "rows": len(after), "identical": equal})
    if not equal:
        failed.append("Replay trace changed: " + path.name)
if not results or not traces:
    failed.append("Baseline must include screenshots and replay traces")
report = {"status": "FAIL" if failed else "PASS", "baseline": str(old), "candidate": str(new),
          "excludedArea": "Only the runtime FPS label rectangle; all other pixels must match exactly",
          "screens": results, "traces": traces, "failures": failed}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(report["status"], len(results), "pixel comparisons,", len(traces), "replay comparisons")
if failed:
    raise SystemExit("\n".join(failed))
