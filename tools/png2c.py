#!/usr/bin/env python3
"""Writes a C file with a PNG's pixels: an 8-bit RGBA (or RGB) PNG without
interlace, as upstream's sprite sheet is, decoded with zlib and the five
PNG row filters; the pixels as 0xRRGGBBAA words, the same on either byte
order.

    png2c.py IN.png OUT.c NAME

gives `const unsigned long NAME[]`, `NAME_w` and `NAME_h`.
MIT, Copyright (c) 2026 AmiCradle Team."""
import struct
import sys
import zlib


def decode(data):
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('not a PNG')
    pos, idat, w = 8, b'', None
    while pos < len(data):
        n, kind = struct.unpack('>I4s', data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + n]
        if kind == b'IHDR':
            w, h, depth, ctype, _, _, interlace = struct.unpack('>IIBBBBB', body)
            if depth != 8 or ctype not in (2, 6) or interlace:
                raise ValueError('only 8-bit RGB or RGBA, not interlaced')
            bpp = 4 if ctype == 6 else 3
        elif kind == b'IDAT':
            idat += body
        elif kind == b'IEND':
            break
        pos += 12 + n
    if w is None:
        raise ValueError('no IHDR')
    raw = zlib.decompress(idat)
    stride = w * bpp
    prev = bytearray(stride)
    pixels = []
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 255
            elif f != 0:
                raise ValueError('bad filter %d' % f)
        for x in range(w):
            px = line[x * bpp:(x + 1) * bpp]
            r, g, bl = px[0], px[1], px[2]
            al = px[3] if bpp == 4 else 255
            pixels.append((r << 24) | (g << 16) | (bl << 8) | al)
        prev = line
    return w, h, pixels


def write_c(w, h, pixels, name):
    out = ['/* written by tools/png2c.py; 0xRRGGBBAA */',
           'const unsigned int %s_w = %d, %s_h = %d;' % (name, w, name, h),
           'const unsigned long %s[%d] = {' % (name, w * h)]
    for i in range(0, len(pixels), 8):
        out.append('\t' + ', '.join('0x%08xUL' % p for p in pixels[i:i + 8]) + ',')
    out.append('};')
    return '\n'.join(out) + '\n'


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    w, h, px = decode(open(sys.argv[1], 'rb').read())
    open(sys.argv[2], 'w').write(write_c(w, h, px, sys.argv[3]))


if __name__ == '__main__':
    main()
