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
env -u XDG_DATA_HOME HOME=$home MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "no --root installs into \$HOME/.satl, over a satl it did not put there, proven" "0|Hello, World!" \
       "$?|$(hello "$home/.satl")"
expect "... keeping that satl as satl.bak-<date>, and leaving 003's satl-term alone" "1|same|same" \
       "$(ls "$home/.satl" | grep -c '^satl\.bak-')|$(cmp -s /bin/true "$home/.satl"/satl.bak-* && echo same)|$(cmp -s /bin/false "$home/.satl/satl-term" && echo same)"
expect "... and the launcher opens that satl by its absolute path, 003's kept as a .bak" "1|1|1|0" \
       "$(grep -cxF "Exec=$home/.satl/satl --console %f" "$launcher")|$(grep -cxF "TryExec=$home/.satl/satl" "$launcher")|$(ls "$home/.local/share/applications" | grep -c '\.desktop\.bak-')|$(desktop-file-validate "$launcher" 2>&1 | grep -c 'error')"
env -u XDG_DATA_HOME HOME=$home PATH="$home/.satl:$PATH" MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "\$HOME/.satl on PATH, installed over its own install, keeps no second copy of satl or the launcher" "0|1|1|1" \
       "$?|$(ls "$home/.satl" | grep -c '^satl\.bak-')|$(ls "$home/.local/share/applications" | grep -c '\.desktop\.bak-')|$(says 'the word satl runs this install')"
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
touch "$good/satellite-numbers/9.9.9.so"
install_into "$good"
expect "a reinstall over its own install, and a stale 9.9.9.so is gone" "0|no" \
       "$status|$([ -e "$good/satellite-numbers/9.9.9.so" ] && echo yes || echo no)"

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

# A STOP BETWEEN THE LIBRARIES' RENAMES PUTS THE OLD ONES BACK: an mv that fails
# the move of the NEW folder into <root>/satellite-numbers, the exchange included.
mkdir -p "$scratch/shim"
cat > "$scratch/shim/mv" <<EOF
#!/bin/sh
for word in "\$@"; do source=\$last; last=\$word; done
case \$source in */.satellite-004-staging.*/satellite-numbers) exit 1 ;; esac
exec /usr/bin/mv "\$@"
EOF
chmod +x "$scratch/shim/mv"
PATH="$scratch/shim:$PATH" MAKE=true sh "$installer" --root "$good" > "$scratch/out" 2>&1
expect "a failed rename of the new libraries leaves the old install running" "1|Hello, World!" "$?|$(hello "$good")"
install_into "$good"
expect "... and the next install finishes, with nothing left behind" "0|0" \
       "$status|$(ls -A "$good" | grep -c '^\.satellite-004-\(staging\|proof\)')"

echo "check_install.sh: $passed passed, $failed failed"
[ "$failed" = 0 ]
