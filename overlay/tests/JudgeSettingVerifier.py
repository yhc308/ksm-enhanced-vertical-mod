"""Check captured UI pixels, native input traces and immutable authoring resources."""
from pathlib import Path
import json, csv, hashlib, argparse, os
import numpy as np
from PIL import Image

root=Path(__file__).resolve().parents[1]
run=json.loads((root/'build/judge-setting-tests.json').read_text(encoding='utf-8-sig'))
folder=Path(run['reference'])
parser=argparse.ArgumentParser()
parser.add_argument('--authoring-root',type=Path,default=Path(os.environ.get('KSM_AUTHORING_ROOT', str(root/'authoring/judge-setting'))),help='Explicit immutable authoring snapshot for scoped resource-preservation checks')
parser.add_argument("--skip-authoring",action="store_true",help="Check UI/replay pixels without an external authoring snapshot; report this limitation explicitly")
args=parser.parse_args()
checks=[]
def check(ok,name):
    if not ok: raise AssertionError(name)
    checks.append(name)
def crop(img,rect):
    x,y,w,h=rect
    return img[max(0,round(y)):round(y+h),max(0,round(x)):round(x+w)]
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
    for stage in (0,2):
        stem=f'judge-setting-{w}-{h}-{stage}'
        meta=json.loads((folder/(stem+'.json')).read_text(encoding='utf-8-sig'))
        img=np.asarray(Image.open(folder/(stem+'.png')).convert('RGB')).astype(float)
        check(img.shape[:2]==(h,w),f'native dimensions {stem}')
        check(meta['worldScale']==1.0,f'original gameplay projection preserved {stem}')
        for name in ('left','right','mini','bottom'):
            x,y,rw,rh=meta[name]
            check(x>=0 and y>=0 and x+rw<=w+.1 and y+rh<=h+.1,f'{name} on screen {stem}')
        area=crop(img,meta['judgeColor']);center=area.shape[1]//2
        fast=(88,132,220) if meta['palette']==0 else (220,88,100)
        slow=(220,88,100) if meta['palette']==0 else (88,132,220)
        check(np.count_nonzero(np.abs(area[:,:center]-fast).max(2)<48)>0,f'F uses selected FAST palette {stem}')
        check(np.count_nonzero(np.abs(area[:,center:]-slow).max(2)<48)>0,f'S uses selected SLOW palette {stem}')
        for i in range(7):
            x,y,rw,rh=meta['bottom'];s=rw/720
            if rw:
                pixels=crop(img,(x+(112+80*i)*s,y+117*s,16*s,17*s))
                check(np.count_nonzero(pixels.min(2)>180)>0,f'bottom count inside cell {i} {stem}')
            x,y,rw,rh=meta['mini'];s=rw/280
            pixels=crop(img,(x+215*s,y+(55+26*i)*s,23*s,20*s))
            check(np.count_nonzero(pixels.min(2)>180)>0,f'mini count aligned with label {i} {stem}')
