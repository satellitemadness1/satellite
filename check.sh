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
expect "the start-up threads are warm" 1 "$(grep -cE "^\[satellite\] threads.startup\(warm\): $(config_row threads_startup) threads parked in [0-9.]+ ms" build/debug.out)"

echo "$passed passed, $failed failed"
[ "$failed" = 0 ]
