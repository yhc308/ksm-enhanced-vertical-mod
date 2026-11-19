"""Installer-only updates and protected skins-folder transaction regressions."""
from pathlib import Path
import runpy,json,shutil,subprocess,uuid,tempfile
ctx=runpy.run_path(str(Path(__file__).with_name('TestEngine.py')))
globals().update({k:ctx[k] for k in ('ROOT','WORK','builder','paths','targets','fixture','inventory','run','check','checks','put','js','EXE')})

def absent(p):
 q=(p/'skins').resolve();q.relative_to(WORK.resolve());shutil.rmtree(q)

def package(revision):
 old=builder.RELEASE['installerRevisionNumber'];builder.RELEASE['installerRevisionNumber']=revision
 try:
  exe=WORK/('revision-'+str(revision)+'.exe');builder.assemble(ROOT/'build/native-installer/SetupTestStub.exe',exe,paths,targets,fixture=True);return exe
 finally:builder.RELEASE['installerRevisionNumber']=old
try:
 old=package(0);new=package(1)
 p=fixture('empty-skins');absent(p);original=inventory(p);run(p,'inspect');check(not(p/'skins').exists(),'Inspect never creates skins')
 run(p,'install',exe=new);check((p/'skins').is_dir(),'Install prepares empty skins');run(p,'remove',exe=new);check((p/'skins').is_dir(),'Removal preserves empty skins');check(inventory(p)==original,'Removal restores files and preserves user data')
 p=fixture('existing-skins');before=inventory(p);run(p,'install');run(p,'remove');check(inventory(p)==before,'Existing skin contents unchanged')
 p=fixture('skins-name-file');absent(p);put(p/'skins',b'USER FILE');before=inventory(p);result,_=run(p,'install',False);check(result['code']=='skins_directory' and inventory(p)==before,'File conflict blocks before managed file changes')
 p=fixture('installer-only-update');run(p,'install',exe=old);state=js(p/'.ksm-enhanced-install.json');state.pop('installerRevisionNumber');put(p/'.ksm-enhanced-install.json',json.dumps(state));absent(p)
 result,_=run(p,'inspect',exe=new);check(result['updateAvailable'],'Missing revision number reads as zero and enables installer-only update')
 run(p,'update',exe=new);check(js(p/'.ksm-enhanced-install.json')['installerRevisionNumber']==1 and(p/'skins').is_dir(),'Same game build update creates skins and records revision')
 run(p,'update',False,exe=new);run(p,'update',False,exe=old);run(p,'remove',exe=new)
 p=fixture('skins-rollback');absent(p);before=inventory(p);run(p,'install',False,{'KSM_SETUP_SKINS_FAIL':'1'});check(inventory(p)==before and not(p/'skins').exists(),'Failure rolls back managed files and owned empty skins')
 for keep in (False,True):
  p=fixture('skins-crash-'+str(keep));absent(p);before=inventory(p);_,code=run(p,'install',None,{'KSM_SETUP_SKINS_CRASH':'1'});check(code==94 and(p/'skins').is_dir(),'Crash injected after durable directory identity')
  if keep:put(p/'skins/user-after-crash.txt',b'USER ADDED')
  run(p,'inspect');check((p/'skins').exists()==keep,'Recovery removes only owned empty skins; user additions survive')
  if keep:check((p/'skins/user-after-crash.txt').read_bytes()==b'USER ADDED','Recovery preserves user-created content')
  else:check(inventory(p)==before,'Recovery restores pre-install files')
 p=fixture('tampered-skins-journal');absent(p);run(p,'install',None,{'KSM_SETUP_SKINS_CRASH':'1'});j=js(p/'.ksm-enhanced-journal.json');j['skinsDirectory']['path']='score';put(p/'.ksm-enhanced-journal.json',json.dumps(j));before=inventory(p);result,_=run(p,'inspect',False);check(result['code']=='journal' and inventory(p)==before,'Forged directory path rejected before rollback')
 source=fixture('skins-junction');absent(source);ntfs=Path(tempfile.mkdtemp(prefix='KsmSkinsJunction-'));p=ntfs/'App';shutil.copytree(source,p);external=ntfs/'external-skin-data';put(external/'keep.png',b'EXTERNAL USER DATA')
 command=subprocess.run(['cmd','/d','/c','mklink','/J',str(p/'skins'),str(external)],capture_output=True)
 check(command.returncode==0,'Create isolated junction fixture');before=(external/'keep.png').read_bytes();run(p,'install');run(p,'remove');check((p/'skins').is_junction() if hasattr(Path,'is_junction') else bool((p/'skins').stat()),'User junction remains');check((external/'keep.png').read_bytes()==before,'Junction target unchanged')
 result={'status':'PASS','checks':len(checks),'fixture':str(WORK),'cases':checks}
except Exception as e:result={'status':'FAIL','checks':len(checks),'fixture':str(WORK),'error':str(e)};raise
finally:
 (ROOT/'build/installer-refresh-engine-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
