"""Export a prepared local source tree into the private patch/overlay repository.

The source tree must be based on the pinned upstream revision. Only mod deltas
are stored; generated runtime inputs, original song collections and binaries are
rejected. No commits, pushes or upstream repository modifications are performed.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", required=True, type=Path)
args = parser.parse_args()
source = args.source.resolve()
manifest_path = root / "SOURCE_MANIFEST.json"
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
base = manifest["baseCommit"]

def git(directory, *arguments):
    return subprocess.check_output(["git", "-c", "safe.directory=" + directory.as_posix(), "-C", str(directory), *arguments])

def sha(data):
    return hashlib.sha256(data).hexdigest()

# The dependency gitlinks remain upstream-pinned; local source changes are patches.
dep_manifest_path = source / "patches/dependencies/manifest.json"
dep_manifest = json.loads(dep_manifest_path.read_text(encoding="utf-8"))
allowed = {".cpp", ".hpp", ".h", ".c", ".cc", ".cxx", ".inl", ".txt", ".md", ".json", ".vcxproj", ".filters", ".props", ".cmake"}
for item in dep_manifest["dependencies"]:
    dependency = source / item["path"]
    assert git(dependency, "rev-parse", "HEAD").decode().strip() == item["baseCommit"]
    names = set(item["changedFiles"])
    names.update(git(dependency, "diff", "--name-only").decode().splitlines())
    names = sorted(name for name in names if Path(name).suffix.lower() in allowed)
    if names:
        git(dependency, "add", "--", *names)
        patch = git(dependency, "diff", "--cached", "--binary", "--full-index", item["baseCommit"], "--", *names)
        (source / item["patch"]).write_bytes(patch)
        item["sha256"] = sha(patch)
        item["changedFiles"] = git(dependency, "diff", "--cached", "--name-only", item["baseCommit"]).decode().splitlines()
dep_manifest_path.write_text(json.dumps(dep_manifest, indent=2) + "\n", encoding="utf-8")

git(source, "add", "-A")
parts = git(source, "diff", "--cached", "--name-status", "--no-renames", "-z", base).decode().split("\0")
added, patched = [], []
for i in range(0, len(parts) - 1, 2):
    status, name = parts[i:i+2]
    if name.startswith(("build/", "test-runs/", "play-test/", "reference/", "imgs/", "tests/fixtures/", "kshootmania/App/songs/")):
        raise SystemExit("Private/external runtime path cannot be exported: " + name)
    if Path(name).suffix.lower() in {".exe", ".dll", ".lib", ".pdb", ".obj", ".zip", ".ksc", ".psd", ".psb", ".mp4", ".log"}:
        raise SystemExit("Generated or private artifact cannot be exported: " + name)
    if status == "A":
        added.append(name)
    elif status in {"M", "D"}:
        # Do not publish checkout-only CRLF normalization of untouched upstream files.
        if status == "M":
            before = git(source, "show", base + ":" + name)
            after = git(source, "show", ":" + name)
            if before.replace(b"\r\n", b"\n") == after.replace(b"\r\n", b"\n"):
                continue
        patched.append(name)
    else:
        raise SystemExit("Unexpected source status: " + status + " " + name)
patch = git(source, "diff", "--cached", "--binary", "--full-index", base, "--", *patched)
(root / manifest["mainPatch"]).write_bytes(patch)
for name in {item["path"] for item in manifest["overlayFiles"]} - set(added):
    path = root / "overlay" / name
    assert path.resolve().is_relative_to((root / "overlay").resolve())
    path.unlink()
records = []
for name in added:
    data = git(source, "show", ":" + name)
    path = root / "overlay" / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    records.append({"path": name, "sha256": sha(data)})
manifest["mainPatchSha256"] = sha(patch)
manifest["overlayFiles"] = records
manifest["refactorBranch"] = "refactor/upstream-style-parity"
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"overlayFiles": len(added), "patchedFiles": len(patched), "patchBytes": len(patch)}))
