from pathlib import Path
import json,copy
r=Path(__file__).resolve().parents[1]
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def save(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def walk(n):
 if isinstance(n,list):
  for x in n:yield from walk(x)
 elif isinstance(n,dict):
  yield n
  for x in n.get('children',[]):yield from walk(x)
def find(d,name):return next(n for n in walk(d) if n.get('name')==name)
result=load(r/'kshootmania/App/mods/enhanced/ui/result.noco')
template=copy.deepcopy(find(result,'DetailedJudgmentPanel'))
def panel(pos,size=(228,46),anchor=(0,0),scale=(1,1)):
 n=copy.deepcopy(template);n['name']='ExScorePanel';n['children']=[];n['activeSelf']=False;n.pop('activeSelf_paramRef',None)
 n['region']={'type':'AnchorRegion','anchorMin':list(anchor),'anchorMax':list(anchor),'posDelta':list(pos),'sizeDelta':list(size),'sizeDeltaPivot':[0,0] if anchor==(0,0) else [.5,.5]}
 n['transform']['scale']=list(scale)
 n['components']=[c for c in n['components'] if c.get('type')=='Sprite']
 for c in n['components']:
  c['textureFilePath']='';c.pop('textureFilePath_paramRef',None);c.pop('color_paramRef',None);c['color']=[255,255,255,255];c['preserveAspect']=True
 return n
# Preserve original part; enhanced and vertical own their new panel layout.
for mode in ['enhanced','vertical']:
 p=r/f'kshootmania/App/mods/{mode}/ui/select_item_center.noco'
 d=load(r/'kshootmania/App/ui/parts/select_item_center.noco') if mode=='enhanced' else load(p)
 song=find(d,'Song');song['children']=[x for x in song['children'] if x.get('name')!='ExScorePanel'];song['children'].append(panel((13,87)));save(p,d)
 p=r/f'kshootmania/App/mods/{mode}/ui/select.noco';d=load(p)
 if mode=='enhanced':
  for n in walk(d):
   for c in n.get('components',[]):
    if c.get('canvasPath')=='ui/parts/select_item_center.noco':c['canvasPath']='mods/enhanced/ui/select_item_center.noco'
 save(p,d)
 p=r/f'kshootmania/App/mods/{mode}/ui/result.noco';d=load(p);d['children']=[x for x in d['children'] if x.get('name')!='ExScorePanel']
 d['children'].append(panel((471,698)) if mode=='vertical' else panel((183,85.6),anchor=(.5,.5),scale=(.78,.78)))
 save(p,d)

# Best-score checker uses its own source dimensions plus a 2px glow margin.
for mode in ['enhanced','vertical']:
 p=r/f'kshootmania/App/mods/{mode}/ui/result.noco';d=load(p)
 d['children']=[x for x in d['children'] if x.get('name')!='ResultBestScorePanel']
 if mode=='vertical':
  n=panel((586,829.9),size=(248,214),anchor=(0,0),scale=(.78,.78))
  n['region']['sizeDeltaPivot']=[.5,.5]
 else:
  n=panel((183,169.2186),size=(248,214),anchor=(.5,.5),scale=(.5874,.5874))
 n['name']='ResultBestScorePanel';d['children'].append(n);save(p,d)
