from pathlib import Path
import csv,html,json,math,sys
from PIL import Image,ImageDraw,ImageFont
sys.stdout.reconfigure(encoding='utf-8')
root=Path(__file__).resolve().parents[2]/'버티컬_인게임_각도비교_샘플_20261017'
data=root/'측정자료';manifest=json.loads((data/'샘플목록.json').read_text(encoding='utf-8-sig'))
assert len(manifest['cases'])==46 and manifest['configuration_preserved']
normalized=root/'720x1280_비교용';sheets=root/'비교표';sheets.mkdir(exist_ok=True)
font=ImageFont.truetype('C:/Windows/Fonts/malgun.ttf',18)
small=ImageFont.truetype('C:/Windows/Fonts/malgun.ttf',14)
groups={};measure=[]
for case in manifest['cases']:
 source=root/case['folder']/(case['name']+'.png')
 with Image.open(source) as image:
  assert image.size==(1080,1920),(source,image.size)
  target=normalized/case['folder']/source.name;target.parent.mkdir(parents=True,exist_ok=True)
  image.convert('RGB').resize((720,1280),Image.Resampling.LANCZOS).save(target)
 groups.setdefault(case['folder'],[]).append(case)
 with (data/case['folder']/(case['name']+'.기어.csv')).open(encoding='utf-8-sig') as f:g=next(csv.DictReader(f))
 assert g['ray_valid']=='true',source
 row={'분류':case['folder'],'파일':source.name,'표시조건':case['flags']}
 for src,dest in [('anchor_x','앵커X'),('anchor_y','앵커Y'),('bar_x','판정선X'),('bar_y','판정선Y'),('end_y','기어하단Y'),('anchor_error','앵커간거리')]:row[dest]=round(float(g[src])/1.5,5)
 row['하단길이']=float(g['tail_pixels']);row['교점유효']=g['ray_valid'];measure.append(row)
with (data/'기어_위치측정_720기준.csv').open('w',newline='',encoding='utf-8-sig') as f:
 writer=csv.DictWriter(f,fieldnames=list(measure[0]));writer.writeheader();writer.writerows(measure)
