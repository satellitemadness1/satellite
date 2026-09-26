#!/bin/bash
# sweep_corpus.sh <satl> <out_folder> -- every .satl in a fresh COPY of ~/code/satl (the author's
# programs), each run from its own folder with no display, no window and a HOME of its own.
# Writes <out_folder>/results.tsv (path, exit code, md5 of the output without the title lines)
# and <out_folder>/out/ (each program's whole output). Compare two runs with:
#
#     join -t $'\t' <(sort a/results.tsv) <(sort b/results.tsv) | awk -F'\t' '$2 != $4'
#
# ONE PROGRAM AT A TIME ON PURPOSE: some of his programs start threads, and a thread test froze
# this machine once (memory: thread-limit-on-this-machine). A sweep of 400 takes about a minute.
# Never add `ulimit -v`: satl reserves address space at start-up and fails for no real reason.
set -u
satl=$(realpath "$1"); out=$(realpath -m "$2")
rm -rf "$out"; mkdir -p "$out/out" "$out/home/.satl" "$out/xdg"; chmod 700 "$out/xdg"
cp -a ~/code/satl "$out/corpus"
cp ~/.satl/config.ini "$out/home/.satl/" 2>/dev/null
cd "$out/corpus" || exit 1
find . -name '*.satl' | sort > "$out/list.txt"
while read -r f; do
    d=$(dirname "$f"); b=$(basename "$f")
    result=$(cd "$d" && env -u DISPLAY -u WAYLAND_DISPLAY -u XAUTHORITY -u GDK_BACKEND SATL_NO_WINDOW=1 \
             XDG_RUNTIME_DIR="$out/xdg" HOME="$out/home" nice -n 19 timeout 20 "$satl" "$b" < /dev/null 2>&1)
    code=$?
    printf '%s' "$result" > "$out/out/$(echo "$f" | tr '/' '_').out"
    printf '%s\t%s\t%s\n' "$f" "$code" "$(printf '%s' "$result" | grep -v '^VERSION\|BUILD\|^CLANG' | md5sum | cut -c1-8)"
done < "$out/list.txt" | sort > "$out/results.tsv"
echo "swept $(wc -l < "$out/results.tsv") programs into $out/results.tsv"
cut -f2 "$out/results.tsv" | sort | uniq -c | sort -rn
