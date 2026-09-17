# satellite 004 -- the copy, the rename and the record.
#
# STAGED INSIDE THE ROOT, so every move into place is a rename on one filesystem:
# a satl starting mid-install finds the old binary or the new one, never half of
# either. satellite-numbers/ is a fresh folder renamed over the old one, so a
# library no word names any more does not survive (satl loads every .so there).
#
# THE RECORD IS WRITTEN FIRST, holding the new sums AND the old record's lines,
# so an install stopped halfway still recognises every file it may have left;
# it is rewritten with the new sums alone once everything is in place.

step "installing into $root"
mkdir -p -- "$root" || die "cannot create $root"
[ -w "$root" ] || die "cannot write in $root"
stage=$(mktemp -d -- "$root/.satellite-004-staging.XXXXXX") || die "cannot write in $root"
trap 'rm -rf -- "$stage"' EXIT

cp -- "$build/satl" "$stage/satl"
mkdir -- "$stage/satellite-numbers"
cp -- "$build"/satellite-numbers/*.so "$stage/satellite-numbers/"
[ "$with_window" = no ] || cp -- "$build/satl-term" "$stage/satl-term"

(
    cd -- "$stage"
    if [ "$with_window" = yes ]; then sha256sum satl satl-term satellite-numbers/*.so; else sha256sum satl satellite-numbers/*.so; fi
) > "$stage/record" || die "cannot write the record in $stage"

cat -- "$stage/record" > "$stage/record.while_installing"
[ ! -f "$record" ] || cat -- "$record" >> "$stage/record.while_installing"
mv -f -- "$stage/record.while_installing" "$record"

if [ -e "$root/satellite-numbers" ] || [ -L "$root/satellite-numbers" ]; then
    mv -- "$root/satellite-numbers" "$stage/satellite-numbers.old"
fi
mv -- "$stage/satellite-numbers" "$root/satellite-numbers"
mv -f -- "$stage/satl" "$root/satl"
if [ "$with_window" = yes ]; then
    mv -f -- "$stage/satl-term" "$root/satl-term"
elif [ -f "$root/satl-term" ]; then
    # Ours (040-root.sh checked), from an install on a machine that could build it.
    rm -f -- "$root/satl-term"
    step "removed the satl-term an earlier install left: this build has none"
fi

mv -f -- "$stage/record" "$record"
rm -rf -- "$stage"
trap - EXIT