def gallery(cases,name,cols=3,width=240):
 height=round(width*1280/720);rows=math.ceil(len(cases)/cols)
 sheet=Image.new('RGB',(cols*(width+12)+12,rows*(height+48)+14),(17,23,33));d=ImageDraw.Draw(sheet)
 for i,case in enumerate(cases):
  x=12+(i%cols)*(width+12);y=10+(i//cols)*(height+48)
  label=case['label'].replace('노브 ','표시 ').replace(' 도','°')
  d.text((x,y),label,font=small,fill='white')
  source=root/case['folder']/(case['name']+'.png')
  with Image.open(source) as image:sheet.paste(image.convert('RGB').resize((width,height),Image.Resampling.LANCZOS),(x,y+28))
 sheet.save(sheets/name)
for group,cases in groups.items():gallery(cases,group+'.png',3 if len(cases)>4 else 2,240 if len(cases)>4 else 360)
for group in ['01_노브_기울기','02_상단_확대축소','03_하단_확대축소']:
 cases=groups[group];gallery([cases[0],cases[4],cases[-1]],group+'_끝값비교.png',3,360)
# Keep exactly the same screen crop and scale for every silhouette comparison.
core=Image.new('RGB',(3*720+48,3*(680+42)+12),(17,23,33));draw=ImageDraw.Draw(core)
for r,group in enumerate(['01_노브_기울기','02_상단_확대축소','03_하단_확대축소']):
 cases=groups[group]
 for c,case in enumerate([cases[0],cases[4],cases[-1]]):
  x=12+c*732;y=12+r*722;draw.text((x,y),case['label'].replace(' 도','°'),font=font,fill='white')
  image=Image.open(normalized/case['folder']/(case['name']+'.png')).convert('RGB').crop((0,320,720,1000));core.paste(image,(x,y+32))
core.save(sheets/'핵심_9자세_기어영역.png')
readme='''# 버티컬 인게임 각도 비교 샘플 · 2026-10-17

최신 v17 게임 실행 파일의 실제 렌더러로 생성한 원본 PNG 46장입니다. 사볼 화면과 기어 형태·투시·움직임을 비교할 수 있도록 정리했습니다.

## 폴더

- 00_기본_인게임: 디폴트 외곽, 노트/레이저, 판정 효과/와이드, 고정 체인 HUD 4장.
- 01_노브_기울기: -10 / -7.5 / -5 / -2.5 / 0 / +2.5 / +5 / +7.5 / +10도 9장.
- 02_상단_확대축소: zoom_top -100부터 +100까지 25단위, 9장.
- 03_하단_확대축소: zoom_bottom -100부터 +100까지 25단위, 9장.
- 04_좌우_이동: shiftX -50 / -25 / 0 / +25 / +50, 5장.
- 05_복합_변형: 상단/하단 -100/+100 조합, 기울기 +10도, 4장.
- 06_원본_회전연출: Tornaido Test의 양방향 원본 회전 이벤트, 진행 25/50/75%, 6장.
- 720x1280_비교용: 원본과 같은 구성을 720×1280으로 비례 축소한 PNG 46장. 기존 가이드와 겹쳐 비교할 때 사용하세요.
- 비교표: 폴더별 전체 비교, 노브/상단/하단의 끝값-중립-끝값 비교, 핵심9자세 기어 부분 확대 비교.
- 측정자료: 적용한 값, 명령행, 기어/판정 앵커 및 배경 메쉬의 엔진 출력 CSV. 위치측정표는 720×1280으로 환산했습니다.

## 공통 조건과 해석

1080×1920, night 배경, techno 벽, 게이지0%, 점수0, HiSPEED700, 추가 배경 입체 움직임 ON입니다. 레이아웃은 v16 기본 투시이며, 체인은 v17 고정 HUD입니다. 화면별 FPS 표시는 실행 시점에 따라 다릅니다.

기어 비교용은 JURER/U*KEY의 메타데이터와 리소스를 사용하고, 시작 시점을 고정하여 외곽이 가려지지 않도록 했습니다. 노트/효과 예시는 별도 합성 진단 채보이며, 회전 예시는 실제 Tornaido Test 이벤트를 사용합니다. 원본 PNG에는 가이드나 치수 문자를 추가하지 않았습니다.

노브 비교표의 ±100%는 이번 표에서 ±10도에 붙인 정규화 표기입니다. 게임 표시 기울기를 직접 지정한 진단으로, 실제 노브 입력량이나 사볼의 내부 각도를 측정한 값은 아닙니다. 상·하단 값은 퍼센트 확대율이 아니라 케슛의 원본 zoom_top/zoom_bottom 입력값입니다. 이 값의 차이가 기어의 위치·각도·크기에 어떻게 나타나는지를 비교하시면 됩니다.

사볼 사진을 겹칠 때는 촬영된 모니터의 원근과 컨트롤러 부분을 먼저 구분해 주세요. 비교표의 기어영역 이미지는 모두 720×1280 화면의 y=320~1000 구간을 같은 위치와 배율로 잘라서, 이미지마다 자동으로 크기를 맞추지 않았습니다.

실제 플레이 입력·판정·음원 재생을 수행하는 검사는 아니며, 게임의 표시 렌더를 재현한 이미지입니다. 게임 설정과 실행 파일은 변경하지 않았습니다.
'''
(root/'샘플_안내.md').write_text(readme,encoding='utf-8')
parts=['<!doctype html><html lang="ko"><meta charset="utf-8"><title>버티컬 인게임 각도 비교</title><style>body{background:#111822;color:#e9edf2;font-family:Malgun Gothic,sans-serif;margin:24px}a{color:#83d4ff}nav a{margin-right:18px;line-height:2}.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(210px,1fr));gap:18px}.card{background:#1d2938;padding:12px;border-radius:8px}.card img{width:100%;height:auto}h2{margin-top:38px}.note{color:#b4c3d4;font-size:14px}</style><h1>버티컬 인게임 각도 비교 · 2026-10-17</h1><p>1080×1920 · night / techno · 게이지 0% · HiSPEED 700 · v17 고정 체인 HUD</p><p class="note">이미지를 누르면 원본 PNG를 엽니다. 노브 표기는 주입한 표시 기울기이며 사볼 내부 각도와 동일하다는 뜻은 아닙니다.</p><p><a href="샘플_안내.md">샘플 안내</a> / <a href="비교표/핵심_9자세_기어영역.png">핵심 9자세 기어영역 비교</a></p><nav>']
for group in groups:parts.append(f'<a href="#{group}">{group}</a>')
parts.append('</nav>')
for group,cases in groups.items():
 parts.append(f'<section id="{group}"><h2>{group}</h2><p><a href="비교표/{group}.png">전체 비교표</a></p><div class="grid">')
 for case in cases:
  link=html.escape(case['folder']+'/'+case['name']+'.png',quote=True);label=html.escape(case['label']);normalized_link='720x1280_비교용/'+link
  parts.append(f'<div class="card"><p>{label}</p><a href="{link}"><img loading="lazy" src="{link}" alt="{label}"></a><p><a href="{link}">1080 원본</a> / <a href="{normalized_link}">720 비교용</a></p></div>')
 parts.append('</div></section>')
parts.append('</html>');(root/'샘플_목록.html').write_text(''.join(parts),encoding='utf-8')
summary={'original_images':46,'normalized_images':46,'comparison_sheets':len(list(sheets.glob('*.png'))),'native_resolution':[1080,1920],'normalized_resolution':[720,1280],'executable_sha256':manifest['executable_sha256'],'configuration_preserved':True,'source_executable_unchanged':True,'groups':{k:len(v) for k,v in groups.items()}}
(data/'생성결과.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False))
