"""Production compatibility and previous-release removal checks in an owned runtime copy."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,zipfile,uuid
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--setup',type=Path,required=True);args=parser.parse_args()
setup=args.setup.resolve();work=ROOT/'build/installer-0.5.2-beta2'/('production-'+uuid.uuid4().hex);work.mkdir(parents=True)
reference=json.loads((ROOT/'build/native-installer-runtime-smoke.json').read_text(encoding='utf-8-sig'))
assert reference['status']=='PASS' and Path(reference['setup']).resolve()==setup
app=work/'App';shutil.copytree(Path(reference['fixture'])/'App',app)
# No installed state should remain after the smoke suite's final removal.
assert not (app/'.ksm-enhanced-install.json').exists()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def inventory():
 return {p.relative_to(app).as_posix():sha(p) for p in app.rglob('*') if p.is_file() and not p.relative_to(app).parts[0].startswith('.ksm-enhanced') and p.suffix not in ('.sqlite','.sqlite3') and p.relative_to(app).parts[0]!='log'}
cases=[]
def run(action,exe=setup,success=True):
 report=work/(uuid.uuid4().hex+'.json')
 process=subprocess.run([str(exe),'--quiet','--action',action,'--target',str(app),'--report',str(report)],timeout=60)
 data=json.loads(report.read_text(encoding='utf-8-sig'))
 assert (process.returncode==0)==success,data
 return data
try:
 # Real user-modified valid images and UI, nonstandard nested names and persisted settings.
 for name in ('imgs/base.gif','ui/scene/title.noco','ui/scene/select.noco','ui/scene/result.noco','ui/scene/play_hud.noco'):
  path=app/name
  if name.endswith('.noco'):path.write_bytes(path.read_bytes()+b'\n ')
  else:path.write_bytes(path.read_bytes()+b'USER-IMAGE-METADATA')
 extra=app/'imgs/日本語 自定义/任意.png';extra.parent.mkdir(parents=True);extra.write_bytes(b'USER-IMAGE')
 before=inventory();run('inspect');run('install');run('verify');run('remove');assert inventory()==before
 cases.append('Modified real images/UI and custom folders accepted; install/remove restores exact original bytes')
 # Missing root UI and base.gif must not prevent installation; mod-managed overrides remain in payload.
 saved={}
 for name in ('imgs/base.gif','ui/scene/title.noco','ui/scene/select.noco','ui/scene/result.noco','ui/scene/play_hud.noco'):
  path=app/name;saved[name]=path.read_bytes();path.unlink()
 before=inventory();run('inspect');run('install');run('verify');run('remove');assert inventory()==before
 cases.append('Missing base image and four original root scene files accepted; absent paths remain absent after removal')
 for name,data in saved.items():(app/name).write_bytes(data)
 # Real original DLL with different bytes: trailing content changes its hash, not PE functions.
 dll=app/'bass.dll';original=dll.read_bytes();dll.write_bytes(original+b'USER-HASH-DIFFERENCE');before=inventory()
 run('inspect');run('install');run('remove');assert inventory()==before;dll.write_bytes(original)
 cases.append('Actual original packed BASS with a different file hash accepted without loading or modifying the DLL')
 # Malformed DLL has a specific read-only diagnostic and cannot start file mutation.
 dll.write_bytes(b'NOT-A-DLL');before=inventory();result=run('inspect',success=False)
 assert result['code']=='dependency_invalid' and 'bass.dll' in result['message'];run('install',success=False);assert inventory()==before;dll.write_bytes(original)
 cases.append('Malformed required DLL reports its filename and reason; all original/user files preserved')
 # Restore known root resources before old installer, whose strict contract requires them.
 original_app=Path(r'F:\ksh_vertical_project\kshootmania_v2.1.0_win\kshootmania_v2')
 for name in saved:shutil.copyfile(original_app/name,app/name)
 old_zip=next((ROOT/'releases/ksm-enhanced-vertical-0.5.1-beta1-installer-cleanup1').glob('*.zip'))
 with zipfile.ZipFile(old_zip) as z:
  item=next(n for n in z.namelist() if n.endswith('KsmEnhancedVerticalSetup.exe'))
  old=work/'OldSetup.exe';old.write_bytes(z.read(item))
 before=inventory();run('install',exe=old)
 # Preserve new records added after the old installation.
 earned=app/'score/earned-after-old_additional.ksc';earned.parent.mkdir(exist_ok=True);earned.write_bytes(b'EX=AFTER-OLD-INSTALL');before['score/earned-after-old_additional.ksc']=sha(earned)
 result=run('inspect');assert result['version']=='0.5.1-beta1' and result['sameVersion'] is False
 run('remove');assert inventory()==before
 run('install');run('verify');run('remove');assert inventory()==before
 cases.append('Actual 0.5.1-beta1 production install removed by new installer; new EX record preserved through 0.5.2-beta2 reinstall')
 result={'status':'PASS','setup':str(setup),'setupSha256':sha(setup),'fixture':str(work),'cases':cases,'checks':len(cases)}
except Exception as e:
 result={'status':'FAIL','error':str(e),'fixture':str(work),'cases':cases};raise
finally:
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print(json.dumps(result,indent=2))
