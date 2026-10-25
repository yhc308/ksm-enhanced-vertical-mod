"""Create an immutable installer ZIP and English external verification artifacts."""
from pathlib import Path
import csv, hashlib, io, json, zipfile, argparse
from datetime import datetime, timezone
ROOT=Path(__file__).resolve().parents[1]
sha=lambda data:hashlib.sha256(data).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
parser=argparse.ArgumentParser()
parser.add_argument('--setup',type=Path,default=ROOT/'build/native-installer/cleanup-revision-3/KsmEnhancedVerticalSetup.exe')
parser.add_argument('--output-directory',type=Path,default=ROOT/'releases/ksm-enhanced-vertical-0.5.1-beta1-installer-cleanup1')
args=parser.parse_args()
setup=args.setup.resolve()
index=read(setup.with_suffix('.payload.json'))
runtime=read(ROOT/'build/native-installer-runtime-smoke.json')
gui=read(ROOT/'build/native-installer/ui-revision-evidence.json')
wording=read(ROOT/'build/native-installer/ui2-wording-evidence.json')
cleanupGui=read(ROOT/'build/native-installer/cleanup-gui-evidence.json')
cleanupRun=read(ROOT/'build/native-installer/gui-cleanup-result.json')
legacyRun=read(ROOT/'build/native-installer/legacy-directory-result.json')
assert cleanupGui['status']=='PASS' and cleanupRun['status']=='PASS' and legacyRun['status']=='PASS'
assert runtime.get('emptyDirectoryCleanup') and Path(runtime['setup']).resolve()==setup
assert wording['status']=='PASS' and '선택된 경로에 kshootmania.exe가 없어' in wording['tree']
assert gui['status']=='PASS' and gui['install']['progress']==100 and gui['remove']['progress']==100 and gui['failure']['progress']<100
engine=read(max((ROOT/'build/native-installer-tests').glob('*/results.json'),key=lambda p:p.stat().st_mtime))
assert runtime['status']=='PASS' and runtime['runtimeCases']==4 and engine['status']=='PASS'
assert index['fixture'] is False
stub=(ROOT/'build/native-installer/SetupStub.exe').read_bytes()
for token in ('KSM_SETUP_FAIL_AFTER','KSM_SETUP_CRASH_AFTER','KSM_SETUP_LOW_SPACE','KSM_SETUP_DIRECTORY_DENY','KSM_SETUP_DIRECTORY_RACE','KSM_SETUP_CLEANUP_CRASH_AFTER','KSM_SETUP_DIRECTORY_RENAME_RACE'):
 assert token.encode('utf-16le') not in stub and token.encode() not in stub, 'Test hook present in production stub'
assert setup.read_bytes().startswith(stub), 'Packaged executable does not match current production stub'
assert index['buildId']=='20261023-234835-524ce9c3'
assert sha((ROOT/'build/bin'/index['buildId']/'ksm-enhanced-vertical.exe').read_bytes())=='f5e014b9e6de87037403a49f1652c9b6de308a61de49bee4b66100882d232f70'
out=args.output_directory.resolve()
archive=out/'ksm-enhanced-vertical-0.5.1-beta1-installer.zip'
assert not out.exists(), 'Never overwrite an existing installer release'
info={'product':'KSM Enhanced Vertical Mod','version':'0.5.1-beta1','packageType':'Native Windows x64 installer',
      'createdUtc':datetime.now(timezone.utc).isoformat(),'gameBuildId':index['buildId'],'gameConfiguration':'Release','diagnostics':False,
      'gameSha256':'f5e014b9e6de87037403a49f1652c9b6de308a61de49bee4b66100882d232f70',
      'setupSha256':sha(setup.read_bytes()),'setupBytes':setup.stat().st_size,'payloadFileCount':len(index['files']),
      'supportedGame':'K-Shoot Mania 2.1.0 Windows x64','defaultInstallerLanguage':'Japanese','installerLanguages':['EN','JP','KR','SC','TC'],
      'endUserDependencies':'Built-in Windows APIs; no PowerShell, Python or .NET installation required',
      'installExecutable':'ksm-enhanced-vertical.exe','removeExecutable':'Use the same KsmEnhancedVerticalSetup.exe',
      'originalExecutableIncluded':False,'ordinarySongsIncluded':False,'sdkIncluded':False,'userDataIncluded':False,
      'signature':'Unsigned test build','gameSettingsPolicy':'Never overwritten or restored by setup',
      'recordPolicy':'Preserve all existing and newly earned normal, EX and course records',
      'imagePolicy':'Preserve user-edited or deleted images in place',
      'languagePolicy':'Restore pre-install originals; preserve edited copies in recovery',
      'automaticUpdates':False,'installerRevision':'20261026-cleanup1','icon':'Original package/download icon, seven sizes','progressDisplay':'Determinate file progress; successful install/remove finish at 100 percent','emptyDirectoryPolicy':'Recorded installation-created directories only; empty-only deletion after file commit; limited exclusive areas for legacy records; shared/user/backup directories preserved'}
