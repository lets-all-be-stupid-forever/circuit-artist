# Creates the folder with the wasm assets.
#
# usage: stage_wasm_assets.py <assets_dir> <staged_dir>
#
# Mirrors assets/ into the wasm build tree for --preload-file (everything in
# <staged_dir> ends up in ca_wasm.data). assets/ itself is never modified, so
# the native "ca" build is unaffected. Differences from a plain copy:
#   1. shaders are rewritten from desktop GLSL (#version 330) to GLSL ES 3.00,
#      which is what WebGL2 compiles
#   2. the UI font is cut down to the glyphs the UI draws (3.5 MB -> ~28 KB)
#   3. ca-ui-font.woff2 is left out: the browser fetches it over HTTP from
#      static/ (see the configure_file in CMakeLists.txt), so a copy inside
#      MEMFS is dead weight.
import os
import re
import shutil
import sys

UI_FONT = "font/ark-pixel-12px-proportional-latin.bdf"

# Codepoints kept in the staged UI font: ASCII + Latin-1 + the typographic
# marks the text assets use. uifont.c only loads codepoints 32..255
# (FIRST_CP/NUM_GLYPHS), so the marks above 255 are headroom for when that
# range is widened.
WANTED_CODEPOINTS = (
    set(range(0x20, 0x7F))
    | set(range(0xA0, 0x100))
    | {0x2018, 0x2019, 0x201C, 0x201D, 0x2013, 0x2014, 0x2026, 0x2022,
       0x00B7, 0x2190, 0x2191, 0x2192, 0x2193, 0x2264, 0x2265}
)


def is_excluded(rel):
    """Files under assets/ that are not copied as-is."""
    d, name = os.path.split(rel)
    if d == "font" and (name.endswith(".bdf") or name.endswith(".woff2")):
        return True  # .bdf is staged as a subset below; .woff2 is web-only
    return False


def copy_if_changed(src, dst):
    """Like rsync -a: skip files whose size and mtime already match."""
    if os.path.exists(dst):
        s, d = os.stat(src), os.stat(dst)
        if s.st_size == d.st_size and int(s.st_mtime) == int(d.st_mtime):
            return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)


def is_shader(rel):
    return rel.startswith("shaders/") and rel.endswith(".glsl")


def stage_shader(src, dst):
    """Writes a GLSL ES 3.00 copy of a desktop GLSL 330 shader.

    GLSL ES needs "#version" as the literal first line plus a default float
    precision; the shader bodies are otherwise compatible.
    """
    with open(src, encoding="utf-8", newline="") as f:
        text = f.read()
    first, _, rest = text.partition("\n")
    if first.strip() != "#version 330":
        sys.exit(f"{src}: first line must be '#version 330', got {first!r}")
    rest = re.sub(r"\A\s*\n", "", rest)  # drop blank lines after #version
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="utf-8", newline="") as f:
        f.write("#version 300 es\nprecision mediump float;\n" + rest)


def mirror(src_dir, dst_dir):
    """Copies src_dir into dst_dir, returns the set of relative paths staged.

    Files are compared by size+mtime, so an incremental build only copies what
    changed. Shaders are always regenerated (they are small).
    """
    staged = set()
    for root, _, files in os.walk(src_dir):
        for name in files:
            src = os.path.join(root, name)
            rel = os.path.relpath(src, src_dir).replace(os.sep, "/")
            if is_excluded(rel):
                continue
            dst = os.path.join(dst_dir, rel)
            if is_shader(rel):
                stage_shader(src, dst)
            else:
                copy_if_changed(src, dst)
            staged.add(rel)
    return staged


def remove_stale(dst_dir, keep):
    """Deletes staged files that are no longer in assets/ (or are now
    excluded), so they stop being packed into ca_wasm.data."""
    for root, dirs, files in os.walk(dst_dir, topdown=False):
        for name in files:
            path = os.path.join(root, name)
            rel = os.path.relpath(path, dst_dir).replace(os.sep, "/")
            if rel not in keep:
                os.remove(path)
        for name in dirs:
            path = os.path.join(root, name)
            if not os.listdir(path):
                os.rmdir(path)


def subset_bdf(src, dst, wanted):
    """Writes a copy of a BDF bitmap font keeping only the `wanted` codepoints.

    BDF is a text format: a header, then one STARTCHAR..ENDCHAR block per
    glyph (its codepoint is on the ENCODING line), then ENDFONT. Keeping it as
    BDF means freetype and uifont.c need no changes.
    """
    # latin-1 + newline='' round-trips every byte and line ending unchanged.
    with open(src, encoding="latin-1", newline="") as f:
        lines = f.readlines()

    header = []  # everything before the first STARTCHAR, minus ENDFONT
    kept = []    # whole STARTCHAR..ENDCHAR blocks we are keeping
    total = 0
    glyph = None  # lines of the glyph being read, None outside a glyph
    enc = None
    for line in lines:
        if glyph is None:
            if line.startswith("STARTCHAR"):
                glyph, enc = [line], None
            elif not line.startswith("ENDFONT"):
                header.append(line)
            continue
        glyph.append(line)
        m = re.match(r"ENCODING\s+(-?\d+)", line)
        if m:
            enc = int(m.group(1))
        if line.startswith("ENDCHAR"):
            total += 1
            if enc in wanted:
                kept.append("".join(glyph))
            glyph = None

    if not kept:
        sys.exit(f"{src}: no glyphs matched the wanted set")
    # CHARS has to match the number of glyphs that follow it, or readers
    # reject the font.
    n_fixed = 0
    for i, line in enumerate(header):
        if re.fullmatch(r"CHARS\s+\d+\s*", line):
            header[i] = f"CHARS {len(kept)}\n"
            n_fixed += 1
    if n_fixed != 1:
        sys.exit(f"{src}: expected exactly one CHARS line, found {n_fixed}")

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", encoding="latin-1", newline="") as f:
        f.writelines(header)
        f.writelines(kept)
        f.write("ENDFONT\n")
    print(f"  subset font: {total} -> {len(kept)} glyphs", file=sys.stderr)


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: stage_wasm_assets.py <assets_dir> <staged_dir>")
    src_dir, dst_dir = sys.argv[1], sys.argv[2]
    os.makedirs(dst_dir, exist_ok=True)

    staged = mirror(src_dir, dst_dir)
    subset_bdf(os.path.join(src_dir, UI_FONT), os.path.join(dst_dir, UI_FONT),
               WANTED_CODEPOINTS)
    staged.add(UI_FONT)
    remove_stale(dst_dir, staged)


if __name__ == "__main__":
    main()
