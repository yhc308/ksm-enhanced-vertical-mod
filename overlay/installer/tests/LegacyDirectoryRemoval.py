"""Use an actual older setup to install, then the current setup to remove an isolated runtime."""
from pathlib import Path
import argparse,hashlib,json,subprocess,uuid
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--setup',type=Path,required=True);parser.add_argument('--previous-setup',type=Path,default=ROOT/'build/native-installer/ui-revision-2-final/KsmEnhancedVerticalSetup.exe');args=parser.parse_args()
runtime=json.loads((ROOT/'build/native-installer-runtime-smoke.json').read_text(encoding='utf-8-sig'));target=(Path(runtime['fixture'])/'App').resolve();target.relative_to(ROOT/'build/native-installer-runtime')
assert not (target/'.ksm-enhanced-install.json').exists()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
protected={p.relative_to(target).as_posix():sha(p)for p in target.rglob('*')if p.is_file() and (p.suffix.lower()=='.ksc' or p.name in ('config.ini','skin_preset.json','audio_calibration_profiles.json') or any(x in p.relative_to(target).parts for x in ('skins','songs')))}
work=ROOT/'build/native-installer'/('legacy-cleanup-'+uuid.uuid4().hex);work.mkdir()
def run(exe,action):
 report=work/(action+'.json');p=subprocess.run([str(exe.resolve()),'--quiet','--action',action,'--target',str(target),'--report',str(report)],timeout=90);assert p.returncode==0,report.read_text(encoding='utf-8-sig');return json.loads(report.read_text(encoding='utf-8-sig'))
run(args.previous_setup,'install');state=json.loads((target/'.ksm-enhanced-install.json').read_text());assert 'createdDirectories' not in state
result=run(args.setup,'remove')
assert not (target/'mods/enhanced').exists() and not (target/'mods/vertical').exists() and not (target/'skin_preview').exists()
assert (target/'mods').is_dir(),'Legacy shared mods parent must remain'
for name,h in protected.items():assert sha(target/name)==h,name
report={'status':'PASS','method':'Actual prior production installer record, not synthetic metadata','protectedFiles':len(protected),'legacySharedModsRetained':True,'exclusiveAreasRemoved':True,'result':result}
(ROOT/'build/native-installer/legacy-directory-result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='result'},indent=2))
