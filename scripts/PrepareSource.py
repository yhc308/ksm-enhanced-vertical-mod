"""Reconstruct the mod source without storing a complete upstream tree in this repository."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "SOURCE_MANIFEST.json").read_text())
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--destination", type=Path, default=root / "upstream-worktree")
parser.add_argument("--upstream", default=manifest["upstream"], help="Optional local mirror for offline reconstruction")
args = parser.parse_args()
dest = args.destination.resolve()
if dest.exists():
    raise SystemExit("Destination already exists; choose a new directory.")
patch = root / manifest["mainPatch"]
assert hashlib.sha256(patch.read_bytes()).hexdigest() == manifest["mainPatchSha256"]
for item in manifest["overlayFiles"]:
    source = root / "overlay" / item["path"]
    assert hashlib.sha256(source.read_bytes()).hexdigest() == item["sha256"], source
subprocess.run(["git", "clone", "--no-hardlinks", "--no-checkout", args.upstream, str(dest)], check=True)
def git(*parts):
    subprocess.run(["git", "-c", "safe.directory=" + dest.as_posix(), "-C", str(dest), *parts], check=True)
git("checkout", "--detach", manifest["baseCommit"])
git("apply", "--check", str(patch))
git("apply", str(patch))
for item in manifest["overlayFiles"]:
    output = dest / item["path"]
    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / "overlay" / item["path"], output)
git("submodule", "update", "--init", "--recursive")
subprocess.run([sys.executable, str(dest / "scripts/ApplyDependencyPatches.py")], check=True)
print("Source prepared at " + str(dest))
