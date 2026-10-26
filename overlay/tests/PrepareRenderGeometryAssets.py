from pathlib import Path
import sys
from PIL import Image
app=Path(sys.argv[1]);assets=app/'imgs'
# Deliberately solid fixtures expose exact draw bounds; real gameplay assets are never modified.
for name in ['judge.gif','judge1.gif','judge_p.gif','judge_p1.gif','laser_cur.png','bt_chip.gif','fx_chip.gif','fx_chip_se.gif']:
 p=assets/name
 im=Image.open(p)
 Image.new('RGB',im.size,'white').save(p)

# A thin symmetric bar exposes its actual rendered center without artwork-dependent glow.
p=assets/'cline.png'
im=Image.open(p)
bar=Image.new('RGBA',im.size,(0,0,0,0))
from PIL import ImageDraw
ImageDraw.Draw(bar).rectangle((0,im.height//2-1,im.width-1,im.height//2+1),fill=(255,255,255,255))
bar.save(p)

# Place a thin marker at the animation's ring pivot (overlay row 70), not its text/art bounds.
for name in ['judge1.gif','judge_p1.gif']:
 p=assets/name
 im=Image.open(p)
 marker=Image.new('RGB',im.size,'black')
 draw=ImageDraw.Draw(marker)
 for y in range(0,im.height,150):
  draw.rectangle((0,y+91,im.width-1,y+93),fill='white')
 marker.save(p)
