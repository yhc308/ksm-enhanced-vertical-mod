from pathlib import Path
import json,copy
r=Path(__file__).resolve().parents[1]
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def nodes(x):
 if isinstance(x,dict):
  yield x
  for c in x.get('children',[]):yield from nodes(c)
 elif isinstance(x,list):
  for c in x:yield from nodes(c)
def find(d,name):return next(n for n in nodes(d) if n.get('name')==name)
def param(d,name,kind,value):d['params'][name]={'type':kind,'value':value}
def region(x,y,w,h):return dict(type='AnchorRegion',anchorMin=[0,0],anchorMax=[0,0],posDelta=[x,y],sizeDelta=[w,h],sizeDeltaPivot=[0,0])
legacy=read(r/'kshootmania/App/ui/scene/select.noco')
originalBody=copy.deepcopy(find(legacy,'BTOptionPanel_C')['children'][0])
# Use native menu font metrics and independent labels only for the new four-row block.
template=copy.deepcopy(originalBody['children'][0])
def label(name,text,x,y,w,color='newPlusColor',valueRef=None):
 n=copy.deepcopy(template);n['name']=name;n['region']=region(x,y,w,60);n['children']=[]
 c=n['components'][0];c['text']=text;c.pop('text_paramRef',None)
 if valueRef:c['text_paramRef']=valueRef
 c.update(fontSize=36.,characterSpacing=[0.,0.],horizontalOverflow='Overflow',sizingMode='AutoShrinkWidth',color_paramRef=color,outlineFactorOuter=0.,outlineFactorInner=0.)
 return n
for mode in ['enhanced','vertical']:
 p=r/f'kshootmania/App/mods/{mode}/ui/select.noco';d=read(p)
 # Remove obsolete eight-row parameters before defining the split menu.
 for key in list(d['params']):
  if key.startswith('timingMenu'):del d['params'][key]
 for letter in 'ABCD':
  panel=find(d,'BTOptionPanel_'+letter)
  if letter=='C':panel['children']=[copy.deepcopy(originalBody)]
  body=panel['children'][0]
  for c in body['components']:
   if c.get('type')=='Sprite':c['preserveAspect']=True
  if mode=='vertical':body['region'].update(posDelta=[155.,170.25],sizeDelta=[640.,369.])
 base=find(d,'BTOptionPanel_C')['children'][0]
 extension=copy.deepcopy(base);extension['name']='NewPlusBody';extension['activeSelf_paramRef']='newPlusVisible';extension['activeSelf']=False
 extension['region']['posDelta']=[130.,-68.5] if mode=='enhanced' else [-172.,170.25]
 for c in extension['components']:
  if c.get('type')=='Sprite':c['color_paramRef']='newPlusColor'
 extension['children']=[label('judgeColorLabel','JUDGE COLOR:',40,64.5,380)]
 for name,text,x,w,color,ref in [('judgeLeft','',420,28,'newPlusColor','judgeLeft'),('judgeF','F',452,32,'judgeFastColor',None),('judgeSlash','/',492,28,'newPlusColor',None),('judgeS','S',532,32,'judgeSlowColor',None),('judgeRight','',572,28,'newPlusColor','judgeRight')]:extension['children'].append(label(name,text,x,64.5,w,color,ref))
 for i in range(1,4):
  for suffix,x,w in [('Label',40,380),('Value',430,170)]:
   name=f'timingMenu{i}{suffix}';param(d,name,'String','');extension['children'].append(label(name,'',x,64.5+60*i,w,valueRef=name))
 for name in ['newPlusColor','judgeFastColor','judgeSlowColor']:param(d,name,'Color',[255,255,255,0])
 for name in ['judgeLeft','judgeRight']:param(d,name,'String','')
 param(d,'newPlusVisible','Bool',False)
 find(d,'BTOptionPanel_C')['children'].append(extension)
 p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8-sig')
 # The result image path is a logical skin resource selected by the played palette.
 p=r/f'kshootmania/App/mods/{mode}/ui/result.noco';d=read(p)
 for n in nodes(d):
  for c in n.get('components',[]):
   if c.get('textureFilePath')=='imgs/result_v2_judge.png':c['textureFilePath_paramRef']='judgePanelImage'
 param(d,'judgePanelImage','String','imgs/result_v2_judge.png')
 p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8-sig')
print('Generated original-aspect BT panels, NEW+ blocks and palette result bindings')
