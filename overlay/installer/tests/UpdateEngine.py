"""Exercise native update transactions using actual old/new embedded packages."""
from pathlib import Path
import runpy,json,uuid
ctx=runpy.run_path(str(Path(__file__).with_name('TestEngine.py')))
globals().update({k:ctx[k] for k in ('ROOT','WORK','builder','paths','targets','fixture','inventory','run','check','checks','put','js','sha')})
newfiles=dict(paths);inputs=WORK/'upgrade-inputs';inputs.mkdir()
for name,data in {'ksm-enhanced-vertical.exe':b'UPDATED-MOD','lang/Korean.json':b'{"label":"UPDATED"}','imgs/bg_vertical.png':b'UPDATED-IMAGE','mods/enhanced/new.txt':b'NEW-VERSION-FILE'}.items():
 p=inputs/str(len(list(inputs.iterdir())));p.write_bytes(data);newfiles[name]=p
newfiles.pop('skin_preview/assets/example.dat')
def package(version,files,tag):
 exe=WORK/(tag+'.exe');builder.assemble(ROOT/'build/native-installer/SetupTestStub.exe',exe,files,targets,fixture=True,version=version,build_id=tag);return exe
new=package('0.5.4-beta4',newfiles,'upgrade-new')
try:
 for version,schema in [('0.4.0-beta.1',2),('0.5.1-beta1',3),('0.5.2-beta2',3)]:
  p=fixture('upgrade-'+version);original=inventory(p);old=package(version,paths,'old-'+version);run(p,'install',exe=old)
  state=js(p/'.ksm-enhanced-install.json');state['schemaVersion']=schema
  if schema==2:
   state.pop('createdDirectories',None)
   for f in state['files']:
    f['sha256']=f.pop('installedHash');f['originalSha256']=f.pop('originalHash');f['backup']=f.pop('originalBackup')
  put(p/'.ksm-enhanced-install.json',json.dumps(state));data,_=run(p,'inspect',exe=new);check(data['updateAvailable'],'Supported old version detected: '+version)
  run(p,'update',exe=new);check(js(p/'.ksm-enhanced-install.json')['schemaVersion']==4,'Update writes schema 4')
  check((p/'ksm-enhanced-vertical.exe').read_bytes()==b'UPDATED-MOD','Updated executable applied');check(not(p/'skin_preview/assets/example.dat').exists(),'Retired owned file removed')
  run(p,'update',False,exe=new);run(p,'update',False,exe=old)
  run(p,'remove',exe=new);check(inventory(p)==original,'Earliest original restored after '+version+' update');run(p,'install',exe=new);run(p,'remove',exe=new);check(inventory(p)==original,'Reinstall after upgrade removal preserves originals')
 # Same public version: accept a strictly newer timestamped build only.
 sameOld=package('0.5.4-beta4',paths,'20261105-180035-32156be9')
 sameNew=package('0.5.4-beta4',newfiles,'20261105-195949-671d1730')
 p=fixture('same-version-hotfix');original=inventory(p);run(p,'install',exe=sameOld)
 put(p/'imgs/bg_vertical.png',b'USER-HOTFIX-IMAGE');put(p/'score/hotfix_additional.ksc',b'NEW-HOTFIX-EX')
 data,_=run(p,'inspect',exe=sameNew);check(data['updateAvailable'],'Same-version newer build is updateable')
 run(p,'update',exe=sameNew);check((p/'ksm-enhanced-vertical.exe').read_bytes()==b'UPDATED-MOD','Same-version hotfix replaces executable')
 check((p/'imgs/bg_vertical.png').read_bytes()==b'USER-HOTFIX-IMAGE' and (p/'score/hotfix_additional.ksc').read_bytes()==b'NEW-HOTFIX-EX','Hotfix preserves edited image and new EX record')
 run(p,'update',False,exe=sameNew);run(p,'update',False,exe=sameOld)
 unknown=package('0.5.4-beta4',newfiles,'unversioned-manual');run(p,'update',False,exe=unknown)
 equal=package('0.5.4-beta4',newfiles,'20261105-195949-ffffffff');run(p,'update',False,exe=equal)
 run(p,'remove',exe=sameNew);check((p/'lang/Korean.json').read_bytes()==b'{"label":"ORIGINAL"}','Same-version hotfix removal restores earliest original language')
 # Two upgrades must carry the first-install baseline through both generations.
 p=fixture('two-updates');original=inventory(p);first=package('0.4.0-beta.1',paths,'chain-first');middle=package('0.5.1-beta1',newfiles,'chain-middle');run(p,'install',exe=first);run(p,'update',exe=middle);run(p,'update',exe=new);run(p,'remove',exe=new);check(inventory(p)==original,'Two upgrades restore the first-install baseline')
 old=package('0.5.2-beta2',paths,'edits-old')
 p=fixture('upgrade-user-edits');run(p,'install',exe=old);put(p/'imgs/bg_vertical.png',b'USER-EDIT');(p/'imgs/new-image.png').unlink();put(p/'lang/Korean.json',b'USER-LANGUAGE');put(p/'score/new_additional.ksc',b'NEW-EX');before=inventory(p)
 report,_=run(p,'update',exe=new);check((p/'imgs/bg_vertical.png').read_bytes()==b'USER-EDIT' and not(p/'imgs/new-image.png').exists(),'Update preserves edited and deleted images');check((p/report['recoveryDirectory']/'edited/lang/Korean.json').read_bytes()==b'USER-LANGUAGE','Updated edited language has recovery copy')
 run(p,'remove',exe=new);check((p/'imgs/bg_vertical.png').read_bytes()==b'USER-EDIT','Remove after update retains edited image');check((p/'score/new_additional.ksc').read_bytes()==b'NEW-EX','Post-install EX survives update and remove');check((p/'lang/Korean.json').read_bytes()==b'{"label":"ORIGINAL"}','Remove after update restores original language')
 for fault in ('KSM_SETUP_FAIL_AFTER','KSM_SETUP_CRASH_AFTER'):
  p=fixture('upgrade-'+fault);run(p,'install',exe=old);before=inventory(p);state=(p/'.ksm-enhanced-install.json').read_bytes();report,code=run(p,'update',False if 'FAIL' in fault else None,env={fault:'2'},exe=new)
  if 'CRASH' in fault:check(code==93,'Update crash injected');run(p,'inspect',exe=new)
  check(inventory(p)==before and (p/'.ksm-enhanced-install.json').read_bytes()==state,'Failed/interrupted update restores old runtime and state: '+fault);run(p,'update',exe=new);run(p,'remove',exe=new)
 p=fixture('upgrade-low-space');run(p,'install',exe=old);before=inventory(p);run(p,'update',False,{'KSM_SETUP_LOW_SPACE':'1'},exe=new);check(inventory(p)==before,'Low-space update changes no runtime files')
 p=fixture('upgrade-bad-backup');run(p,'install',exe=old);state=js(p/'.ksm-enhanced-install.json');entry=next(f for f in state['files'] if f['originalBackup']);put(p/entry['originalBackup'],b'DAMAGED');before=inventory(p);run(p,'update',False,exe=new);check(inventory(p)==before,'Damaged original backup blocks update before changes')
 p=fixture('upgrade-cancel');run(p,'install',exe=old);before=inventory(p)
 import subprocess
 cancelled=subprocess.run([str(ROOT/'build/native-installer/CancelEngine.exe'),str(new),str(p),'--update'],timeout=45)
 check(cancelled.returncode==0 and inventory(p)==before,'Update cancellation rolls back after first applied operation')
 for index in range(1,7):
  p=fixture('update-fail-boundary-'+str(index));run(p,'install',exe=old);before=inventory(p);state=(p/'.ksm-enhanced-install.json').read_bytes();run(p,'update',False,{'KSM_SETUP_FAIL_AFTER':str(index)},exe=new)
  check(inventory(p)==before and (p/'.ksm-enhanced-install.json').read_bytes()==state,'Update rollback at operation boundary '+str(index))
 result={'status':'PASS','checks':len(checks),'fixture':str(WORK),'engine':'Native schema 2/3/4 install/update/remove and real fault injection'}
except Exception as e:
 result={'status':'FAIL','checks':len(checks),'fixture':str(WORK),'error':str(e)};raise
finally:
 (WORK/'update-results.json').write_text(json.dumps(result,indent=2)+'\n');(ROOT/'build/native-installer-update-latest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
