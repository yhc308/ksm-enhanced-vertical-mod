"""Validate the production installer with real old packages and exact normal EXE.
All visible game/GUI work runs on a private desktop that is never switched to.
"""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,uuid,zipfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--setup',type=Path,required=True);p.add_argument('--build-information',type=Path,required=True);p.add_argument('--original',type=Path,required=True);args=p.parse_args()
setup=args.setup.resolve();original=args.original.resolve();build=json.loads(args.build_information.read_text(encoding='utf-8-sig'));work=ROOT/'build/native-installer-production'/uuid.uuid4().hex;work.mkdir(parents=True);checks=[];runtime=[];gui=[];upgrades=[]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def check(ok,message):
 if not ok:raise AssertionError(message)
 checks.append(message);print('PASS '+message,flush=True)
def protected(app):
 return {p.relative_to(app).as_posix():sha(p) for p in app.rglob('*') if p.is_file() and (p.suffix.lower()=='.ksc' or p.name in ('config.ini','skin_preset.json','audio_calibration_profiles.json') or 'skins' in p.relative_to(app).parts)}
def call(exe,app,action,success=True):
 before=protected(app);report=work/(action+'-'+uuid.uuid4().hex+'.json');proc=subprocess.run([str(exe),'--quiet','--action',action,'--target',str(app),'--report',str(report)],timeout=120);data=json.loads(report.read_text(encoding='utf-8-sig'));check((proc.returncode==0)==success,action+' expected '+str(success)+' '+app.parent.name+' '+data.get('message',''));check(protected(app)==before,action+' preserves settings, skins and every record');return data
base=work/'base';base.mkdir()
for folder in ('assets','dll','engine','imgs','lang','se','shaders','ui'):
 src=original/folder
 check(src.is_dir(),'Original input directory '+folder)
 check(not any(getattr(p.lstat(),'st_file_attributes',0)&0x400 for p in src.rglob('*')),'Original runtime input has no reparse points '+folder)
 shutil.copytree(src,base/folder)
for name in ('kshootmania.exe','bass.dll','bass_fx.dll'):shutil.copyfile(original/name,base/name)
def fixture(name,custom=False):
 app=work/name/'App';shutil.copytree(base,app)
 values={'config.ini':'[config]\ncurrentlang=Japanese\nmastervol=0\naudio_backend=default\nvertical_enabled=0\nwindowsize=1280,720\nfullscreen=0\ninputdelay=7\nglobaloffset=3\nlaserdelay=-2\n', 'skin_preset.json':'{"folder":"default","keyBeamMode":0}', 'audio_calibration_profiles.json':'{"version":1,"profiles":{}}','skins/User Skin/skin.json':'{"author":"fixture"}','songs/user/user.ksh':'USER SONG','score/P1/a.ksc':'normal=1,2,3,4,5,6,7,8,9\n','score/P1/a_additional.ksc':'normal=1,2,3,4,5,6,7,8,9,20\n','score/P2/a_additional.ksc':'hard=1,2,3,4,5,6,7,8,9,20,30\n','courses/score/P1/c.ksc':'COURSE','score_backup/u.ksc':'BACKUP'}
 for n,text in values.items():p=app/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text,encoding='utf8')
 if custom:
  with (app/'imgs/base.gif').open('ab') as f:f.write(b'USER METADATA')
  for name in ('ui/scene/title.noco','ui/scene/select.noco','ui/scene/result.noco','ui/scene/play_hud.noco'):(app/name).unlink(missing_ok=True)
 return app
host=ROOT/'build/update-054/NormalRuntimeHost.exe';check(host.is_file(),'Private desktop runtime host exists')
def startup(app,vertical,chart=False):
 config=app/'config.ini';s=config.read_text(encoding='utf-8-sig')
 for key,value in {'vertical_enabled':str(vertical),'windowsize':'720,1280' if vertical else '1280,720','vertical_resolution':'720,1280'}.items():
  s=re.sub(r'(?m)^'+key+r'=.*$',key+'='+value,s) if re.search(r'(?m)^'+key+r'=',s) else s+'\n'+key+'='+value+'\n'
 config.write_text(s,encoding='utf8')
 flags=['-auto','-from','0',str(app/'mods/enhanced/judge_setting/Preview_Piano.kson')] if chart else []
 result=subprocess.run([str(host),str(app/'ksm-enhanced-vertical.exe'),str(app),*flags],capture_output=True,text=True,encoding='utf8',errors='replace',timeout=75)
 (work/('startup-'+uuid.uuid4().hex+'.txt')).write_text(result.stdout+result.stderr,encoding='utf8');check(result.returncode==0,'Normal exact EXE startup/exit '+str(vertical)+' chart='+str(chart)+' '+result.stdout.strip());runtime.append({'status':'PASS','vertical':vertical,'chart':chart,'privateDesktop':True})
