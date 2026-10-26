"""Arrange actual engine captures into contact sheets and a linked gallery."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parent.parent / 'test-output/background-v2-samples'
records = json.loads((root / 'manifest.json').read_text(encoding='utf-8-sig'))
font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 18)
small = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 14)
for kind in ('base', 'layer'):
    names = sorted({r['theme'] for r in records if r['kind'] == kind})
    columns = 5 if kind == 'base' else 4
    rows = (len(names) + columns - 1) // columns
    width, height = 308, 306
    sheet = Image.new('RGB', (columns*width, rows*height + 44), '#101820')
    draw = ImageDraw.Draw(sheet)
    draw.text((12, 12), f'{kind.upper()} THEMES | left: gauge 0%, right: gauge 70% | engine captures', font=font, fill='white')
    for i, name in enumerate(names):
        x, y = (i % columns)*width, 44 + (i // columns)*height
        draw.text((x+10, y+3), name, font=font, fill='white')
        for j, gauge in enumerate((0, 70)):
            path = root / f'{kind}-{name}-gauge{gauge}.png'
            capture = Image.open(path).convert('RGB')
            capture.thumbnail((144, 256), Image.Resampling.LANCZOS)
            sheet.paste(capture, (x+8+j*150, y+28))
            draw.text((x+8+j*150, y+286), f'{gauge}%', font=small, fill='#b6cfda')
    sheet.save(root / f'{kind}-overview.png')

lines = ['# 배경 테마별 실제 엔진 샘플', '',
         '각 테마를 게이지 0%와 70%에서 캡처했습니다. 렌더링 전용 미리보기이며 실제 플레이 결과가 아닙니다.', '',
         '기본 배경은 arrow 레이어, 애니메이션 비교는 night 배경을 사용합니다. 같은 고정 시간·콤보를 사용하며 배경 상태 전환은 원본 방식입니다.', '']
for kind, label in [('base','기본 배경 13종'),('layer','애니메이션 7종')]:
    lines += [f'## {label}', '', f'![{label}]({(root / (kind+"-overview.png")).as_posix()})', '', '| 테마 | 게이지 0% | 게이지 70% |', '|---|---|---|']
    for name in sorted({r['theme'] for r in records if r['kind'] == kind}):
        links = [f'[원본 크기]({(root / f"{kind}-{name}-gauge{g}.png").as_posix()})' for g in (0,70)]
        lines.append(f'| {name} | {links[0]} | {links[1]} |')
    lines += ['']
(root/'샘플_목록.md').write_text('\n'.join(lines), encoding='utf-8')
print(f'Generated gallery and two contact sheets for {len(records)} captures.')
