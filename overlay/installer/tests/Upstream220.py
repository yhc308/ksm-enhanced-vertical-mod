"""Real 2.2.0 installer transactions, without launching the game or audio."""
from pathlib import Path
import argparse, hashlib, importlib.util, json, shutil, subprocess, sys, uuid

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser()
p.add_argument('--setup', type=Path, required=True)
p.add_argument('--original22', type=Path, required=True)
p.add_argument('--original21', type=Path, required=True)
p.add_argument('--previous-setup', type=Path)
a = p.parse_args()
sys.path.insert(0, str(ROOT / 'installer'))
spec = importlib.util.spec_from_file_location('package220', ROOT / 'installer/BuildPackage.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
work = ROOT / 'build/installer220-tests' / uuid.uuid4().hex
work.mkdir(parents=True)
checks = []
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()

def check(value, message):
    assert value, message
    checks.append(message)

def fixture(name):
    app = work / name
    app.mkdir()
    for n in ('kshootmania.exe', 'bass.dll', 'bass_fx.dll'):
        shutil.copy2(a.original22 / n, app / n)
    for n in ('imgs', 'ui', 'lang'):
        shutil.copytree(a.original22 / n, app / n)
    for n in ('score/base.ksc', 'score/base_additional.ksc', 'courses/score/base.ksc',
              'skins/custom/image.png', 'songs/custom/chart.ksh', 'config.ini',
              'skin_preset.json', 'audio_calibration_profiles.json'):
        f = app / n
        f.parent.mkdir(parents=True, exist_ok=True)
        f.write_bytes(('USER DATA ' + n).encode())
    return app

def inventory(app):
    return {p.relative_to(app).as_posix(): sha(p) for p in app.rglob('*')
            if p.is_file() and not p.relative_to(app).parts[0].startswith('.ksm-enhanced')}

def run(app, action, success=True, exe=None):
    report = work / (uuid.uuid4().hex + '.json')
    result = subprocess.run([str(exe or a.setup), '--quiet', '--action', action,
                             '--target', str(app), '--report', str(report)], timeout=180)
    data = json.loads(report.read_text(encoding='utf-8-sig'))
    check((result.returncode == 0) == success, action + ': expected outcome')
    return data

try:
    app = fixture('日本語 한국어 2.2.0')
    before = inventory(app)
    run(app, 'inspect'); run(app, 'install'); run(app, 'verify')
    for n in ('ui/font/font_monospace.noco', 'ui/parts/option_setting_heading.noco',
              'ui/parts/select_folder_loading.noco', 'ui/scene/option.noco'):
        check((app / n).read_bytes() == (ROOT / 'kshootmania/App' / n).read_bytes(),
              'New upstream UI included: ' + n)
    check(sha(app / 'kshootmania.exe') == before['kshootmania.exe'], 'Original 2.2 EXE unchanged')
    run(app, 'remove')
    check(inventory(app) == before, '2.2 install/remove restores every baseline file')
    run(app, 'install'); run(app, 'remove')
    check(inventory(app) == before, '2.2 reinstall/remove preserves every baseline file')

    app = fixture('edited and missing resources')
    (app / 'imgs/base.gif').unlink()
    (app / 'ui/scene/play_hud.noco').unlink()
    (app / 'ui/scene/option.noco').write_bytes(b'USER CUSTOM UI')
    before = inventory(app)
    run(app, 'install'); run(app, 'remove')
    check(inventory(app) == before, 'Missing images/UI accepted; customized original UI restored')

    app = fixture('reject 2.1')
    shutil.copy2(a.original21 / 'kshootmania.exe', app / 'kshootmania.exe')
    before = inventory(app)
    result = run(app, 'inspect', False)
    check(result['code'] == 'target', '2.1 original EXE explicitly rejected by 2.2 setup')
    run(app, 'install', False)
    check(inventory(app) == before, 'Rejected 2.1 installation has no file changes')

    app = fixture('old mod record with new original exe')
    state = app / '.ksm-enhanced-install.json'
    state.write_text(json.dumps({'schemaVersion': 4, 'modId': 'ksm-enhanced-vertical',
        'runtimeContract': 'ksm-2.1.0-enhanced-skin-v2'}), encoding='utf8')
    before = inventory(app); old_state = state.read_bytes()
    for action in ('inspect', 'install', 'update', 'remove'):
        result = run(app, action, False)
        check(result['code'] == 'runtime_mismatch', 'Cross-base state blocked before inventory access: ' + action)
        check(inventory(app) == before and state.read_bytes() == old_state,
              'Cross-base rejection preserves all data: ' + action)

    app = fixture('2.2 same base update')
    original = inventory(app)
    older = work / 'Older220Setup.exe'
    builder.assemble(ROOT / 'build/native-installer/SetupStub.exe', older,
        builder.sources(), json.loads((ROOT / 'compatibility/targets.json').read_text()),
        version='0.5.4-beta4', build_id='20261116-000000-00000000')
    run(app, 'install', exe=older)
    run(app, 'update'); run(app, 'verify'); run(app, 'remove')
    check(inventory(app) == original, 'Same-base 2.2 update restores earliest original on removal')

    app = fixture('preserve edits after install')
    run(app, 'install')
    index = json.loads(a.setup.with_suffix('.payload.json').read_text())
    image = next(x['path'] for x in index['files'] if x['path'].startswith('imgs/') and x['kind'] == 'image')
    (app / image).write_bytes(b'USER EDIT AFTER INSTALL')
    new_ex = app / 'score/after_additional.ksc'
    new_ex.write_bytes(b'NEW EX MAX RECORD')
    run(app, 'remove')
    check((app / image).read_bytes() == b'USER EDIT AFTER INSTALL', 'Post-install image edits preserved')
    check(new_ex.read_bytes() == b'NEW EX MAX RECORD', 'Post-install EX records preserved')
    if a.previous_setup:
        app = fixture('actual previous installer revision')
        owned_skins = (app / 'skins').resolve(); owned_skins.relative_to(work.resolve())
        shutil.rmtree(owned_skins)
        baseline = inventory(app)
        run(app, 'install', exe=a.previous_setup)
        previous_state = json.loads((app / '.ksm-enhanced-install.json').read_text(encoding='utf-8-sig'))
        previous_index = json.loads(a.previous_setup.with_suffix('.payload.json').read_text(encoding='utf-8-sig'))
        check(previous_state.get('installerRevisionNumber', 0) == previous_index.get('installerRevisionNumber', 0), 'Previous actual 2.2 installer revision matches its published package')
        report = run(app, 'inspect')
        check(report['updateAvailable'], 'Actual prior 2.2 installation offers installer-only update')
        game_hash = sha(app / 'ksm-enhanced-vertical.exe')
        run(app, 'update'); run(app, 'verify')
        expected_game = next(x['sha256'] for x in json.loads(a.setup.with_suffix('.payload.json').read_text())['files'] if x['path'] == 'ksm-enhanced-vertical.exe')
        check(sha(app / 'ksm-enhanced-vertical.exe') == expected_game, 'Production update applies the exact verified game EXE')
        check((app / 'skins').is_dir(), 'Installer-only production update prepares skins')
        run(app, 'remove')
        check(inventory(app) == baseline, 'Installer-only production update removal restores first-install originals')
    result = {'status': 'PASS', 'checks': len(checks), 'cases': checks,
              'fixture': str(work), 'gameLaunched': False, 'audioDevicesUsed': False}
except Exception as e:
    result = {'status': 'FAIL', 'checks': len(checks), 'error': str(e), 'fixture': str(work)}
    raise
finally:
    (ROOT / 'build/installer220-results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
