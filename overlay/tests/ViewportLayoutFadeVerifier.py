"""Check fixed-age captures produced by the real timing HUD renderer."""
from pathlib import Path
import argparse,json
import numpy as np
from PIL import Image

parser=argparse.ArgumentParser()
parser.add_argument('app',type=Path)
parser.add_argument('--width',type=int,required=True)
parser.add_argument('--height',type=int,required=True)
a=parser.parse_args()
ages=[0,849,850,975,1099,1100]
frames={age:np.asarray(Image.open(a.app/f'timing-fade-{age}.png').convert('RGB')) for age in ages}
for age,pixels in frames.items():
    assert pixels.shape==(a.height,a.width,3),(age,pixels.shape)
energy={age:int(pixels.sum(dtype=np.uint64)) for age,pixels in frames.items()}
assert energy[0]>1000,'Timing glyphs were not rendered'
assert np.array_equal(frames[0],frames[849]),'Fade started before 850ms'
assert np.array_equal(frames[0],frames[850]),'850ms must retain the full initial opacity'
assert energy[0]>energy[975]>energy[1099], 'Text/glow must fade after 850ms'
# The existing renderer truncates opacity to an 8-bit alpha: 0.9 * (1/250) * 255 = 0.918 -> 0.
# The unit test separately requires a positive logical opacity at 1099ms.
expected_late_alpha=int(.9*(1100-1099)/250*255)
assert expected_late_alpha==0 and energy[1099]==0, '1099ms capture differs from the existing 8-bit alpha conversion'
assert energy[1100]==0,'Timing text/glow remains after 1100ms'
report={'status':'PASS','size':[a.width,a.height],'agesMs':ages,'renderedEnergy':energy,'holdImagesIdentical':True,'fullyHiddenAt1100ms':True,'rasterAlphaAt1099ms':expected_late_alpha}
(a.app/'timing-fade-verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report))
