#!/usr/bin/env python3
"""Generate the platform icon files from the pixel-art PNGs in assets/imgs.

  python3 packaging/make_icons.py

Produces:
  packaging/windows/ca.ico   (16, 24, 32, 48, 256)
  packaging/macos/ca.icns    (via iconutil; macOS only)

The 16/24/32/48 sizes are the hand-drawn assets. Larger sizes are
nearest-neighbor integer upscales of the 32px icon so the pixel art stays
crisp. No PIL/ImageMagick needed: only RGBA8 non-interlaced PNGs are read.
"""
import os
import platform
import shutil
import struct
import subprocess
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMGS = os.path.join(ROOT, "assets", "imgs")
OUT_WIN = os.path.join(ROOT, "packaging", "windows")
OUT_MAC = os.path.join(ROOT, "packaging", "macos")


def read_png(path):
    """Returns (w, h, rows) with rows as list of bytearray(RGBA*w)."""
    with open(path, "rb") as f:
        data = f.read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", path
    pos, idat, w, h = 8, b"", 0, 0
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos : pos + 4])
        ctype = data[pos + 4 : pos + 8]
        body = data[pos + 8 : pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            w, h, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", body)
            assert (depth, color, interlace) == (8, 6, 0), f"{path}: need RGBA8 non-interlaced"
        elif ctype == b"IDAT":
            idat += body
    raw = zlib.decompress(idat)
    stride, bpp = w * 4, 4
    rows, prev = [], bytearray(stride)
    for y in range(h):
        filt = raw[y * (stride + 1)]
        cur = bytearray(raw[y * (stride + 1) + 1 : (y + 1) * (stride + 1)])
        for i in range(stride):
            a = cur[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if filt == 1:
                cur[i] = (cur[i] + a) & 0xFF
            elif filt == 2:
                cur[i] = (cur[i] + b) & 0xFF
            elif filt == 3:
                cur[i] = (cur[i] + ((a + b) >> 1)) & 0xFF
            elif filt == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                cur[i] = (cur[i] + pred) & 0xFF
        rows.append(cur)
        prev = cur
    return w, h, rows


def write_png(w, h, rows):
    def chunk(ctype, body):
        return struct.pack(">I", len(body)) + ctype + body + struct.pack(">I", zlib.crc32(ctype + body) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )


def scale_nearest(w, h, rows, k):
    out = []
    for r in rows:
        nr = bytearray()
        for x in range(w):
            nr += r[x * 4 : x * 4 + 4] * k
        out.extend([nr] * k)
    return w * k, h * k, out


def png_of(size):
    """PNG bytes for the icon at `size`: hand-drawn if it exists, else 32px upscaled."""
    src = {16: "icon2.png", 24: "icon24.png", 32: "icon32.png", 48: "icon48.png"}.get(size)
    if src:
        with open(os.path.join(IMGS, src), "rb") as f:
            return f.read()
    assert size % 32 == 0, size
    w, h, rows = read_png(os.path.join(IMGS, "icon32.png"))
    return write_png(*scale_nearest(w, h, rows, size // 32))


def make_ico(path, sizes):
    images = [(s, png_of(s)) for s in sizes]
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = len(header) + 16 * len(images)
    entries, blobs = b"", b""
    for s, blob in images:
        dim = 0 if s >= 256 else s  # 0 means 256 in the ICO directory
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(blob), offset + len(blobs))
        blobs += blob
    with open(path, "wb") as f:
        f.write(header + entries + blobs)
    print("wrote", os.path.relpath(path, ROOT))


def make_icns(path):
    iconset = os.path.join(OUT_MAC, "ca.iconset")
    shutil.rmtree(iconset, ignore_errors=True)
    os.makedirs(iconset)
    for base in (16, 32, 128, 256, 512):
        for mult, suffix in ((1, ""), (2, "@2x")):
            with open(os.path.join(iconset, f"icon_{base}x{base}{suffix}.png"), "wb") as f:
                f.write(png_of(base * mult))
    if platform.system() != "Darwin":
        print("skipping ca.icns: iconutil is only available on macOS (iconset left in place)")
        return
    subprocess.run(["iconutil", "-c", "icns", iconset, "-o", path], check=True)
    shutil.rmtree(iconset)
    print("wrote", os.path.relpath(path, ROOT))


if __name__ == "__main__":
    make_ico(os.path.join(OUT_WIN, "ca.ico"), [16, 24, 32, 48, 256])
    make_icns(os.path.join(OUT_MAC, "ca.icns"))
