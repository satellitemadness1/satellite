#!/bin/bash
# utility/push_to_main.sh -- put this branch's commits on GitHub's main, and keep
# the main it replaces under a name of its own, every single time.
#
# The author, 2026-09-23: "Commit to the github main, and the old main gets a
# different name, every single time". So before main moves, the commit it points at
# is pushed to a branch of its own:
#
#     archive/main-<date>-<its short hash>        e.g. archive/main-2026-09-23-5c74a6a
#
# -- the name the older ones already follow (archive/main-004-revision-02,
# archive/main-pre-restructure). The date says when it stopped being main; the hash
# says exactly which commit it was, and makes two names the same only if they are
# the same commit.
#
# ONLY FORWARD. main moves only when the commits on GitHub are all in this branch
# already (a fast-forward). When they are not -- somebody pushed from elsewhere --
# this stops and says so instead of pushing over them: a force is a decision, and
# not this script's to make.
#
#     utility/push_to_main.sh            from anywhere inside the repository
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
remote=origin

git fetch --quiet "$remote" main
old=$(git rev-parse "$remote/main")
new=$(git rev-parse HEAD)

if [ "$old" = "$new" ]; then
    echo "GitHub's main is already $(git rev-parse --short "$new") -- nothing to push."
    exit 0
fi
if ! git merge-base --is-ancestor "$old" "$new"; then
    echo "GitHub's main ($(git rev-parse --short "$old")) has commits this branch does not;" >&2
    echo "nothing was pushed. Merge or rebase onto $remote/main first." >&2
    exit 1
fi
if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "note: files changed and not committed are not in this push -- only commits go." >&2
fi

archive="archive/main-$(date +%Y-%m-%d)-$(git rev-parse --short "$old")"

# THE OLD MAIN FIRST, so it has its name before main moves off it.
git push --quiet "$remote" "$old:refs/heads/$archive"
git push --quiet "$remote" "HEAD:main"

# The local main follows, so `main` means the same thing here as on GitHub. Not when
# another worktree has main checked out -- git refuses that, and it is only a label.
if [ "$(git rev-parse --abbrev-ref HEAD)" != "main" ]; then
    git branch --force main HEAD 2> /dev/null ||
        echo "note: local main was not moved (checked out in another worktree)" >&2
fi

echo "GitHub's main was $(git rev-parse --short "$old"), and is kept as $archive"
echo "GitHub's main is now $(git rev-parse --short "$new"): $(git log -1 --format=%s "$new")"
