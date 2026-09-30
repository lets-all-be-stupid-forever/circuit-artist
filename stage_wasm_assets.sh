#!/usr/bin/env bash
# Stages assets/ into the wasm build tree for --preload-file, rewriting
# shaders from desktop GLSL (#version 330) to GLSL ES 3.00 (WebGL2) along the
# way. assets/shaders/*.glsl itself is never touched -- it stays desktop-only
# GLSL 330 for the native "ca"/"cav" builds.
set -euo pipefail

src_dir="$1"
dst_dir="$2"

mkdir -p "$dst_dir"
rsync -a --delete "$src_dir/" "$dst_dir/"

# GLSL ES requires "#version" to be the literal first line of the file, but
# some desktop sources have a leading blank line or comment before it. Strip
# everything up to and including the "#version 330" line so the ES header
# always lands on line 1.
perl -0777 -pi -e 's/\A.*?#version 330\s*\n/#version 300 es\nprecision mediump float;\n/s' \
  "$dst_dir"/shaders/*.glsl
