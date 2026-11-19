"""Collect real, silent KSM 2.2 migration evidence for PackageRelease.py.

This validates evidence; it does not simulate an interactive or hardware audio test.
"""
from pathlib import Path
import argparse, hashlib, json, re, xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--setup',type=Path,required=True);p.add_argument('--evidence',type=Path,required=True);a=p.parse_args()
def read(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))
def passed(p):
 r=read(p);assert r['status']=='PASS',str(p);return r
b=read(ROOT/'build/latest.json');assert not b['diagnostics'] and b['configuration']=='Release'
x=ET.parse(ROOT/'build/unit-tests/Release/results.xml').getroot();suites=list(x.iter('testsuite'))
assert suites and all(int(s.get('errors','0'))==int(s.get('failures','0'))==0 for s in suites)
reports={'unit':{'status':'PASS','checks':sum(int(s.get('tests','0')) for s in suites),'cases':len(list(x.iter('testcase')))},
 'installerEngine':passed(ROOT/'build/native-installer-update-latest.json'),
 'actualOriginal':passed(ROOT/'build/installer220-results.json'),
 'sourceContracts':passed(ROOT/'build/migration220-source-checks.json')}
audio=passed(ROOT/'build/audio-engine-latest.json');assert audio['buildId']==b['buildId']
reports['mockAudio']={'status':'PASS','scenarios':len(audio['cases']),'cases':audio['cases'],'realOutputUsed':False}
groups={'core':{},'portrait':{},'landscape':{}}
for log in sorted(a.evidence.glob('migration-silent-*.log')):
 for line in log.read_text(encoding='utf-8-sig',errors='replace').splitlines():
  if not line.startswith('{'):continue
  r=json.loads(line)
  if r.get('status')!='PASS':continue
  fixture=Path(r['fixture']);report=Path(r['report']);assert report.is_file()
  key='core' if r['case'] in ('ex','aps','judge','skin') else ('portrait' if '720x1280' in str(fixture) else 'landscape')
  if key=='landscape' and not '1280x720' in str(fixture):continue
  assert r['privateDesktop'] and r['audioSink']=='mock discard only' and not r['physicalTimingOrPerformanceClaim']
  groups[key][r['case']]={'report':str(report),'sha256':hashlib.sha256(report.read_bytes()).hexdigest()}
for key,cases in groups.items():reports[key]={'status':'PASS','cases':cases,'renderer':'WARP','interactiveDesktopUsed':False}
text=(ROOT/'build/unicode220-final/App/unicode-path-results.txt').read_text(encoding='utf-8-sig')
m=re.search(r'TOTAL checks=(\d+) failures=(\d+)',text);assert m and int(m[2])==0
reports['unicode']={'status':'PASS','checks':int(m[1])}
reports['installerRefresh']=passed(ROOT/'build/installer-refresh-engine-results.json')
reports['onlineUpdate']=passed(ROOT/'build/online-update-results.json')
reports['installerGui']=passed(ROOT/'build/installer-refresh-gui-results.json')
assert reports['installerGui']['setupSha256']==hashlib.sha256(a.setup.read_bytes()).hexdigest()
result={'status':'PASS','validationKind':'KSM220-silent-migration-QA','setupSha256':hashlib.sha256(a.setup.read_bytes()).hexdigest(),
 'gameBuildId':b['buildId'],'reports':reports,'limitations':['No physical WASAPI/ASIO output or acoustic/input latency measurement','No hardware-GPU/OBS frame-time performance claim','Normal Release built and packaged; runtime probes used the matching diagnostic code with discard-only mock ASIO and WARP','Linux/macOS not built or executed','Not every upstream fix has a dedicated input-driven end-to-end scenario; see upstream integration ledger']}
(ROOT/'build/migration220-validation.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
print(json.dumps({k: {a:b for a,b in v.items() if a not in ('cases',)} for k,v in reports.items()},indent=2))
