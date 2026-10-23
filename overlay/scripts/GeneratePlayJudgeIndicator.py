"""Build the portrait indicator canvas from the existing FPS and timing font definitions."""
from pathlib import Path
import copy,json
root=Path(__file__).resolve().parents[1]
ui=root/'kshootmania/App/mods/vertical/ui'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def find(nodes,name):
 for n in nodes:
  if n['name']==name:return n
  found=find(n['children'],name)
  if found:return found
def box(n,x,y,w,h):
 n['region']={'type':'AnchorRegion','anchorMin':[0,0],'anchorMax':[0,0],
  'posDelta':[x,y],'sizeDelta':[w,h],'sizeDeltaPivot':[0,0]}
timing=read(ui/'timing_feedback.noco');hud=read(ui/'play_hud.noco')
doc=copy.deepcopy(timing);doc['referenceSize']=[720,1280];doc['autoFitMode']='Contain'
doc['children']=[];doc['params']={'countersVisible':{'type':'Bool','value':True}}
for i in range(7):
 node=copy.deepcopy(find(hud['children'],'FPSNumber'));node['name']=f'JudgeCount{i}'
 node['activeSelf_paramRef']='countersVisible';box(node,84+80*i,1070,72,36)
 c=node['components'][0];c['serializedParamBindingsJSON']=json.dumps({'text':f'count{i}'})
 settings=json.loads(c['serializedParamsJSON']);settings.update(characterSize=[14,14],horizontalAlign='Center',horizontalOverflow='Overflow',sizingMode='AutoShrinkWidth')
 c['serializedParamsJSON']=json.dumps(settings);doc['children'].append(node)
 doc['params'][f'count{i}']={'type':'String','value':'0'}
for name,x,align in [('average',176,'Right'),('absolute',409,'Left')]:
 node=copy.deepcopy(find(timing['children'],'Timing0Ms'));node['name']=name.title()
 node.pop('activeSelf_paramRef',None);node['activeSelf']=True;box(node,x,1026,136,34)
 c=node['components'][0];c['text_paramRef']=name+'Text';c['fontSize']=18;c['horizontalAlign']=align;c['sizingMode']='AutoShrink';c['minFontSize']=8
 for field,suffix in [('gradationColor1','Top'),('gradationColor2','Bottom'),('outlineColor','Outline'),('shadowColor','Shadow')]:
  c[field+'_paramRef']=name+suffix
  doc['params'][name+suffix]={'type':'Color','value':[178,178,178,255]}
 doc['params'][name+'Text']={'type':'String','value':'0.0ms' if name=='average' else '±0.0ms'}
 doc['children'].append(node)
(ui/'play_judge_indicator.noco').write_text(json.dumps(doc,ensure_ascii=False,indent=2)+'\n',encoding='utf-8-sig')
print('Generated portrait play judge indicator canvas')

# Local 140x160 mini canvas: the renderer scales/translates the entire resource and text together.
mini=copy.deepcopy(timing);mini['referenceSize']=[140,160];mini['autoFitMode']='Contain'
mini['children']=[];mini['params']={'countersVisible':{'type':'Bool','value':True}}
landscape=read(root/'kshootmania/App/ui/scene/play_hud.noco')
for i in range(7):
 node=copy.deepcopy(find(landscape['children'],'BPMNumber'));node['name']=f'MiniCount{i}'
 node['activeSelf_paramRef']='countersVisible';box(node,64,28+13*i,51,11)
 c=node['components'][0];c['serializedParamBindingsJSON']=json.dumps({'text':f'count{i}'})
 settings=json.loads(c['serializedParamsJSON']);settings.update(horizontalAlign='Right',horizontalOverflow='Overflow',sizingMode='AutoShrinkWidth')
 c['serializedParamsJSON']=json.dumps(settings);mini['children'].append(node)
 mini['params'][f'count{i}']={'type':'String','value':'0'}
for name,x,width,align in [('average',42,34,'Left'),('absolute',78,44,'Left')]:
 node=copy.deepcopy(find(timing['children'],'Timing0Ms'));node['name']='Mini'+name.title()
 node.pop('activeSelf_paramRef',None);node['activeSelf']=True;box(node,x,123,width,13)
 c=node['components'][0];c['text_paramRef']=name+'Text';c['fontSize']=9.5;c['horizontalAlign']=align;c['sizingMode']='AutoShrink';c['minFontSize']=5
 for field,suffix in [('gradationColor1','Top'),('gradationColor2','Bottom'),('outlineColor','Outline'),('shadowColor','Shadow')]:
  c[field+'_paramRef']=name+suffix
  mini['params'][name+suffix]={'type':'Color','value':[178,178,178,255]}
 mini['params'][name+'Text']={'type':'String','value':'0.0' if name=='average' else '±0.0ms'}
 mini['children'].append(node)
# SubCanvas numeric glyphs use the canvas's own layout transform. Bake the same local
# design into each reference canvas so glyphs and regular labels share one transform.
for vertical,origin,factor in [(False,(20,116),1.0),(True,(24,318),1.05)]:
 doc=copy.deepcopy(mini)
 doc['referenceSize']=[720,1280] if vertical else [640,480]
 doc['autoFitMode']='Contain' if vertical else 'FitHeightMatchWidth'
 for n in doc['children']:
  rect=n['region'];rect['posDelta']=[origin[i]+factor*rect['posDelta'][i] for i in range(2)]
  rect['sizeDelta']=[factor*v for v in rect['sizeDelta']]
  if not vertical:
   rect['anchorMin'][0]=rect['anchorMax'][0]=0.5
   rect['posDelta'][0]-=320
  c=n['components'][0]
  if c['type']=='SubCanvas':
   settings=json.loads(c['serializedParamsJSON']);settings['characterSize']=[factor*v for v in settings['characterSize']];c['serializedParamsJSON']=json.dumps(settings)
  else:
   c['fontSize']*=factor;c['minFontSize']*=factor
 suffix='_vertical' if vertical else ''
 (ui/f'play_judge_indicator_minimum{suffix}.noco').write_text(json.dumps(doc,ensure_ascii=False,indent=2)+'\n',encoding='utf-8-sig')
print('Generated mini indicator reference canvases from the same local BPM design')
