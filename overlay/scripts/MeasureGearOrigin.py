import math,json,csv
from pathlib import Path
out=Path(__file__).resolve().parents[1]/'test-output/gear-origin'
out.mkdir(parents=True,exist_ok=True)
def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,b):return tuple(x*b for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
fov=math.radians(80); H=608.4; W=304*2/13
anchor=(0,0,-H/2+H*14/1024)
t=math.tan(fov/2); alpha=math.atan((2*.675-1)*t)
d=1280*W/(2*t*720*.76*math.cos(alpha)); beta=(1-2*.35)*t
pitch=math.atan(beta)+math.asin(d*(math.sin(alpha)+beta*math.cos(alpha))/((H/2-anchor[2])*math.sqrt(1+beta*beta)))
eye=sub(anchor,mul((0,-math.sin(pitch+alpha),math.cos(pitch+alpha)),d))
f=1280/(2*t);up=(0,math.cos(pitch),math.sin(pitch));forward=(0,-math.sin(pitch),math.cos(pitch))
def project(p):
 q=sub(p,eye);z=dot(q,forward);return (360+f*q[0]/z,640-f*dot(q,up)/z)
def rotate(p,a,axis,pivot):
 v=sub(p,pivot); c=math.cos(a);s=math.sin(a)
 return add(pivot,add(add(mul(v,c),mul(cross(axis,v),s)),mul(axis,dot(axis,v)*(1-c))))
far=(0,0,H/2);bar=(0,2.6,-H/2-.4)
barhalf=720*.98/(project(add(bar,(1,0,0)))[0]-project(bar)[0])/2
# Hand landmarks: SDVX top centers approximately 307.5,304,307 pixels;
# bar center approximately 307,235,382 at 614-pixel image width.
# Unequal frame times / occlusion: use symmetric displacement, not literal angle labels.
landmarks={'method':'manual silhouette landmarks, approximately +/-3 source pixels; no SDVX internal angles available',
 'source_width':614,'source_height':1094,'visible_game_bottom_approx':806,
 'neutral':{'far_center':[307.5,383.5],'bar_center':[307,774]},
 'positive':{'far_center':[304,385],'bar_center':[235.5,765]},
 'negative':{'far_center':[307,385],'bar_center':[381.5,766]}}
(out/'sdvx-landmarks.json').write_text(json.dumps(landmarks,indent=2))
target_far=2.05;target_bar=85.6;target_angle=8.6
best=None
for axis100 in range(0,701,2):
 phi=math.radians(axis100/100); axis=(0,math.sin(phi),-math.cos(phi))
 for height10 in range(0,601,2):
  pivot=(0,height10/10,0);a=math.radians(10)
  fp=project(rotate(far,a,axis,pivot));bp=project(rotate(bar,a,axis,pivot))
  bl=project(rotate(add(bar,(-barhalf,0,0)),a,axis,pivot));br=project(rotate(add(bar,(barhalf,0,0)),a,axis,pivot))
  angle=math.degrees(math.atan2(br[1]-bl[1],br[0]-bl[0]))
  err=((abs(fp[0]-360)-target_far)/2)**2+((abs(bp[0]-360)-target_bar)/4)**2+((abs(angle)-target_angle)/2)**2
  if best is None or err<best[0]:best=(err,axis100/100,height10/10,fp,bp,angle)
print(best)
(out/'calibration.json').write_text(json.dumps({'source':'manual approximate SDVX landmarks; diagnostic 10 degrees is not a claim of SDVX world angle','fit':best,'neutral_camera_eye':eye},indent=2))
with (out/'projection-fit.csv').open('w',newline='') as stream:
 w=csv.writer(stream);w.writerow(['profile','degrees','far_x','far_y','bar_x','bar_y'])
 for name,phi,py in [('before',0,42),('after',best[1],best[2])]:
  axis=(0,math.sin(math.radians(phi)),-math.cos(math.radians(phi)))
  for degree in [-25,-17.5,-10,-7.5,-5,-2.5,0,2.5,5,7.5,10,17.5,25,90,180,360]:
   fp=project(rotate(far,math.radians(degree),axis,(0,py,0)));bp=project(add(rotate((0,0,0),math.radians(degree),axis,(0,py,0)),bar)) if name=='before' else project(rotate(bar,math.radians(degree),axis,(0,py,0)))
   w.writerow([name,degree,*fp,*bp])