# Verify the polished menus against captured native pixels, not a recreated mockup.
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
    captures={}
    metadata={}
    for stage in (0,1,4,5,6):
        stem=f'judge-setting-{w}-{h}-{stage}'
        captures[stage]=np.asarray(Image.open(folder/(stem+'.png')).convert('RGB')).astype(float)
        metadata[stage]=json.loads((folder/(stem+'.json')).read_text(encoding='utf-8-sig'))
    m=metadata[0]
    for i in range(10):
        parent=m['left' if i<5 else 'right'];px,py,pw,ph=parent
        for key in [f'title{i}']+([f'value{i}'] if i<8 else []):
            x,y,rw,rh=m[key]
            check(x>=px and y>=py and x+rw<=px+pw+.01 and y+rh<=py+ph+.01,f'{key} fits menu panel {w}x{h}')
    check(m['arrows0']==[True,False] and m['arrows1']==[False,True],f'initial endpoint arrows {w}x{h}')
    check(metadata[6]['arrows0']==[False,True] and metadata[6]['arrows5']==[False,True],f'lower endpoint arrows {w}x{h}')
    area=crop(captures[0],m['value3'])
    check(np.count_nonzero(np.abs(area-204).max(2)<12)>0,f'choice text uses cccccc {w}x{h}')
    if h>w:
        check(captures[0][:int(h/1280*125),int(48*h/480):].max()<=40,f'portrait black alpha letterbox {w}x{h}')
    for stage,side in [(1,'right'),(5,'left')]:
        x,y,rw,rh=m[side];pad=max(2,rw*.025)
        region=(x-pad,y+rh*.2,pad,rh*.5)
        before=crop(captures[0],region).mean();after=crop(captures[stage],region).mean()
        check(after>before+1,f'white outer glow follows {side} panel {w}x{h}')
    # Stage 4 has a confirmed +30ms sample, using the inverted FAST red palette.
    expected=np.array([208,192,93])*.7+np.array([220,88,100])*.3
    center=captures[4][:,:]  # crop to the central statistic band only
    if h>w: region=(w*.3,h*312/1280,w*.4,h*150/1280)
    else: region=(w*.35,h*36/480,w*.3,h*100/480)
    area=crop(center,region)
    check(np.count_nonzero(np.abs(area-expected).max(2)<8)>0,f'30ms gold plus FAST color mixture {w}x{h}')
# Header anchors and statistic spacing use the actual native drawing/hit geometry.
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
    meta=json.loads((folder/f'judge-setting-{w}-{h}-4.json').read_text(encoding='utf-8-sig'))
    check(meta['windowTitle']=='K-Shoot Mania V2.1.0 - Enhanced Vertical Mod (Unofficial Build)',f'window caption updated {w}x{h}')
    heading=meta['heading'];base_close=meta['headerClose'];close=meta['close']
    check(abs(close[2]-48*h/480)<.01 and abs(close[3]-48*h/480)<.01,f'BACK matches original OPTION size including portrait {w}x{h}')
    speed_scale=w/720 if h>w else h/480
    panel_scale=.56 if h>w else .5
    x,y,sw,sh=meta['hispeed']
    check(abs(sw-452*panel_scale*speed_scale)<.01 and abs(sh-54*panel_scale*speed_scale)<.01,f'hispeed matches original loading panel scale {w}x{h}')
    check(abs(x+sw-w)<.01 and abs(y+sh-h)<.01,f'hispeed anchored lower right {w}x{h}')

    check(abs(heading[1]-(191*h/1280 if h>w else 12*min(h/720,w/960)))<.01,f'heading moves near the window or portrait letterbox boundary {w}x{h}')
    if w>h:
        u=min(h/720,w/960);controls=meta['controls'];stop=meta['stop']
        check(abs(controls[1]-10*u)<.01,f'landscape hints retain a small window-edge margin {w}x{h}')
        check(abs(controls[0]+controls[2]-(w-12*u))<.01,f'hints reclaim screen-right width {w}x{h}')
        check(close[:2]==[0,0],f'original BACK anchored at upper left {w}x{h}')
        check(heading[0]+heading[2]<controls[0],f'title and controls are horizontally disjoint {w}x{h}')
        centers=[meta[k][1]+meta[k][3]/2 for k in ['recentTitle','recentValue','lastTitle','lastValue']]
        font=meta['statFontSize'];scale=h/480
        for i,old_pitch in enumerate([25.5,22.5,25.5]):
            old_blank=old_pitch*scale-font;new_blank=centers[i+1]-centers[i]-font
            check(abs(new_blank-old_blank/2)<.01,f'statistic interline whitespace halved {w}x{h} gap {i}')
    else:
        check(close[:2]==[0,0],f'portrait original BACK anchored at upper left {w}x{h}')
