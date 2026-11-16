"""Run migrated game diagnostics with a verified discard-only mock, on a private desktop.

Requires a diagnostic build. This never changes system volume or real audio devices.
"""
from pathlib import Path
import argparse, hashlib, json, os, shutil, subprocess, sys, uuid

sys.stdout.reconfigure(encoding="utf-8")
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--runtime', type=Path, required=True, help='Unmodified official KSM 2.2.0 resource directory')
p.add_argument('--cases', nargs='+', choices=['ex', 'aps', 'judge', 'skin', 'geometry', 'viewport', 'options', 'laser', 'range', 'context', 'select'],
               default=['ex', 'aps', 'judge', 'skin', 'geometry'])
p.add_argument('--width', type=int, default=720)
p.add_argument('--height', type=int, default=1280)
p.add_argument('--laser-charts', type=Path)
a = p.parse_args()
build = json.loads((ROOT / 'build/latest.json').read_text(encoding='utf-8-sig'))
assert build['diagnostics'], 'Normal executables ignore diagnostic flags; never launch them in this host'
assert hashlib.sha256(Path(build['executable']).read_bytes()).hexdigest() == build['sha256'].lower()
audio = json.loads((ROOT / 'build/audio-engine-latest.json').read_text(encoding='utf-8-sig'))
assert audio['status'] == 'PASS'
mock = Path(audio['runtime']) / 'bassasio.dll'
assert b'FakeAsioSilentOnly' in mock.read_bytes(), 'Require discard-only test DLL'
host = ROOT / 'build/silent-qa/SilentRuntimeHost.exe'
assert host.is_file()
assert hashlib.sha256((a.runtime / 'kshootmania.exe').read_bytes()).hexdigest() == '10f75e152252e519761a462a6e613d6908e5f8f86e65590c673e8472f6c3df34'
work = ROOT / 'build/silent-qa' / (str(a.width) + 'x' + str(a.height) + '-' + uuid.uuid4().hex)
work.mkdir(parents=True)
results = []

def copy_resource(src, dst):
    # Only immutable music/font files share storage. Config, images, DLLs and test outputs never do.
    if Path(src).suffix.lower() in {'.wav', '.ogg', '.mp3', '.ttf', '.otf'}:
        try:
            if Path(dst).exists(): Path(dst).unlink()
            os.link(src, dst)
            return dst
        except OSError:
            pass
    return shutil.copy2(src, dst)

cases = {
    'ex': (['--ex-score-test'], 'ex-score-test.txt', 'EX checks passed', 90),
    'aps': (['--aps-test'], 'aps-test.txt', 'APS checks passed', 120),
    'judge': (['--judge-setting-test'], 'judge-setting-test.txt', 'judge-setting checks passed', 240),
    'skin': (['--skin-core-test'], 'core-passed.txt', 'passed', 90),
    'geometry': (['--render-geometry-test'], 'render-geometry-passed.txt', 'PASS', 120),
    'viewport': (['--skin-smoke-test', '--viewport-options-smoke'], 'viewport-options-passed.txt', 'PASS', 180),
    'options': (['--options-220-test'], 'options-220-test.txt', 'option integration checks passed', 120),
    'laser': (['--laser-render-test'], 'laser-render-passed.txt', 'Completed real laser draws', 360),
    'range': (['--laser-range-test'], 'laser-range-passed.txt', 'comparisons passed', 120),
    'context': (['--render-context-test'], 'render-context-passed.txt', 'checks passed', 60),
    'select': (['--vertical-preview-select', '--vertical-preview-fast', '--select-220-force-loading'], 'select-preview-state.json', 'count', 60),
}

