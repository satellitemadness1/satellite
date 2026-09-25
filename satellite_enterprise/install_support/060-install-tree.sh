# satellite 004 -- the copy, the swap and the record.
#
# ONE INSTALL INTO A ROOT AT A TIME: the root folder itself is locked (flock, when
# the machine has it), and under the lock anything a killed install left behind
# -- .satellite-004-staging.* and .satellite-004-proof.* -- is removed.
#
# STAGED INSIDE THE ROOT, so every move into place is a rename on one filesystem:
# a satl starting mid-install finds the old binary or the new one, never half of
# either. satellite-numbers/ is a fresh folder EXCHANGED with the old one in one
# rename (mv --exchange, renameat2), so there is no moment with no libraries and
# no library a word no longer has survives; where mv cannot exchange, the old
# folder is moved aside first and put back if the install stops before the new one
# is in place (review of M0.5: a stop between the two renames deleted both).
#
# THE RECORD IS WRITTEN FIRST, holding the new sums AND the old record's lines,
# so an install stopped halfway still recognises every file it may have left;
# it is rewritten with the new sums alone once everything is in place.

step "installing into $root"
mkdir -p -- "$root" || die "cannot create $root"

if command -v flock > /dev/null 2>&1; then
    exec 9< "$root" || die "cannot open $root"
    flock -n 9 || die "another install into $root is running; wait for it, then run this again"
    for _left in "$root"/.satellite-004-staging.* "$root"/.satellite-004-proof.*; do
        [ -e "$_left" ] || [ -L "$_left" ] || continue
        rm -rf -- "$_left" && step "removed $_left, which an install that was stopped left behind"
    done
fi

# make has run since 040 looked: look again, just before writing.
check_the_root_is_ours

# 003's satl, KEPT BEFORE ANYTHING IS REPLACED: a second name for the same file, so
# `satl` never stops existing, and the new one then takes the name by rename.
if [ "$set_aside_satl" = yes ]; then
    _kept=$root/satl.bak-$(date +%Y%m%d-%H%M%S)
    ln -- "$root/satl" "$_kept" 2> /dev/null || cp -p -- "$root/satl" "$_kept" ||
        die "cannot keep the satl this installer did not put in $root; nothing was replaced"
    step "kept the satl that was in $root, which this installer did not put there, as $(basename -- "$_kept")"
fi

stage=$(mktemp -d -- "$root/.satellite-004-staging.XXXXXX") || die "cannot write in $root"
put_back_old_libraries() {
    if [ ! -e "$root/satellite-numbers" ] && [ -d "$stage/satellite-numbers.old" ]; then
        mv -T -- "$stage/satellite-numbers.old" "$root/satellite-numbers" &&
            printf 'install.sh: %s\n' "the install stopped; the libraries that were installed before it are back in place" >&2
    fi
    rm -rf -- "$stage"
}
trap put_back_old_libraries EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

cp -- "$build/satl" "$stage/satl" || die "cannot copy $build/satl into $root"
mkdir -- "$stage/satellite-numbers" || die "cannot write in $root"
cp -- "$build"/satellite-numbers/*.so "$stage/satellite-numbers/" || die "cannot copy the libraries into $root"

(
    cd -- "$stage"
    sha256sum satl satellite-numbers/*.so
) > "$stage/record" || die "cannot write the record in $root"

cat -- "$stage/record" > "$stage/record.while_installing" || die "cannot write the record in $root"
[ ! -f "$record" ] || cat -- "$record" >> "$stage/record.while_installing" || die "cannot read $record"
mv -fT -- "$stage/record.while_installing" "$record" || die "cannot write $record; nothing else was changed"

if [ -d "$root/satellite-numbers" ]; then
    if mv --exchange -T -- "$stage/satellite-numbers" "$root/satellite-numbers" 2> /dev/null; then
        mv -T -- "$stage/satellite-numbers" "$stage/satellite-numbers.replaced" || :
    else
        mv -T -- "$root/satellite-numbers" "$stage/satellite-numbers.old" ||
            die "cannot move the old satellite-numbers/ aside; the install that was there is unchanged"
        mv -T -- "$stage/satellite-numbers" "$root/satellite-numbers" ||
            die "cannot put the new satellite-numbers/ in place"
    fi
else
    mv -T -- "$stage/satellite-numbers" "$root/satellite-numbers" || die "cannot put satellite-numbers/ in $root"
fi
mv -fT -- "$stage/satl" "$root/satl" || die "cannot put satl in $root (its libraries are already the new ones)"
if [ -f "$root/satl-term" ] && [ ! -L "$root/satl-term" ] && [ -f "$record" ] && ours satl-term; then
    # Ours, from an install made before satl-term was removed: satl opens its own
    # console now. One that is not ours -- 003's, in \$HOME/.satl -- stays.
    rm -f -- "$root/satl-term"
    step "removed the satl-term an earlier install left: satl opens its own console now"
fi

# THE HELP FILES, BESIDE satl (the error sweep, 2026-09-25). satellite.help() at the
# prompt reads them beside satl or one folder up (satellite/satl/prompt_help.cpp), and
# no install ever put them there, so every installed satl answered "the help files are
# not beside satl" (the website writers found it, 2026-09-24). Replaced whole, as
# satellite-numbers/ is, so a topic the tree no longer has does not survive -- and only
# a folder this installer made (it carries .satellite-004-help) is ever replaced.
if [ -d "$repo/satellite.help" ]; then
    if [ -e "$root/satellite.help" ] && [ ! -f "$root/satellite.help/.satellite-004-help" ]; then
        step "left $root/satellite.help alone: this installer did not put it there"
    else
        cp -R -- "$repo/satellite.help" "$stage/satellite.help" &&
            : > "$stage/satellite.help/.satellite-004-help" || die "cannot copy the help files into $root"
        rm -rf -- "$root/satellite.help"
        mv -T -- "$stage/satellite.help" "$root/satellite.help" || die "cannot put satellite.help/ in $root"
    fi
fi

mv -fT -- "$stage/record" "$record" || die "cannot rewrite $record"
rm -rf -- "$stage"
trap - EXIT INT TERM HUP
