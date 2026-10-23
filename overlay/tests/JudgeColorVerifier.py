from pathlib import Path
import csv,json,hashlib,sys
from PIL import Image
app=Path(sys.argv[1]);w,h,palette=map(int,sys.argv[2:5]);portrait=h>w;r=Path(__file__).resolve().parents[1]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for base,folder,size in [('result_v2_judge','세부판정리절트',(312,389)),('play_judge_indicator','트랙저지인디케이터',(720,242)),('play_judge_indicator_minimum','트랙저지인디케이터',(280,320))]:
 for suffix in ['', '_inversion']:
  name=base+suffix+'.png';source=r/'imgs'/folder/name
  assert sha(source)==sha(app/'imgs'/name),name
  im=Image.open(source);assert im.size==size and im.mode=='RGBA'
mode='vertical' if portrait else 'enhanced'
d=json.loads((app/f'mods/{mode}/ui/select.noco').read_text(encoding='utf-8-sig'))
def nodes(x):
 if isinstance(x,dict):
  yield x
  for c in x.get('children',[]):yield from nodes(c)
 elif isinstance(x,list):
  for c in x:yield from nodes(c)
def find(name):return next(n for n in nodes(d) if n.get('name')==name)
bodies=[find('BTOptionPanel_'+letter)['children'][0] for letter in 'ABCD']
assert all(b['region']==bodies[0]['region'] for b in bodies),'basic menu differs from A/B/D'
assert all(b['region']['sizeDelta']==[640,369] and b['transform']['scale']==[.5,.5] for b in bodies)
for b in bodies+[find('NewPlusBody')]:
 s=next(c for c in b['components'] if c['type']=='Sprite')
 assert s['textureFilePath']=='imgs/msel_optionbg.png' and s['preserveAspect'] and not s['nineSliceEnabled']
assert bodies[2]['children'][0]['components'][0]['text_paramRef']=='overlay_btOptionPanelText_C'
assert find('NewPlusBody')['region']['posDelta']==([-172,170.25] if portrait else [130,-68.5])
assert bodies[0]['region']['posDelta']==([155,170.25] if portrait else [130,120])
trace=[list(map(float,line)) for line in csv.reader((app/'timing-captures/menu-fade.csv').open(encoding='utf-8-sig'))]
assert {int(row[1]) for row in trace}=={0,1,2}
assert any(0<a<1 for t,m,a in trace if m==2) and any(0<a<1 for t,m,a in trace if m==1),'missing fade transition'
assert max(a for t,m,a in trace if m==2)>.999
fall=[row for row in trace if row[1]==1]
end=next(row[0] for row in fall if row[2]==0)
assert all(a==0 for t,m,a in trace if t>fall[0][0]+.20 and m!=2),'hidden menu remains visible'
assert abs(end-fall[0][0]-.15)<.04,('fade duration differs from 150ms',end-fall[0][0])
# Compare source label colors at untouched, opaque pixels of the actual result panel.
result=Image.open(app/'result-detail.png').convert('RGB');assert result.size==(w,h)
asset=Image.open(app/('imgs/result_v2_judge'+('_inversion' if palette else '')+'.png')).convert('RGB')
s=h/480 if not portrait else w/720
cx,cy,scale=(w/2+183*s,192*s,.5874*s) if not portrait else (586*s,541*s,.78*s)
errors=[]
for x,y in [(110,30),(110,355),(160,100)]:
 actual=result.getpixel((round(cx+(x-156)*scale),round(cy+(y-194.5)*scale)))
 expected=asset.getpixel((x,y));errors.append(max(abs(a-b) for a,b in zip(actual,expected)))
assert max(errors)<35,('wrong result image variant',errors)
assert not any(p.is_file() for p in (app/'score').rglob('*')) if (app/'score').exists() else True
out={'status':'PASS','palette':palette,'menuLayout':'original aspect; same base A/B/C/D','fadeSeconds':.15,'resultImagePixelErrors':errors,'sixResourcesByteExact':True,'scoreNotSaved':True}
(app/'judge-color-verification.json').write_text(json.dumps(out,indent=2),encoding='utf-8')
print('JUDGE PALETTE PIXELS PASS',w,h,palette,out)
