#!/bin/sh
# check_install.sh -- install.sh against the roots it must refuse and the failures
# a review of M0.5 found (2026-09-17). make test runs it; it needs a built tree.
#
#     sh satellite_enterprise/check_install.sh
#
# Every install goes into build/install_checks/ and runs with MAKE=true, so it
# installs what make already built and builds nothing. \$HOME/.satl -- where a
# bare install goes since D0.5.1 was ruled -- is checked under a HOME of its own
# in there, with stand-ins for 003's satl and satl-term: never the real one.

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo=$(dirname -- "$here")
installer=$here/install.sh
scratch=$repo/build/install_checks
rm -rf -- "$scratch" && mkdir -p -- "$scratch" || exit 1
passed=0 failed=0
# SATELLITE_ICONS IS NOT THE CALLER'S HERE (the review, 2026-10-20): an exported no -- the way to
# keep make install's icons out -- failed five icon rows of `make test`, and any other word refused
# every install. The rows that need it say it themselves.
unset SATELLITE_ICONS

expect() {   # expect <description> <wanted> <got>
    if [ "$2" = "$3" ]; then passed=$((passed + 1)); echo "  ok    $1 -> $3"
    else failed=$((failed + 1)); echo "  FAIL  $1 -> wanted $2, got $3"; fi
}
install_into() {   # install_into <root> [more words]: status in $status, output in $scratch/out
    MAKE=true sh "$installer" --root "$@" > "$scratch/out" 2>&1
    status=$?
}
says() { grep -c -- "$1" "$scratch/out"; }
hello() { "$1/satl" --run "$repo/examples/hello_world.satl" 2> /dev/null | head -n 1; }

[ -x "$repo/build/satl" ] || { echo "check_install.sh: build/satl is not built; run make first"; exit 1; }

home="$scratch/home"
mkdir -p "$home/.satl" "$home/.local/share/applications" && cp /bin/true "$home/.satl/satl" && cp /bin/false "$home/.satl/satl-term"
printf '[Desktop Entry]\nType=Application\nName=Satellite\nExec=satl-term %%f\n' > "$home/.local/share/applications/org.satellite.terminal.desktop"
launcher="$home/.local/share/applications/org.satellite.terminal.desktop"
# A ~/.satl/console/ IS THE PERSON'S (KEY_PLAN.md KEY-1): what they type into a console is kept there, so
# an install -- which replaces only the names it made (060-install-tree.sh) -- leaves one it finds
# exactly as it was. A day's folder and a file in it, 0700 and 0600 as the plan's rule 2 has them, go in
# before the two installs into this HOME's ~/.satl; each name's kind, inode, mode, size and time, and the
# file's bytes, are compared after both.
mkdir -p "$home/.satl/console/2026-10-19" && chmod 700 "$home/.satl/console" "$home/.satl/console/2026-10-19" &&
    printf 'satellite.console.display("typed the day before")\n' > "$home/.satl/console/2026-10-19/console-001.c4" &&
    chmod 600 "$home/.satl/console/2026-10-19/console-001.c4"
console_now() { find "$home/.satl/console" -printf '%y %i %m %s %T@ %P\n' | sort; cat "$home/.satl/console/2026-10-19/console-001.c4"; }
console_now > "$scratch/console_before"
env -u XDG_DATA_HOME HOME=$home MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "no --root installs into \$HOME/.satl, over a satl it did not put there, proven" "0|Hello, World!" \
       "$?|$(hello "$home/.satl")"
expect "... keeping that satl as satl.bak-<date>, and leaving 003's satl-term alone" "1|same|same" \
       "$(ls "$home/.satl" | grep -c '^satl\.bak-')|$(cmp -s /bin/true "$home/.satl"/satl.bak-* && echo same)|$(cmp -s /bin/false "$home/.satl/satl-term" && echo same)"
expect "... and the launcher opens that satl by its absolute path, 003's kept as a .bak" "1|1|1|0" \
       "$(grep -cxF "Exec=$home/.satl/satl --console %f" "$launcher")|$(grep -cxF "TryExec=$home/.satl/satl" "$launcher")|$(ls "$home/.local/share/applications" | grep -c '\.desktop\.bak-')|$(desktop-file-validate "$launcher" 2>&1 | grep -c 'error')"
# satl'S ICON AND THE .satl FILE TYPE GO IN WITH IT (072-icons.sh; the author, 2026-10-20), into that
# HOME's ~/.local/share: the whole of icons/hicolor/ and nothing else -- no SVG beside the photographs
# -- and the type's packet, nothing left half-written. And the mime database there takes the packet,
# which update-mime-database can skip and still exit 0, so gio is asked: a .satl file is
# application/x-satellite, drawn with its own icon.
data="$home/.local/share"
expect "... and satl's icon and the .satl file type go into its ~/.local/share, every file and no other" "0|same|0" \
       "$(diff -rq "$here/icons/hicolor" "$data/icons/hicolor" 2>&1 | wc -l)|$(cmp -s "$here/icons/application-x-satellite.xml" "$data/mime/packages/application-x-satellite.xml" && echo same)|$(find "$data" -name '*.installing' | wc -l)"
