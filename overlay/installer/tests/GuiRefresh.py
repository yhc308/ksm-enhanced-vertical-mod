from pathlib import Path
import shutil,uuid,json,subprocess,argparse
p=argparse.ArgumentParser();p.add_argument('--setup',type=Path,required=True);p.add_argument('--original22',type=Path,required=True);args=p.parse_args()
w=Path(__file__).resolve().parents[2];original=args.original22;work=w/'build/installer-gui-refresh'/uuid.uuid4().hex;app=work/'App';app.mkdir(parents=True)
for n in ('kshootmania.exe','bass.dll','bass_fx.dll'):shutil.copy2(original/n,app/n)
for n in ('imgs','ui','lang'):shutil.copytree(original/n,app/n)
for n in ('score/test_additional.ksc','config.ini'):
 p=app/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'USER DATA')
setup=args.setup.resolve()
r=subprocess.run([str(w/'build/native-installer/PrivateGuiTest.exe'),str(setup),str(app),str(work/'screens')],capture_output=True,timeout=210)
(work/'host.log').write_bytes(r.stdout+r.stderr)
result=json.loads((work/'screens/result.json').read_text(encoding='utf-8-sig'));result['fixture']=str(work)
assert(app/'score/test_additional.ksc').read_bytes()==b'USER DATA' and(app/'config.ini').read_bytes()==b'USER DATA'
(w/'build/installer-refresh-gui-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));assert r.returncode==0
from PIL import Image
for p in(work/'screens').glob('*.bmp'):Image.open(p).save(p.with_suffix('.png'))
