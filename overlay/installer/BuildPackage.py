"""Create an offline single-EXE package with XPRESS_HUFF blocks and SHA256 index."""
from pathlib import Path
import argparse
import ctypes as ct
from ctypes import wintypes as wt
import hashlib
import json
import struct
import sys
from PeImage import PeImage

sys.stdout.reconfigure(encoding='utf-8')
ROOT = Path(__file__).resolve().parents[1]
GAME_BUILD = '20261027-194127-779e4d6d'
GAME_SHA = 'ef68345d4e8126d35ac7ecf6c552b3e51b5cfce22419f0db2c7a69d999bb73e3'
cabinet = ct.WinDLL('cabinet', use_last_error=True)
cabinet.CreateCompressor.argtypes = [wt.DWORD, ct.c_void_p, ct.POINTER(ct.c_void_p)]
cabinet.Compress.argtypes = [ct.c_void_p, ct.c_void_p, ct.c_size_t, ct.c_void_p, ct.c_size_t, ct.POINTER(ct.c_size_t)]
cabinet.CloseCompressor.argtypes = [ct.c_void_p]

def compress(data):
    handle = ct.c_void_p()
    if not cabinet.CreateCompressor(4, None, ct.byref(handle)):
        raise ct.WinError(ct.get_last_error())
    try:
        source = ct.create_string_buffer(data)
        needed = ct.c_size_t()
        cabinet.Compress(handle, source, len(data), None, 0, ct.byref(needed))
        output = ct.create_string_buffer(needed.value)
        if not cabinet.Compress(handle, source, len(data), output, needed.value, ct.byref(needed)):
            raise ct.WinError(ct.get_last_error())
        return output.raw[:needed.value]
    finally:
        cabinet.CloseCompressor(handle)

def kind(path):
    if path.startswith('lang/'):
        return 'language'
    return 'image' if Path(path).suffix.lower() in {'.png','.gif','.jpg','.jpeg','.bmp','.webp','.ico'} else 'other'

def sources():
    build = json.loads((ROOT / 'build/latest.json').read_text(encoding='utf-8-sig'))
    assert build['buildId'] == GAME_BUILD and build['configuration'] == 'Release' and not build['diagnostics']
    executable = Path(build['executable'])
    assert hashlib.sha256(executable.read_bytes()).hexdigest() == GAME_SHA
    result = {'ksm-enhanced-vertical.exe': executable}
    app = ROOT / 'kshootmania/App'
    for directory in ('mods/vertical','mods/enhanced','imgs','skin_preview'):
        for path in (app / directory).rglob('*'):
            if path.is_file():
                assert not path.is_symlink()
                result[path.relative_to(app).as_posix()] = path
    for path in (app / 'shaders').glob('aps_color_dodge.*'):
        result[path.relative_to(app).as_posix()] = path
    for name in ('English','Japanese','Korean','Simplified Chinese','Traditional Chinese'):
        result[f'lang/{name}.json'] = app / f'lang/{name}.json'
    for name in ('custom','judge','vertical'):
        result[f'ui/menu_{name}_icon.png'] = app / f'ui/menu_{name}_icon.png'
    for name in ('bassasio.dll','bassmix.dll'):
        result[name] = app / name
    result['mods/enhanced/LICENSE'] = ROOT / 'LICENSE'
    result['mods/enhanced/licenses/json.txt'] = ROOT / 'installer/third_party/LICENSE.json.txt'
    return result

def assemble(stub, destination, files, targets, fixture=False):
    assert not destination.exists(), 'Do not overwrite an existing setup executable'
    blocks, inventory, offset = [], [], 0
    for relative, path in sorted(files.items()):
        assert Path(relative).suffix.lower() not in {'.ksc','.ini','.sqlite','.sqlite3','.psd','.psb'}
        assert not any(x.lower() in {'score','score_backup','songs','songs_default','courses','skins','cache'} for x in relative.split('/'))
        data = path.read_bytes()
        assert data, 'Empty payload file: ' + relative
        packed = compress(data)
        inventory.append({'path':relative,'kind':kind(relative),'size':len(data),'compressedSize':len(packed),
                          'offset':offset,'sha256':hashlib.sha256(data).hexdigest()})
        blocks.append(packed); offset += len(packed)
    index = {'installerRevision':'20261027-compatibility2','schemaVersion':1,'modId':'ksm-enhanced-vertical','version':'0.5.2-beta2','buildId':GAME_BUILD,
             'fixture':fixture,'targets':targets,'files':inventory}
    encoded = json.dumps(index,ensure_ascii=True,separators=(',',':')).encode()
    blob = b'KSMSET01' + struct.pack('<Q',len(encoded)) + encoded + b''.join(blocks)
    image = stub.read_bytes()
    assert image[:2] == b'MZ'
    footer = b'KSMTAIL1' + struct.pack('<QQ',len(image),len(blob)) + hashlib.sha256(blob).hexdigest().encode('ascii')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(image + blob + footer)
    return index

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stub',type=Path,default=ROOT/'build/native-installer/SetupStub.exe')
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    targets = json.loads((ROOT/'compatibility/targets.json').read_text(encoding='utf-8-sig'))
    files = sources()
    imports = PeImage(files['ksm-enhanced-vertical.exe'].read_bytes()).imports()
    for profile in targets['profiles']:
        assert profile['requiredFiles'] == []
        assert profile['requiredLibraries'] == [{'path':name,'requiredExports':imports[name]} for name in ('bass.dll','bass_fx.dll')]
    index = assemble(args.stub,args.output,files,targets)
    args.output.with_suffix('.payload.json').write_text(json.dumps(index,indent=2)+'\n')
    print(json.dumps({'executable':str(args.output),'files':len(index['files']),'bytes':args.output.stat().st_size,
                      'sha256':hashlib.sha256(args.output.read_bytes()).hexdigest()},indent=2))
