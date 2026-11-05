"""Package a validated update-capable native setup; never overwrite a release."""
from pathlib import Path
from datetime import datetime,timezone
import argparse,csv,hashlib,json,zipfile
ROOT=Path(__file__).resolve().parents[1]
def read(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))
def sha(b):return hashlib.sha256(b).hexdigest()
p=argparse.ArgumentParser();p.add_argument('--setup',type=Path,required=True);p.add_argument('--validation',type=Path,required=True);p.add_argument('--build-information',type=Path,required=True);p.add_argument('--output-directory',type=Path);a=p.parse_args()
release=read(ROOT/'installer/release.json');index=read(a.setup.with_suffix('.payload.json'));v=read(a.validation);build=read(a.build_information);setup=a.setup.read_bytes();game=Path(build['executable']).read_bytes()
assert v['status']=='PASS' and v['setupSha256']==sha(setup) and v['gameBuildId']==build['buildId']
assert index['version']==release['version'] and index['buildId']==build['buildId'] and not index['fixture']
assert build['configuration']=='Release' and not build['diagnostics'] and sha(game)==build['sha256'].lower()
assert next(x['sha256']for x in index['files']if x['path']=='ksm-enhanced-vertical.exe')==sha(game)
assert all(report['status']=='PASS' for report in v['reports'].values())
assert len(v['reports']['production']['upgrades'])==4 and len(v['reports']['production']['runtimeCases'])>=13 and v['reports']['production']['sameVersionHotfix']['status']=='PASS' and len(v['reports']['production']['gui'])==5
stub=(ROOT/'build/native-installer/SetupStub.exe').read_bytes();assert setup.startswith(stub)
for marker in ('KSM_SETUP_FAIL_AFTER','KSM_SETUP_CRASH_AFTER','KSM_SETUP_LOW_SPACE','KSM_SETUP_DIRECTORY_DENY','KSM_SETUP_CLEANUP_CRASH_AFTER'):
 assert marker.encode()not in stub and marker.encode('utf-16le')not in stub
setupName='KsmEnhancedVertical_'+release['version']+'_Setup.exe'
info={'setupFileName':setupName,'product':'KSM Enhanced Vertical Mod','version':release['version'],'installerRevision':index['installerRevision'],'createdUtc':datetime.now(timezone.utc).isoformat(),'gameBuildId':build['buildId'],'gameConfiguration':'Release','diagnostics':False,'gameSha256':sha(game),'setupSha256':sha(setup),'setupBytes':len(setup),'payloadFileCount':len(index['files']),'supportedGame':'K-Shoot Mania 2.1.0 Windows x64','defaultInstallerLanguage':'Japanese','installerLanguages':['EN','JP','KR','SC','TC'],'endUserDependencies':'Built-in Windows APIs; no PowerShell, Python or .NET installation required','originalExecutableIncluded':False,'ordinarySongsIncluded':False,'sdkIncluded':False,'userDataIncluded':False,'signature':'Unsigned beta build','updatePolicy':'Single rollback-capable update transaction; supports verified older installations; no downgrade or duplicate-build reinstall','removalPolicy':'Restore original first-install baseline; preserve user-modified/deleted images and all user data','asioCorrection':'Per-device additional judgment calibration; default +15ms, range -100..+100ms; separate from driver-reported latency'}
files={setupName:setup,'BUILD_INFO.json':(json.dumps(info,indent=2)+'\n').encode()}
for n in ('README_KO.txt','README_EN.txt','README_JA.txt'):
 b=(ROOT/'installer/readme'/n).read_bytes();assert release['version'].encode()in b;files[n]=b
out=a.output_directory or ROOT/'releases'/('ksm-enhanced-vertical-'+release['version']+'-installer');assert not out.exists(),'Never overwrite an existing release';out.mkdir(parents=True)
archive=out/('ksm-enhanced-vertical-'+release['version']+'-installer.zip')
with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for n,b in files.items():z.writestr(n,b)
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and set(z.namelist())==set(files)
 for n,b in files.items():assert sha(z.read(n))==sha(b)
for n,b in files.items():
 if n!=setupName:(out/n).write_bytes(b)
with(out/'FILES_SHA256.csv').open('w',encoding='utf8',newline='')as f:
 w=csv.writer(f);w.writerow(['Path','Bytes','SHA256']);w.writerows((n,len(b),sha(b))for n,b in files.items())
v.update(version=release['version'],archiveIntegrity='PASS',archiveFileHashes='PASS',zipBytes=archive.stat().st_size,zipSha256=sha(archive.read_bytes()),payload=index['files']);(out/'VERIFICATION.json').write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf8');archive.with_suffix('.zip.sha256').write_text(v['zipSha256']+'  '+archive.name+'\n',encoding='ascii');print(json.dumps({'status':'PASS','zip':str(archive),'bytes':v['zipBytes'],'sha256':v['zipSha256']},indent=2))
