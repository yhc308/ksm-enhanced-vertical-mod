"""Create a standalone ZIP from managed runtime assets and the standard K-Shoot MANIA songs."""
from pathlib import Path
from collections import OrderedDict
from datetime import datetime,timezone,timedelta
import argparse,csv,hashlib,io,json,os,re,zipfile

root=Path(__file__).resolve().parents[1]
runtime=root/'play-test/App';canonical=root/'kshootmania/App'
build=json.loads((root/'build/latest.json').read_text(encoding='utf-8-sig'))
release=json.loads((root/'config/release.json').read_text(encoding='utf-8-sig'))
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--version',default=release['version'],help='Explicit mod release version; never inferred from the base game.')
parser.add_argument('--output-directory',type=Path,help='Override the output directory for isolated packaging verification.')
parser.add_argument('--check',action='store_true',help='Validate release naming and guide without creating a package.')
args=parser.parse_args()
version=args.version
number=r'(?:0|[1-9][0-9]*)'
identifier=r'(?:0|[1-9][0-9]*|[0-9]*[A-Za-z-][0-9A-Za-z-]*)'
pattern=rf'{number}\.{number}\.{number}(?:-{identifier}(?:\.{identifier})*)?(?:\+[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?'
if not re.fullmatch(pattern,version):raise ValueError('Invalid SemVer release version: '+version)
name='ksm-enhanced-vertical-'+version
out=args.output_directory.resolve() if args.output_directory else root/'releases'/name
archive=out/(name+'.zip')
prefix=release['archiveRoot']+'/'
if prefix!='ksm-enhanced-vertical-test/':raise ValueError('Unexpected release archive root')
guide_template=(root/'config/release-usage-template.txt').read_text(encoding='utf-8-sig')
if '{version}' not in guide_template or '{buildId}' not in guide_template:raise ValueError('Missing release guide tokens')
guide=guide_template.replace('{version}',version).replace('{buildId}',build['buildId'])
english_template=(root/'config/release-usage-template-en.txt').read_text(encoding='utf-8-sig')
if '{version}' not in english_template or '{buildId}' not in english_template:raise ValueError('Missing English guide tokens')
english_guide=english_template.replace('{version}',version).replace('{buildId}',build['buildId'])
if args.check:
 print(json.dumps({'version':version,'archive':str(archive),'archiveRoot':prefix,'guideTemplate':'config/release-usage-template.txt','launcherScripts':False},ensure_ascii=True))
 raise SystemExit(0)
if archive.exists():raise FileExistsError(archive)
out.mkdir(parents=True,exist_ok=True)
partial=out/(name+'.zip.partial')
entries={}
def linked(p):return p.is_symlink() or bool(getattr(p.lstat(),'st_file_attributes',0)&0x400)
def add_tree(base,target,skip_authoring=False):
 for folder,dirs,files in os.walk(base,followlinks=False):
  for child in dirs:
   if linked(Path(folder)/child):raise RuntimeError('Linked directory cannot be shipped: '+str(Path(folder)/child))
  for name in files:
   p=Path(folder)/name
   if linked(p):raise RuntimeError('Linked file cannot be shipped: '+str(p))
   rel=p.relative_to(base).as_posix()
   if name.lower() in ['desktop.ini','thumbs.db']:continue
   if skip_authoring and p.suffix.lower() in ['.zip','.psd','.psb','.bak','.tmp']:continue
   entries[target+rel]=p
for folder in ['assets','dll','engine','skins','lang','mods','se','shaders','skin_preview','ui']:
 add_tree(runtime/folder,'App/'+folder+'/',folder=='skins')
# Latest managed code/resources win even when the user's running Play could not be refreshed.
for folder in ['mods','lang','shaders','skin_preview','ui']:
 add_tree(canonical/folder,'App/'+folder+'/')
