"""Apply the mod's dependency changes to the upstream-pinned submodules."""
from pathlib import Path
import hashlib
import json
import subprocess

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / "patches/dependencies/manifest.json").read_text())
for item in manifest["dependencies"]:
    repo = root / item["path"]
    patch = root / item["patch"]
    assert hashlib.sha256(patch.read_bytes()).hexdigest() == item["sha256"], patch
    def git(*args, check=True):
        return subprocess.run(["git", "-c", "safe.directory=" + repo.as_posix(), "-C", str(repo), *args], check=check, capture_output=True, text=True)
    if git("rev-parse", "HEAD").stdout.strip() != item["baseCommit"]:
        raise SystemExit("Unexpected dependency revision: " + item["path"])
    if git("apply", "--reverse", "--check", str(patch), check=False).returncode == 0:
        print(item["path"] + ": already applied")
        continue
    result = git("apply", "--check", str(patch), check=False)
    if result.returncode:
        raise SystemExit(result.stderr)
    git("apply", str(patch))
    print(item["path"] + ": applied")
