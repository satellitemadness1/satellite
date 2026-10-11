#!/bin/bash
# timed.sh <dir> <file.satl> -- runs one program through probe.sh from inside <dir> (the only way satl is run here),
# writes <file>.out and <file>.err beside it, and adds one line to <dir>/times.txt: exit code, seconds, stdout,
# any .se file left behind, and the first report lines on stderr
P=/tmp/claude-1000/-home-madness-code-cxx-satellite/3c0549d6-c849-4347-acef-4adf2f059c62/scratchpad/chain/probe.sh
cd "$1" || exit 1
f=$2
s=$(date +%s.%N)
$P "$f" > "${f%.satl}.out" 2> "${f%.satl}.err"
rc=$?
e=$(date +%s.%N)
printf "%s exit=%d seconds=%.2f stdout=[%s] leftover=[%s] err=[%s]\n" "$f" $rc "$(echo "$e - $s" | bc)" \
    "$(head -c 400 "${f%.satl}.out" | tr '\n' '|')" "$(ls *.se 2>/dev/null | tr '\n' ' ')" \
    "$(grep -m3 -E '^S[0-9]{3}|satl\(|^directory:|^syntax:' "${f%.satl}.err" | tr '\n' ' ' | head -c 400)" >> times.txt
