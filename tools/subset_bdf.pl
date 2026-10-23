#!/usr/bin/env perl
#
# subset_bdf.pl <in.bdf> <out.bdf>
#
# Writes a copy of a BDF bitmap font containing only the codepoints the UI
# actually draws. The source fonts carry all of Unicode (~24k glyphs, ~3.4 MB
# each), which dominates the wasm --preload-file bundle.
#
# This never touches assets/: stage_wasm_assets.sh calls it to produce a
# reduced font inside the staged tree, under the same filename, so uifont.c
# keeps loading the same path and the desktop build keeps the full font (and
# therefore full CJK coverage).
#
# Keeping the format as BDF rather than converting means freetype and uifont.c
# need no changes at all.
use strict;
use warnings;

my ($in, $out) = @ARGV;
die "usage: subset_bdf.pl <in.bdf> <out.bdf>\n" unless defined $out;

# ASCII + Latin-1 + the typographic marks the text assets use, with headroom.
# A scan of assets/locale and assets/default_mod turns up exactly two non-ASCII
# codepoints in rendered strings: U+00D7 (in Latin-1) and U+2014 (below).
# Adding a non-English locale would need this widened -- or the subset dropped.
my %want;
$want{$_} = 1 for (0x20 .. 0x7E), (0xA0 .. 0xFF);
$want{$_} = 1 for (0x2018, 0x2019, 0x201C, 0x201D, 0x2013, 0x2014, 0x2026,
                   0x2022, 0x00B7, 0x2190, 0x2191, 0x2192, 0x2193,
                   0x2264, 0x2265);

open(my $fh, '<', $in) or die "$in: $!";

my @header;   # everything before the first STARTCHAR, minus ENDFONT
my @kept;     # whole STARTCHAR..ENDCHAR blocks we are keeping
my ($cur, $enc, $in_glyph, $total) = ('', undef, 0, 0);

while (my $line = <$fh>) {
  if (!$in_glyph && $line =~ /^STARTCHAR/) {
    $in_glyph = 1;
    $cur = $line;
    $enc = undef;
    next;
  }
  if ($in_glyph) {
    $cur .= $line;
    $enc = $1 if $line =~ /^ENCODING\s+(-?\d+)/;
    if ($line =~ /^ENDCHAR/) {
      $in_glyph = 0;
      $total++;
      push @kept, $cur if defined $enc && $want{$enc};
    }
    next;
  }
  push @header, $line unless $line =~ /^ENDFONT/;
}
close $fh;

# CHARS has to match the number of glyphs that follow it, or readers reject
# the font.
my $fixed = 0;
for my $l (@header) {
  $fixed++ if $l =~ s/^CHARS\s+\d+\s*$/"CHARS " . scalar(@kept) . "\n"/e;
}
die "$in: no CHARS line found\n" unless $fixed == 1;
die "$in: no glyphs matched the wanted set\n" unless @kept;

open(my $o, '>', $out) or die "$out: $!";
print $o @header, @kept, "ENDFONT\n";
close $o;

printf STDERR "  subset font: %d -> %d glyphs\n", $total, scalar(@kept);
