"""Check added mod C++ files without reformatting restored upstream code."""
from pathlib import Path
import os
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
base = '41bfdaa7331808aff7239a9ec283cdba01b3b406'
original = set(subprocess.check_output(['git', '-c', 'safe.directory=' + root.as_posix(), '-C', str(root),
                                      'ls-tree', '-r', '--name-only', base], text=True).splitlines())
fallback = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/bin/clang-format.exe'
formatter = os.environ.get('KSM_CLANG_FORMAT') or shutil.which('clang-format') or str(fallback)
assert '19.1.5' in subprocess.check_output([formatter, '--version'], text=True)
checked = []
for path in sorted((root / 'kshootmania/src').rglob('*')):
    if path.suffix not in {'.cpp', '.hpp', '.h'} or path.relative_to(root).as_posix() in original:
        continue
    assert path.read_bytes().startswith(b'\xef\xbb\xbf'), 'Missing UTF-8 BOM: ' + str(path)
    subprocess.run([formatter, '--style=file:' + str(root / '.clang-format'), '--dry-run', '--Werror', str(path)], check=True)
    checked.append(path.relative_to(root).as_posix())
assert len(checked) == 73
print('Review style PASS:', len(checked), 'added game C++ files; upstream files are protected by compiler parity, not a global formatter pass')
