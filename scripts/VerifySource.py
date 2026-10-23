"""Validate the mod-only source package without an SDK or private runtime."""
from pathlib import Path
import argparse
import ast
import hashlib
import json


def validate_repository(root):
    root = Path(root).resolve()
    manifest = json.loads((root / "SOURCE_MANIFEST.json").read_text(encoding="utf-8"))
    entries = [(manifest["mainPatch"], manifest["mainPatchSha256"])]
    entries += [("overlay/" + row["path"], row["sha256"]) for row in manifest["overlayFiles"]]
    seen = set()
    for relative, expected in entries:
        path = (root / relative).resolve()
        if not path.is_relative_to(root) or Path(relative).is_absolute() or ".." in Path(relative).parts:
            raise ValueError("Source path escapes repository: " + relative)
        if relative.casefold() in seen:
            raise ValueError("Duplicate source path: " + relative)
        seen.add(relative.casefold())
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError("Source hash mismatch: " + relative)
    actual = {p.relative_to(root).as_posix().casefold() for p in (root / "overlay").rglob("*") if p.is_file()}
    expected = {name for name in seen if name.startswith("overlay/")}
    if actual != expected:
        raise ValueError("Overlay and manifest differ")
    for path in list((root / "scripts").glob("*.py")) + list((root / "overlay/scripts").glob("*.py")) + list((root / "overlay/tests").glob("*.py")):
        ast.parse(path.read_text(encoding="utf-8-sig"), filename=str(path.relative_to(root)))
    return len(entries)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    print("Source package PASS: " + str(validate_repository(args.root)) + " verified entries")
