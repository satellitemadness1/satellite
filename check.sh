#!/bin/bash
# Runs satellite-004 on every example and test, and checks the machine code
# (the exit status) and, where it matters, the output.
cd "$(dirname "$0")"
interpreter=build/satellite-004
passed=0 failed=0

expect() {   # expect <description> <wanted code> <actual code>
    if [ "$2" = "$3" ]; then passed=$((passed + 1)); echo "  ok    $1 -> $3";
    else failed=$((failed + 1)); echo "  FAIL  $1 -> wanted $2, got $3"; fi
}

# The author's rows, read by the same reader the build uses (build_number.py).
config_row() { python3 satellite/config/build_number.py --print "arguments.$1"; }
title="THE SATELLITE PROGRAMMING LANGUAGE
VERSION $(printf %03d "$(config_row version)") REVISION $(printf %02d "$(config_row revision)") BUILD $(printf %04d "$(config_row build)")"
compiler_line='^(CLANG\+\+|G\+\+) [0-9]+ [A-Z0-9 .-]+$'
rule=---------------------------------------------------------------

expect "--version shows the title lines" "$title" "$($interpreter --version | head -2)"
expect "--version's third line names the compiler and system" 1 "$($interpreter --version | sed -n 3p | grep -cE "$compiler_line")"
expect "--version is three lines" 3 "$($interpreter --version | wc -l)"
expect "-V shows the title lines" "$title" "$($interpreter -V | head -2)"
expect "--help starts with the start-up block" "$rule" "$($interpreter --help | sed -n 4p)"
$interpreter --version > /dev/full 2> build/full.err; code=$?
expect "--version into /dev/full" 2 $code
expect "--version into /dev/full says why" 1 "$(grep -c 'machine_code: 2 display_error' build/full.err)"
$interpreter --version extra > /dev/null 2> build/extra.err; code=$?
expect "--version with another word is refused" 23 $code

$interpreter examples/hello_world.satl > build/hello.out 2> build/hello.err; code=$?
expect "hello_world.satl runs" 0 $code
wanted=$'Hello, World!\na // inside a string is not a comment\n42\ntrue'
expect "hello_world.satl output" "$wanted" "$(cat build/hello.out)"
if [ "$(config_row startup_display)" = true ]; then
    expect "the start-up block on stderr: the title lines" "$title" "$(head -2 build/hello.err)"
    expect "the start-up block on stderr: the compiler and system" 1 "$(sed -n 3p build/hello.err | grep -cE "$compiler_line")"
    expect "the start-up block on stderr: 63 dashes, then an empty line" "$rule|" "$(sed -n 4p build/hello.err)|$(sed -n 5p build/hello.err)"
    expect "the start-up block is five lines" 5 "$(wc -l < build/hello.err)"
else
    expect "arguments.startup_display is false: nothing on stderr" 0 "$(wc -c < build/hello.err)"
fi
python3 -c "import os, subprocess, sys; r, w = os.pipe(); os.close(r); sys.exit(subprocess.run(['$interpreter', 'examples/hello_world.satl'], stderr=w, stdout=subprocess.DEVNULL).returncode & 255)"
expect "stderr a pipe nobody reads: the program still runs" 0 $?
$interpreter examples/hello_world.satl --version > /dev/null 2>&1; expect "a --version after the file is not satl's" 0 $?

$interpreter build/no_such_file.satl > /dev/null 2>&1; expect "missing file" 8 $?
$interpreter > /dev/null 2>&1; expect "no file named" 8 $?
$interpreter tests/missing_include.satl > /dev/null 2>&1; expect "missing include" 10 $?
$interpreter tests/missing_main.satl > /dev/null 2>&1; expect "missing main" 11 $?
$interpreter tests/missing_return.satl > /dev/null 2>&1; expect "missing return" 12 $?
$interpreter tests/not_understood.satl > build/nu.out 2>&1; code=$?
expect "a line with no scenario" 13 $code
expect "nothing ran before the refusal" "" "$(grep -x before build/nu.out)"
# THESE TWO CHANGED ON 2026-09-16, WHEN THE ARITHMETIC TOKENS WERE WIRED TO
# satellite_number's FAST PATHS, and they changed from "refuses" to "answers".
# Both used to assert a refusal, and both refusals were placeholders for work
# that had not been done -- the rows read "(no scenario yet)" and "a number too
# large" in a language whose whole number type cannot BE too large. They are kept
# as checks of the ANSWER, not deleted, so the behaviour stays pinned:
#   + joins two strings (003 DESIGN §6.6, the author at M19)
#   a literal of 23 digits is held exactly, not refused and not truncated
expect "\"some\" + \"str\" joins them" "somestr" "$($interpreter tests/two_strings.satl 2>/dev/null)"
# `.find(` -- period + the method's own 16-bit code + `(` (the author, 2026-09-16).
# A quoted argument and an object argument both work; an undeclared one is 25.
expect "s.find() on a literal and on an object" "6|0|6" \
       "$($interpreter tests/find.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
