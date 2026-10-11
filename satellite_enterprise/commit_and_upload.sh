#!/bin/sh
# commit_and_upload.sh -- what every build does after it installs: commit the tree, then
# upload it. make runs it straight after install.sh (080-install.mk's run_the_installer),
# so only a build that succeeded and installed ever reaches it.
#
# THE AUTHOR, 2026-10-10: "build needs to start running install.sh, wherever it is", "and we
# need to commit as part of the build process, just not necessarily to the main branch...",
# "and we need to upload to the next available source using satellite_backup ... yea build
# that in too, "next" free repository on the satellite server", and "have satellite_backup
# upload the same file list as .gitignore". So, in order:
#
#   1. COMMIT. `git add -A`, then one commit on the branch that is checked out, whichever it
#      is: this never switches branches and never pushes. Its first line is the build's own
#      (satl VERSION 004 REVISION 10 BUILD nnnn), git's count of what changed goes under it,
#      and COMMIT_MESSAGE, when it is in the environment (COMMIT_MESSAGE="..." make), goes
#      last -- so a trailer at its end is the message's end. Nothing changed since the last
#      commit: nothing is committed.
#   2. UPLOAD. ~/code/py/satellite_backup's `satellite next` -- the first free slot of a
#      hundred on the server, its unchanged files linked from the newest slot there -- with
#      the profile's upload list, which is git's (satellite.upload.txt.c4 says git). --yes:
#      a build does not stop to ask "upload N files?"; erasing a slot when all hundred are
#      used is still asked at a terminal (a minute of silence is no), and refused without one.
#      The commit that went up is written in .git/satellite-last-upload, and a build whose
#      commit is already up -- nothing changed -- does not fill another slot with the same copy.
#
# ONLY THE AUTHOR'S CHECKOUT COMMITS AND UPLOADS (the author, 2026-10-11: "ask to install, but
# what is going to be asked when someone pulls this from the git repo??"). The two are his record
# of his own builds, so they happen in one folder only: the git checkout this machine's
# satellite_backup profile backs up. Anything else -- a clone from GitHub, a `cp -a` copy, a copy
# from the server, a tarball, a folder inside some other repository -- commits nothing and
# uploads nothing, says so in one line, and the build stands. Until that day any clone committed
# in its own git, and a clone whose git had no name to sign with failed its make after installing.
# `commit_and_upload.sh --check` asks the question alone, saying nothing: exit 0 when this is the
# author's checkout. 050-build.mk asks it before its own question, so everyone else is asked only
# "Install it into ~/.satl?". SATELLITE_BACKUP names another folder for satellite_backup_main.py
# and its profile.
#
# AND WHERE A COMMIT WOULD BE WRONG, IT REFUSES (the review of 2026-10-10), with the build
# installed either way: a merge, cherry-pick, revert or rebase not finished, or a file still in
# conflict -- `git add -A` would mark the conflict resolved and commit its <<<<<<< lines; and a
# git repository inside the tree, which `git add -A` would keep as a bare link with none of its
# files (it is taken back out of the index, and named). A commit or an upload that FAILS is an
# error too, and says what did happen.
#
# POSIX sh, like install.sh beside it.

set -eu
unset CDPATH

here=$(cd -- "$(dirname -- "$0")" && pwd -P)
repo=$(cd -- "$here/.." && pwd -P)
backup=${SATELLITE_BACKUP:-$HOME/code/py/satellite_backup}
profile=satellite

say() { printf 'commit_and_upload.sh: %s\n' "$*"; }
fail() { printf 'commit_and_upload.sh: %s\n' "$*" >&2; exit 1; }

case ${1:-} in
    "" | --check) ;;
    *) fail "takes nothing, or --check -- not $1" ;;
esac

# THE AUTHOR'S CHECKOUT, OR NOT, ASKED FIRST: a git checkout of its own, and the folder this
# machine's satellite_backup profile names. Anywhere else nothing below is read or written.
not_his=
top=$(git -C "$repo" rev-parse --show-toplevel 2> /dev/null) || top=
settings=$backup/$profile.settings.txt.c4
if [ -z "$top" ] || [ "$(cd -- "$top" && pwd -P)" != "$repo" ]; then
    not_his="$repo is not a git checkout of its own"
elif [ ! -f "$backup/satellite_backup_main.py" ] || [ ! -f "$settings" ]; then
    not_his="not the author's checkout (no satellite_backup $profile profile in $backup)"