add_tree(canonical/'imgs','App/imgs/',True)
add_tree(canonical/'skins','App/skins/',True)
for name in ['bg_vertical.png','standby_vertical.png','er_aps.png','score_header_ex.png','result_bestscore_checker.png','msel_exscore_1.png','msel_exscore_2.png','msel_exscore_3.png','judge_p.gif','judge_p1.gif','play_judge_indicator.png','play_judge_indicator_minimum.png','play_judge_indicator_inversion.png','play_judge_indicator_minimum_inversion.png','result_v2_judge.png','result_v2_judge_inversion.png']:
 entries['App/imgs/'+name]=canonical/'imgs'/name
for name in ['bg_vertical.png','standby_vertical.png','result_v2_judge.png','result_v2_judge_inversion.png']:
 entries['App/imgs/'+name]=canonical/'imgs'/name
for name in ['bass.dll','bass_fx.dll','bassasio.dll','bassmix.dll','icon.ico','icon_editor.ico']:
 entries['App/'+name]=canonical/name
entries['App/ksm-enhanced-vertical.exe']=Path(build['executable'])
assert hashlib.sha256(Path(build['executable']).read_bytes()).hexdigest().upper()==build['sha256'].upper()
for retired in ['er_aps_back.png','er_aps_front.png']:
 entries.pop('App/imgs/'+retired,None)
entries.pop('App/ui/viewport_info_icon.png',None)
assert not any(p.startswith(('App/imgs/default/','App/skins/default/')) for p in entries)
# Only the standard collection is distributable in this test package.
# Do not mutate or traverse the user's TEST/other song collections.
song_roots=('K-Shoot MANIA',)
song_files={}
standard=runtime/'songs'/song_roots[0]
if not standard.is_dir() or linked(standard):raise RuntimeError('Missing or linked standard song root')
for p in sorted(standard.rglob('*')):
 if linked(p):raise RuntimeError('Linked song path: '+str(p))
 if p.is_file():
  rel=p.relative_to(runtime/'songs').as_posix();song_files[rel]=p;entries['App/songs/'+rel]=p
charts=sum(p.suffix.lower() in ['.ksh','.kson'] for p in song_files.values())
settings=OrderedDict()
for line in (root/'config/default-windows.ini').read_text(encoding='utf-8-sig').splitlines():
 m=re.match(r'^([A-Za-z0-9_]+)=(.*)$',line)
 if m:settings[m[1]]=m[2]
settings.update({'key':'83,68,75,76,67,77,81,87,79,80,32,13,27,122','key2':','.join(['-1']*14),
 'currentlang':'Japanese','currentplayer':'PLAYER','currentdir':'K-Shoot MANIA','currentitem':'1',
 'windowsize':'1280,720','fullscr':'0','vertical_enabled':'1','vertical_display_mode':'0',
 'vertical_resolution':'720,1280','vertical_width':'720','vertical_height':'1280','vsync':'1',
 'audio_backend':'default','mastervol':'100','automaticmute':'0','globaloffset':'0','inputdelay':'0',
 'viewtiming':'2','timing_ms_display':'3','timing_ms_color':'2','output':'0','output_path':'output/',
 'esckey_bt3':'0'})
config=('[config]\r\n'+'\r\n'.join(k+'='+v for k,v in settings.items())+'\r\n').encode('utf-8')
entries['App/config.ini']=config
entries['App/skin_preset.json']=(json.dumps({'folder':'default','missing':'default','background1Brightness':100,
 'background2Opacity':100,'laserWidthPercent':100.0,'noteHeightPercent':100,'indicatorPortrait':1,'indicatorLandscape':True,'keyBeamMode':1},indent=2)+'\n').encode('utf-8')

entries['README_KO.txt']=guide.encode('utf-8-sig')
entries['README_EN.txt']=english_guide.encode('utf-8-sig')
for src,dst in [(root/'LICENSE','LICENSE.txt'),(root/'ksmaudio/THIRD_PARTY_LICENSE.txt','licenses/ksmaudio.txt'),
 (root/'ksmaudio/ThirdParty_Windows/bassasio/bassasio.txt','licenses/bassasio.txt'),
 (root/'ksmaudio/ThirdParty_Windows/bassmix/bassmix.txt','licenses/bassmix.txt'),
 (root/'kson/LICENSE','licenses/kson.txt'),(root/'ksmaxis/LICENSE','licenses/ksmaxis.txt'),
 (root/'kshootmania/ThirdParty/NocoUI/LICENSE','licenses/NocoUI.txt'),
 (root/'kshootmania/ThirdParty/CoTaskLib/LICENSE','licenses/CoTaskLib.txt'),
 (root/'kshootmania/ThirdParty/SQLiteCpp/LICENSE.txt','licenses/SQLiteCpp.txt')]:
 if src.is_file():entries['App/licenses/LICENSE.txt' if dst=='LICENSE.txt' else 'App/'+dst]=src