try:
 for custom in (False,True):
  app=fixture('customized' if custom else 'clean',custom);before={p.relative_to(base).as_posix():sha(p) for p in base.rglob('*') if p.is_file()};call(setup,app,'install');check(sha(app/'ksm-enhanced-vertical.exe')==build['sha256'].lower(),'Exact normal Release installed');call(setup,app,'verify')
  for v in (0,1):
   for chart in (False,True):startup(app,v,chart)
  call(setup,app,'remove');check(not(app/'ksm-enhanced-vertical.exe').exists(),'Normal mod removed');call(setup,app,'install');call(setup,app,'remove')
 # Use the preserved, actually distributed old EXEs rather than synthetic state alone.
 for folder in ('ksm-enhanced-vertical-0.4.0-beta.1-installer','ksm-enhanced-vertical-0.5.1-beta1-installer','ksm-enhanced-vertical-0.5.1-beta1-installer-cleanup1','ksm-enhanced-vertical-0.5.2-beta2-installer'):
  archive=next((ROOT/'releases'/folder).glob('*.zip'));old=work/(folder+'.exe')
  with zipfile.ZipFile(archive) as z:old.write_bytes(z.read('KsmEnhancedVerticalSetup.exe'))
  app=fixture(folder);baseline={p.relative_to(base).as_posix():sha(p)for p in base.rglob('*')if p.is_file()};call(old,app,'install');(app/'score/new_additional.ksc').write_text('normal=1,2,3,4,5,6,7,8,9,20,30',encoding='utf8');state=call(setup,app,'inspect');check(state['updateAvailable'],'Actual old package supports update '+folder);call(setup,app,'update');check(sha(app/'ksm-enhanced-vertical.exe')==build['sha256'].lower(),'Actual upgrade uses latest normal EXE');startup(app,0);call(setup,app,'remove')
  check(all((app/n).is_file() and sha(app/n)==h for n,h in baseline.items()),'Actual upgrade removal restores all original runtime files '+folder);check((app/'score/new_additional.ksc').is_file(),'New EX survives actual upgrade '+folder);call(setup,app,'install');call(setup,app,'remove');upgrades.append({'status':'PASS','previousPackage':folder,'oldSetupSha256':sha(old)})
 # Observe the actual production GUI on its own invisible desktop in all five languages.
 app=fixture('gui');old=work/'ksm-enhanced-vertical-0.5.2-beta2-installer.exe';call(old,app,'install')
 for language,label in enumerate(('Update mod','モードを更新','모드 업데이트','更新模组','更新模組')):
  env=os.environ.copy();env['KSM_HOST_EXPECT_TITLE']='Enhanced Vertical Mod'
  result=subprocess.run([str(host),str(setup),str(app),'--target',str(app),'--language',str(language)],env=env,capture_output=True,text=True,encoding='utf8',errors='replace',timeout=75)
  check(result.returncode==0 and 'installEnabled=1, removeEnabled=1' in result.stdout and label in result.stdout,'Production GUI update button language '+str(language));gui.append({'status':'PASS','language':language,'label':label,'privateDesktop':True})
 call(setup,app,'update');call(setup,app,'remove')
 result={'status':'PASS','setup':str(setup),'setupSha256':sha(setup),'buildId':build['buildId'],'fixture':str(work),'checks':len(checks),'runtimeCases':runtime,'upgrades':upgrades,'gui':gui,'limitations':['Exact normal startup runs on a private, never-switched desktop; interactive-desktop visual layout is checked with the separate diagnostic build.','Physical input and analog output latency are not measured; TOPPING +15ms was confirmed by the user.']}
except Exception as e:
 result={'status':'FAIL','setup':str(setup),'fixture':str(work),'checks':len(checks),'error':str(e),'runtimeCases':runtime,'upgrades':upgrades,'gui':gui};raise
finally:
 (work/'results.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8');(ROOT/'build/native-installer-production-latest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8');print(json.dumps({k:v for k,v in result.items() if k not in ('runtimeCases','upgrades','gui')},ensure_ascii=False,indent=2),flush=True)
