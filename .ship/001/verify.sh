#!/usr/bin/env bash
#
# verify.sh -- prove that this folder works, on this machine, right now.
#
# Runs every runnable thing in .ship/001 and exits non-zero if any of it fails.
# It is deliberately paranoid about two things that silently pass otherwise:
#
#   1. WHERE THE LIBRARY COMES FROM. bin/satellite links against
#      libsatellite_core.so. If the repository's build/ tree is still on disk,
#      a mislinked binary will happily load the library from there and every
#      test will pass for the wrong reason. Check 2 asserts the loaded .so is
#      the one in bin/.
#
#   2. WHETHER ANYTHING ACTUALLY COMPILED. satellite.cxx blocks are cached by
#      content hash in ~/.satellite/cache, so a run can report success having
#      compiled nothing at all. --cold empties the cache first and asserts that
#      the blocks really are compiled from source.
#
# Usage:
#   ./verify.sh            run against the cache as it is (fast)
#   ./verify.sh --cold     clear the block cache first and force real compiles
#
# Exit status: 0 = everything passed, 1 = at least one check failed.

set -u

here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
sat="$here/bin/satellite"
core="$here/bin/libsatellite_core.so"

cold=0
[ "${1:-}" = "--cold" ] && cold=1

pass=0
fail=0

# ok <name>              -- record a pass
# bad <name> <detail...> -- record a failure and say why
ok()  { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf '  FAIL  %s\n' "$1"
        shift; for line in "$@"; do printf '          %s\n' "$line"; done; }

# expect <name> <expected-exit> <needle> -- <command...>
#
# Runs the command, then asserts BOTH the exit status and that <needle> appears
# in the combined output. Asserting on output matters: `satellite run` on a file
# with no C++ blocks exits 0 and prints a disclaimer, so exit status alone would
# call that a pass.
expect() {
    local name="$1" want_rc="$2" needle="$3"; shift 4   # shift past the --
    local out rc
    out="$("$@" 2>&1)"; rc=$?

    if [ "$rc" -ne "$want_rc" ]; then
        bad "$name" "expected exit $want_rc, got $rc" "output was:" "$out"
        return
    fi
    if [ -n "$needle" ] && [[ "$out" != *"$needle"* ]]; then
        bad "$name" "expected to find: $needle" "output was:" "$out"
        return
    fi
    ok "$name"
}

echo "satellite 001 -- verifying $here"
echo

# ---------------------------------------------------------------------------
echo "[1] files are present and non-empty"
# ---------------------------------------------------------------------------
for f in README.md MANIFEST.txt common_headers.txt verify.sh \
         bin/satellite bin/libsatellite_core.so \
         include/satellite/cxx.hpp include/satellite/container.hpp \
         examples/hello_main.satl examples/cxx_hello.satl \
         examples/cxx_math.satl examples/cxx_vector.satl \
         examples/cxx_template.satl examples/cxx_error.satl \
         examples/satellite_full_test.satl; do
    if [ -s "$here/$f" ]; then ok "$f"; else bad "$f" "missing or empty"; fi
done
[ -x "$sat" ] && ok "bin/satellite is executable" \
              || bad "bin/satellite is executable" "chmod +x it"

# ---------------------------------------------------------------------------
echo
echo "[2] the binary is self-contained"
# ---------------------------------------------------------------------------
# $ORIGIN in the RPATH is what makes bin/ relocatable. Without it the binary
# hardcodes an absolute path to whatever build tree produced it.
rpath="$(objdump -x "$sat" 2>/dev/null \
         | awk '/^[[:space:]]*(RPATH|RUNPATH)[[:space:]]/ {print $2}')"
if [ "$rpath" = '$ORIGIN' ]; then
    ok "RPATH is \$ORIGIN"
else
    bad "RPATH is \$ORIGIN" "found: ${rpath:-<none>}"
fi

# The load-bearing check: resolve the library the loader will actually use.
loaded="$(ldd "$sat" 2>/dev/null | awk '/libsatellite_core\.so/ {print $3}')"
if [ "$loaded" = "$core" ]; then
    ok "libsatellite_core.so resolves inside bin/"
else
    bad "libsatellite_core.so resolves inside bin/" \
        "expected: $core" "resolved: ${loaded:-<unresolved>}"
fi

