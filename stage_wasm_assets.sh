#!/usr/bin/env bash
# Stages assets/ into the wasm build tree for --preload-file, with two
# web-only transforms applied to the STAGED COPY only -- assets/ itself is
# never modified, so the native "ca" build is unaffected:
#   1. shaders are rewritten from desktop GLSL (#version 330) to GLSL ES 3.00
#   2. the UI font is cut down to the glyphs the UI draws, and the 13 unused
#      locale variants are dropped entirely
set -euo pipefail

src_dir="$1"
dst_dir="$2"

here="$(cd "$(dirname "$0")" && pwd)"

mkdir -p "$dst_dir"

# assets/font/ holds 14 locale variants of the same BDF at ~3.4 MB each (~48 MB
# of the ~52 MB bundle) and only one is ever loaded -- see uifont.c, which opens
# font/ark-pixel-12px-proportional-latin.bdf. Exclude them all here and stage a
# subset of just that one below. assets/font/ is left untouched, so the desktop
# build keeps every variant and full Unicode coverage.
# --delete-excluded matters: with plain --delete, rsync *protects* destination
# files matching an --exclude, so variants staged by an earlier run would linger
# forever. ca-ui-font.woff2 is excluded too -- the browser fetches it over HTTP
# from next to the page (see the configure_file in CMakeLists.txt), so a copy
# inside MEMFS is dead weight.
rsync -a --delete --delete-excluded \
  --exclude='font/*.bdf' --exclude='font/*.woff2' \
  "$src_dir/" "$dst_dir/"

ui_font="font/ark-pixel-12px-proportional-latin.bdf"
mkdir -p "$dst_dir/$(dirname "$ui_font")"
perl "$here/tools/subset_bdf.pl" "$src_dir/$ui_font" "$dst_dir/$ui_font"
