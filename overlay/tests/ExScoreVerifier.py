from pathlib import Path
from PIL import Image,ImageChops,ImageStat
import sys,json,hashlib,sqlite3
app,dest=map(Path,sys.argv[1:3]);w,h=map(int,sys.argv[3:5]);state=sys.argv[5];r=Path(__file__).resolve().parents[1]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for i in [1,2,3]:
 name=f'msel_exscore_{i}.png';source=r/'imgs/세부판정리절트'/name
 assert sha(source)==sha(app/'imgs'/name)
 assert Image.open(source).size==(222,40) and Image.open(source).mode=='RGBA'
core=Path((r/'build/ex-score-core-session.txt').read_text(encoding='utf-8-sig').strip())/'App'
expected=None if state=='none' else Image.open(core/f'ex-panel-{dict(percent=6630,minus=6698,max=6800)[state]}.png').convert('RGBA')
metrics={}
for scene,metadata in [('select','menu'),('result','result')]:
 im=Image.open(dest/f'{scene}.png').convert('RGB');assert im.size==(w,h)
 layout=read(dest/f'{metadata}-layout.json');assert layout['visible']==(state!='none')
 if state=='none':continue
 x,y,ww,hh=[layout[k] for k in ['x','y','width','height']]
 assert ww>0 and hh>0 and x>=0 and y>=0 and x+ww<=w and y+hh<=h,(scene,layout)
 patch=im.crop((round(x),round(y),round(x+ww),round(y+hh))).resize((228,46))
 # Compare opaque panel pixels, excluding compositing-dependent glow outside the border.
 reference=expected.convert('RGB');error=[]
 for xx,yy in [(12,10),(20,20),(60,10),(180,38),(210,28)]:
  a=patch.getpixel((xx,yy));b=reference.getpixel((xx,yy));error.append(max(abs(c-d) for c,d in zip(a,b)))
 assert max(error)<95,(scene,layout,error)
 # Dynamic digit regions contain the requested gold fill after viewport scaling.
 gold=sum(1 for p in patch.crop((122,4,220,16)).getdata() if p[0]>190 and p[1]>155 and p[2]<85)
 assert gold>30,(scene,'missing numeric glyphs',gold)
 if scene=='result' and h>w:
  scale=w/720;assert abs(x-471*scale)<2 and abs(y-698*scale)<2
 metrics[scene]={'layout':layout,'pixelErrors':error,'goldDigitPixels':gold}
# First menu scan must have computed and stored chart-specific theoretical maxima.
databases=list((app/'cache/songs_index').rglob('cache.sqlite3'))
assert databases,'no chart index cache'
found=[]
for path in databases:
 with sqlite3.connect(path) as db:
  version=db.execute("select value from cache_meta where key='schema_version'").fetchone()
  assert version and int(version[0])>=3
  found.extend(db.execute('select chart_path, ex_maximum from charts').fetchall())
assert found and any(maximum>0 for path,maximum in found)
assert not any(p.is_file() for p in (app/'score').rglob('*')),'capture saved a player record'
(dest/'verification.json').write_text(json.dumps({'status':'PASS','state':state,'metrics':metrics,'cachedTheoreticalCharts':found},ensure_ascii=False,indent=2),encoding='utf-8')
print('EX PIXELS PASS',w,h,state)
