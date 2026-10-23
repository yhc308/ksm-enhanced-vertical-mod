"""Generate a small MJPEG AVI for render QA using Pillow, without extra codecs."""
from io import BytesIO
from pathlib import Path
import struct
from PIL import Image, ImageDraw

def chunk(kind, data):
    return kind + struct.pack('<I', len(data)) + data + (b'\0' if len(data) & 1 else b'')

def list_chunk(kind, data):
    return chunk(b'LIST', kind + data)

width, height, fps, count = 320, 180, 30, 90
frames = []
for index in range(count):
    frame = Image.new('RGB', (width, height), (20, 45, 85))
    draw = ImageDraw.Draw(frame)
    draw.rectangle((12 + index * 2, 65, 52 + index * 2, 105), fill=(40, 220, 170))
    draw.text((15, 20), 'VERTICAL MOVIE / 16:9', fill='white')
    stream = BytesIO()
    frame.save(stream, 'JPEG', quality=85)
    frames.append(stream.getvalue())
max_size = max(map(len, frames))
avih = struct.pack('<14I', 1_000_000 // fps, max_size * fps, 0, 0x10, count, 0, 1, max_size, width, height, 0, 0, 0, 0)
strh = struct.pack('<4s4sIHH8I4h', b'vids', b'MJPG', 0, 0, 0, 0, 1, fps, 0, count, max_size, 0xFFFFFFFF, 0, 0, 0, width, height)
strf = struct.pack('<IiiHH4sIiiII', 40, width, height, 1, 24, b'MJPG', width * height * 3, 0, 0, 0, 0)
hdrl = list_chunk(b'hdrl', chunk(b'avih', avih) + list_chunk(b'strl', chunk(b'strh', strh) + chunk(b'strf', strf)))
movie = b''
index_data = b''
for frame in frames:
    index_data += struct.pack('<4sIII', b'00dc', 0x10, 4 + len(movie), len(frame))
    movie += chunk(b'00dc', frame)
data = b'AVI ' + hdrl + list_chunk(b'movi', movie) + chunk(b'idx1', index_data)
target = Path(__file__).resolve().parents[1] / 'test-output' / 'vertical-movie-fixture.avi'
target.parent.mkdir(exist_ok=True)
target.write_bytes(chunk(b'RIFF', data))
print(target)
