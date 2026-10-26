from pathlib import Path
from PIL import Image
import json,sys,hashlib
app,dest=map(Path,sys.argv[1:3]);w,h=map(int,sys.argv[3:5]);state=sys.argv[5]
r=Path(__file__).resolve().parents[1]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(r/'imgs/세부판정리절트/result_bestscore_checker.png')==sha(app/'imgs/result_bestscore_checker.png')
im=Image.open(dest/'result-detail.png').convert('RGB');assert im.size==(w,h)
layout=read(dest/'result-best-layout.json');assert layout['visible']==(state!='hidden')
if state!='hidden':
 x,y,ww,hh=[layout[k] for k in ['x','y','width','height']]
 assert x>=0 and y>=0 and x+ww<=w and y+hh<=h,(state,layout)
 assert abs(ww/hh-248/214)<.01,layout
 patch=im.crop((round(x),round(y),round(x+ww),round(y+hh))).resize((248,214),Image.Resampling.BICUBIC)
 patch.save(dest/'checker.png')
 gold=lambda box:sum(1 for p in patch.crop(box).getdata() if p[0]>190 and p[1]>150 and p[2]<90)
 assert gold((95,139,231,161))>10,'missing best EX digits'
 assert gold((95,171,231,193))>5,'missing EX difference digits'
 ex=read(dest/'result-ex-layout.json')
 if h>w:
  scale=w/720
  assert abs((x+ww/2)-586*scale)<2 and abs((y+2*ww/248)-748*scale)<2,layout
 else:
  scale=ww/(248*.5874)
  # Compare rendered background top against the EX panel outer bottom plus a four-pixel gap.
  assert abs((y+2*ww/248)-(ex['y']+ex['height']+4*scale))<2, (layout,ex)
 core=Path((r/'build/ex-score-core-session.txt').read_text(encoding='utf-8-sig').strip())/'App'
 idx={'positive':0,'negative':1,'zero':5,'large':6}[state]
 expected=Image.open(core/f'best-checker-{idx}.png').convert('RGB')
 # Compare the stable interior of the upper row; all glyphs originate from the same selected image sheet.
 error=[]
 for xx,yy in [(160,53),(180,55),(214,54),(80,100),(20,110),(30,130)]:
  a=patch.getpixel((xx,yy));b=expected.getpixel((xx,yy));error.append(max(abs(i-j) for i,j in zip(a,b)))
 assert max(error)<145,(state,error)
assert not any(p.is_file() for p in (app/'score').rglob('*')),'diagnostic capture wrote a player score'
(dest/'verification.json').write_text(json.dumps({'status':'PASS','state':state,'layout':layout},indent=2),encoding='utf-8')
print('BEST SCORE PIXELS PASS',w,h,state)
