"""Snapshot or verify immutable runtime resources for behavior-preserving refactoring."""
import argparse
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("root", type=Path)
parser.add_argument("manifest", type=Path)
parser.add_argument("--snapshot", action="store_true")
parser.add_argument("--allow-checkout-line-endings", action="store_true", help="Allow only CRLF/LF differences in known text resources; binary hashes stay exact")
args = parser.parse_args()
root = args.root.resolve()
files = {}
normalized_text = {}
for folder in ("imgs", "lang", "mods", "shaders", "skin_preview", "ui"):
    for path in sorted((root / folder).rglob("*")):
        if path.is_symlink():
            raise SystemExit("Linked resource rejected: " + str(path))
        if path.is_file():
            name = path.relative_to(root).as_posix()
            data = path.read_bytes()
            files[name] = hashlib.sha256(data).hexdigest()
            if path.suffix.lower() in {".txt", ".json", ".noco", ".hlsl", ".glsl"}:
                normalized_text[name] = hashlib.sha256(data.replace(b"\r\n", b"\n")).hexdigest()
if not files:
    raise SystemExit("No managed runtime resources found")
if args.snapshot:
    if args.manifest.exists():
        raise SystemExit("Baseline already exists; it cannot be overwritten")
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps({"schemaVersion": 2, "files": files, "textLfHashes": normalized_text}, indent=2) + "\n", encoding="utf-8")
    print("Resource baseline created:", len(files), "files")
else:
    baseline = json.loads(args.manifest.read_text(encoding="utf-8-sig"))
    expected = baseline["files"]
    text_expected = baseline.get("textLfHashes", {})
    changed = []
    checkout_only = []
    for name in sorted(set(files) | set(expected)):
        if files.get(name) == expected.get(name):
            continue
        if (args.allow_checkout_line_endings and name in files and name in expected
                and name in text_expected and normalized_text.get(name) == text_expected[name]):
            checkout_only.append(name)
        else:
            changed.append(name)
    if changed:
        raise SystemExit("Runtime resources differ from the baseline:\n" + "\n".join(changed))
    print("Resource baseline PASS:", len(files) - len(checkout_only), "byte-identical files;",
          len(checkout_only), "text files differ only in Git checkout line endings")
    for name in checkout_only:
        print("CHECKOUT LINE ENDINGS ONLY:", name)