info={'version':version,'buildId':build['buildId'],'baseVersion':'2.1.0','platform':'Windows x64','songFiles':len(song_files),
 'chartFiles':charts,'songDirectories':list(song_roots),'songCollectionFolders':sum(p.is_dir() for p in standard.iterdir()),
 'skins':['default']+sorted(p.name for p in (runtime/'skins').iterdir() if p.is_dir() and p.name.lower()!='default') if (runtime/'skins').is_dir() else ['default'],'audioBackend':'default',
 'initialMode':'vertical 720x1280','initialLanguage':'Japanese','personalDataIncluded':False,
 'createdAt':datetime.now(timezone(timedelta(hours=9))).isoformat()}
if not any(p.startswith('App/skins/') for p in entries):entries['App/skins/']=b''
entries['BUILD_INFO.json']=(json.dumps(info,ensure_ascii=False,indent=2)+'\n').encode('utf-8-sig')
manifest=[]
with zipfile.ZipFile(partial,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as z:
 for rel,source in sorted(entries.items()):
  if isinstance(source,Path):
   before=source.stat();data=source.read_bytes();after=source.stat()
   if (before.st_mtime_ns,before.st_size)!=(after.st_mtime_ns,after.st_size):raise RuntimeError('Source changed while reading: '+str(source))
  else:data=source
  z.writestr(prefix+rel,data)
  manifest.append({'path':rel,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
 stream=io.StringIO(newline='');w=csv.DictWriter(stream,fieldnames=['path','bytes','sha256']);w.writeheader();w.writerows(manifest)
 manifest_bytes=stream.getvalue().encode('utf-8-sig')
with zipfile.ZipFile(partial) as z:
 assert z.testzip() is None
 for row in manifest:assert hashlib.sha256(z.read(prefix+row['path'])).hexdigest()==row['sha256'],row['path']
 songs={row['path'][10:]:row for row in manifest if row['path'].startswith('App/songs/')}
 assert set(songs)==set(song_files)
 assert all(rel.startswith('K-Shoot MANIA/') for rel in songs)
 assert {rel.split('/')[0] for rel in songs}==set(song_roots)
 for rel,p in song_files.items():assert hashlib.sha256(p.read_bytes()).hexdigest()==songs[rel]['sha256'],rel
 forbidden=['App/log/','App/cache/','App/score/','App/score_backup/','App/Screenshot/','App/audio_calibration_profiles.json']
 assert not any(any(row['path'].startswith(item) for item in forbidden) for row in manifest)
partial.rename(archive)
# Keep release inspection files alongside the versioned ZIP, matching the user's layout.
for rel,source in entries.items():
 if rel=='BUILD_INFO.json':
  destination=out/rel;destination.parent.mkdir(parents=True,exist_ok=True)
  destination.write_bytes(source.read_bytes() if isinstance(source,Path) else source)
(out/'FILES_SHA256.csv').write_bytes(manifest_bytes)
digest=hashlib.sha256(archive.read_bytes()).hexdigest()
(out/(archive.name+'.sha256')).write_text(digest+'  '+archive.name+'\n',encoding='ascii')
result={'version':version,'status':'PASS','archive':str(archive),'sha256':digest,'bytes':archive.stat().st_size,
 'fileCount':len(manifest),'songFiles':len(song_files),'chartFiles':charts,'buildId':build['buildId'],
 'sourceSongFolder':str(standard),'songDirectories':list(song_roots),'manifestVerified':True,'songsByteIdentical':True}
(root/'build/test-play-package-latest.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=True))
