"""Package a verified native installer without overwriting an existing release."""
from pathlib import Path
from datetime import datetime,timezone
import argparse,csv,hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
sha=lambda data:hashlib.sha256(data).hexdigest()
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
parser=argparse.ArgumentParser()
parser.add_argument('--setup',type=Path,required=True)
parser.add_argument('--production-evidence',type=Path,required=True)
parser.add_argument('--gui-evidence',type=Path,required=True)
parser.add_argument('--engine-evidence',type=Path,required=True)
parser.add_argument('--runtime-evidence',type=Path,default=ROOT/'build/native-installer-runtime-smoke.json')
parser.add_argument('--output-directory',type=Path,default=ROOT/'releases/ksm-enhanced-vertical-0.5.2-beta2-installer')
args=parser.parse_args();setup=args.setup.resolve();index=read(setup.with_suffix('.payload.json'))
customized=read(ROOT/'build/native-installer-customized-smoke.json')
build=read(ROOT/'build/latest.json');runtime=read(args.runtime_evidence);production=read(args.production_evidence);gui=read(args.gui_evidence);engine=read(args.engine_evidence)
assert all(report['status']=='PASS' for report in (runtime,customized,production,gui,engine))
assert Path(runtime['setup']).resolve()==setup and production['setupSha256']==sha(setup.read_bytes()) and gui['setupSha256']==sha(setup.read_bytes())
assert Path(customized['setup']).resolve()==setup and customized['customizedResources'] and customized['runtimeCases']==4
assert runtime['runtimeCases']==4 and runtime['emptyDirectoryCleanup'] and runtime['userSkinsAndRecordsPreserved']
assert index['version']=='0.5.2-beta2' and not index['fixture']
assert build['buildId']==index['buildId']==runtime['buildId'] and build['configuration']=='Release' and not build['diagnostics']
game_sha=sha(Path(build['executable']).read_bytes());assert game_sha==build['sha256'].lower()
assert next(f['sha256'] for f in index['files'] if f['path']=='ksm-enhanced-vertical.exe')==game_sha
assert all(p['requiredFiles']==[] and {f['path'] for f in p['requiredLibraries']}=={'bass.dll','bass_fx.dll'} for p in index['targets']['profiles'])
stub=(ROOT/'build/native-installer/SetupStub.exe').read_bytes()
assert setup.read_bytes().startswith(stub)
for token in ('KSM_SETUP_FAIL_AFTER','KSM_SETUP_CRASH_AFTER','KSM_SETUP_LOW_SPACE','KSM_SETUP_DIRECTORY_DENY','KSM_SETUP_DIRECTORY_RACE','KSM_SETUP_CLEANUP_CRASH_AFTER','KSM_SETUP_DIRECTORY_RENAME_RACE'):
 assert token.encode() not in stub and token.encode('utf-16le') not in stub
info={'product':'KSM Enhanced Vertical Mod','version':index['version'],'installerRevision':index['installerRevision'],
 'packageType':'Native Windows x64 installer','createdUtc':datetime.now(timezone.utc).isoformat(),
 'gameBuildId':index['buildId'],'gameConfiguration':'Release','diagnostics':False,'gameSha256':game_sha,
 'setupSha256':sha(setup.read_bytes()),'setupBytes':setup.stat().st_size,'payloadFileCount':len(index['files']),
 'supportedGame':'K-Shoot Mania 2.1.0 Windows x64','defaultInstallerLanguage':'Japanese','installerLanguages':['EN','JP','KR','SC','TC'],
 'endUserDependencies':'Built-in Windows APIs; no PowerShell, Python or .NET installation required',
 'originalExecutableIncluded':False,'ordinarySongsIncluded':False,'sdkIncluded':False,'userDataIncluded':False,
 'signature':'Unsigned test build','installExecutable':'ksm-enhanced-vertical.exe','removeExecutable':'KsmEnhancedVerticalSetup.exe',
 'updatePolicy':'Remove previous mod before installing; automatic overwrite updates are not supported',
 'recordPolicy':'Preserve all existing and newly earned normal, EX and course records, settings, songs and user skins',
 'imagePolicy':'Back up replaced originals; edited or deleted installed images are preserved in place on removal',
 'compatibilityPolicy':'Known original EXE identity; required x64 audio DLL structures and direct exports; no original hash requirement for user resources',
 'emptyDirectoryPolicy':'Recorded new directories only; empty-only deletion after commit; conservative legacy cleanup; protected paths retained'}
files={'KsmEnhancedVerticalSetup.exe':setup.read_bytes(),'BUILD_INFO.json':(json.dumps(info,indent=2)+'\n').encode()}
for name in ('README_KO.txt','README_EN.txt','README_JA.txt'):
 data=(ROOT/'installer/readme'/name).read_bytes();assert index['version'].encode() in data;assert b'0.5.1-beta1' not in data;files[name]=data
out=args.output_directory.resolve();assert not out.exists(),'Never overwrite an existing release';out.mkdir(parents=True)
archive=out/('ksm-enhanced-vertical-'+index['version']+'-installer.zip')
with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for name,data in files.items():z.writestr(name,data)
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None and set(z.namelist())==set(files)
 for name,data in files.items():assert sha(z.read(name))==sha(data)
for name in ('README_KO.txt','README_EN.txt','README_JA.txt','BUILD_INFO.json'):(out/name).write_bytes(files[name])
with (out/'FILES_SHA256.csv').open('w',encoding='utf-8',newline='') as f:
 writer=csv.writer(f);writer.writerow(['Path','Bytes','SHA256']);writer.writerows((n,len(d),sha(d)) for n,d in files.items())
verification={'status':'PASS','version':index['version'],'installerRevision':index['installerRevision'],
 'archiveIntegrity':'PASS','archiveFileHashes':'PASS','gameSha256':game_sha,'gameBuildId':index['buildId'],
 'setupSha256':info['setupSha256'],'zipBytes':archive.stat().st_size,'zipSha256':sha(archive.read_bytes()),
 'nativeEngine':engine,'realRuntime':runtime,'customizedRuntime':customized,'productionCompatibility':production,'gui':gui,'payload':index['files'],
 'limitations':['DLL metadata and export checks do not guarantee behavior of every third-party DLL build.',
 'Interactive UAC elevation was not re-tested. Existing target/action handoff and revalidation remain unchanged.',
 'Real ASIO hardware was not re-tested; this update does not change audio code.',
 'Fault tests use a separate test build; production runtime tests use the exact packaged installer.',
 'Game build retains existing conversion/import/PDB warnings; Release build and normal runtime checks passed.']}
(out/'VERIFICATION.json').write_text(json.dumps(verification,indent=2)+'\n',encoding='utf-8')
archive.with_suffix('.zip.sha256').write_text(verification['zipSha256']+'  '+archive.name+'\n',encoding='ascii')
print(json.dumps({k:verification[k] for k in ('status','version','zipBytes','zipSha256','gameBuildId')},indent=2));print(archive)
