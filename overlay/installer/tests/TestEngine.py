"""Exercise the actual Win32 installer engine in isolated fixtures, including crash recovery."""
from pathlib import Path
import ctypes as ct
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import time
import tempfile
import uuid

sys.stdout.reconfigure(encoding='utf-8')
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('build_package', ROOT/'installer/BuildPackage.py')
builder = importlib.util.module_from_spec(spec); spec.loader.exec_module(builder)
WORK = ROOT/'build/native-installer-tests'/uuid.uuid4().hex
WORK.mkdir(parents=True)
checks=[]
sha=lambda b: hashlib.sha256(b).hexdigest()
def check(value, message):
    assert value, message
    checks.append(message)
def put(path, data):
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(data.encode() if isinstance(data,str) else data)
def js(path): return json.loads(path.read_text(encoding='utf-8-sig'))
payload=WORK/'inputs'; payload.mkdir()
FILES={
 'ksm-enhanced-vertical.exe':b'MOD-BINARY',
 'mods/enhanced/ui/test.noco':b'MOD-UI',
 'imgs/bg_vertical.png':b'MOD-IMAGE',
 'imgs/new-image.png':b'NEW-IMAGE',
 'lang/Korean.json':b'{"label":"MOD"}',
 'bassmix.dll':b'MOD-DLL',
 'mods/vertical/deep/nested/new.noco':b'NEW-MOD-RESOURCE',
 'skin_preview/assets/example.dat':b'PREVIEW-DATA',
}
paths={}
for i,(name,data) in enumerate(FILES.items()):
    p=payload/str(i); put(p,data); paths[name]=p
targets={'schemaVersion':2,'modId':'ksm-enhanced-vertical','profiles':[
 {'id':'fixture','runtimeContract':'ksm-2.1.0-enhanced-skin-v2','executableHashes':[sha(b'ORIGINAL-A')],
  'requiredFiles':[{'path':'ui/scene/title.noco','sha256':sha(b'UI-A')}]}]}
EXE=WORK/'SetupTest.exe'
builder.assemble(ROOT/'build/native-installer/SetupTestStub.exe', EXE, paths, targets, fixture=True)
def fixture(name):
    p=WORK/name; p.mkdir()
    for key,data in {'kshootmania.exe':b'ORIGINAL-A','ui/scene/title.noco':b'UI-A',
                     'imgs/bg_vertical.png':b'USER-BEFORE','lang/Korean.json':b'{"label":"ORIGINAL"}',
                     'mods/enhanced/ui/test.noco':b'ORIGINAL-UI','config.ini':b'USER-SETTINGS',
                     'skin_preset.json':b'USER-PRESET','audio_calibration_profiles.json':b'USER-CALIBRATION',
                     'skins/My Skin/custom.png':b'USER-SKIN','songs/user/chart.ksh':b'USER-CHART',
                     'score/P1/chart.ksc':b'normal=1,2,3,4,5,6,7,8,9\n',
                     'score/P2/chart_additional.ksc':b'hard=1,2,3,4,5,6,7,8,9,20\n',
                     'score/P3/chart_additional.ksc':b'hard=1,2,3,4,5,6,7,8,9,20,30\n',
                     'score_backup/user.ksc':b'BACKUP','courses/score/P1/course.ksc':b'COURSE'}.items(): put(p/key,data)
    return p
def inventory(p):
    return {x.relative_to(p).as_posix():sha(x.read_bytes()) for x in p.rglob('*') if x.is_file()
            and not x.relative_to(p).parts[0].startswith('.ksm-enhanced')}
def run(target,action,success=True,env=None,exe=EXE):
    report=WORK/('report-'+uuid.uuid4().hex+'.json')
    e=os.environ.copy(); e.update(env or {})
    result=subprocess.run([str(exe),'--action',action,'--target',str(target),'--quiet','--report',str(report)],env=e,timeout=45)
    data=js(report) if report.exists() else {'code':'no_report'}
    if success is not None: check((result.returncode==0)==success,f'{action}: expected {"success" if success else "rejection"}: {target.name} ({data.get("code","")})')
    return data,result.returncode

