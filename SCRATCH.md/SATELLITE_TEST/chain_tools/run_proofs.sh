#!/bin/bash
# run_proofs.sh <dir>... -- runs loss_chain.satl in each folder through timed.sh, four at a time (the most satl runs
# allowed at once here), each in its own folder (the file test writes beside its program); then writes ALL_DONE
D=$(cd "$(dirname "$0")" && pwd)
for d in "$@"; do rm -f "$d/times.txt"; done
printf '%s\n' "$@" | xargs -P 4 -I{} "$D/timed.sh" {} loss_chain.satl
echo DONE > "$D/proofs/ALL_DONE.$$"
