"""Verify native APS captures, source preservation and bounded HUD changes."""
from pathlib import Path
from PIL import Image,ImageChops
import hashlib,json,math
import numpy as np
root=Path(__file__).resolve().parents[1]
folder=root/'reference/aps'
checks=[]
def check(ok,name):
 if not ok:raise AssertionError(name)
 checks.append(name)
for name in ['er_aps.png']:
 source=root/'imgs/APS게이지'/name;target=root/'kshootmania/App/imgs'/name
 check(hashlib.sha256(source.read_bytes()).digest()==hashlib.sha256(target.read_bytes()).digest(),'unchanged source hash '+name)
 image=Image.open(target)
 check(image.size==(192,570) and image.mode=='RGBA' and image.getextrema()[3][0]==0,'native size and transparency '+name)
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
 images=[Image.open(folder/f'aps-{w}-{h}-{i}.png').convert('RGBA') for i in range(8)]
 check(all(im.size==(w,h) for im in images),f'native capture sizes {w}x{h}')
 check(ImageChops.difference(images[3],images[4]).convert('RGB').getbbox() is None,f'spent APS exactly matches plain NORMAL {w}x{h}')
 if w>h:
  scale=h/960;x=w/2+155*h/480;y=88*h/480
 else:
  viewport=min(w/720,h/1280);scale=46*viewport/55
  x=597*viewport-29*scale;y=392*viewport-122*scale
 bounds=(math.floor(x)-2,math.floor(y)-2,math.ceil(x+192*scale)+2,math.ceil(y+570*scale)+2)
 if w<h:
  for state,percent in [(0,100),(1,30),(2,0)]:
   px=597*viewport+43*scale
   py=395*viewport-20*scale+347*viewport*(431/434)*(1-percent/100)
   pixels=np.array(images[state].convert('RGB')).astype(float)
   top=pixels[max(0,int(py+3*scale)-2):int(py+6*scale)+3,int(px+20*scale):int(px+80*scale)]
   matches=(np.abs(top-np.array([145,158,175])).max(2)<36).mean(1)
   check(matches.max()>.65,f'portrait percent box top follows {percent}% with landscape offset {w}x{h}')
   right=pixels[int(py+9*scale):int(py+32*scale),int(px+89*scale)-2:int(px+92*scale)+3]
   matches=(np.abs(right-np.array([145,158,175])).max(2)<36).mean(0)
   check(matches.max()>.65,f'portrait percent box matches landscape width and height {percent}% {w}x{h}')
   check(px+96*scale<=w,f'portrait moving percent box stays on screen {percent}% {w}x{h}')
 for i in range(3):
  diff=ImageChops.difference(images[i],images[i+5]);rgb=diff.convert('RGB');box=rgb.getbbox()
  check(box is not None,f'APS visible at percent state {i} {w}x{h}')
  check(box[0]>=bounds[0] and box[1]>=bounds[1] and box[2]<=bounds[2] and box[3]<=bounds[3],f'APS changes confined to own rectangle state {i} {w}x{h}')
  if w>h and i==0:
   # The opaque central part of er_p overlaps the APS front at 100%.
   px=w/2+155*h/480+72*scale;py=88*h/480+106*scale
   crop=(math.ceil(px+6*scale),math.ceil(py+8*scale),math.floor(px+84*scale),math.floor(py+33*scale))
   check(ImageChops.difference(images[0].crop(crop),images[5].crop(crop)).convert('RGB').getbbox() is None,f'single banner leaves percent box and numbers unchanged {w}x{h}')
report={'status':'PASS','count':len(checks),'checks':checks}
(root/'build/aps-visual-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'TOTAL {len(checks)} APS pixel/resource checks passed')
