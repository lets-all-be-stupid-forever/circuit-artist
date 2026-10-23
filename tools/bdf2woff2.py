#!/usr/bin/env python3
"""Convert the game's BDF pixel font into a WOFF2 the browser can use.

Each lit pixel becomes a square outline, so the result stays crisp instead of
being autotraced into curves. unitsPerEm is set so that one font pixel is
exactly one CSS px at the font's design size -- see UNITS/PIXEL_SIZE below.
Subsets to the codepoints a UI needs; the source covers all of Unicode.
"""
import sys
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

UNITS = 100  # font units per font pixel

WANTED = set(range(0x20, 0x7F)) | set(range(0xA0, 0x100)) | {
    0x2018, 0x2019, 0x201C, 0x201D, 0x2013, 0x2014, 0x2026, 0x2022,
    0x00B7, 0x2190, 0x2191, 0x2192, 0x2193, 0x00D7, 0x00F7, 0x2264, 0x2265,
}

def parse_bdf(path):
    props, glyphs = {}, {}
    cur = None
    reading_bitmap = False
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            if reading_bitmap:
                if line.startswith('ENDCHAR'):
                    reading_bitmap = False
                    if cur and cur['enc'] in WANTED:
                        glyphs[cur['enc']] = cur
                    cur = None
                else:
                    cur['rows'].append(line.strip())
                continue
            parts = line.split()
            if not parts:
                continue
            kw = parts[0]
            if kw == 'STARTCHAR':
                cur = {'enc': None, 'dwidth': 0, 'bbx': (0, 0, 0, 0), 'rows': []}
            elif kw == 'ENCODING' and cur is not None:
                cur['enc'] = int(parts[1])
            elif kw == 'DWIDTH' and cur is not None:
                cur['dwidth'] = int(parts[1])
            elif kw == 'BBX' and cur is not None:
                cur['bbx'] = tuple(int(p) for p in parts[1:5])
            elif kw == 'BITMAP' and cur is not None:
                reading_bitmap = True
            elif kw in ('FONT_ASCENT', 'FONT_DESCENT', 'PIXEL_SIZE'):
                props[kw] = int(parts[1])
    return props, glyphs

def glyph_outline(g):
    """Square outline per lit pixel, horizontal runs merged to cut point count."""
    pen = TTGlyphPen(None)
    w, h, xoff, yoff = g['bbx']
    for r, row in enumerate(g['rows']):
        if not row:
            continue
        bits = bin(int(row, 16))[2:].zfill(len(row) * 4)[:w]
        c = 0
        while c < w:
            if bits[c] == '1':
                start = c
                while c < w and bits[c] == '1':
                    c += 1
                x0 = (xoff + start) * UNITS
                x1 = (xoff + c) * UNITS
                y0 = (yoff + h - 1 - r) * UNITS
                y1 = y0 + UNITS
                pen.moveTo((x0, y0))
                pen.lineTo((x1, y0))
                pen.lineTo((x1, y1))
                pen.lineTo((x0, y1))
                pen.closePath()
            else:
                c += 1
    return pen.glyph()

def main(src, dst):
    props, glyphs = parse_bdf(src)
    px = props.get('PIXEL_SIZE', 12)
    ascent = props.get('FONT_ASCENT', 13)
    descent = props.get('FONT_DESCENT', 3)
    upem = px * UNITS

    order = ['.notdef'] + [f'uni{cp:04X}' for cp in sorted(glyphs)]
    cmap = {cp: f'uni{cp:04X}' for cp in sorted(glyphs)}

    fb = FontBuilder(upem, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)

    pen = TTGlyphPen(None)
    glyf = {'.notdef': pen.glyph()}
    metrics = {'.notdef': (px * UNITS, 0)}
    for cp in sorted(glyphs):
        g = glyphs[cp]
        name = f'uni{cp:04X}'
        glyf[name] = glyph_outline(g)
        metrics[name] = (g['dwidth'] * UNITS, g['bbx'][2] * UNITS)

    fb.setupGlyf(glyf)
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=ascent * UNITS, descent=-descent * UNITS)
    fb.setupNameTable({
        'familyName': 'Ark Pixel 12px Prop Latin',
        'styleName': 'Regular',
        'psName': 'ArkPixel12pxPropLatin-Regular',
    })
    fb.setupOS2(sTypoAscender=ascent * UNITS, sTypoDescender=-descent * UNITS,
                usWinAscent=ascent * UNITS, usWinDescent=descent * UNITS)
    fb.setupPost()
    fb.font.flavor = 'woff2'
    fb.save(dst)
    print(f"glyphs kept : {len(glyphs)} of {len(WANTED)} requested")
    print(f"upem        : {upem}  (1 font px = {UNITS} units -> font-size {px}px = 1:1)")
    print(f"ascent/desc : {ascent}/{descent} px")

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
