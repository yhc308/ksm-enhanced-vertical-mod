from pathlib import Path
import csv,json,hashlib,sys
from PIL import Image,ImageDraw
from PreviewTraceRules import verify_preview_loop_boundary, verify_preview_loop_period, verify_preview_edit_clock
def verify_trace(dest):
    rows=[]
    with (dest/'preview-trace.csv').open(encoding='utf-8-sig',newline='') as f:
        for r in csv.reader(f):
            rows.append(dict(wall=float(r[0]),chart=float(r[1]),score=int(r[2]),gauge=float(r[3]),folder=r[4],policy=int(r[5]),bg1=int(r[6]),bg2=int(r[7]),laser=float(r[8]),note=int(r[9]),judged=int(r[10]) if len(r)>10 else None,samples=int(r[11]) if len(r)>11 else None,mean=float(r[12]) if len(r)>14 else None,absolute=float(r[13]) if len(r)>14 else None,demo=int(r[14]) if len(r)>14 else None,ex=int(r[22]) if len(r)>23 else None,exmax=int(r[23]) if len(r)>23 else None))
    frame_count=len(rows)
    assert rows and max(r['wall'] for r in rows)>38
    loops=[(a,b) for a,b in zip(rows,rows[1:]) if b['chart']<a['chart']-10]
    assert len(loops)>=2, f'Expected 2 loops: {len(loops)}'
    if rows[0]['judged'] is not None:
        assert all(a['judged']>0 and b['judged']==0 and b['samples']==0 for a,b in loops), 'indicator must reset with each preview loop'
        assert all(r['demo'] is not None for r in rows),'missing preview demo trace'
        offsets=[150,75,30,0,-35,-95,-150]
        for r in rows:
            values=[offsets[i%7] for i in range(r['demo']) if i%7 not in (0,6)]
            assert r['samples']==len(values),'ERROR/long/laser included in demo mean or missing sample'
            assert abs(r['mean']-(sum(values)/len(values) if values else 0))<.001,'demo mean mismatch'
            assert abs(r['absolute']-(sum(map(abs,values))/len(values) if values else 0))<.001,'demo absolute mean mismatch'
        assert any(r['mean']>1 for r in rows) and any(r['mean']<-1 for r in rows),'demo mean should move FAST to SLOW' 
        for a,b in zip(rows,rows[1:]):
            if b['chart']>=a['chart'] and b['chart']-a['chart']<1.5:
                assert b['demo']>=a['demo'], 'skin changes and pause must not reset chip sequence'
                assert b['judged']>=a['judged'], 'skin changes and pause must not clear live counts'
    if rows[0].get('ex') is not None:
        assert all(0<=x['ex']<=x['exmax'] and x['exmax']>0 for x in rows)
        assert all(a['ex']>0 and b['ex']==0 for a,b in loops),'EX must reset with preview loops'
        for a,b in zip(rows,rows[1:]):
            if b['chart']>=a['chart'] and b['chart']-a['chart']<1.5:
                assert b['ex']>=a['ex'],'skin/numeric changes reset EX'
    for previous, current in loops:
        verify_preview_loop_boundary(previous, current)
    for first, second in zip(loops, loops[1:]):
        verify_preview_loop_period(first, second)
    assert all(r['score']==0 and abs(r['gauge'])<1e-5 for r in rows if 17.55<r['chart']<17.95)
    before=[r for r in rows if 16.8<r['chart']<17]
    half=[r for r in rows if 17.2<r['chart']<17.3]
    assert before and half and min(r['score'] for r in before)>0
    assert 0<half[0]['score']<before[0]['score']
    changes=[(a,b) for a,b in zip(rows,rows[1:]) if (a['folder'],a['policy'])!=(b['folder'],b['policy'])]
    assert len(changes)>=3
    for previous, current in changes:
        verify_preview_edit_clock(previous, current)
    numeric_changes=[(a,b) for a,b in zip(rows,rows[1:]) if any(a[k]!=b[k] for k in ['bg1','bg2','laser','note'])]
    assert len(numeric_changes)>=8
    for previous, current in numeric_changes:
        verify_preview_edit_clock(previous, current)
    assert {0,50,75,100}.issubset({r['bg1'] for r in rows})
    assert {0,40,50,100}.issubset({r['bg2'] for r in rows})
    assert {50,100,125,200}.issubset({r['note'] for r in rows})
    assert all(50 <= r['laser'] <= 125 for r in rows)
    assert {50,100,125}.issubset({r['laser'] for r in rows})
    assert any(abs(r['laser']-800/11)<0.01 for r in rows)
    for name in ['preview-default.png','preview-partial.png','preview-error.png','preview-reset-half.png','preview-reset-zero.png','preview-second-loop.png','preview-confirm.png','title-restored.png']:
        assert (dest/name).exists(),name
    for name in ['viewport-bg1-zero.png','viewport-both-zero.png','viewport-background-half.png','viewport-width-50-note-50.png','viewport-vertical-note-100.png','viewport-width-125-note-200.png','viewport-curves-125.png']:
        assert (dest/name).exists(),name
    dark=Image.open(dest/'viewport-both-zero.png').convert('RGB')
    layer=Image.open(dest/'viewport-bg1-zero.png').convert('RGB')
    w,h=dark.size
    u=min(h/720,w/960)
    rw,rh=(720,1280) if h>w else (w,h)
    scale=0.95*min((w*2/3-48*u)/rw,(h-230*u)/rh)
    pw,ph=rw*scale,rh*scale
    px,py=(w*2/3-pw)/2,(274*w/720 if h>w else 172*u)
    if h>w:
        # No portrait controls or hints may occupy the artwork's letterbox bands.
        for band in [(0,0,w,int(110*w/720)),(0,int(1170*w/720),w,h)]:
            assert not any(min(c)>150 for c in dark.crop(band).getdata()), 'portrait UI leaked into letterbox'
        # The progress line belongs below the large title, before the small song panel.
        # A host-size/preview-size mixup moves it outside this strip at 1080x1920.
        x1,x2=int(px+60*scale),int(px+650*scale)
        rows=range(int(py+169*scale)-2,int(py+179*scale)+3)
        coverage=max(sum(min(dark.getpixel((x,y)))>75 and max(dark.getpixel((x,y)))-min(dark.getpixel((x,y)))<50 for x in range(x1,x2))/(x2-x1) for y in rows)
        assert coverage>.65, f'portrait progress bar displaced: coverage={coverage}'
    # Background-only patch, away from HUD and lane: both zero must be black.
    # At 4:3 the forced gameplay mini-indicator occupies the old .32-.65 patch.
    # Sample below it, still outside the highway; keep the same strict black threshold.
    classic=h<w and w/h<1.5
    box=(int(px+5*u),int(py+ph*(.36 if h>w else .70 if classic else .32)),int(px+pw*.12),int(py+ph*(.50 if h>w else .85 if classic else .65)))
    dark_pixels=list(dark.crop(box).getdata())
    layer_pixels=list(layer.crop(box).getdata())
    assert sum(max(c)<=3 for c in dark_pixels)/len(dark_pixels)>.99, 'both backgrounds zero are not black'
    assert sum(max(c)>20 for c in layer_pixels)>len(layer_pixels)*.05, 'animation layer not independent of background1'
    right=(int(w*2/3+4*u),int(180*u),w-int(20*u),int(1035*w/720) if h>w else h-int(100*u))
    assert dark.crop(right).tobytes()==layer.crop(right).tobytes(), 'background edit changes menu/list brightness'
    with (dest/'title-bgm-return.csv').open(encoding='utf-8-sig',newline='') as f:
        fade=list(csv.DictReader(f))
    assert [(v['phase'],v['playing'],v['fading']) for v in fade]==[('start','1','1'),('middle','1','1'),('done','1','0')],fade
    return {'frames':frame_count,'loops':len(loops),'switches':len(changes),'numeric_changes':len(numeric_changes),'loop_boundaries':[(a['chart'],b['chart']) for a,b in loops]}