if command -v update-mime-database > /dev/null 2>&1; then
    printf 'satellite.include(satellite)\n' > "$scratch/typed.satl"
    if command -v gio > /dev/null 2>&1; then
        expect "... and its mime database takes the packet: gio calls a .satl file application/x-satellite, drawn with its own icon" \
               "application/x-satellite|application-x-satellite" \
               "$(XDG_DATA_HOME=$data GIO_USE_VFS=local gio info -a standard::content-type "$scratch/typed.satl" | sed -n 's/^ *standard::content-type: //p')|$(XDG_DATA_HOME=$data GIO_USE_VFS=local gio info -a standard::icon "$scratch/typed.satl" | sed -n 's/^ *standard::icon: \([^,]*\).*/\1/p')"
    else
        expect "... and its mime database takes the packet" yes "$([ -f "$data/mime/application/x-satellite.xml" ] && echo yes)"
    fi
fi
touch "$scratch/before_second_install"
env -u XDG_DATA_HOME HOME=$home PATH="$home/.satl:$PATH" MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "\$HOME/.satl on PATH, installed over its own install, keeps no second copy of satl or the launcher" "0|1|1|1" \
       "$?|$(ls "$home/.satl" | grep -c '^satl\.bak-')|$(ls "$home/.local/share/applications" | grep -c '\.desktop\.bak-')|$(says 'the word satl runs this install')"
expect "... and finds satl's icon and the .satl file type in place: says so, writes nothing, runs no tool" "1|0" \
       "$(says 'already installed in')|$(find "$data/icons" "$data/mime" -newer "$scratch/before_second_install" | wc -l)"
expect "both installs leave the ~/.satl/console/ they found exactly as it was: a day's folder and the file in it" same \
       "$(console_now | cmp -s - "$scratch/console_before" && echo same || echo changed)"
# --icons-current IS WHAT THE BARE make ASKS before it asks the person (050-build.mk): 0 where every
# file is in place, 1 where one is not, and nothing printed. It is the whole command line.
env -u XDG_DATA_HOME HOME=$home sh "$installer" --icons-current > "$scratch/out" 2>&1; in_place=$?
env -u XDG_DATA_HOME HOME="$scratch/a home with no icons" sh "$installer" --icons-current >> "$scratch/out" 2>&1
expect "--icons-current: 0 where the icons are in place, 1 where they are not, printing nothing" "0|1|0" \
       "$in_place|$?|$(wc -c < "$scratch/out")"
env -u XDG_DATA_HOME HOME=$home sh "$installer" --icons-current --root x > "$scratch/out" 2>&1
expect "... and it is the whole command line" "23|1" "$?|$(says 'is the whole command line')"
# AN ICON CHANGED SINCE is not current, and the next install writes that one file back, alone.
printf 'x' >> "$data/icons/hicolor/48x48/apps/org.satellite.terminal.png"
env -u XDG_DATA_HOME HOME=$home sh "$installer" --icons-current; changed=$?
env -u XDG_DATA_HOME HOME=$home MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "an icon changed since is not current, and the next install writes it back alone" "1|0|1|0" \
       "$changed|$?|$(says '1 of their 19 files had changed')|$(diff -rq "$here/icons/hicolor" "$data/icons/hicolor" 2>&1 | wc -l)"
# SATELLITE_ICONS=no -- the bare make's n -- installs satl and leaves them out, saying so; a word
# that is neither yes nor no is refused, as a command line is.
rm -rf "$data/icons" "$data/mime"
env -u XDG_DATA_HOME HOME=$home SATELLITE_ICONS=no MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "SATELLITE_ICONS=no installs satl and leaves the icons out, saying so" "0|1|no" \
       "$?|$(says 'as SATELLITE_ICONS=no says')|$([ -e "$data/icons" ] || [ -e "$data/mime" ] && echo yes || echo no)"
env -u XDG_DATA_HOME HOME=$home SATELLITE_ICONS=maybe MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "... and SATELLITE_ICONS=maybe is refused" "23|1" "$?|$(says 'SATELLITE_ICONS=maybe is not a word')"
# THE HELP FILES GO BESIDE satl (060-install-tree.sh, 2026-09-25), so satellite.help at an
# installed satl's prompt finds them; before, every one said "the help files are not beside satl".
expect "the help files are installed beside satl, every one, marked as this installer's" "$(find "$repo/satellite.help" -type f | wc -l)|1" \
       "$(find "$home/.satl/satellite.help" -type f ! -name .satellite-004-help | wc -l)|$(ls -A "$home/.satl/satellite.help" | grep -c '^\.satellite-004-help$')"
