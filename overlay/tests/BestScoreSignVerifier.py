from pathlib import Path
from PIL import Image
import json
r=Path(__file__).resolve().parents[1]
core=Path((r/'build/ex-score-core-session.txt').read_text(encoding='utf-8-sig').strip())/'App'
def box(im,rect,test):
 l,t,rr,bb=rect;pts=[(x,y) for y in range(t,bb) for x in range(l,rr) if test(im.getpixel((x,y)))]
 assert pts,rect
 return min(x for x,y in pts),min(y for x,y in pts),max(x for x,y in pts),max(y for x,y in pts)
gold=lambda p:p[:3]==(255,227,12)
white=lambda p:min(p[:3])>235
checks=[]
for direction in [-1,1]:
 reference=None;normalReference=None
 for n,value in enumerate([7,30,102,1234,19999],1):
  im=Image.open(core/f'best-sign-{value}-{direction}.png').convert('RGBA')
  cell=228-21*(n+1)-5+2
  bounds=box(im,(cell,169,cell+18,195),gold)
  normal=box(im,(29,75,53,101),white)
  if reference:
   assert bounds==(reference[0]-21*(n-1),reference[1],reference[2]-21*(n-1),reference[3]),(value,bounds,reference)
   assert normal==normalReference,(value,normal,normalReference)
  else:reference=bounds;normalReference=normal
  assert bounds[2]<228-21*n-5+2,(value,'sign touches digits')
  assert normal[0] in range(31,36),normal
  checks.append({'value':direction*value,'exSignBounds':bounds,'normalSignBounds':normal})
result={'status':'PASS','cases':checks,'ordinarySign':'fixed sample position','exSign':'one number cell before first digit','cellAdvance':21,'zeroSign':'hidden by existing zero-delta behavior'}
(r/'build/best-sign-pixels.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print('BEST SIGN PIXELS PASS: both signs, 1..5 digits, fixed ordinary sign and dynamic EX sign')
