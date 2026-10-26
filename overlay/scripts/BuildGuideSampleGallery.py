from pathlib import Path
import csv,json
import numpy as np
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
workspace=root.parent
out=root/'test-output/guide-v16'
public=workspace/'기어_투시조정_v16_샘플/night_techno'
fontpath='C:/Windows/Fonts/malgun.ttf'
font=ImageFont.truetype(fontpath,22)
small=ImageFont.truetype(fontpath,18)
def gallery(files,labels,name,width):
 h=round(width*1280/720)
 image=Image.new('RGB',(len(files)*(width+12)+12,h+78),(17,23,33))
 d=ImageDraw.Draw(image)
 for i,(file,label) in enumerate(zip(files,labels)):
  x=12+i*(width+12);d.text((x,12),label,font=font,fill='white')
  image.paste(Image.open(public/file).convert('RGB').resize((width,h),Image.Resampling.LANCZOS),(x,52))
 image.save(public/name)
gallery(['02_기울기_-10도.png','03_기울기_-5도.png','01_중립.png','04_기울기_+5도.png','05_기울기_+10도.png'],['기울기 -10°','기울기 -5°','중립','기울기 +5°','기울기 +10°'],'비교_노브기울기.png',288)
gallery(['06_상단_-100.png','01_중립.png','07_상단_+100.png'],['상단 -100','중립','상단 +100'],'비교_상단변형.png',360)
gallery(['08_하단_-100.png','01_중립.png','09_하단_+100.png'],['하단 -100','중립','하단 +100'],'비교_하단변형.png',360)
base=Image.open(public/'01_중립.png').convert('RGB')
layer=Image.new('RGBA',base.size)
d=ImageDraw.Draw(layer);s=base.width/720;m=.5508076027535542
P=lambda x,y:(round(x*s),round(y*s))
for sign in (-1,1):
 for ratio in (1,.75):
  X=lambda y:360+sign*ratio*m*(y-400)
  d.line([P(X(448),448),P(X(934),934)],fill=(0,255,60,210),width=2)
  for y0,y1 in ((400,448),(934,1002)):
   d.line([P(X(y0),y0),P(X(y1),y1)],fill=(255,65,55,200),width=2)
d.line([P(360,390),P(360,1100)],fill=(255,238,0,190),width=1)
for y in (448,934):
 half=m*(y-400);d.line([P(360-half,y),P(360+half,y)],fill=(255,238,0,210),width=2)
d.line([P(0,906),P(720,906)],fill=(255,150,0,235),width=2)
Image.alpha_composite(base.convert('RGBA'),layer).convert('RGB').save(public/'가이드_중첩_1080.png')
# Preserve the supplied guide as a reference panel, without changing it.
reference=workspace/'기어 디폴트 각도 조정안/기어디폴트각도_조정안_00_디폴트기어_가이드라인.png'
comparison=Image.new('RGB',(3*720+48,1350),(17,23,33));draw=ImageDraw.Draw(comparison)
for i,(label,im) in enumerate([('사용자 원본 가이드',Image.open(reference)),('수정 후 실제 렌더',base),('공통 투시 가이드 중첩',Image.open(public/'가이드_중첩_1080.png'))]):
 x=12+i*732;draw.text((x,12),label,font=font,fill='white');comparison.paste(im.convert('RGB').resize((720,1280),Image.Resampling.LANCZOS),(x,52))
comparison.save(public/'가이드_비교.png')
old=workspace/'Night_Techno_각도별_샘플_20261015/01_중립.png'
if old.exists():
 beforeafter=Image.new('RGB',(2*720+36,1345),(17,23,33));d=ImageDraw.Draw(beforeafter)
 for i,(label,path) in enumerate([('변경 전: 72% / 14% / 14%',old),('변경 후: 75% / 12.5% / 12.5%',public/'01_중립.png')]):
  x=12+i*732;d.text((x,12),label,font=font,fill='white');beforeafter.paste(Image.open(path).convert('RGB').resize((720,1280),Image.Resampling.LANCZOS),(x,50))
 beforeafter.save(public/'변경_전후.png')
validation={'depth_off_equals_bypass':[],'wall_on_geometry':[]}
for folder in sorted((out/'themes').iterdir()):
 for bottom in (-100,0,100):
  off=np.asarray(Image.open(folder/f'bottom{bottom}-off.png').convert('RGB'))
  bypass=np.asarray(Image.open(folder/f'bottom{bottom}-bypass.png').convert('RGB'))
  delta=np.max(np.abs(off.astype(np.int16)-bypass.astype(np.int16)),axis=2);raw_difference=int(np.count_nonzero(delta))
  delta[int(off.shape[0]*.96):,int(off.shape[1]*.80):]=0
  max_delta=int(delta.max());equal=max_delta<=1;assert equal,(folder.name,bottom,max_delta)
  validation['depth_off_equals_bypass'].append({'theme':folder.name,'bottom':bottom,'equivalent_except_fps':equal,'max_channel_delta':max_delta,'raw_difference_pixels':raw_difference})
  with (folder/f'bottom{bottom}-on.csv').open(encoding='utf-8-sig') as f: row=next(csv.DictReader(f))
  valid=row['valid']=='true';assert valid or folder.name=='hidden',(folder.name,bottom,row)
  if valid:
   assert float(row['uv_error'])<1e-6 and float(row['shared_edge_error'])<.01 and float(row['straight_error'])<.01
   assert float(row['min_area_ratio'])>=.1999 and float(row['cover_margin'])>=1.48
  validation['wall_on_geometry'].append({'theme':folder.name,'bottom':bottom,'active':valid,'uv_error':float(row['uv_error']),'straight_error':float(row['straight_error']),'seam_error':float(row['shared_edge_error']),'cover_margin':float(row['cover_margin'])})
validation['compound_states']=[]
for path in sorted((out/'compound').glob('*.csv')):
 if path.name.endswith('.geometry.csv'): continue
 with path.open(encoding='utf-8-sig') as f: row=next(csv.DictReader(f))
 assert row['valid']=='true' and float(row['uv_error'])<1e-6 and float(row['min_area_ratio'])>=.1999,path
 validation['compound_states'].append({'file':path.name,**row})
with (root/'kshootmania/App/vertical-guide-calibration.csv').open(encoding='utf-8-sig') as f: guide=list(csv.DictReader(f))
assert len(guide)==6 and all(r['status']=='PASS' and float(r['max_error_px'])<=1 for r in guide)
validation['guide']=guide
(out/'render-validation.json').write_text(json.dumps(validation,ensure_ascii=False,indent=2),encoding='utf-8')
(public/'샘플_안내.txt').write_text('실제 게임 렌더러의 표시 진단 샘플입니다. 1080×1920, night 배경, techno 벽, 게이지 0%, 점수 0, HiSPEED 700, JURER/U*KEY 메타데이터와 리소스를 사용합니다. 채보 연출·판정의 수동 플레이 재현 영상은 아닙니다. 기울기는 표시 각도를 주입했고, 상하단 값은 원본 확대축소 계산에 전달했습니다. 벽의 원래 회전 연동과 추가 입체 움직임을 켠 상태입니다. 원본 PNG와 노브·상단·하단 비교 PNG를 각각 제공합니다. 가이드 중첩은 비교 표시를 추가한 이미지이며 원본 캡처는 01_중립.png입니다.\n',encoding='utf-8')
print('PASS: 가이드 6해상도, 배경 ON 21조건, OFF/우회 일치 21조건, 복합 8조건, 샘플 비교 이미지')