printf 'satellite.help(include)\n' | env HOME=$home SATL_NO_WINDOW=1 "$home/.satl/satl" --repl > "$scratch/help_out" 2>&1
expect "... and satellite.help(include) at that satl's prompt reads them" "0|yes" \
       "$(grep -c 'help files are not beside satl' "$scratch/help_out")|$(grep -q 'SATELLITE 004: satellite.include()' "$scratch/help_out" && echo yes || echo no)"
mkdir -p "$scratch/their_help/satellite.help" && : > "$scratch/their_help/satellite.help/theirs.txt"
install_into "$scratch/their_help"
expect "a satellite.help/ this installer did not make is left alone" "0|1|yes" \
       "$status|$(says 'left .*satellite.help alone')|$([ -f "$scratch/their_help/satellite.help/theirs.txt" ] && echo yes || echo no)"
env -u XDG_DATA_HOME HOME=$home MAKE=true sh "$installer" --root "$home/.satl/inner" > "$scratch/out" 2>&1
expect "a root inside \$HOME/.satl" "1|1" "$?|$(says 'inside')"
install_into x --link
expect "--link is 003's" "23|1" "$status|$(says "is satellite 003's installer's")"

good="$scratch/a root with a space"
install_into "$good"
expect "an install into a root holding a space, proven" "0|Hello, World!" "$status|$(hello "$good")"
expect "... with no icons: they go in with \$HOME/.satl's install alone, and it says so" 1 "$(says 'no icons for --root')"
# AN EARLIER INSTALL'S satellite-numbers/ (every word is inside satl since 2026-10-07): the
# record names it, so it goes; a folder the record does not name is not this installer's.
mkdir -p "$good/satellite-numbers" && : > "$good/satellite-numbers/9.9.9.so" &&
    printf '%s  satellite-numbers/9.9.9.so\n' "$(sha256sum < "$good/satellite-numbers/9.9.9.so" | cut -d' ' -f1)" >> "$good/.satellite-004-install"
install_into "$good"
expect "a reinstall over its own install removes the satellite-numbers/ an earlier install left" "0|no|1" \
       "$status|$([ -e "$good/satellite-numbers" ] && echo yes || echo no)|$(says 'removed the satellite-numbers/')"
mkdir -p "$good/satellite-numbers" && : > "$good/satellite-numbers/theirs.so"
install_into "$good"
expect "... and leaves a satellite-numbers/ the record does not name" "0|yes" \
       "$status|$([ -f "$good/satellite-numbers/theirs.so" ] && echo yes || echo no)"
rm -rf "$good/satellite-numbers"

mkdir -p "$scratch/relative"
(cd "$scratch/relative" && MAKE=true sh "$installer" --root -x > "$scratch/out" 2>&1)
expect "--root -x (the proof runs through --run)" "0|Hello, World!" "$?|$(hello "$scratch/relative/-x")"

mkdir -p "$scratch/on_path"
(cd "$scratch/on_path" && PATH="$PATH:" MAKE=true sh "$installer" --root . > "$scratch/out" 2>&1)
expect "a PATH ending in ':' puts the current folder on PATH" "1|1|no" \
       "$?|$(says 'it is on PATH')|$([ -e "$scratch/on_path/satl" ] && echo yes || echo no)"

mkdir -p "$scratch/foreign" && cp /bin/true "$scratch/foreign/satl"
install_into "$scratch/foreign"
expect "a satl this installer did not put there" "1|1|same" \
       "$status|$(says 'did not put there')|$(cmp -s /bin/true "$scratch/foreign/satl" && echo same)"

cp -r "$good" "$scratch/symlinked" && mv "$scratch/symlinked/satl" "$scratch/symlinked/satl.real" &&
    ln -s satl.real "$scratch/symlinked/satl"
install_into "$scratch/symlinked"
expect "a satl that is a symlink, refused as a symlink" "1|1" "$status|$(says 'its satl is a symlink')"

mkdir -p "$scratch/record_folder/.satellite-004-install"
install_into "$scratch/record_folder"
expect "a record that is a folder" "1|1" "$status|$(says 'is not a plain file')"

hostile=$(printf '%s/esc\033]2;pwned\007x' "$scratch")
install_into "$hostile"
expect "a root named with ESC ] 2 ; BEL installs, is named escaped, and no ESC or BEL is written" "0|0|yes" \
       "$status|$(tr -cd '\033\007' < "$scratch/out" | wc -c)|$([ "$(says 'esc\\x1b]2;pwned\\x07x')" -gt 0 ] && echo yes)"

(cd "$repo" && CDPATH=. sh satellite_enterprise/install.sh --help > "$scratch/out" 2>&1)
expect "an exported CDPATH" 0 $?

install_into "$good"
expect "the next install finishes, with nothing left behind" "0|0" \
       "$status|$(ls -A "$good" | grep -c '^\.satellite-004-\(staging\|proof\)')"

echo "check_install.sh: $passed passed, $failed failed"
[ "$failed" = 0 ]
