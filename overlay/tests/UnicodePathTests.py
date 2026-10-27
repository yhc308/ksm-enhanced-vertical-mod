"""Run production-path Unicode regressions in an isolated build/ runtime.

Usage: python tests/UnicodePathTests.py DIAGNOSTIC_EXE [--run-root build/NAME]
No supplied music or user scores are required; fixtures are synthesized here.
"""
from pathlib import Path
import argparse
import json
import math
import shutil
import struct
import subprocess
import time
import wave
import zlib


def png():
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data) & 0xffffffff)
    pixels = b'\0' + bytes([255, 0, 0, 0, 255, 0]) + b'\0' + bytes([0, 0, 255, 255, 255, 255])
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 2, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(pixels)) + chunk(b'IEND', b'')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--run-root', type=Path, default=Path('build/unicode-path-regression'))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    run = (root / args.run_root).resolve()
    run.relative_to((root / 'build').resolve())
    if run == root / 'build':
        raise ValueError('Choose a dedicated directory under build/')
    app = run / 'App'
    if app.exists() and not (app / 'unicode-path-fixture').exists():
        raise ValueError('Existing runtime lacks isolation marker')
    app.mkdir(parents=True, exist_ok=True)
    (app / 'unicode-path-fixture').write_text('isolated Unicode regression', encoding='utf-8')
    for name in ['assets', 'dll', 'engine', 'imgs', 'lang', 'mods', 'se', 'shaders', 'ui']:
        if not (app / name).exists():
            shutil.copytree(root / 'kshootmania/App' / name, app / name)
    for name in ['bass.dll', 'bass_fx.dll', 'bassasio.dll', 'bassmix.dll']:
        shutil.copy2(root / 'kshootmania/App' / name, app / name)
    if (app / 'cache').exists():
        # Preserve deliberately corrupted test DBs, rather than deleting them.
        (app / 'cache').rename(run / ('cache-' + str(time.time_ns())))
    shutil.copy2(args.executable.resolve(), app / 'ksm-enhanced-vertical.exe')
    (app / 'config.ini').write_text('[config]\nmastervol=0\naudio_backend=default\ncurrentlang=Japanese\nvertical_enabled=0\nwindowsize=1280,720\nvsync=0;300\ncurrentplayer=記録 简体 繁體 한글 🎵\n', encoding='utf-8-sig')
    title = '三原色 简体 繁體 한글 𠮷 🎵 👩🏽‍💻'
    audio = '音楽 简体 繁體 한글 🎵.wav'
    jacket = '画像 简体 繁體 한글 🎨.png'
    text = f'title={title}\nartist=Unicode fixture\nt=128\nm={audio}\njacket={jacket}\ndifficulty=infinite\nlevel=1\n--\n1000|00|--\n0000|00|--\n--\n'
    names = ['ASCII', 'おふとんクイズ会 2 Memorial Festival', '简体中文 测试', '繁體中文 測試', '한국어 시험', '𠮷野家 拡張漢字', '音楽 🎵', '家族 👨‍👩‍👧‍👦', '職業 👩🏽‍💻', '国旗 🇯🇵🇨🇳🇰🇷', '記号 ［］（）＆＃％ +,;=', 'Combining e\u0301 か\u3099', 'Composed é が']
    charts = [app / 'songs' / name / '00_IN' / '譜面 简体 繁體 한글 🎶.ksh' for name in names]
    for target in [259, 280, 320]:
        folder = app / 'songs' / ('Long' + str(target))
        while len(str(folder / 'a.ksh')) < target - 35:
            folder = folder / ('日本語简繁' + 'x' * 12)
        folder = folder / ('x' * max(1, target - len(str(folder / 'a.ksh')) - 1))
        charts.append(folder / 'a.ksh')
    samples = b''.join(struct.pack('<h', int(2000 * math.sin(2 * math.pi * 440 * n / 44100))) for n in range(8820))
    cases = []
    for chart in charts:
        chart.parent.mkdir(parents=True, exist_ok=True)
        chart.write_text(text, encoding='utf-8-sig')
        (chart.parent / jacket).write_bytes(png())
        with wave.open(str(chart.parent / audio), 'wb') as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(44100)
            wav.writeframes(samples)
        cases.append({'path': chart.relative_to(app).as_posix(), 'title': title, 'maximum': 5})
    (charts[0].parent / 'broken.kson').write_text('{ invalid json', encoding='utf-8')
    for name, encoding in [('cp932', 'cp932'), ('utf8-bom', 'utf-8-sig'), ('utf8-ascii', 'ascii')]:
        legacy_title = 'ASCII chart' if encoding == 'ascii' else '日本語 譜面'
        (app / (name + '.ksh')).write_text(f'title={legacy_title}\nt=128\n--\n1000|00|--\n0000|00|--\n--\n', encoding=encoding)
    invalid = {'format_version': '0.8.0', 'meta': {'title': 'Invalid timing fixture', 'artist': 'TEST', 'difficulty': 3, 'level': 1}, 'beat': {'bpm': [[0, 0.0]], 'time_sig': [[0, [4, 4]]]}, 'note': {'bt': [[[0, 0]], [], [], []]}}
    (app / 'invalid-bpm.kson').write_text(json.dumps(invalid), encoding='utf-8')
    (app / 'unicode-cases.json').write_text(json.dumps(cases, ensure_ascii=False, indent=2), encoding='utf-8')
    result = subprocess.run([str(app / 'ksm-enhanced-vertical.exe'), '--unicode-path-test', '会 简体 繁體 한글 𠮷 🎵 👩🏽‍💻'], cwd=app, timeout=120, creationflags=0x08000000)
    report = (app / 'unicode-path-results.txt').read_text(encoding='utf-8-sig')
    if result.returncode or 'failures=0' not in report or '\nFAIL ' in report:
        raise RuntimeError(report)
    print(report.splitlines()[-1])
    print(app / 'unicode-path-results.txt')


if __name__ == '__main__':
    main()
