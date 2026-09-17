# satellite 004 -- the proof, and what was installed.
#
# PROVEN BY RUNNING A PROGRAM with the installed satl, which loads every library
# in <root>/satellite-numbers/ before it runs a line. Never with --version, which
# answers before any library loads, so a root with no libraries would pass it.
# The program is a copy, run in a folder of its own inside the root, because a
# run writes a .sate beside the program it runs.

step "proving the install: $root/satl runs examples/hello_world.satl"
proof=$(mktemp -d -- "$root/.satellite-004-proof.XXXXXX") || die "cannot write in $root"
trap 'rm -rf -- "$proof"' EXIT
cp -- "$repo/examples/hello_world.satl" "$proof/hello_world.satl"

if "$root/satl" "$proof/hello_world.satl" > "$proof/out" 2> "$proof/err" < /dev/null; then
    proof_status=0
else
    proof_status=$?
fi
proof_out=$(cat -- "$proof/out")
if [ "$proof_status" != 0 ] || [ "$proof_out" != "$hello_wanted" ]; then
    printf '%s\n' "--- what $root/satl wrote on stdout:" "$proof_out" "--- and on stderr:" >&2
    cat -- "$proof/err" >&2
    die "the installed satl did not run examples/hello_world.satl as it should (exit status $proof_status). The files are in place; the install is NOT proven"
fi

# THE TITLE LINES, from the binary just installed: version, revision and build.
title=$("$root/satl" --version) || die "the installed satl ran a program but refused --version"
rm -rf -- "$proof"
trap - EXIT

libraries=$(ls -- "$root/satellite-numbers" | grep -c '\.so$' || :)
printf '\n%s\n\n' "$title"
printf 'installed into %s:\n' "$root"
printf '    satl                  the interpreter -- run it by its path: %s/satl <file.satl>\n' "$root"
printf '    satellite-numbers/    %s libraries\n' "$libraries"
if [ "$with_window" = yes ]; then
    printf '    satl-term             the window, which runs the satl beside it\n'
fi
printf '    %s   the record of what this installer put there\n' "$record_name"

# THE WORD satl IS NOT THIS INSTALL'S, on purpose (D0.5.1). Said, so nobody types
# satl expecting 004.
word=$(command -v satl 2>/dev/null || :)
if [ -n "$word" ]; then
    printf '\nthe word satl still runs %s, which this installer did not touch.\n' "$word"
else
    printf '\nthe word satl runs nothing on this PATH; this installer did not add it.\n'
fi
