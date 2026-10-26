from pathlib import Path
import json,copy
r=Path(__file__).resolve().parents[1]
def load(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def walk(n):
 yield n
 for child in n.get('children',[]):yield from walk(child)
def find(d,name):return next(n for n in walk(d) if n.get('name')==name)
def box(n,x,y,w,h,anchor=(0,0)):
 n['region']={'type':'AnchorRegion','anchorMin':list(anchor),'anchorMax':list(anchor),'posDelta':[x,y],'sizeDelta':[w,h],'sizeDeltaPivot':[0,0]}
for mode in ['enhanced','vertical']:
 p=r/f'kshootmania/App/mods/{mode}/ui/play_hud.noco'
 d=load(r/'kshootmania/App/ui/scene/play_hud.noco') if mode=='enhanced' else load(p)
 d['children']=[n for n in d['children'] if n.get('name') not in ['ExScoreCaption','ExScoreNumber']]
 caption=copy.deepcopy(find(d,'ScoreCaption'));caption['name']='ExScoreCaption'
 for c in caption['components']:
  if c.get('type')=='Sprite':c['textureFilePath']='imgs/score_header_ex.png';c['preserveAspect']=True
 number=copy.deepcopy(find(d,'ScoreNumber'));number['name']='ExScoreNumber'
 sub=next(c for c in number['components'] if c.get('type')=='SubCanvas')
 params=json.loads(sub['serializedParamsJSON'])
 params.update({'characterSize':[13,17],'characterSpacing':[0,0],'horizontalAlign':'Right','horizontalOverflow':'Overflow','sizingMode':'AutoShrinkWidth','text':'{}'})
 sub['serializedParamsJSON']=json.dumps(params,separators=(',',':'))
 sub['serializedParamBindingsJSON']='{"text":"exScoreNumber"}'
 sub['serializedParamBindingModesJSON']='{"text":"Format"}'
 if mode=='enhanced':
  box(caption,142,68,56.5,22.5,(.5,0));box(number,201,70.5,67,17,(.5,0))
 else:
  total=find(d,'ScoreNumber');box(total,439,257,272,34)
  c=next(c for c in total['components'] if c.get('type')=='SubCanvas');a=json.loads(c['serializedParamsJSON']);a['characterSize']=[33,34];c['serializedParamsJSON']=json.dumps(a,separators=(',',':'))
  box(caption,563,288,65.54,26.1);box(number,630,292,73,17)
 d['children'] += [caption,number]
 d['params']['exScoreNumber']={'type':'String','value':'0'}
 p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
