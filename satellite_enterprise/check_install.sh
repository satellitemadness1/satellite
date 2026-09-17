#!/bin/sh
# check_install.sh -- install.sh against the roots it must refuse and the failures
# a review of M0.5 found (2026-09-17). make test runs it; it needs a built tree.
#
#     sh satellite_enterprise/check_install.sh
#
# Every install goes into build/install_checks/ and runs with MAKE=true, so it
# installs what make already built and builds nothing. Nothing outside build/ is
# written: the refusals of $HOME/.satl and of PATH are checked by the refusal alone.

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

MAKE=true sh "$installer" > "$scratch/out" 2>&1
expect "no --root" "23|1" "$?|$(says 'root <folder> is required')"
install_into "$HOME/.satl"
expect "\$HOME/.satl is 003's" "1|1" "$status|$(says "satellite 003's install")"
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
