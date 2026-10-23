from pathlib import Path
from PIL import Image
import json,sys,hashlib
app,dest=map(Path,sys.argv[1:3]);w,h=map(int,sys.argv[3:5]);kind=sys.argv[5];r=Path(__file__).resolve().parents[1]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(r/'imgs/EX_SCORE/score_header_ex.png')==sha(app/'imgs/score_header_ex.png')
assert Image.open(app/'imgs/score_header_ex.png').size==(113,45)
widths=[];actual=[]
for i in range(10):
 im=Image.open(dest/f'hud-{i}.png').convert('RGB');assert im.size==(w,h)
 d=read(dest/f'hud-{i}.json');value=d['exScore'];assert value==str(int(value)),'leading zero or invalid formatting'
 actual.append(int(d['actualExScore']))
 if kind=='fixture':assert int(value)==[0,5,42,999,12345,19999,19999,21,123,0][i]
 else:assert int(value)==int(d['actualExScore']),'HUD differs from confirmed EX total'
 n=d['ExScoreNumber'];c=d['ExScoreCaption'];total=d['ScoreNumber']
 for rect in [n,c,total]:
  assert rect['x']>=0 and rect['y']>=0 and rect['x']+rect['width']<=w+1 and rect['y']+rect['height']<=h+1,(i,rect)
 assert total['y']+total['height']<=n['y']+2,'EX overlaps total score'
 assert c['x']+c['width']<=n['x']+1,'caption overlaps EX digits'
 assert abs(c['width']/c['height']-113/45)<.01,'banner aspect changed'
 crop=im.crop((round(n['x']),round(n['y']),round(n['x']+n['width']),round(n['y']+n['height'])))
 points=[(x,y) for y in range(crop.height) for x in range(crop.width) if min(crop.getpixel((x,y)))>190]
 assert len(points)>2,(i,'EX digits missing',value,n)
 left=min(x for x,y in points);right=max(x for x,y in points);inkwidth=right-left+1
 scale=n['width']/(73 if h>w else 67)
 assert inkwidth<(len(value)*13+2)*scale+3,(value,inkwidth,'unexpected extra leading digits')
 widths.append(inkwidth)
 if i in [0,4,5]:crop.resize((crop.width*3,crop.height*3)).save(dest/f'ex-number-{i}.png')
if kind=='fixture':assert widths[0]<widths[4]*.4,'zero padded to five digits'
else:assert max(actual)>0 and all(a<=b for a,b in zip(actual,actual[1:])),'live EX did not advance monotonically'
assert not any(p.is_file() for p in (app/'score').rglob('*')),'diagnostic saved player records'
(dest/'verification.json').write_text(json.dumps({'status':'PASS','kind':kind,'actualEx':actual,'digitWidths':widths},indent=2),encoding='utf-8')
print('EX HUD PIXELS PASS',w,h,kind)
