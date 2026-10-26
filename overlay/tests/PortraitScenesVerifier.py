from pathlib import Path
import json,re,shutil,sys
from PIL import Image,ImageChops,ImageStat
app=Path(sys.argv[2]);root=Path(__file__).resolve().parents[1]
def capture(name):
 matches=list(app.rglob(name));assert len(matches)==1,(name,matches);return matches[0]
if sys.argv[1]=='setup':
 original=app/'songs/K-Shoot MANIA/flavorofyouth';text=(original/'flavorofyouth_ex.ksh').read_text(encoding='utf-8-sig')
 def chart(title):
  value=text
  for key,entry in [('title',title),('artist','VerticalModTest'),('level','5'),('difficulty','extended'),('t','208'),('jacket','../../K-Shoot MANIA/flavorofyouth/flavorofyouth.jpg'),('m','../../K-Shoot MANIA/flavorofyouth/flavorofyouth.ogg')]:
   value=re.sub('^'+key+'=.*$',key+'='+entry,value,count=1,flags=re.M)
  return value
 for i in range(1,15):
  directory=app/f'songs/LayoutSelection/Track{i:02}';directory.mkdir(parents=True)
  (directory/'track.ksh').write_text(chart(f'Preview {i:02}'),encoding='utf-8')
 fixture=app/'layout-fixture';fixture.mkdir()
 value=chart('VerticalModTest').replace('jacket=../../K-Shoot MANIA/flavorofyouth/flavorofyouth.jpg','jacket=cover.jpg')
 (fixture/'prepare.ksh').write_text(value,encoding='utf-8');shutil.copyfile(original/'flavorofyouth.jpg',fixture/'cover.jpg')
else:
 width=int(sys.argv[3]);p=width/720
 results={}
 for prefix,background in [('prepare','bg3_vertical.png'),('option-scene','bg_vertical.png')]:
  name=f'vertical-{prefix}-preview.png';im=Image.open(capture(name)).convert('RGB');reference=Image.open(root/'imgs'/background).convert('RGB').resize(im.size,Image.Resampling.NEAREST)
  errors=[]
  for x,y,w,h in [(5,180,30,20),(660,180,30,20),(5,1050,30,20),(630,1090,30,20)]:
   rect=tuple(int(v*p) for v in (x,y,x+w,y+h));diff=ImageChops.difference(im.crop(rect),reference.crop(rect));errors.append(max(ImageStat.Stat(diff).mean))
  assert max(errors)<3,(prefix,errors)
  results[prefix+'BackgroundMaxMeanError']=max(errors)
 # The native sprite has transparent margins: check the visible title panel, not just its node size.
 header=Image.open(capture('vertical-prepare-preview.png')).convert('RGB')
 xs=[x for x in range(int(80*p),int(640*p)) if (lambda c:c[0]>70 and c[1]>180 and c[1]>c[0]*1.2)(header.getpixel((x,int(660*p))))]
 assert xs and abs((max(xs)-min(xs)+1)/p-478)<4,(min(xs) if xs else None,max(xs) if xs else None)
 results['visibleTitleWidth']=(max(xs)-min(xs)+1)/p
 # Animation size is measured from the real jacket against its background, not from tween settings.
 boxes=[]
 for stage in [1,2]:
  im=Image.open(capture(f'vertical-prepare-stage-{stage}.png')).convert('RGB');bg=Image.open(root/'imgs/bg3_vertical.png').convert('RGB').resize(im.size,Image.Resampling.NEAREST)
  rect=tuple(int(v*p) for v in (140,200,580,640));diff=ImageChops.difference(im.crop(rect),bg.crop(rect));mask=diff.convert('L').point(lambda v:255 if v>15 else 0);box=mask.getbbox();assert box,stage
  box=tuple(v+rect[i%2] for i,v in enumerate(box));boxes.append(box)
  assert abs((box[0]+box[2])/2/p-360)<3 and abs((box[1]+box[3])/2/p-426)<3,(stage,box)
 assert (boxes[0][2]-boxes[0][0])-(boxes[1][2]-boxes[1][0])>8*p,boxes
 assert abs((boxes[1][2]-boxes[1][0])/p-344)<8,boxes
 states={k:json.loads((app/f'select-state-{k}.json').read_text(encoding='utf-8-sig')) for k in ['base','down','up']}
 b=states['base'];assert states['down']['cursor']==(b['cursor']+1)%b['count'],states;assert states['up']['cursor']==(b['cursor']-1)%b['count'],states
 assert all(v['chart'] for v in states.values()) and len({v['chart'] for v in states.values()})==3,states
 results.update({'jacketBounds':boxes,'selection':states,'size':[width,width*16//9]})
 (app/'portrait-scenes-verification.json').write_text(json.dumps(results,indent=2),encoding='utf-8');print('PORTRAIT SCENES PASS',json.dumps(results))
