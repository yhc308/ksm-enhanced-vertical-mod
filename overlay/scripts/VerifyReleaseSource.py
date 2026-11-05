"""Verify the tested release source without requiring a private SDK or runtime."""
from pathlib import Path
import hashlib,json,subprocess
from compiler_parity import tokens,signature,directives,CPP
root=Path(__file__).resolve().parents[1]
reference=json.loads((root/'docs/review/RELEASE_0.5.4_SOURCE_REFERENCE.json').read_text(encoding='utf8'))
actual={p.relative_to(root).as_posix()for p in(root/'kshootmania/src').rglob('*')if p.suffix in CPP}
assert actual==set(reference['gameCpp']),'Game source inventory differs from tested release'
for group in ('gameCpp','dependencyCpp'):
 for name,expected in reference[group].items():
  data,parsed=tokens((root/name).read_bytes());values=[x for x in signature(parsed)if x[0]!=4];digest=hashlib.sha256(json.dumps(values,ensure_ascii=True,separators=(',',':')).encode()).hexdigest()
  assert digest==expected['tokens'],'Compiler token mismatch: '+name
  assert hashlib.sha256(directives(data)).hexdigest()==expected['directives'],'Directive mismatch: '+name
for name,expected in reference['runtimeAssets'].items():
 data=(root/name).read_bytes()
 if expected['mode']=='text-lf':data=data.replace(b'\r\n',b'\n')
 assert hashlib.sha256(data).hexdigest()==expected['sha256'],'Runtime resource mismatch: '+name
base=reference['upstreamCommit'];original=set(subprocess.check_output(['git','-c','safe.directory='+root.as_posix(),'-C',str(root),'ls-tree','-r','--name-only',base],text=True).splitlines());assert all((root/p).exists()for p in original if p.startswith('kshootmania/src/')),'Original game source was removed'
report={'status':'PASS','version':reference['version'],'testedBuildId':reference['gameBuildId'],'gameCppFiles':len(actual),'dependencyCppFiles':len(reference['dependencyCpp']),'runtimeAssets':len(reference['runtimeAssets'])}
(root/'build').mkdir(exist_ok=True)
(root/'build/release-source-parity.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
