"""Compare the same GameMain frame on the scene and the preview render target."""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

root = Path(sys.argv[1])
direct = np.asarray(Image.open(root / 'viewport-direct-native.png').convert('RGB')).astype(np.int16)
preview = np.asarray(Image.open(root / 'viewport-offscreen-native.png').convert('RGB')).astype(np.int16)
assert direct.shape == preview.shape, 'Preview must retain the complete native resolution'
height, width, _ = direct.shape
delta = np.max(np.abs(direct - preview), axis=2)
# The engine FPS counter is drawn independently of GameMain at the end of a frame.
mask = np.ones((height, width), dtype=bool)
mask[int(height * .94):, int(width * .75):] = False
changed = (delta > 3) & mask
fraction = float(changed.sum() / mask.sum())
report = {'status': 'PASS' if fraction < .005 else 'FAIL', 'width': width,
          'height': height, 'changedPixelFraction': fraction,
          'meanChannelDelta': float(np.abs(direct - preview)[mask].mean()),
          'tolerance': 'At most 0.5% changed pixels (>3 RGB levels), excluding engine FPS'}
(root / 'viewport-frame-parity.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
assert fraction < .005, f'Native and preview raster differ: {report}'
print(json.dumps(report))