if ldd "$sat" 2>/dev/null | grep -q 'not found'; then
    bad "all shared libraries resolve" "$(ldd "$sat" | grep 'not found' | tr -s ' ')"
else
    ok "all shared libraries resolve"
fi

# g++ is a hard runtime requirement for satellite.cxx -- not for version/check.
if command -v g++ >/dev/null 2>&1; then
    ok "g++ on PATH ($(g++ --version | head -1))"
else
    bad "g++ on PATH" "satellite.cxx blocks cannot compile without it"
fi

# ---------------------------------------------------------------------------
echo
echo "[3] satellite version"
# ---------------------------------------------------------------------------
expect "version prints 001" 0 "001" -- "$sat" version

# ---------------------------------------------------------------------------
echo
echo "[4] satellite check -- every example lexes clean"
# ---------------------------------------------------------------------------
for f in "$here"/examples/*.satl; do
    expect "check $(basename "$f")" 0 ": ok" -- "$sat" check "$f"
done

# The conformance file is pinned to its exact token count: a change in the lexer
# that alters tokenisation should fail here rather than pass quietly.
expect "full test lexes to 1150 tokens" 0 "1150 tokens, 117 statements" \
    -- "$sat" check "$here/examples/satellite_full_test.satl"

# ---------------------------------------------------------------------------
echo
echo "[5] satellite check -- rejects bad input (proves it can fail)"
# ---------------------------------------------------------------------------
# Without these, [4] passing would be consistent with `check` always saying ok.
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

printf 'satellite.include(satellite)\nsatellite.variable.string s = "oops\n' \
    > "$tmp/unterminated_string.satl"
printf 'satellite.include(satellite)\n/* never closed\n' \
    > "$tmp/unterminated_comment.satl"

expect "rejects unterminated string"  1 "unterminated string" \
    -- "$sat" check "$tmp/unterminated_string.satl"
expect "rejects unterminated comment" 1 "unterminated block comment" \
    -- "$sat" check "$tmp/unterminated_comment.satl"

# ---------------------------------------------------------------------------
echo
echo "[6] satellite run -- every cxx block produces its documented output"
# ---------------------------------------------------------------------------
if [ "$cold" -eq 1 ]; then
    cache="${HOME:-}/.satellite/cache"
    echo "  (--cold: clearing $cache)"
    rm -rf "$cache"
    # With the cache empty this MUST report a real compile. If it says
    # "from cache" instead, the cache was not where we think it is and every
    # timing/compilation claim below is meaningless.
    expect "cold start really compiles" 0 "1 compiled, 0 from cache" \
        -- "$sat" run "$here/examples/cxx_hello.satl"
fi

# name -> the exact string the example's header comment promises.
run_case() {
    local file="$1" needle="$2"
    expect "run $(basename "$file")" 0 "$needle" -- "$sat" run "$here/examples/$file"
    # A block that failed to compile still exits 0 with "1 failed" in the summary.
    expect "run $(basename "$file") -- 0 failed" 0 "0 failed" \
        -- "$sat" run "$here/examples/$file"
}

run_case hello_main.satl   "hello from satellite 001"
run_case cxx_hello.satl    "hello from C++"
run_case cxx_math.satl     "5050"
run_case cxx_vector.satl   "[1, 4, 9, 16, 25, 36, 49, 64]"
run_case cxx_template.satl "ints -> 15, reals -> 4"
run_case cxx_error.satl    "cxx error: index 7 is past the end (size 3)"

# The three blocks inside the conformance file also have to run.
expect "run satellite_full_test.satl" 0 "3 block(s)" \
    -- "$sat" run "$here/examples/satellite_full_test.satl"
expect "run satellite_full_test.satl -- 0 failed" 0 "0 failed" \
    -- "$sat" run "$here/examples/satellite_full_test.satl"

if [ "$cold" -eq 1 ]; then
    # After a cold start the FIRST run above must have compiled, not cached.
    # Re-running now must hit the cache -- that proves caching works too.
    expect "second run hits the cache" 0 "from cache" \
        -- "$sat" run "$here/examples/cxx_hello.satl"
fi

# ---------------------------------------------------------------------------
echo
printf '%d passed, %d failed\n' "$pass" "$fail"
if [ "$fail" -ne 0 ]; then
    echo "VERIFY FAILED"
    exit 1
fi
echo "VERIFY OK"
exit 0
