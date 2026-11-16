"""SDK-free checks for the isolated 2.2.0 branch; not a gameplay test."""
from pathlib import Path
import hashlib, json, re, subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = '63d6546808fe3afad4469f3e4d12ca85145650d8'
CONTRACT = 'ksm-2.2.0-enhanced-skin-v1'
read = lambda p: (ROOT / p).read_text(encoding='utf-8-sig')
manifest = json.loads(read('patches/dependencies/manifest.json'))
assert manifest['upstreamCommit'] == BASE
for item in manifest['dependencies']:
    assert hashlib.sha256((ROOT / item['patch']).read_bytes()).hexdigest() == item['sha256']
    repo = ROOT / item['path']
    actual = subprocess.check_output(['git', '-c', 'safe.directory=' + repo.as_posix(), '-C', str(repo), 'rev-parse', 'HEAD'], text=True).strip()
    assert actual == item['baseCommit']
assert 'kAppVersion = U"v2.2.0"' in read('kshootmania/src/Common/CommonDefines.hpp')
assert 'K-Shoot Mania V2.2.0 - Enhanced Vertical Mod' in read('kshootmania/src/Main.cpp')
profiles = json.loads(read('compatibility/targets.json'))['profiles']
assert len(profiles) == 1 and profiles[0]['upstreamVersion'] == '2.2.0'
assert profiles[0]['sourceBaseCommit'] == BASE and profiles[0]['runtimeContract'] == CONTRACT
assert profiles[0]['requiredFiles'] == [], 'User-modifiable resources must not be hash-gated'
assert CONTRACT in read('installer/src/InstallerCore.cpp') and 'runtime_mismatch' in read('installer/src/InstallerCore.cpp')
for name in ('ui/parts/option_setting_heading.noco', 'ui/parts/select_folder_loading.noco', 'ui/font/font_monospace.noco'):
    assert (ROOT / 'kshootmania/App' / name).is_file()
    assert name in read('installer/BuildPackage.py')
# Keep every native 2.2 test case; mod tests can extend the suite.
git = ['git', '-c', 'safe.directory=' + ROOT.as_posix(), '-C', str(ROOT)]
files = subprocess.check_output(git + ['ls-tree', '-r', '--name-only', BASE], text=True).splitlines()
case = re.compile(r'TEST_CASE\("([^"\n]+)"')
current = set().union(*(set(case.findall(p.read_text(encoding='utf-8-sig'))) for p in (ROOT / 'tests').glob('*.cpp')))
native = set()
for name in files:
    if name.startswith('tests/') and name.endswith('.cpp'):
        native.update(case.findall(subprocess.check_output(git + ['show', BASE + ':' + name]).decode('utf-8-sig')))
assert native <= current, 'Missing native 2.2 regression cases: ' + str(native - current)
original = [name for name in files if name.startswith('kshootmania/src/')]
assert all((ROOT / name).is_file() for name in original), 'Do not remove native game source'
result = {'status': 'PASS', 'baseCommit': BASE, 'nativeTestCasesRetained': len(native), 'testCasesTotal': len(current),
          'nativeGameFilesRetained': len(original), 'runtimeContract': CONTRACT, 'scope': 'Static contracts only; build/runtime reports are separate'}
(ROOT / 'build').mkdir(exist_ok=True)
(ROOT / 'build/migration220-source-checks.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
