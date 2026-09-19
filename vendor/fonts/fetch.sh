#!/bin/sh
# vendor/fonts/fetch.sh -- IBM Plex Mono, the satellite look, beside satellite.
#
#     sh vendor/fonts/fetch.sh            verifies what is here against SHA256SUMS
#     sh vendor/fonts/fetch.sh --install  also copies it into ~/.local/share/fonts
#
# WHY THE FONT IS PART OF THE REPOSITORY. The author's rule is that every window title
# is IBM Plex Mono, 11px, never bold or italic -- the font IS the look, not a preference
# of whatever machine satl lands on. And it stopped being optional on 2026-09-19: a GTK4
# binary built from vendor/gtk/ and run where NO font is reachable does not fall back to
# tofu, it dies --
#     GtkImage reported baselines of minimum -2147483648 ... sizes of minimum 16
#     g_object_ref: assertion 'G_IS_OBJECT (object)' failed
#     exit 139   (twice; exit 0 twice with fonts present)
# -- so a self-contained satl must carry one and register it before the first widget is
# realized. IBMPlexMono-Regular.ttf is 133 KB, and that is the entire cost.
#
# ONLY Regular AND OFL.txt ARE IN GIT. The spec says never bold and never italic, so the
# other thirteen faces would be dead weight in every clone forever. They are gitignored;
# this script's --family mode fetches them back for anyone who wants the whole set.
#
# THESE ARE THE AUTHOR'S OWN FILES, byte for byte, from the zip he downloaded from Google
# Fonts. They are NOT the same bytes as github.com/google/fonts/ofl/ibmplexmono, and the
# difference was measured rather than guessed:
#
#     version 2.3 in both;  930 mapped codepoints in both, zero either way;
#     identical GSUB (aalt ccmp dnom frac numr ordn salt sinf ss01-ss05 sups zero)
#     and identical GPOS (mark).
#
# Same typeface, same coverage, same features. The google/fonts copy is ~1750 bytes
# larger per face because it carries a vestigial 8-byte DSIG stub and five unmapped
# glyphs, and its OFL.txt still points at the dead http://scripts.sil.org/OFL instead of
# https://openfontlicense.org. The author's copy is the smaller, current one, so it wins
# -- and it is HIS copy, which is reason enough on its own. Never substitute the other.
#
# THE LICENCE IS THE OFL. Embedding a font in a binary is redistribution, and the OFL
# requires its notice to travel along. Whatever satl ships, ships OFL.txt with it.
set -eu

here=$(cd "$(dirname "$0")" && pwd)
cd "$here"
DIR=ibm-plex-mono
RAW=https://raw.githubusercontent.com/google/fonts/main/ofl/ibmplexmono

if [ "${1:-}" = "--family" ]; then
    # The thirteen faces that are not in git. NOTE these come from google/fonts and so
    # are the slightly larger build described above -- fine for looking at, not the
    # bytes to ship. Regular and OFL.txt are never overwritten here.
    mkdir -p "$DIR"
    for f in Bold BoldItalic ExtraLight ExtraLightItalic Italic Light LightItalic \
             Medium MediumItalic SemiBold SemiBoldItalic Thin ThinItalic; do
        [ -f "$DIR/IBMPlexMono-$f.ttf" ] || {
            echo "vendor/fonts: downloading IBMPlexMono-$f.ttf"
            curl -sSL --max-time 300 -o "$DIR/IBMPlexMono-$f.ttf" "$RAW/IBMPlexMono-$f.ttf"
        }
    done
    echo "vendor/fonts: the full family is in $DIR (only Regular is shipped)"
    exit 0
fi

[ -f "$DIR/IBMPlexMono-Regular.ttf" ] || {
    echo "vendor/fonts: IBMPlexMono-Regular.ttf is missing -- it is in git, so this is a" >&2
    echo "  broken checkout rather than something to download." >&2
    exit 1
}

# Only the files actually present are checked, so a checkout with just Regular passes
# and a full family is checked in full.
missing=0
while read -r sum name; do
    [ -f "$DIR/$name" ] || { missing=$((missing+1)); continue; }
    got=$(sha256sum "$DIR/$name" | cut -d' ' -f1)
    [ "$got" = "$sum" ] || {
        echo "vendor/fonts: $name DOES NOT MATCH SHA256SUMS" >&2
        echo "  expected $sum" >&2
        echo "  got      $got" >&2
        exit 1
    }
done < SHA256SUMS

present=$(ls "$DIR"/*.ttf 2>/dev/null | wc -l)
echo "vendor/fonts: $present face(s) verified against SHA256SUMS ($missing not present)"

if [ "${1:-}" = "--install" ]; then
    # Copied byte for byte -- never subsetted, never re-hinted, never "optimised".
    target="$HOME/.local/share/fonts/ibm-plex-mono"
    mkdir -p "$target"
    cp -f "$DIR"/*.ttf "$DIR/OFL.txt" "$target/"
    fc-cache -f "$target" >/dev/null 2>&1 || true
    echo "vendor/fonts: installed to $target"
    echo "vendor/fonts: fontconfig sees $(fc-list 2>/dev/null | grep -c 'IBM Plex Mono') IBM Plex Mono faces"
fi
