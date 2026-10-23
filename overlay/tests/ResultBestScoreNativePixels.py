from pathlib import Path
import json
from PIL import Image
r=Path(__file__).resolve().parents[1]
core=Path((r/'build/ex-score-core-session.txt').read_text(encoding='utf-8-sig').strip())/'App'
zero=Image.open(core/'best-checker-5.png').convert('RGB');pos=Image.open(core/'best-checker-0.png').convert('RGBA');bg=Image.open(r/'imgs/세부판정리절트/result_bestscore_checker.png').convert('RGBA')
ratios=[]
for dy in range(22):
 for dx in range(20):
  full=zero.getpixel((204+dx,46+dy));ghost=zero.getpixel((57+dx,46+dy))
  if min(full)>235:
   base=bg.getpixel((55+dx,44+dy));ratios.append(sum(ghost[i]-base[i] for i in range(3))/sum(255-base[i] for i in range(3)))
assert ratios and abs(sum(ratios)/len(ratios)-.25)<.015
for x,y in [(30,74),(181,167)]:
 for dy in range(22):
  for dx in range(18):
   assert zero.getpixel((x+dx+2,y+dy+2))==bg.getpixel((x+dx,y+dy))[:3],'zero difference has sign'
for y in [5,28,105,130,202]:
 for x in range(4,240):
  a=pos.getpixel((x+2,y+2));b=bg.getpixel((x,y))
  assert a[3]==b[3] and (b[3]==0 or a==b),(x,y,a,b)
assert any(p[:3]==(255,227,12) for p in pos.crop((100,139,230,161)).getdata())
large=Image.open(core/'best-checker-6.png').convert('RGBA')
# The sign occupies the cell immediately before a five-digit EX deficit.
for y in range(169,191):
 for x in range(116,118):
  assert large.getpixel((x+2,y+2))[:3]!=(255,227,12),(x,y,'EX sign/digits overlap')
print('NATIVE CHECKER PIXELS PASS: leading zero alpha',sum(ratios)/len(ratios),'zero signs absent; exact gold; original background unchanged')
(r/'build/result-best-native-pixels.json').write_text(json.dumps({'status':'PASS','leadingZeroOpacity':sum(ratios)/len(ratios),'zeroSigns':'absent','fill':'#ffe30c','background':'unchanged'},indent=2))