files={'KsmEnhancedVerticalSetup.exe':setup.read_bytes(),'BUILD_INFO.json':(json.dumps(info,indent=2)+'\n').encode()}
for name in ('README_KO.txt','README_EN.txt','README_JA.txt'):files[name]=(ROOT/'installer/readme'/name).read_bytes()
out.mkdir(parents=True)
for name in ('README_KO.txt','README_EN.txt','README_JA.txt'):(out/name).write_bytes(files[name])
with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for name,data in files.items():z.writestr(name,data)
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 assert set(z.namelist())==set(files)
 for name,data in files.items():assert sha(z.read(name))==sha(data)
with (out/'FILES_SHA256.csv').open('w',newline='',encoding='utf-8') as f:
 w=csv.writer(f);w.writerow(['Path','Bytes','SHA256']);w.writerows((name,len(data),sha(data)) for name,data in files.items())
verification={'status':'PASS','archiveIntegrity':'PASS','archiveFileHashes':'PASS','fixedNormalReleaseBuild':'PASS',
 'nativeEngineChecks':engine['checks'],'nativeEngineCases':engine['cases'],
 'realRuntime':{k:v for k,v in runtime.items() if k not in ('fixture','setup')},
 'automaticTargetDetection':'PASS; module directory used independently of working directory',
 'guiReview':'Prior completion/icon/wording baselines retained. Current production GUI inspected with a real directory lock: removal finished at 100 percent, warning listed the retained folder, and protected records/settings stayed byte-identical.',
 'originalData':'Only isolated copies were installed; development runtime, play-test and original runtime were not installed into',
 'limitations':['Interactive UAC consent round trip was not exercised; elevation launch preserves target/action and revalidates.',
               'Folder selection and physical install/remove button clicks were not automated; actual GUI actions were started with --gui-action and the visible results inspected.',
               'Low disk space and interrupted writes were injected in a separate test build; no real disk was filled.',
               'Native engine fault tests use synthetic payloads; normal game startup tests use the production payload.',
               'ASIO hardware and other Windows installations were not re-tested; the fixed game executable was not changed.'],
 'zipBytes':archive.stat().st_size,'zipSha256':sha(archive.read_bytes()),'setupSha256':info['setupSha256'],
 'payloadFileCount':len(index['files']),'payload':index['files'],'installerRevision':'20261026-cleanup1','guiEvidence':gui,'wordingEvidence':wording,'cleanupGuiEvidence':cleanupGui,'cleanupProductionRun':cleanupRun,'legacyDirectoryRun':legacyRun}
(out/'VERIFICATION.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
(archive.with_suffix('.zip.sha256')).write_text(verification['zipSha256']+'  '+archive.name+'\n',encoding='ascii')
print(json.dumps({k:verification[k] for k in ('status','zipBytes','zipSha256','setupSha256','nativeEngineChecks')},indent=2))
print(archive)
