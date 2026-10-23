from pathlib import Path
from PIL import Image
import json
r=Path(__file__).resolve().parents[1]
core=Path((r/'build/ex-score-core-session.txt').read_text(encoding='utf-8-sig').strip())/'App'
before=r/'build/score-panel-alignment-before'
def bbox(im,rect,predicate):
 x0,y0,x1,y1=rect;points=[(x,y) for y in range(y0,y1) for x in range(x0,x1) if predicate(im.getpixel((x,y)))]
 assert points,rect
 return (min(x for x,y in points),min(y for x,y in points),max(x for x,y in points),max(y for x,y in points))
white=lambda p:min(p[:3])>235
gold=lambda p:p[0]>190 and p[1]>150 and p[2]<90
old=Image.open(before/'best-checker-0.png').convert('RGBA');new=Image.open(core/'best-checker-0.png').convert('RGBA')
for rect,predicate in [((50,46,231,68),white),((50,78,231,100),white),((105,139,231,161),gold),((183,171,231,193),gold)]:
 a=bbox(old,rect,predicate);b=bbox(new,rect,predicate)
 assert b==(a[0]-5,a[1],a[2]-5,a[3]),(a,b,'number offset')
for oldrect,newrect,predicate,dx in [((15,74,42,103),(25,74,55,103),white,10),((75,167,103,196),(155,167,181,196),gold,80)]:
 a=bbox(old,oldrect,predicate);b=bbox(new,newrect,predicate)
 assert b[0]==a[0]+dx and b[2]==a[2]+dx and (b[3]-b[1]+1)>(a[3]-a[1]+1),(a,b,'sign alignment/height')
oldex=Image.open(before/'ex-panel-6630.png').convert('RGBA');newex=Image.open(core/'ex-panel-6630.png').convert('RGBA')
# Player number remains right aligned and moves down exactly one source pixel.
a=bbox(oldex,(125,5,166,18),gold);b=bbox(newex,(125,5,166,19),gold)
assert b==(a[0],a[1]+1,a[2],a[3]+1),(a,b)
small=Image.open(core/'ex-panel-8.png').convert('RGBA')
right=bbox(small,(179,6,222,20),gold)
assert right[0] in range(180,185) and right[2]<193,right
# The ordinary four-digit maximum starts at the same left anchor as a one-digit maximum.
regular=bbox(newex,(179,6,222,20),gold);assert right[0]==regular[0],(right,regular)
result={'status':'PASS','bestNumberOffsetX':-5,'signHeightScale':1.2,'exTopNumberOffsetY':1,'maximumAlignment':'left','smallMaximumBounds':right}
(r/'build/score-panel-alignment-pixels.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print('SCORE PANEL ALIGNMENT PIXELS PASS')
