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

$interpreter examples/hello_world.satl > build/hello.out 2> build/hello.err; code=$?
expect "hello_world.satl runs" 0 $code
wanted=$'Hello, World!\na // inside a string is not a comment\n42\ntrue'
expect "hello_world.satl output" "$wanted" "$(cat build/hello.out)"

$interpreter build/no_such_file.satl > /dev/null 2>&1; expect "missing file" 8 $?
$interpreter > /dev/null 2>&1; expect "no file named" 8 $?
$interpreter tests/missing_include.satl > /dev/null 2>&1; expect "missing include" 10 $?
$interpreter tests/missing_main.satl > /dev/null 2>&1; expect "missing main" 11 $?
$interpreter tests/missing_return.satl > /dev/null 2>&1; expect "missing return" 12 $?
$interpreter tests/not_understood.satl > build/nu.out 2>&1; code=$?
expect "a line with no scenario" 13 $code
expect "nothing ran before the refusal" "" "$(grep -x before build/nu.out)"
$interpreter tests/two_strings.satl > /dev/null 2>&1; expect "\"some\" + \"str\" (no scenario yet)" 4 $?
$interpreter tests/big_number.satl > /dev/null 2>&1; expect "a number too large" 3 $?
$interpreter examples/hello_world.satl > /dev/full 2> /dev/null; expect "output refused (/dev/full)" 2 $?

mkdir -p build/alone && cp $interpreter build/alone/satellite-004
build/alone/satellite-004 examples/hello_world.satl > /dev/null 2>&1; expect "no libraries beside the interpreter" 5 $?

$interpreter --debug examples/hello_world.satl > build/debug.out 2>&1; code=$?
expect "--debug runs" 0 $code
expect "--debug shows the index being defined" 1 "$(grep -c 'vector.number.index(defined) (machine_code: 6 number_vector_defined)' build/debug.out)"
expect "--debug shows memory as a size" 1 "$(grep -cE '^\[satellite\] arguments.memory.total = [0-9.]+ (kilo|mega|giga|tera)?bytes' build/debug.out)"

echo "$passed passed, $failed failed"
[ "$failed" = 0 ]