# Each statistic must reflect its own value, including opposite signs and differing magnitudes.
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
    for stage,recent,last in [(4,30,30),(7,-30,-30),(8,30,90)]:
        stem=f'judge-setting-{w}-{h}-{stage}'
        meta=json.loads((folder/(stem+'.json')).read_text(encoding='utf-8-sig'))
        img=np.asarray(Image.open(folder/(stem+'.png')).convert('RGB')).astype(float)
        x,y,rw,rh=meta['close']
        check(0<=x and 0<=y and x+rw<=w and y+rh<=h,f'close button on screen {stem}')
        for key,ms in [('recentValue',recent),('lastValue',last)]:
            fast=(88,132,220) if meta['palette']==0 else (220,88,100)
            slow=(220,88,100) if meta['palette']==0 else (88,132,220)
            amount=min(1,abs(ms)//10*.1)
            expected=np.array([208,192,93])*(1-amount)+np.array(fast if ms>=0 else slow)*amount
            area=crop(img,meta[key])
            check(np.count_nonzero(np.abs(area-expected).max(2)<8)>0,f'{key} uses its own {ms}ms color {stem}')
for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
    scale=w/720 if h>w else min(w/640,h/720)
    size=(64 if h>w else 56)*scale
    for focus in range(3):
        frames=[np.asarray(Image.open(folder/f'judge-setting-main-icons-{w}-{h}-{i}.png').convert('RGB')).astype(float) for i in range(3)]
        x=w/2+(focus-1)*(size+12*scale)-size/2
        y=(622 if h>w else 20)*scale
        selected=crop(frames[focus],(x-4*scale,y+size*.25,3*scale,size*.5)).mean()
        unselected=crop(frames[(focus+1)%3],(x-4*scale,y+size*.25,3*scale,size*.5)).mean()
        check(selected>unselected+2,f'mod icon white outside glow {w}x{h} focus {focus}')
for mode in ('landscape','portrait'):
    rows=list(csv.DictReader((folder/f'judge-setting-{mode}-frames.csv').open(encoding='utf-8-sig')))
    menus=[r for r in rows if r['menu']=='true']
    check(bool(menus) and len({r['totalSamples'] for r in menus})==1,f'{mode} menu does not gather input samples')
    check(max(int(r['loops']) for r in rows)>=2,f'{mode} crosses two 16 second repeats')
    for previous,current in zip(rows,rows[1:]):
        if previous['totalSamples']==current['totalSamples']:
            check(previous['mean']==current['mean'] and previous['last']==current['last'],f'{mode} displayed statistics hold without a new note at '+current['time'])
    for row in rows:
        t=float(row['time']);expected=t%16
        check(abs(float(row['chartTime'])-expected)<1e-6,f'{mode} repeat time {t:.3f}')
    check(int(rows[-1]['totalSamples'])>int(rows[0]['totalSamples']),f'{mode} real input measurements survive repeats')
    check((folder/f'judge-setting-{mode}.mp4').stat().st_size>10000,f'{mode} comparison video saved')
if not args.skip_authoring:
    author=args.authoring_root
    for src in author.iterdir():
        if src.suffix.lower() not in ('.kson','.mp3'):continue
        dst=root/'kshootmania/App/mods/enhanced/judge_setting'/src.name
        check(hashlib.sha256(src.read_bytes()).digest()==hashlib.sha256(dst.read_bytes()).digest(),'authoring song hash '+src.name)
    src=root/'imgs/menu_judge_icon.png';dst=root/'kshootmania/App/ui/menu_judge_icon.png'
    check(src.read_bytes()==dst.read_bytes(),'authoring menu icon unchanged')
    src=root/'imgs/판정세팅모드/Judge_Setting_Mode_banner.png';dst=root/'kshootmania/App/mods/enhanced/judge_setting/Judge_Setting_Mode_banner.png'
    check(src.read_bytes()==dst.read_bytes(),'authoring menu panel unchanged')
report={'status':'PASS','count':len(checks),'reference':str(folder),'authoringRoot':None if args.skip_authoring else str(args.authoring_root),'authoringChecksSkipped':args.skip_authoring,'checks':checks}
(root/'build/judge-setting-pixels.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'TOTAL {len(checks)} judge-setting pixel/trace/resource checks passed')
