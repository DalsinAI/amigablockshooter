#!/usr/bin/env python3
"""tools/png2c.py: PNGs made here with every row filter, RGB and RGBA,
decode to the pixels they were made from. MIT, Copyright (c) 2026 AmiCradle Team."""
import os
import random
import struct
import sys
import zlib

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
import png2c  # noqa: E402


def chunk(kind, body):
    return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xffffffff)


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else (b if pb <= pc else c)


def make_png(w, h, rows, bpp):
    raw = b''
    prev = bytes(w * bpp)
    for y, line in enumerate(rows):
        f = y % 5
        enc = bytearray()
        for i, v in enumerate(line):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            pred = [0, a, b, (a + b) // 2, paeth(a, b, c)][f]
            enc.append((v - pred) & 255)
        raw += bytes([f]) + bytes(enc)
        prev = line
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 6 if bpp == 4 else 2, 0, 0, 0)
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')


def main():
    rnd = random.Random(3)
    n = 0
    for bpp in (3, 4):
        w, h = 7, 11
        rows = [bytes(rnd.randrange(256) for _ in range(w * bpp)) for _ in range(h)]
        W, H, px = png2c.decode(make_png(w, h, rows, bpp))
        want = []
        for line in rows:
            for x in range(w):
                p = line[x * bpp:(x + 1) * bpp]
                want.append((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3] if bpp == 4 else 255))
        assert (W, H) == (w, h) and px == want, 'bpp %d' % bpp
        n += 1
    c = png2c.write_c(2, 1, [0x11223344, 0xffffffff], 'sheet')
    assert 'sheet_w = 2' in c and '0x11223344UL' in c
    n += 1
    try:
        png2c.decode(b'nope')
        raise AssertionError('accepted a non-PNG')
    except ValueError:
        n += 1
    print('png2c: %d checks pass' % n)


if __name__ == '__main__':
    main()
