from pathlib import Path
import json,sys,statistics
from PIL import Image
app=Path(sys.argv[1]);w=int(sys.argv[2]);h=int(sys.argv[3]);r=Path(__file__).resolve().parents[1]
portrait=h>w
scale=w/720*1.05 if portrait else h/480
x,y=(24*w/720,318*w/720) if portrait else (w/2-300*scale,116*scale)
inverted=any(line.strip()=="judge_color=1" for line in (app/"config.ini").read_text(encoding="utf-8-sig").splitlines())
asset_name='play_judge_indicator_minimum'+('_inversion' if inverted else '')+'.png'
asset=Image.open(r/'imgs/트랙저지인디케이터'/asset_name)
asset=asset.convert('RGBA')
assert asset.width*160==asset.height*140
asset=asset.resize((140,160),Image.Resampling.LANCZOS)
assert (r/'imgs/트랙저지인디케이터'/asset_name).read_bytes()==(app/'imgs'/asset_name).read_bytes()
def local(name):
 im=Image.open(app/name).convert('RGB');assert im.size==(w,h),(name,im.size)
 return im.resize((140,160),Image.Resampling.LANCZOS,box=(x,y,x+140*scale,y+160*scale))
def white(im,box):
 return [(a,b) for b in range(box[1],box[3]) for a in range(box[0],box[2]) if min(im.getpixel((a,b)))>170 and max(im.getpixel((a,b)))-min(im.getpixel((a,b)))<35]
results={}
for kind in ['positive','negative','zero','large']:
 im=local('mini-'+kind+'.png')
 ends=[];widths=[]
 for i in range(7):
  hit=white(im,(64,28+13*i,116,38+13*i));assert hit,(kind,i,'missing counter')
  left=min(a for a,b in hit);right=max(a for a,b in hit)
  assert 108<=right<=115,(kind,i,left,right,'right alignment')
  assert left>=64,(kind,i,'left overflow')
  ends.append(right);widths.append(right-left+1)
 assert max(ends)-min(ends)<=2,(kind,ends)
 if kind!='large':assert widths[3]>statistics.median(widths[:3])*1.5,(kind,widths,'digit count')
 colors=[im.getpixel((a,b)) for b in range(124,136) for a in range(38,123)]
 colored=sum((min(red,green,blue)>145) if kind=='zero' else ((red>145 and red>blue*1.3) if ((kind in ['positive','large'])==inverted) else (blue>145 and blue>red*1.3)) for red,green,blue in colors)
 assert colored>5,(kind,colored,'average color/text')
 neutral=white(im,(38,124,123,136));assert neutral,(kind,'absolute mean text')
 if kind in ['positive','negative','large']:
  colored_points=[(a,b) for b in range(124,136) for a in range(38,123) if ((im.getpixel((a,b))[2]>145 and im.getpixel((a,b))[2]>im.getpixel((a,b))[0]*1.3) if ((kind in ['positive','large'])!=inverted) else (im.getpixel((a,b))[0]>145 and im.getpixel((a,b))[0]>im.getpixel((a,b))[2]*1.3))]
  assert 41<=min(a for a,b in colored_points)<=45,(kind,'mean not left-aligned after Avg')
  first_neutral=min(a for a,b in neutral);last_color=max(a for a,b in colored_points)
  assert 0<=first_neutral-last_color<=8,(kind,first_neutral,last_color,'gap between mean and deviation')
 else:assert 41<=min(a for a,b in neutral)<=45,'zero mean not inset-left-aligned' 
 assert max(a for a,b in neutral)<=122,(kind,'deviation escapes black band')
 results[kind]={'rightEdges':ends,'glyphWidths':widths,'averagePixels':colored}
# Opaque corners prove that the supplied PNG is at the intended anchor and scale.
points=[(15,124),(124,124),(15,136),(124,136)]
on=local('mini-positive.png')
errors=[max(abs(a-b) for a,b in zip(on.getpixel(p),asset.getpixel(p)[:3])) for p in points]
assert max(errors)<45,errors
results['panelPixelErrors']=errors
if (app/'mini-off.png').exists():
 off=local('mini-off.png')
 assert sum(max(abs(a-b) for a,b in zip(on.getpixel(p),off.getpixel(p))) for p in points)>80,'Off left mini visible'
 results['offHidesMini']=True
if (app/'mini-bottom-only.png').exists():
 bottom=local('mini-bottom-only.png')
 assert sum(max(abs(a-b) for a,b in zip(on.getpixel(p),bottom.getpixel(p))) for p in points)>80,'Bottom mode left mini visible'
 full=Image.open(app/'mini-positive.png').convert('RGB')
 base=Image.open(app/'mini-bottom-only.png').convert('RGB')
 off=Image.open(app/'mini-off.png').convert('RGB')
 v=w/720
 p=(round(40*v),round(998*v))
 assert max(abs(a-b) for a,b in zip(full.getpixel(p),base.getpixel(p)))<10,'Both changed the bottom panel'
 assert max(abs(a-b) for a,b in zip(base.getpixel(p),off.getpixel(p)))>15,'Off left bottom panel visible'
 results['bothAndBottomVisibility']=True
if (app/'mini-live.png').exists():
 live=local('mini-live.png')
 hits=white(live,(64,28,116,38));assert hits,'real zero counter missing'
 assert max(a for a,b in hits)-min(a for a,b in hits)<11,'zero padded to extra digits'
 results['realAutoSingleZeroNoPadding']=True
# Verify the BPM font's parameters are reused, with only alignment/overflow adjusted.
canvas=json.loads((app/('mods/vertical/ui/play_judge_indicator_minimum_vertical.noco' if portrait else 'mods/vertical/ui/play_judge_indicator_minimum.noco')).read_text(encoding='utf-8-sig'))
for n in canvas['children'][:7]:
 c=n['components'][0];p=json.loads(c['serializedParamsJSON'])
 assert c['canvasPath']=='ui/font/num2.noco' and all(abs(a-b)<1e-6 for a,b in zip(p['characterSize'],[10.5,9.45] if portrait else [10.0,9.0])) and p['horizontalAlign']=='Right'
 assert p['sizingMode']=='AutoShrinkWidth'
if not portrait:
 for n in canvas['children']:
  assert n['region']['anchorMin'][0]==n['region']['anchorMax'][0]==0.5,'mini not attached to centered song-info group'
 results['songInfoCenteredAnchor']=True
results['bpmFontReused']=True
(app/'mini-indicator-verification.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
print('MINI PIXELS PASS',w,h,json.dumps(results))
