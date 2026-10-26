"""Launch the real GUI against an isolated runtime while an owned directory is locked."""
from pathlib import Path
import argparse,ctypes as ct,hashlib,json,os,subprocess,tempfile,time,uuid
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--setup',type=Path,required=True);args=parser.parse_args()
setup=args.setup.resolve();runtime=json.loads((ROOT/'build/native-installer-runtime-smoke.json').read_text(encoding='utf-8-sig'))
assert Path(runtime['setup']).resolve()==setup and runtime['status']=='PASS'
target=(Path(runtime['fixture'])/'App').resolve();target.relative_to(ROOT/'build/native-installer-runtime')
assert not (target/'.ksm-enhanced-install.json').exists()
work=ROOT/'build/native-installer'/('gui-cleanup-'+uuid.uuid4().hex);work.mkdir()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected={p.relative_to(target).as_posix():sha(p)for p in target.rglob('*')if p.is_file() and (p.suffix.lower()=='.ksc' or p.name in ('config.ini','skin_preset.json','audio_calibration_profiles.json') or 'skins' in p.relative_to(target).parts)}
report=work/'install.json';p=subprocess.run([str(setup),'--quiet','--action','install','--target',str(target),'--report',str(report)],timeout=90);assert p.returncode==0
name='mods/enhanced/judge_setting';directory=target/name;assert directory.is_dir()
k=ct.WinDLL('kernel32',use_last_error=True);k.CreateFileW.argtypes=[ct.c_wchar_p,ct.c_uint32,ct.c_uint32,ct.c_void_p,ct.c_uint32,ct.c_uint32,ct.c_void_p];k.CreateFileW.restype=ct.c_void_p;k.CloseHandle.argtypes=[ct.c_void_p]
handle=k.CreateFileW(str(directory),0x80000000,3,None,3,0x02000000,None);assert handle not in (None,ct.c_void_p(-1).value)
try:
    started=time.time_ns();gui=subprocess.Popen([str(setup),'--target',str(target),'--language','2','--gui-action','remove'])
    last=Path(tempfile.gettempdir())/'KsmEnhancedVerticalSetup-last-result.json';deadline=time.monotonic()+45;data=None
    while time.monotonic()<deadline:
        if gui.poll() is not None:raise RuntimeError('GUI exited before observation')
        try:
            if last.stat().st_mtime_ns>=started:
                current=json.loads(last.read_text(encoding='utf-8-sig'))
                if current.get('action')=='remove' and current.get('status')=='PASS' and any(w['path']==name for w in current.get('cleanupWarnings',[])):data=current;break
        except (OSError,ValueError):pass
        time.sleep(.1)
    assert data is not None,'No expected completed removal warning'
finally:k.CloseHandle(handle)
for name,h in protected.items():assert sha(target/name)==h,name
assert directory.is_dir() and not (target/'ksm-enhanced-vertical.exe').exists()
result={'status':'PASS','method':'Actual production GUI; real directory sharing lock; no synthetic fault hook','guiPid':gui.pid,'target':str(target),'protectedFiles':len(protected),'result':data}
(work/'result.json').write_text(json.dumps(result,indent=2)+'\n')
(ROOT/'build/native-installer/gui-cleanup-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'status':'PASS','guiPid':gui.pid,'warningPaths':[w['path']for w in data['cleanupWarnings']],'removedDirectories':len(data['removedDirectories']),'protectedFiles':len(protected)},indent=2))