try:
    p=fixture('한글 日本語 space'); original=inventory(p)
    run(p,'inspect'); check(inventory(p)==original,'Inspect is read-only')
    run(p,'install'); run(p,'verify')
    for name in original:
        if name not in FILES: check(sha((p/name).read_bytes())==original[name],'Installation protects '+name)
    run(p,'install',False)
    put(p/'score/P1/after.ksc',b'NEW-NORMAL'); put(p/'score/P4/after_additional.ksc',b'NEW-EX')
    put(p/'courses/score/P2/new.ksc',b'NEW-COURSE'); put(p/'config.ini',b'CHANGED-AFTER-INSTALL')
    put(p/'imgs/bg_vertical.png',b'EDITED-IMAGE'); (p/'imgs/new-image.png').unlink()
    put(p/'lang/Korean.json',b'{"label":"USER-EDITED"}'); put(p/'mods/enhanced/ui/test.noco',b'EDITED-UI')
    put(p/'mods/enhanced/user-extra.txt',b'NOT-OWNED')
    before_remove=inventory(p)
    data,_=run(p,'remove')
    check(not (p/'ksm-enhanced-vertical.exe').exists(),'Mod executable removed')
    check((p/'imgs/bg_vertical.png').read_bytes()==b'EDITED-IMAGE','Edited image stays in place')
    check(not (p/'imgs/new-image.png').exists(),'Deleted image is not recreated')
    check(sha((p/'lang/Korean.json').read_bytes())==original['lang/Korean.json'],'Original language restored')
    check(sha((p/'mods/enhanced/ui/test.noco').read_bytes())==original['mods/enhanced/ui/test.noco'],'Original other resource restored')
    recovery=p/data['recoveryDirectory']
    check((recovery/'edited/lang/Korean.json').read_bytes()==b'{"label":"USER-EDITED"}','Edited language copied to recovery')
    check((recovery/'edited/mods/enhanced/ui/test.noco').read_bytes()==b'EDITED-UI','Other edit copied to recovery')
    check((recovery/'before-install/imgs/bg_vertical.png').read_bytes()==b'USER-BEFORE','Before-install image backup retained')
    for name in before_remove:
        if name not in FILES: check(sha((p/name).read_bytes())==before_remove[name],'Removal protects '+name)
    run(p,'install'); check((p/'score/P4/after_additional.ksc').read_bytes()==b'NEW-EX','Reinstall retains post-install EX record')
    run(p,'remove'); check((p/'imgs/bg_vertical.png').read_bytes()==b'EDITED-IMAGE','Reinstall/removal restores the same user image')

    for fault in ('KSM_SETUP_FAIL_AFTER','KSM_SETUP_CRASH_AFTER'):
        p=fixture(fault); before=inventory(p)
        data,code=run(p,'install',False if fault.endswith('FAIL_AFTER') else None,env={fault:'2'})
        if fault.endswith('CRASH_AFTER'):
            check(code==93 and (p/'.ksm-enhanced-journal.json').exists(),'Forced termination retains durable journal')
            run(p,'inspect')
        check(inventory(p)==before,'Install rollback/restart restores all original files: '+fault)
        check(not (p/'.ksm-enhanced-install.json').exists(),'Failed install leaves no installation state')
        run(p,'install'); put(p/'lang/Korean.json',b'EDITED-LANGUAGE')
        before=inventory(p)
        data,code=run(p,'remove',False if fault.endswith('FAIL_AFTER') else None,env={fault:'2'})
        if fault.endswith('CRASH_AFTER'): run(p,'inspect')
        check(inventory(p)==before,'Removal rollback/restart restores installed/user state: '+fault)
        run(p,'remove')

    p=fixture('low-space'); before=inventory(p)
    run(p,'install',False,env={'KSM_SETUP_LOW_SPACE':'1'}); check(inventory(p)==before,'Disk-space rejection makes no payload changes')
    p=fixture('missing-executable');(p/'kshootmania.exe').unlink();before=inventory(p)
    for action in ('inspect','install'):
        data,_=run(p,action,False)
        check(data.get('code')=='missing_executable' and 'Cannot open file:' in data.get('message','') and str(p/'kshootmania.exe') in data['message'],'Missing executable has a dedicated error code and detailed path: '+action)
    check(inventory(p)==before,'Missing executable validation preserves all files')
    p=fixture('unknown'); put(p/'kshootmania.exe','WRONG'); before=inventory(p)
    run(p,'install',False); check(inventory(p)==before,'Unknown upstream is preserved')
    p=fixture('bad-resources'); put(p/'ui/scene/title.noco','DIFFERENT'); before=inventory(p)
    run(p,'install',False); check(inventory(p)==before,'Resource contract mismatch is preserved')
    p=fixture('manual'); put(p/'ksm-enhanced-vertical.exe','MANUAL'); before=inventory(p)
    run(p,'install',False); check(inventory(p)==before,'Manual installation is not adopted')
    p=fixture('locked-file'); before=inventory(p)
    kernel=ct.WinDLL('kernel32',use_last_error=True)
    kernel.CreateFileW.restype=ct.c_void_p
    kernel.CreateFileW.argtypes=[ct.c_wchar_p,ct.c_uint32,ct.c_uint32,ct.c_void_p,ct.c_uint32,ct.c_uint32,ct.c_void_p]
    kernel.CloseHandle.argtypes=[ct.c_void_p]
    handle=kernel.CreateFileW(str(p/'lang/Korean.json'),0x80000000,1,None,3,0,None)
    check(handle not in (None,ct.c_void_p(-1).value),'Fixture lock opened')
    try: run(p,'install',False)
    finally: kernel.CloseHandle(handle)
    check(inventory(p)==before,'Lock rejection leaves every original file unchanged')

    p=fixture('tampered-backup'); run(p,'install'); state=js(p/'.ksm-enhanced-install.json')
    entry=next(x for x in state['files'] if x['originalBackup'])
    put(p/entry['originalBackup'],b'DAMAGED'); before=inventory(p)
    run(p,'remove',False); check(inventory(p)==before,'Damaged original backup stops removal before mutation')

    for bad in ('score/user.ksc','courses/score/user.ksc','mods/enhanced/hidden.ksc','config.ini','skins/My Skin/custom.png','../escape','imgs/../score/user.ksc','C:/outside'):
        p=fixture('state-'+uuid.uuid4().hex[:8]); run(p,'install')
        state=js(p/'.ksm-enhanced-install.json'); state['files'][0]['path']=bad
        put(p/'.ksm-enhanced-install.json',json.dumps(state)); before=inventory(p)
        run(p,'remove',False); check(inventory(p)==before,'Tampered state cannot affect '+bad)

    p=fixture('legacy'); run(p,'install'); state=js(p/'.ksm-enhanced-install.json')
    state['schemaVersion']=2;state.pop('createdDirectories',None)
    for f in state['files']:
        f['sha256']=f.pop('installedHash').upper(); f['originalSha256']=f.pop('originalHash'); f['backup']=f.pop('originalBackup'); f['retainOnRemove']=False
        if f['originalSha256']: f['originalSha256']=f['originalSha256'].upper()
    put(p/'.ksm-enhanced-install.json',b'\xef\xbb\xbf'+json.dumps(state).encode())
    run(p,'verify'); run(p,'remove'); check(not (p/'ksm-enhanced-vertical.exe').exists(),'Legacy schema 2 record and uppercase hashes supported')

    corrupt=WORK/'Corrupt.exe'; b=bytearray(EXE.read_bytes()); b[-100]^=1; corrupt.write_bytes(b)
    p=fixture('corrupt-package'); before=inventory(p); run(p,'install',False,exe=corrupt)
    check(inventory(p)==before,'Corrupt embedded package rejected before mutation')

    p=fixture('progress');before=inventory(p)
    c=subprocess.run([str(ROOT/'build/native-installer/CancelEngine.exe'),str(EXE),str(p),'--progress'],timeout=45)
    check(c.returncode==0,'Install and remove progress are monotonic, stay below 100 before UI success, and reach 99 before commit')
    check(inventory(p)==before,'Progress callbacks preserve install/remove data policies')

    p=fixture('cancelled');before=inventory(p)
    c=subprocess.run([str(ROOT/'build/native-installer/CancelEngine.exe'),str(EXE),str(p)],timeout=45)
    check(c.returncode==0,'Real engine cancellation is requested after the first applied operation')
    check(inventory(p)==before and not (p/'.ksm-enhanced-journal.json').exists(),'Cancellation rolls back the transaction and removes its journal')
    run(p,'install');run(p,'remove')
    p=fixture('low-space-remove'); run(p,'install'); before=inventory(p)
    run(p,'remove',False,{'KSM_SETUP_LOW_SPACE':'1'}); check(inventory(p)==before,'Removal space check precedes target mutations')
    # The development drive does not support hardlinks; exercise this on the NTFS temporary volume.
    with tempfile.TemporaryDirectory(prefix='KsmSetupHardlink-') as temp:
        p=Path(temp)/'target'; shutil.copytree(fixture('hard-link'),p)
        peer=Path(temp)/'source';put(peer,b'USER-BEFORE')
        (p/'imgs/bg_vertical.png').unlink(); os.link(peer,p/'imgs/bg_vertical.png'); before=inventory(p)
        run(p,'install',False); check(peer.read_bytes()==b'USER-BEFORE' and inventory(p)==before,'Hard-linked target image is rejected')
    p=fixture('read-only-permission');before=inventory(p)
    kernel.SetFileAttributesW.argtypes=[ct.c_wchar_p,ct.c_uint32]
    check(bool(kernel.SetFileAttributesW(str(p/'lang/Korean.json'),1)),'Read-only permission fixture prepared')
    try:
        data,_=run(p,'install',False)
        check(data['code']=='permission','Write-permission error classified for GUI elevation')
    finally: kernel.SetFileAttributesW(str(p/'lang/Korean.json'),128)
    check(inventory(p)==before,'Permission rejection preserves every target file')
    p=fixture('duplicate-state'); run(p,'install'); state=js(p/'.ksm-enhanced-install.json');state['files'].append(state['files'][0])
    put(p/'.ksm-enhanced-install.json',json.dumps(state)); before=inventory(p)
    run(p,'remove',False); check(inventory(p)==before,'Duplicate ownership paths rejected')
    p=fixture('corrupt-journal'); run(p,'install',None,{'KSM_SETUP_CRASH_AFTER':'2'})
    journal=js(p/'.ksm-enhanced-journal.json');journal['operations'][0]['path']='score/normal.ksc'
    put(p/'.ksm-enhanced-journal.json',json.dumps(journal));before=inventory(p)
    run(p,'inspect',False);check(inventory(p)==before,'Interrupted recovery cannot target score files')
    p=fixture('identical-existing');put(p/'imgs/bg_vertical.png',FILES['imgs/bg_vertical.png']);run(p,'install')
    state=js(p/'.ksm-enhanced-install.json'); check(not next(f for f in state['files'] if f['path']=='imgs/bg_vertical.png')['owned'],'Identical image is not claimed')
    put(p/'imgs/bg_vertical.png',b'USER-LATER');run(p,'remove');check((p/'imgs/bg_vertical.png').read_bytes()==b'USER-LATER','Unowned edited image remains')

    # Directory ownership and nonrecursive cleanup.
    def fresh(name):
        p=fixture(name)
        (p/'mods/enhanced/ui/test.noco').unlink()
        for d in ('mods/enhanced/ui','mods/enhanced','mods'): (p/d).rmdir()
        return p
    p=fresh('new-folders');original=inventory(p);run(p,'install');state=js(p/'.ksm-enhanced-install.json')
    check('mods' in state['createdDirectories'] and 'mods/vertical/deep/nested' in state['createdDirectories'],'New installation records added directory ancestors')
    data,_=run(p,'remove')
    check(not (p/'mods').exists() and not (p/'skin_preview').exists(),'Recorded empty roots and nested mod directories removed')
    check(all((p/d).is_dir() for d in ('imgs','lang','ui','score','score_backup','courses/score','skins')),'Shared and protected directories preserved')
    check(inventory(p)==original,'Folder cleanup preserves original file hashes')
    depths=[x.count('/') for x in data['removedDirectories']]
    check(depths==sorted(depths,reverse=True) and not data['cleanupWarnings'],'Cleanup runs deepest first without warnings')
    check((p/'.ksm-enhanced-backup').is_dir(),'Installer backup tree retained')

    p=fresh('existing-empty-folders');(p/'mods/vertical/deep/nested').mkdir(parents=True);(p/'imgs/user-empty').mkdir();(p/'log/empty').mkdir(parents=True)
    run(p,'install');state=js(p/'.ksm-enhanced-install.json');check('mods/vertical/deep/nested' not in state['createdDirectories'],'Pre-existing empty directory is not claimed')
    run(p,'remove');check((p/'mods/vertical/deep/nested').is_dir() and (p/'imgs/user-empty').is_dir() and (p/'log/empty').is_dir(),'Original empty mod/common/user directories survive')

    for added in ('user-file','hidden-file','user-directory'):
        p=fresh(added);run(p,'install');base=p/'mods/vertical/deep/nested'
        if added=='user-directory':(base/'user-empty').mkdir()
        else:
            put(base/'user.txt',b'USER-DATA')
            if added=='hidden-file':check(bool(kernel.SetFileAttributesW(str(base/'user.txt'),2)),'Hidden-file fixture prepared')
        data,_=run(p,'remove')
        check(base.is_dir() and any(x['path']=='mods/vertical/deep/nested' and x['reason']=='not_empty' for x in data['preservedDirectories']),'User content prevents parent cleanup: '+added)
        if added!='user-directory':check((base/'user.txt').read_bytes()==b'USER-DATA','User file remains byte-identical: '+added)

    p=fresh('explicit-empty-directory-list');run(p,'install');state=js(p/'.ksm-enhanced-install.json');state['createdDirectories']=[];put(p/'.ksm-enhanced-install.json',json.dumps(state));run(p,'remove')
    check((p/'mods/vertical/deep/nested').is_dir(),'Explicit empty directory list does not enable legacy guessing')

    p=fresh('old-directory-record');run(p,'install');state=js(p/'.ksm-enhanced-install.json');state.pop('createdDirectories');put(p/'.ksm-enhanced-install.json',json.dumps(state));data,_=run(p,'remove')
    check(not (p/'mods/vertical').exists() and not (p/'mods/enhanced').exists() and not (p/'skin_preview').exists(),'Legacy record cleans only registered exclusive mod areas')
    check((p/'mods').is_dir(),'Legacy common mods parent is retained')

    for bad in (['score'],['skins'],['log'],['imgs'],['mods/unrelated'],['../outside'],['C:/outside'],['mods','MODS'],['mods/vertical/deep/nested','mods/vertical/../deep'],None,'mods', [42]):
        p=fresh('bad-directory-'+uuid.uuid4().hex[:8]);run(p,'install');state=js(p/'.ksm-enhanced-install.json');state['createdDirectories']=bad;put(p/'.ksm-enhanced-install.json',json.dumps(state));before=inventory(p)
        run(p,'remove',False);check(inventory(p)==before,'Malformed/unrelated/protected directory list rejected before file removal: '+str(bad))

    for protected in ('log','logs','settings','config','.ksm-enhanced-backup','.ksm-enhanced-recovery'):
        p=fresh('nested-protected-'+uuid.uuid4().hex[:8]);run(p,'install');state=js(p/'.ksm-enhanced-install.json')
        path='mods/enhanced/'+protected
        state['files'].append({'path':path+'/unowned.txt','owned':False,'installedHash':'0'*64})
        state['createdDirectories'].append(path);(p/path).mkdir(exist_ok=True)
        put(p/'.ksm-enhanced-install.json',json.dumps(state));before=inventory(p)
        run(p,'remove',False);check(inventory(p)==before and (p/path).is_dir(),'Independent directory denylist protects nested '+protected)

    for recovery_action in ('inspect','remove'):
        p=fresh('cleanup-crash-'+recovery_action);original=inventory(p);run(p,'install')
        _,code=run(p,'remove',None,{'KSM_SETUP_CLEANUP_CRASH_AFTER':'1'});check(code==94,'Forced cleanup termination occurs after file commit')
        check(not (p/'.ksm-enhanced-install.json').exists() and js(p/'.ksm-enhanced-journal.json')['phase']=='committed','Interrupted cleanup retains committed journal instead of restoring files')
        data,_=run(p,recovery_action);check(data.get('cleanupRecovered') and not (p/'mods').exists() and not (p/'skin_preview').exists(),'Restart resumes cleanup through '+recovery_action)
        check(inventory(p)==original and not (p/'.ksm-enhanced-journal.json').exists(),'Recovered cleanup is idempotent and preserves all original records')

    p=fresh('cleanup-permission');original=inventory(p);run(p,'install');data,_=run(p,'remove',env={'KSM_SETUP_DIRECTORY_DENY':'mods/vertical/deep/nested'})
    check(data['cleanupWarnings'] and (p/'mods/vertical/deep/nested').is_dir(),'Directory permission error leaves folder and reports warning')
    check(not (p/'ksm-enhanced-vertical.exe').exists() and inventory(p)==original,'Directory warning does not roll back completed mod removal')

    p=fresh('cleanup-racing-file');run(p,'install');data,_=run(p,'remove',env={'KSM_SETUP_DIRECTORY_RACE':'mods/vertical/deep/nested'})
    check((p/'mods/vertical/deep/nested/concurrent-file.txt').read_bytes()==b'RACE','File added at deletion time is preserved by the empty-only API')
    check(any(x['reason']=='not_empty' for x in data['preservedDirectories']),'Concurrent addition retains the folder')

    p=fresh('cleanup-rename-race');run(p,'install');data,_=run(p,'remove',env={'KSM_SETUP_DIRECTORY_RENAME_RACE':'mods/vertical/deep/nested'})
    check(data.get('guardedRenameAttempts')==1 and not (p/'mods').exists() and not (p/'mods/vertical/deep/nested.moved').exists(),'Open ancestor/leaf guards prevent directory replacement during cleanup')

    p=fresh('cleanup-real-directory-lock');original=inventory(p);run(p,'install')
    handle=kernel.CreateFileW(str(p/'mods/vertical/deep/nested'),0x80000000,3,None,3,0x02000000,None)
    check(handle not in (None,ct.c_void_p(-1).value),'Directory lock opened without delete sharing')
    try:data,_=run(p,'remove')
    finally:kernel.CloseHandle(handle)
    check(data['cleanupWarnings'] and (p/'mods/vertical/deep/nested').is_dir() and inventory(p)==original,'Real directory lock preserves folder, reports warning and retains original data')

    with tempfile.TemporaryDirectory(prefix='KsmSetupJunction-') as temp:
        p=Path(temp)/'target';shutil.copytree(fresh('junction-source'),p);outside=Path(temp)/'outside';outside.mkdir();put(outside/'do-not-delete.txt',b'OUTSIDE-USER-DATA')
        run(p,'install');link=p/'mods/vertical/deep/nested';(link/'new.noco').unlink();link.rmdir()
        quote=lambda x:"'"+str(x).replace("'","''")+"'"
        command='New-Item -ItemType Junction -Path '+quote(link)+' -Value '+quote(outside)+' | Out-Null'
        result=subprocess.run(['powershell.exe','-NoProfile','-NonInteractive','-Command',command],capture_output=True,timeout=30)
        check(result.returncode==0,'NTFS junction fixture created')
        try:
            data,_=run(p,'remove',False)
            check((p/'ksm-enhanced-vertical.exe').exists() and (outside/'do-not-delete.txt').read_bytes()==b'OUTSIDE-USER-DATA','Junction rejection occurs before removal and never touches the external target')
        finally:
            kernel.GetFileAttributesW.argtypes=[ct.c_wchar_p];kernel.GetFileAttributesW.restype=ct.c_uint32
            check(bool(kernel.GetFileAttributesW(str(link))&0x400),'Only the verified junction entry is removed from the test fixture')
            os.rmdir(link)
        check((outside/'do-not-delete.txt').read_bytes()==b'OUTSIDE-USER-DATA','Junction target remains unchanged after test cleanup')

    # A corrupted committed cleanup journal must never be used to delete user paths.
    p=fresh('cleanup-journal-protection');run(p,'install');run(p,'remove',None,{'KSM_SETUP_CLEANUP_CRASH_AFTER':'1'})
    journal=js(p/'.ksm-enhanced-journal.json');journal['directoryCleanup']=['score'];put(p/'.ksm-enhanced-journal.json',json.dumps(journal));before=inventory(p)
    run(p,'inspect',False);check(inventory(p)==before and (p/'score').is_dir(),'Tampered committed directory journal is rejected without touching user data')

    # Production must not honor fault-injection environment variables (tested by real-package suite).
    result={'status':'PASS','checks':len(checks),'cases':checks,'fixture':str(WORK),'engine':'Actual native EXE; test-only fault hooks compiled separately'}
except Exception as e:
    result={'status':'FAIL','checks':len(checks),'cases':checks,'fixture':str(WORK),'error':str(e)}
    raise
finally:
    (WORK/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='cases'},indent=2))