for name in a.cases:
    app = work / name
    app.mkdir()
    for directory in ('engine', 'assets', 'dll', 'imgs', 'lang', 'ui', 'se', 'shaders'):
        shutil.copytree(a.runtime / directory, app / directory, copy_function=copy_resource)
    source = ROOT / 'kshootmania/App'
    for directory in ('mods', 'imgs', 'skin_preview', 'lang', 'ui', 'shaders'):
        shutil.copytree(source / directory, app / directory, dirs_exist_ok=True, copy_function=copy_resource)
    for n in ('bass.dll', 'bass_fx.dll', 'bassmix.dll'):
        shutil.copy2(source / n, app / n)
    shutil.copy2(mock, app / 'bassasio.dll')
    executable = app / 'ksm-enhanced-vertical.exe'
    shutil.copy2(build['executable'], executable)
    text = (ROOT / 'config/default-windows.ini').read_text(encoding='utf-8-sig')
    import re
    values = dict(re.findall(r'^([A-Za-z0-9_]+)=(.*)$', text, re.M))
    values.update(mastervol='0', audio_backend='asio:KSM Mock ASIO', automaticmute='0',
                  vertical_enabled=str(int(a.height > a.width)), fullscr='0',
                  windowsize=f'{a.width},{a.height}', vertical_resolution=f'{a.width},{a.height}',
                  vertical_width=str(a.width), vertical_height=str(a.height), vertical_display_mode='0',
                  vsync='1', hispeed='450')
    if name == 'select':
        values.update(currentdir='Migration220Folder', currentitem='1', currentlevel='0',
                      show_translit_kanji='1', show_translit_hangul='1', show_translit_kana='1')
        from PIL import Image
        for i in range(20):
            song = app / 'songs/Migration220Folder' / f'{i:02d} 日本語 简体 繁體 한국어 🎵'
            song.mkdir(parents=True)
            (song/'chart.ksh').write_text(f'title=曲目 {i:02d} 日本語 中文 한국어 🎵\ntitle_translit=Romanized song {i:02d}\nartist=QA\ndifficulty=light\nlevel={i+1}\nt=120\njacket=jacket.png\n--\n1000|00|--\n0000|00|--\n--\n',encoding='utf-8-sig')
            Image.new('RGB',(128,128),(20+i*10,50,180)).save(song/'jacket.png')
    (app / 'config.ini').write_text('[config]\n' + '\n'.join(k + '=' + v for k, v in values.items()) + '\n', encoding='utf-8-sig')
    for marker in ('audio-test-fixture', 'skin-test-fixture', 'render-geometry-fixture', 'laser-render-fixture'):
        (app / marker).write_text('Owned silent KSM 2.2.0 QA fixture\n')
    if name in ('skin', 'viewport'):
        setup = subprocess.run([sys.executable, str(ROOT / 'tests/SkinTestVerifier.py'), 'setup', str(app)],
                               capture_output=True, text=True)
        assert setup.returncode == 0, setup.stderr
    if name == 'geometry':
        # Geometry contracts measure solid synthetic sprites, not the authored
        # cursor's asymmetric transparent padding. Use the existing fixture.
        setup = subprocess.run([sys.executable, str(ROOT / 'tests/PrepareRenderGeometryAssets.py'), str(app)],
                               capture_output=True, text=True)
        assert setup.returncode == 0, setup.stderr
    if name in ('laser', 'range'):
        assert a.laser_charts and a.laser_charts.is_dir()
        charts = sorted(p for p in a.laser_charts.rglob('*') if p.suffix.lower() in ('.ksh', '.kson'))
        assert charts, 'Never pass a vacuous laser draw test'
        destination = app / 'laser-fixture-charts'
        destination.mkdir()
        for i, chart in enumerate(charts):
            shutil.copy2(chart, destination / (str(i) + chart.suffix))
    flags, report_name, expected, timeout = cases[name]
    if name == 'select':
        flags = flags + (['--vertical-preview-720'] if a.height > a.width else ['--vertical-preview-landscape'])
    env = os.environ.copy()
    env['KSM_ASIO_TEST_MODE'] = 'silent-auto' if name in ('viewport', 'select') else ''
    process = subprocess.run([str(host), str(executable), str(app), str(timeout), '--silent-qa', *flags],
                             env=env, capture_output=True, timeout=timeout + 20)
    (app / 'host.txt').write_bytes(process.stdout + process.stderr)
    report = app / report_name
    failures = list(app.glob('*failure.txt'))
    dialog = app / 'silent-qa-dialog.txt'
    passed = (process.returncode == 0 and report.exists() and
              expected.lower() in report.read_text(encoding='utf-8-sig').lower() and
              not failures and not dialog.exists())
    if passed and name == 'viewport':
        verify = subprocess.run([sys.executable, str(ROOT / 'tests/ViewportFrameVerifier.py'), str(app)],
                                capture_output=True)
        (app / 'frame-verification.txt').write_bytes(verify.stdout + verify.stderr)
        passed = verify.returncode == 0
    if passed and name == 'select':
        state = json.loads(report.read_text(encoding='utf-8-sig'))
        passed = (state['count'] >= 20 and state['topRows'] == (6 if a.height > a.width else 4)
                  and state['bottomRows'] == (7 if a.height > a.width else 3)
                  and (app/'Screenshot/vertical-select-preview.png').exists()
                  and (app/'Screenshot/select-loading-220.png').exists())
    result = {'case': name, 'status': 'PASS' if passed else 'FAIL', 'fixture': str(app),
              'report': str(report), 'privateDesktop': True, 'audioSink': 'mock discard only',
              'renderer': 'WARP', 'physicalTimingOrPerformanceClaim': False}
    results.append(result)
    print(json.dumps(result), flush=True)
    (work / 'results.json').write_text(json.dumps(results, indent=2))
    if not passed:
        for f in failures + ([dialog] if dialog.exists() else []):
            print(f.name + ': ' + f.read_text(encoding='utf-8-sig'), flush=True)
        raise SystemExit(1)

(ROOT / 'build/silent-qa-latest.json').write_text(json.dumps({'status': 'PASS', 'buildId': build['buildId'],
    'width': a.width, 'height': a.height, 'results': results}, indent=2))