else
    # READ AS satellite_backup_main.py's load_profile reads it: the LAST folder line wins, and blanks
    # around the key and the value are not part of either (the review, 2026-10-11).
    folder=$(sed -n 's/^[[:space:]]*folder[[:space:]]*=[[:space:]]*//p' "$settings" | sed 's/[[:space:]]*$//' | tail -n 1)
    case $folder in "~") folder=$HOME ;; "~/"*) folder=$HOME/${folder#"~/"} ;; esac
    if [ -z "$folder" ] || [ ! -d "$folder" ] || [ "$(cd -- "$folder" && pwd -P)" != "$repo" ]; then
        not_his="not the author's checkout (the satellite_backup $profile profile backs up ${folder:-no folder})"
    fi
fi
if [ "${1:-}" = --check ]; then
    [ -n "$not_his" ] || exit 0
    exit 1
fi
if [ -n "$not_his" ]; then
    say "$not_his: nothing committed, nothing uploaded"
    exit 0
fi

branch=$(git -C "$repo" branch --show-current)
[ -n "$branch" ] || fail "no branch is checked out (a detached HEAD): nothing committed, nothing uploaded"
gitdir=$(git -C "$repo" rev-parse --absolute-git-dir)
for unfinished in MERGE_HEAD CHERRY_PICK_HEAD REVERT_HEAD rebase-merge rebase-apply; do
    [ ! -e "$gitdir/$unfinished" ] ||
        fail "a merge, cherry-pick, revert or rebase is not finished ($unfinished): satl is built and installed, nothing committed, nothing uploaded -- finish it or abort it first"
done
[ -z "$(git -C "$repo" ls-files -u)" ] ||
    fail "some files are still in conflict (git ls-files -u): satl is built and installed, nothing committed, nothing uploaded"

# 1. COMMIT
nested=$(git -C "$repo" ls-files --others --exclude-standard --directory | while IFS= read -r d; do
    if [ "${d%/}" != "$d" ] && [ -e "$repo/${d}.git" ]; then printf '%s ' "${d%/}"; fi
done)
[ -z "$nested" ] ||
    fail "a git repository inside the tree -- $nested-- would go in as a bare link, with none of its files: ignore it in .gitignore, or take its .git out. satl is built and installed, nothing committed, nothing uploaded"
added=$(git -C "$repo" add -A 2>&1) ||
    fail "git add failed -- $(printf '%s' "$added" | tr '\n' ' ')-- satl is built and installed, nothing committed"
links=$(git -C "$repo" ls-files -s | awk -F '\t' '{ split($1, f, " "); if (f[1] == "160000") print $2 }')
if [ -n "$links" ]; then
    printf '%s\n' "$links" | while IFS= read -r link; do
        git -C "$repo" rm -q --cached -- "$link" || :
    done
    fail "a git repository inside the tree -- $(printf '%s' "$links" | tr '\n' ' ')-- would go in as a bare link, with none of its files: ignore it in .gitignore, or take its .git out. satl is built and installed, nothing committed, nothing uploaded"
fi
if git -C "$repo" diff --cached --quiet; then
    say "nothing changed since the last commit: nothing committed"
else
    version=$(SATL_NO_WINDOW=1 "$repo/build/satl" --version 2> /dev/null | sed -n 2p)
    [ -n "$version" ] || version="(build/satl gave no version)"
    changed=$(git -C "$repo" diff --cached --shortstat | sed 's/^ *//')
    if [ -n "${COMMIT_MESSAGE:-}" ]; then
        git -C "$repo" commit -q -m "satl $version" -m "$changed" -m "$COMMIT_MESSAGE" ||
            fail "git commit failed: satl is built and installed, nothing committed"
    else
        git -C "$repo" commit -q -m "satl $version" -m "$changed" ||
            fail "git commit failed: satl is built and installed, nothing committed"
    fi
    say "committed $(git -C "$repo" rev-parse --short HEAD) on $branch -- $changed"
fi

# 2. UPLOAD
if ! head=$(git -C "$repo" rev-parse --verify -q HEAD); then
    say "this repository has no commit yet: nothing uploaded"
    exit 0
fi
short=$(git -C "$repo" rev-parse --short HEAD)
marker=$gitdir/satellite-last-upload
if [ -f "$marker" ] && [ "$(cat -- "$marker")" = "$head" ]; then
    say "commit $short is already on the server: nothing uploaded"
    exit 0
fi
say "uploading commit $short: python3 satellite_backup_main.py $profile next --yes (in $backup)"
(cd -- "$backup" && python3 satellite_backup_main.py "$profile" next --yes) ||
    fail "the upload failed: satl is built and installed, and commit $short is made -- it is NOT on the server"
printf '%s\n' "$head" > "$marker"
