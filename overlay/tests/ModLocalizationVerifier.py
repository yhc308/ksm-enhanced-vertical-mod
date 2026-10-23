"""Validate the existing five locales, unchanged base translations and native captures."""
from pathlib import Path
import argparse,json,re,string
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--resources-only',action='store_true');args=parser.parse_args()
checks=[]
def check(ok,name):
    if not ok:raise AssertionError(name)
    checks.append(name)
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
header=(root/'kshootmania/src/I18n/I18n.hpp').read_text(encoding='utf-8-sig')
block=re.search(r'enum class Mod\s*\{([^}]+)\}',header).group(1)
keys=set(re.findall(r'\b(\w+)\s*,',block))
languages=['English','Japanese','Korean','Simplified Chinese','Traditional Chinese']
base=read(root/'kshootmania/App/lang/English.json')['Mod']
formatter=string.Formatter()
def placeholders(s):return {field for _,field,_,_ in formatter.parse(s) if field is not None}
prior_log=root/'build/mod-ui-close-payload.log'
prior=Path(prior_log.read_text(encoding='utf-8-sig').strip())/'payload/lang' if prior_log.exists() else None
for language in languages:
    path=root/'kshootmania/App/lang'/f'{language}.json';data=read(path)
    check(set(data['Mod'])==keys,f'{language} has all {len(keys)} typed keys')
    for key,value in data['Mod'].items():
        check(bool(value.strip()),f'{language} {key} is nonempty')
        check(placeholders(value)==placeholders(base[key]),f'{language} {key} format arguments match English')
        if language!='Korean':check(not re.search('[가-힣]',value),f'{language} {key} has no leftover Korean')
    if prior and (prior/path.name).exists():
        old=read(prior/path.name)
        check({k:v for k,v in data.items() if k!='Mod'}=={k:v for k,v in old.items() if k!='Mod'},f'{language} original game translations preserved')
    for key in ['Play','Select','Change','Enter','PlayStop','Edit','TypeNumber','SaveReturn','ItemSelect','ValueAdjust','GroupItemMove','Close']:
        check(not any(token in data['Mod'][key] for token in ['START:','BT / FX:','↑↓ / R:','←→ / L:']),f'{language} {key} stores explanation only')
for rel in ['Scenes/Title/SkinOverlay.cpp','Scenes/Title/JudgeSettingOverlay.hpp','Scenes/Option/OptionScene.cpp','Common/SkinPreset.cpp']:
    text=(root/'kshootmania/src'/rel).read_text(encoding='utf-8-sig')
    for line in text.splitlines():
        if 'U"한글 Partial"' in line:continue # Fixture folder name is user metadata, never translated.
        check(not re.search('[가-힣]',line),f'{rel} has no fixed Korean UI: '+line[:55])
skin=(root/'kshootmania/src/Scenes/Title/SkinOverlay.cpp').read_text(encoding='utf-8-sig')
check("const auto lines=entry.description.split(U'\\n');" in skin,'skin descriptions bypass the translation system')
check('DefaultSkinDescription' not in header,'skin description has no translation key')
check('I18n::Mod::AuthorLabel' in skin,'only the author label is localized')
check(read(root/'kshootmania/App/imgs/skin.json')['description']=='K-Shoot MANIA v2.1.0 default skin','packaged default skin description is English metadata')
judge=(root/'kshootmania/src/Scenes/Title/JudgeSettingOverlay.hpp').read_text(encoding='utf-8-sig')
check('U"BT / FX: {}     ↑↓ / R: {}     ←→ / L: {}     START: {}"' in judge,'judge key names and Unicode arrows stay unchanged')
check('U"START or ESC or MOUSE Click : {}"' in judge,'PLAY STOP retains colon and untranslated key names')
if not args.resources_only:
    build=read(root/'build/latest.json');bid=build['buildId']
    judge_dir=root/'reference/judge-setting'/bid;viewport_dir=root/'reference/mod-localization'/bid
    for lang in range(5):
        for w,h in [(640,480),(1280,720),(1920,1080),(720,1280),(1080,1920)]:
            for stage in (0,1):
                p=judge_dir/f'judge-setting-l10n-{lang}-{w}-{h}-{stage}.png'
                pixels=np.asarray(Image.open(p).convert('RGB'))
                check(pixels.shape==(h,w,3),f'native judge locale capture {p.name}')
                region=pixels[int(h*.4):int(h*.6),int(w*.25):int(w*.75)] if stage else pixels[:max(40,int(h*.18))]
                check(np.count_nonzero(region.min(2)>160)>10,f'localized UI glyphs rendered {p.name}')
        for w,h in [(1280,720),(720,1280)]:
            for stage in (0,1,2):
                p=viewport_dir/f'viewport-l10n-{lang}-{w}-{h}-{stage}.png'
                pixels=np.asarray(Image.open(p).convert('RGB'))
                check(pixels.shape==(h,w,3),f'native viewport locale capture {p.name}')
                check(np.count_nonzero(pixels.min(2)>160)>100,f'viewport translated glyphs rendered {p.name}')
report={'status':'PASS','count':len(checks),'languages':languages,'resourcesOnly':args.resources_only,'checks':checks}
(root/'build/mod-localization-verifier.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(f'TOTAL {len(checks)} localization checks passed')
