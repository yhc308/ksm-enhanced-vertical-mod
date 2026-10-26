"""Verify captured native geometry and actual GameMain replay transition timing."""
from pathlib import Path
from PIL import Image
import numpy as np
import json,csv,math,hashlib
root=Path(__file__).resolve().parents[1];folder=root/'reference/aps';checks=[]
def check(ok,name):
 if not ok:raise AssertionError(name)
 checks.append(name)
def ease(t):return 1-(1-min(1,max(0,t)))**2
fade_start=.5
times=[0,.2,.4,.6,.7,.8,1,1.2]
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
 imgs=[np.array(Image.open(folder/f'aps-timeline-{w}-{h}-{i}.png'))[:,:,:3].astype(int) for i in range(8)]
 if w<h:
  scale=min(w/720,h/1280);fx,fy,fw,fh=600*scale,395*scale,40*scale,347*scale
  unit=46*scale/55;ax=597*scale-29*unit;ay=392*scale-122*unit
 else:
  unit=h/960;bx=w/2+155*h/480;by=88*h/480
  fx,fy,fw,fh=bx+int(34*unit),by+int(126*unit),47.25*unit,434*unit
  ax=bx;ay=by
 for i,t in enumerate(times):
  x0=int(fx+fw*.35);x1=max(x0+1,int(fx+fw*.65));y0=int(fy+5);y1=int(fy+fh)
  band=imgs[i][y0:y1,x0:x1];mask=(band.max(2)-band.min(2)>35)&(band[:,:,2]>130)
  rows=np.flatnonzero(mask.mean(1)>.4);expected=fy+fh*(1-(4+56*ease(t*1.5))/100)
  check(len(rows)>0 and abs(y0+rows[0]-expected)<3,f'native fill top follows one-second easing {w}x{h} stage {i}')
  if w<h:
   display=4+56*ease(t*1.5)
   px=597*scale+43*unit
   py=395*scale-20*unit+347*scale*(431/434)*(1-display/100)
   top=imgs[i][int(py+3*unit)-2:int(py+6*unit)+3,int(px+20*unit):int(px+80*unit)]
   matches=(np.abs(top-np.array([145,158,175])).max(2)<36).mean(1)
   check(matches.max()>.65,f'portrait percent box follows same interpolated fill at stage {i} {w}x{h}')
 crop=(slice(max(0,int(ay+119*unit)),int(ay+274*unit)),slice(int(ax+2*unit),int(ax+25*unit)))
 check(np.count_nonzero(imgs[7][crop]!=imgs[0][crop])>20,f'banner disappears only at delayed 1200ms deadline {w}x{h}')
 check(np.count_nonzero(imgs[0][crop]!=imgs[7][crop])>20,f'banner remains visible at conversion instant {w}x{h}')
 check(np.count_nonzero(imgs[3][crop]!=imgs[7][crop])>10,f'banner visible at 600ms and absent at 1200ms {w}x{h}')
 check(np.count_nonzero(imgs[1][crop]>imgs[0][crop]+3)>10,f'Color Dodge visibly brightens banner before opacity fade {w}x{h}')
for mode in ['landscape','portrait']:
 rows=list(csv.DictReader((folder/f'{mode}-frames.csv').open(encoding='utf-8-sig')))
 check(len(rows)==181 and int(rows[0]['frame'])==60 and abs(float(rows[-1]['time'])-3)<1e-6,f'{mode} replay includes 60 preroll and 181 post-switch frames')
 start=float(rows[0]['display']);maximum_error=0
 for row in rows:
  t=float(row['time']);actual=float(row['actual']);display=float(row['display']);blend=ease(t*1.5)
  maximum_error=max(maximum_error,abs(display-(start*(1-blend)+actual*blend)))
  fade=min(1,max(0,(t*1.5-fade_start)/.6))
  check(abs(float(row['opacity'])-(1-fade*fade))<1e-7,f'{mode} banner opacity frame {row["frame"]}')
  if t<.8-1e-7:
   check(abs(float(row['dodge'])-ease(t*1.5/.7)*(1-.3*ease((t*1.5-.8)/.4)))<1e-7,f'{mode} image Dodge strength frame {row["frame"]}')
   check(abs(float(row['burn'])-ease((t*1.5-.4)/.4))<1e-7,f'{mode} image Linear Burn strength frame {row["frame"]}')
 check(maximum_error<1e-6,f'{mode} native display tracks latest live NORMAL with exact easing')
 check(float(rows[0]['actual'])>float(rows[0]['display'])+20,f'{mode} actual gauge changes immediately independently of presentation')
 for zoom in ['', '-zoom']:
  p=folder/f'aps-transition-{mode}{zoom}.mp4';check(p.stat().st_size>10000,f'{mode}{zoom} MP4 saved')
for name in ['er_aps_back.png','er_aps_front.png']:
 check((root/'imgs/APS게이지'/name).is_file(),f'authoring original retained {name}')
 check(not(root/'kshootmania/App/imgs'/name).exists(),f'retired default resource removed {name}')
report={'status':'PASS','count':len(checks),'checks':checks}
(root/'build/aps-animation-verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'TOTAL {len(checks)} APS animation/replay checks passed')
