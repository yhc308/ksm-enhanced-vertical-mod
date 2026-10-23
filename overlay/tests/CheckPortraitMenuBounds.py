from pathlib import Path
from PIL import Image
import json,sys
paths=[Path(p) for p in sys.argv[1:]]
for path in paths:
 im=Image.open(path).convert('RGB');w,h=im.size
 assert h>w,path
 for band in [(0,0,w,int(110*w/720)),(0,int(1170*w/720),w,h)]:
  assert not any(min(c)>150 for c in im.crop(band).getdata()),f'UI outside portrait content: {path}'
 print('LETTERBOX PASS',path)
# Compare the edge band around focused and unfocused mod buttons in one stationary frame.
im=Image.open(paths[0]).convert('RGB')
def border_white(left):
 return sum(min(im.getpixel((x,y)))>180 for x in range(left,left+64) for y in range(622,686) if min(x-left,left+63-x,y-622,685-y)<3)
assert border_white(366)>90,(border_white(366),'focused custom border missing')
assert border_white(290)<20,(border_white(290),'unfocused vertical border should be absent')
print('FOCUS PASS',border_white(366),border_white(290))
