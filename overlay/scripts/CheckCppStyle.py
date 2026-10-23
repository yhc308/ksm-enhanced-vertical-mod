"""Check the explicitly audited mod C++ scope against the pinned upstream style."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
fallback = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/bin/clang-format.exe"
parser.add_argument("--formatter", default=os.environ.get("KSM_CLANG_FORMAT") or shutil.which("clang-format") or (str(fallback) if fallback.is_file() else None))
args = parser.parse_args()
if not args.formatter:
    raise SystemExit("clang-format 19.1.5 is required")
version = subprocess.check_output([args.formatter, "--version"], text=True)
if "19.1.5" not in version:
    raise SystemExit("Expected clang-format 19.1.5, got " + version.strip())
scope = json.loads((root / "docs/CPP_STYLE_SCOPE.json").read_text(encoding="utf-8"))
for name in scope["paths"]:
    path = root / name
    if not path.read_bytes().startswith(b"\xef\xbb\xbf"):
        raise SystemExit("UTF-8 BOM required: " + name)
    subprocess.run([args.formatter, "--style=file:" + str(root / ".clang-format"), "--dry-run", "--Werror", str(path)], check=True)
print("C++ style PASS: " + str(len(scope["paths"])) + " files")
