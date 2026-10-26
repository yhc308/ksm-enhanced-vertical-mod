from pathlib import Path
import json,sys
from PIL import Image
app=Path(sys.argv[1]);width=int(sys.argv[2]);root=Path(__file__).resolve().parents[1]
def image(name):
 im=Image.open(app/name).convert('RGB')
 assert im.size==(width,1280*width//720 if width!=1280 else 720),(name,im.size)
 return im.resize((720,1280)) if width!=1280 else im
inverted=any(line.strip()=="judge_color=1" for line in (app/"config.ini").read_text(encoding="utf-8-sig").splitlines())
results={}
if width==1280:
 # Landscape uses its mini canvas; the large portrait panel stays disabled.
 assert not (app/'skin-test-failure.txt').exists()
 image('indicator-live.png');results['landscapeCapture']=True
else:
 live=image('indicator-live.png');asset=Image.open(app/('imgs/play_judge_indicator'+('_inversion' if inverted else '')+'.png')).convert('RGB')
 errors=[]
 for x,y in [(40,35),(680,35),(35,180),(100,150)]:
  actual=live.getpixel((x,y+963));expected=asset.getpixel((x,y))
  errors.append(max(abs(a-b) for a,b in zip(actual,expected)))
 assert max(errors)<25,errors
 results['panelPixelErrors']=errors
 def pixels(im,box):
  for y in range(box[1],box[3]):
   for x in range(box[0],box[2]):yield x,y,im.getpixel((x,y))
 for kind in ['positive','negative','zero','large']:
  im=image(f'indicator-{kind}.png')
  colored=[]
  for x,y,(r,g,b) in pixels(im,(176,1027,312,1055)):
   if (kind in ['positive','negative'] and ((r>150 and r>b*1.3) if ((kind=='positive')==inverted) else (b>150 and b>r*1.3))) or (kind in ['zero','large'] and max(r,g,b)>160):colored.append((x,y))
  assert len(colored)>20,(kind,len(colored))
  if kind=='zero':
   assert not any(b>r*1.3 and b>150 for _,_,(r,g,b) in pixels(im,(176,1027,312,1055))),'negative zero must be neutral'
  # Every numeric cell has centered white glyphs; no leading zero padding or clipping.
  for i in range(7):
   left=80+80*i
   hits=[(x,y) for x,y,(r,g,b) in pixels(im,(left,1070,left+80,1106)) if min(r,g,b)>190 and max(r,g,b)-min(r,g,b)<35]
   assert hits,(kind,i)
   x0=min(x for x,y in hits);x1=max(x for x,y in hits)
   assert abs((x0+x1)/2-(left+40))<3,(kind,i,x0,x1)
   assert left<x0 and x1<left+79,(kind,i,'overflow')
  results[kind]={'meanPixels':len(colored),'sevenCountersCentered':True}
(app/'play-judge-indicator-verification.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
print('INDICATOR PIXELS PASS',width,json.dumps(results))