$interpreter tests/find_no_object.satl > /dev/null 2>&1; expect "s.find(no object) is refused" 25 $?
# Aliases collapse at the lexer (to_string/str/string are ONE code); conversions
# go through satellite_number as the hub; and a chain is a loop, so `s.bin.find(x)`
# is two turns of it (the author, 2026-09-16: "so we can string operations together").
wanted_chain="87|87|87|1010111|1010111|57|87|87|1010111|0|4|100|87!|87x"
expect "aliases, conversions and chained methods" "$wanted_chain" \
       "$($interpreter tests/chain.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
expect "a 23-digit number is held exactly" "99999999999999999999999" "$($interpreter tests/big_number.satl 2>/dev/null)"

# THE SIX FAST PATHS, REACHED THROUGH THEIR TOKENS. The arithmetic itself is
# proven against Python over 482,465 cases (check_numbers.py); what this proves
# is that a `+` in a program reaches it, with 003 DESIGN §6.6's precedence.
# Python is the authority for every line, and writes them here itself.
wanted_math=$(python3 -c "
for n in (34587, 2+3, 10-4, 6*7, 20//3, 20%3, 2**10, 2+3*4, 10-3-2, 2**(3**2), -3, -1,
          99999999999999999999999+1, 2**200, 0b1100+0xFF, sum(range(1000))): print(n)")
expect "the six fast paths through their tokens" "$wanted_math" "$($interpreter tests/arithmetic.satl 2>/dev/null)"
$interpreter tests/arithmetic.satl > /dev/null 2>&1; expect "tests/arithmetic.satl runs" 0 $?
$interpreter tests/divide_by_zero.satl > /dev/null 2>&1; expect "a divisor of zero" 22 $?
$interpreter tests/negative_exponent.satl > /dev/null 2>&1; expect "2 ^ -1 is not a whole number" 24 $?
$interpreter tests/undeclared.satl > build/un.out 2>&1; expect "a name nothing declared" 25 $?
expect "nothing ran before THAT refusal" "" "$(grep -x before build/un.out)"
$interpreter tests/declared_twice.satl > /dev/null 2>&1; expect "a name declared twice" 26 $?
$interpreter tests/kinds_do_not_meet.satl > /dev/null 2>&1; expect "\"n = \" + 4 converts nothing" 27 $?
# ERROR.md: an expression that stops early is refused, not half-stored. `&` has no
# meaning yet, and `n = 1 & 2` used to store 1 and `while(n < 3 & 1)` ran as `n < 3`.
$interpreter tests/unread_assignment.satl > build/unread.out 2>&1; expect "n = 1 & 2 is refused, not stored as 1" 13 $?
expect "... and nothing was displayed" "" "$(grep -x 1 build/unread.out)"
$interpreter tests/unread_while.satl > build/unread.out 2>&1; expect "while(n < 3 & 1) is refused, not run as n < 3" 13 $?
expect "... and the loop never counted to 3" "" "$(grep -x 3 build/unread.out)"
expect "a trailing comment still ends a value" 8 "$($interpreter tests/comment_after_value.satl 2>/dev/null)"
$interpreter tests/unread_trailing.satl > build/unread.out 2>&1; expect "n = 5 6 is refused, not stored as 5" 13 $?
$interpreter tests/compound_assign.satl > build/unread.out 2>&1; expect "n += 1 is refused until += is built, not skipped" 14 $?
expect "... before anything runs, and says how to write it" "1|" \
       "$(grep -c 'n += ... is not built yet -- write n = n + ...' build/unread.out)|$(grep -x before build/unread.out)"
# satellite.variable.binary (the author, 2026-09-16): written with its b, and shown
# exactly as written -- b and leading zeros -- because the width is part of the value.
expect "satellite.variable.binary my_number = b10101010" \
       "b10101010|b0010|b0000|170|b10101010|0000|AA|171|-5|3|false|true|3|true" \
       "$($interpreter tests/binary.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
$interpreter tests/binary.satl > /dev/null 2>&1; expect "tests/binary.satl runs" 0 $?
# "if the user doesn't enter "b" ... spit out an ERROR: expected "b"+whatever they entered"
$interpreter tests/binary_without_b.satl > build/binary_b.out 2>&1; expect "a binary with no b" 27 $?
expect "a binary with no b says ERROR: expected b10101010" 1 "$(grep -c 'ERROR: expected b10101010 ' build/binary_b.out)"
expect "nothing ran before the missing b" "" "$(grep -x before build/binary_b.out)"
$interpreter tests/binary_assigned_without_b.satl > build/binary_b.out 2>&1; expect "a binary given a value with no b" 27 $?
expect "... and says ERROR: expected b1111" 1 "$(grep -c 'ERROR: expected b1111 ' build/binary_b.out)"
$interpreter tests/binary_digits_not_binary.satl > build/binary_b.out 2>&1; expect "a binary given 12" 27 $?
expect "... is told 12 is not binary, not to write b12" 1 "$(grep -c 'ERROR: 12 is not binary' build/binary_b.out)"
$interpreter tests/binary_b_not_binary.satl > build/binary_b.out 2>&1; expect "a binary given b12" 27 $?
expect "... is told b12 is not binary" 1 "$(grep -c 'ERROR: b12 is not binary' build/binary_b.out)"
$interpreter tests/binary_in_brackets.satl > build/binary_b.out 2>&1; expect "a binary with no b inside brackets" 27 $?
expect "... is still ERROR: expected b10101010, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected b10101010 ' build/binary_b.out)|$(grep -x before build/binary_b.out)"
$interpreter tests/binary_0b.satl > build/binary_b.out 2>&1; expect "0b10101010, the C spelling" 27 $?
# satellite.variable.percentage (the author, 2026-09-17): 32 digits after the point,
# rounded half away from zero there. Python's decimal module is the authority.
wanted_percentage=$(python3 -c "
from decimal import Decimal, getcontext, ROUND_HALF_UP
from fractions import Fraction as F
getcontext().prec = 300
def pct(value):
    d = (Decimal(value.numerator) / Decimal(value.denominator)) if isinstance(value, F) else Decimal(value)
    text = format(d.quantize(Decimal(1).scaleb(-32), rounding=ROUND_HALF_UP), 'f')
    return (text.rstrip('0').rstrip('.') if '.' in text else text) + '%'
for line in (pct('50'), pct('33.333333333333333333333333333333335'), pct('1000000000000'), pct('12.5'),
             pct('100.000000000000000000000000000000004'), pct('0.000000000000000000000000000000005'),
             200 * 50 // 100, 5 * 1000000000000 // 100, 50 * 200 // 100, 200 * (100 - 50) // 100,
             200 * (100 + 50) // 100, 200 * 100 // 50,
             pct('75'), pct('-25'), pct(F(50 * 50, 100)), pct(F(50, 4)), pct('-50'), 'true', 'true', pct('50'),
             pct(F(100, 3)), pct(F(200, 3)), 10 * 50 // 100):
    print(line)")
expect "satellite.variable.percentage: literals, rounding and every pair" "$wanted_percentage" \
       "$($interpreter tests/percentage.satl 2>/dev/null)"
$interpreter tests/percentage.satl > /dev/null 2>&1; expect "tests/percentage.satl runs" 0 $?
$interpreter tests/percentage_without_percent.satl > build/percentage.out 2>&1; expect "a percentage with no %" 27 $?
expect "... says ERROR: expected 50%, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected 50% ' build/percentage.out)|$(grep -x before build/percentage.out)"
$interpreter tests/percentage_not_whole.satl > build/percentage.out 2>&1; expect "3 * 50% is not whole" 24 $?
expect "... and says so" 1 "$(grep -c '3 \* 50% is not a whole number' build/percentage.out)"
$interpreter tests/percentage_number_second.satl > build/percentage.out 2>&1; expect "50% + 5 is refused" 27 $?
expect "... and says to put the number first" 1 "$(grep -c 'the number goes first -- 5 + 50%' build/percentage.out)"
expect "... is told ERROR: expected b10101010, not b0" 1 "$(grep -c 'ERROR: expected b10101010 ' build/binary_b.out)"
expect "a declaration inside a loop runs every turn" "0|1|10|11|20|21" \
       "$($interpreter tests/loop_declaration.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
$interpreter examples/hello_world.satl > /dev/full 2> /dev/null; expect "output refused (/dev/full)" 2 $?

mkdir -p build/alone && cp $interpreter build/alone/satellite-004
build/alone/satellite-004 examples/hello_world.satl > /dev/null 2>&1; expect "no libraries beside the interpreter" 5 $?

$interpreter --debug examples/hello_world.satl > build/debug.out 2>&1; code=$?
expect "--debug runs" 0 $code
expect "--debug shows the index being defined" 1 "$(grep -c 'vector.number.index(defined) (machine_code: 6 number_vector_defined)' build/debug.out)"
expect "--debug shows memory as a size" 1 "$(grep -cE '^\[satellite\] arguments.memory.total = [0-9.]+ (kilo|mega|giga|tera)?bytes' build/debug.out)"
expect "--debug shows arguments.threads_startup from the config" 1 "$(grep -c "^\[satellite\] arguments.threads_startup = $(config_row threads_startup) " build/debug.out)"
# Every config number is a satellite_number (the author, 2026-09-16), and infinity's
# two digit counts are rows: 4096 held, 32 shown, "both digits configurable".
expect "--debug shows arguments.infinity from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity = $(config_row infinity) " build/debug.out)"
expect "--debug shows arguments.infinity_display from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity_display = $(config_row infinity_display) " build/debug.out)"
expect "the start-up threads are warm" 1 "$(grep -cE "^\[satellite\] threads.startup\(warm\): $(config_row threads_startup) threads parked in [0-9.]+ ms" build/debug.out)"

echo "$passed passed, $failed failed"
[ "$failed" = 0 ]