app=Path(sys.argv[2])
if sys.argv[1]=='setup':
    skins=app/'skins';skins.mkdir(exist_ok=True);partial=skins/'한글 Partial';partial.mkdir()
    image=Image.open(app/'imgs/score_header.png').convert('RGBA');ImageDraw.Draw(image).rectangle((0,0,image.width-1,image.height-1),fill=(0,180,220,255));image.save(partial/'score_header.png')
    (partial/'standby.jpg').write_bytes(b'corrupt image fixture')
    (partial/'skin.json').write_text(json.dumps({'author':'테스트 제작자','description':'부분 스킨: 점수 헤더만 교체합니다.'},ensure_ascii=False),encoding='utf-8')
    (app/'imgs/Legacy Ignored').mkdir()
    (app/'imgs/Legacy Ignored/skin.json').write_text('{}',encoding='utf-8')
    for name in ['z-last','Alpha','beta']:(skins/name).mkdir()
    import shutil
    for asset in ['result_v2_judge','play_judge_indicator','play_judge_indicator_minimum']:
        for suffix in ['', '_inversion']:
            name=asset+suffix+'.png'
            shutil.copyfile(app/'imgs'/name,skins/'Alpha'/name)
    (skins/'beta/skin.json').write_text('{bad json',encoding='utf-8')
else:
    result=verify_trace(app)
    save=sys.argv[3]=='save'
    value=json.loads((app/'skin_preset.json').read_text(encoding='utf-8-sig'))
    assert value['folder']==('한글 Partial' if save else 'default'),value
    assert value['background1Brightness']==(75 if save else 100),value
    assert value['background2Opacity']==(40 if save else 100),value
    assert abs(value['laserWidthPercent']-(800/11 if save else 100))<1e-8,value
    assert value['noteHeightPercent']==(125 if save else 100),value
    cfg=dict(line.split('=',1) for line in (app/'config.ini').read_text(encoding='utf-8-sig').splitlines() if '=' in line)
    assert cfg['hispeed']=='713',cfg.get('hispeed')
    assert (app/'score/sentinel.txt').read_text()=='USER-SCORE'
    assert len(list((app/'score').rglob('*')))==1
    assert all(Image.open(p).size==tuple(map(int,cfg['windowsize'].split(','))) for p in app.glob('*.png'))
    (app/'skin-verification.json').write_text(json.dumps(result,indent=2))
    print('SKIN PASS',result)

