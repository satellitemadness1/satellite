#!/bin/bash
# Runs satl (satellite 004) on every example and test, and checks the machine code
# (the exit status) and, where it matters, the output.
#
#     ./check.sh                      checks build/satl
#     SATL=<path>/satl ./check.sh     checks that satl instead -- an installed copy
#                                     (PLAN M0.5); the libraries beside it are its own
# SATL is read from where check.sh was STARTED, before it moves to its own folder.
interpreter=$(realpath -s "${SATL:-$(dirname "$0")/build/satl}")
cd "$(dirname "$0")"
passed=0 failed=0

expect() {   # expect <description> <wanted code> <actual code>
    if [ "$2" = "$3" ]; then passed=$((passed + 1)); echo "  ok    $1 -> $3";
    else failed=$((failed + 1)); echo "  FAIL  $1 -> wanted $2, got $3"; fi
}

# The author's rows, read by the same reader the build uses (build_number.py).
config_row() { python3 satellite/config/build_number.py --print "arguments.$1"; }
# Zeros in front, never printf %04d, which stops at 2^63-1 (a quoted row has no ceiling).
padded() { p=$1; while [ ${#p} -lt "$2" ]; do p=0$p; done; printf '%s' "$p"; }
title="THE SATELLITE PROGRAMMING LANGUAGE
VERSION $(padded "$(config_row version)" 3) REVISION $(padded "$(config_row revision)" 2) BUILD $(padded "$(config_row build)" 4)"
compiler_line='^(CLANG\+\+|G\+\+) [0-9]+ [A-Z0-9 .-]+$'
rule=---------------------------------------------------------------

expect "--version shows the title lines" "$title" "$("$interpreter" --version | head -2)"
expect "--version's third line names the compiler and system" 1 "$("$interpreter" --version | sed -n 3p | grep -cE "$compiler_line")"
expect "--version is three lines" 3 "$("$interpreter" --version | wc -l)"
expect "-V shows the title lines" "$title" "$("$interpreter" -V | head -2)"
expect "--help starts with the start-up block" "$rule" "$("$interpreter" --help | sed -n 4p)"
"$interpreter" --version > /dev/full 2> build/full.err; code=$?
expect "--version into /dev/full" 2 $code
expect "--version into /dev/full says why" 1 "$(grep -c 'machine_code: 2 display_error' build/full.err)"
"$interpreter" --version extra > /dev/null 2> build/extra.err; code=$?
expect "--version with another word is refused" 23 $code

"$interpreter" examples/hello_world.satl > build/hello.out 2> build/hello.err; code=$?
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
python3 -c "import os, subprocess, sys; r, w = os.pipe(); os.close(r); sys.exit(subprocess.run([sys.argv[1], 'examples/hello_world.satl'], stderr=w, stdout=subprocess.DEVNULL).returncode & 255)" "$interpreter"
expect "stderr a pipe nobody reads: the program still runs" 0 $?
"$interpreter" examples/hello_world.satl --version > /dev/null 2>&1; expect "a --version after the file is not satl's" 0 $?

"$interpreter" build/no_such_file.satl > /dev/null 2>&1; expect "missing file" 8 $?

# SATL'S COMMAND LINE (PLAN M0.5): arguments/command_line.hpp has the rules.
"$interpreter" > build/bare.out 2> build/bare.err; code=$?
expect "bare satl is not an error (it was 8 before M0.5)" 0 $code
expect "bare satl shows the title lines, then how to start" "$title|1" \
       "$(head -2 build/bare.out)|$(grep -c '^    satl <file.satl> \[words...\] ' build/bare.out)"
expect "bare satl writes nothing on stderr" 0 "$(wc -c < build/bare.err)"
"$interpreter" --debug > /dev/null 2>&1; expect "satl --debug alone is bare satl" 0 $?
"$interpreter" -h > /dev/null 2>&1; expect "-h" 0 $?
"$interpreter" --debug --help > /dev/null 2> build/cl.err; expect "--debug --help is refused" 23 $?
expect "... by name" 1 "$(grep -c 'is the whole command line, and --debug came before it' build/cl.err)"
"$interpreter" --help extra > /dev/null 2>&1; expect "--help with another word is refused" 23 $?
"$interpreter" --run examples/hello_world.satl > build/run.out 2>/dev/null; code=$?
expect "--run runs the file" "0|$wanted" "$code|$(cat build/run.out)"
"$interpreter" --run > /dev/null 2> build/cl.err; expect "--run with no file is refused" 23 $?
expect "... and says a file goes after it" 1 "$(grep -c -- '--run needs the file to run after it' build/cl.err)"
"$interpreter" --run "" > /dev/null 2> build/cl.err; expect "--run \"\" is a file with no name" 8 $?
expect "... and says the name is empty" 1 "$(grep -c 'cannot locate file: (an empty name)' build/cl.err)"
# --repl IS THE SESSION NOW (M0.6), so it READS: given nothing to read it ends at
# once, and given a line it runs it. Never without a pipe here -- a bare --repl
# would wait for this script's own terminal.
"$interpreter" --repl < /dev/null > build/repl.out 2> build/cl.err; expect "--repl with nothing to read ends with 0" 0 $?
expect "... and says no prompt text to something that is not a terminal" 0 "$(grep -c 'satl>' build/repl.out)"
"$interpreter" --repl extra > /dev/null 2>&1; expect "--repl with another word is refused" 23 $?
for word in -- -x.satl --rum -; do
    "$interpreter" "$word" x.satl > /dev/null 2>&1; expect "\"$word\" is not a word satl takes" 23 $?
done
"$interpreter" —run examples/hello_world.satl > /dev/null 2>&1; expect "—run with an em dash is a file name, and missing" 8 $?
cp examples/hello_world.satl build/-x.satl && (cd build && "$interpreter" --run -x.satl > /dev/null 2>&1); \
    expect "--run -x.satl runs a file whose name begins with -" 0 $?
"$interpreter" --debug examples/hello_world.satl --version --debug --repl "" > build/words.out 2>&1; code=$?
expect "every word after the file is the program's" 0 $code
expect "... kept in order as arguments.argument_1 onwards, and counted with the program" \
       "arguments.program = examples/hello_world.satl|arguments.argument_1 = --version|arguments.argument_2 = --debug|arguments.argument_3 = --repl|arguments.argument_4 = |arguments.length = 5" \
       "$(grep -E '^\[satellite\] arguments\.(program|argument_[0-9]+|length) = ' build/words.out | sed 's/^\[satellite\] //; s/ (machine_code: 0 success)$//' | tr '\n' '|' | sed 's/|$//')"
expect "arguments.session.directory is where satl started" 1 \
       "$(grep -cxF "[satellite] arguments.session.directory = $PWD (machine_code: 0 success)" build/words.out)"
# DESIGN §9: a word that is not text never reaches the terminal raw -- ESC ] 2 ; BEL
# would retitle it. Shown escaped under --debug and in every refusal.
"$interpreter" --debug examples/hello_world.satl $'\e]2;title\a' $'\xff' > build/hostile.out 2>&1
expect "an escape sequence as a program word is shown as text" 1 \
       "$(grep -cF 'arguments.argument_1 = \x1b]2;title\x07 (machine_code' build/hostile.out)"
expect "... invalid UTF-8 too" 1 "$(grep -cF 'arguments.argument_2 = \xff (machine_code' build/hostile.out)"
"$interpreter" $'\e]2;title\a.satl' > build/hostile.out 2>&1; expect "an escape sequence as the file" 8 $?
expect "... is named, escaped, and no ESC or BEL byte is written" "1|0" \
       "$(grep -cF 'cannot locate file: \x1b]2;title\x07.satl' build/hostile.out)|$(tr -cd '\033\007' < build/hostile.out | wc -c)"
"$interpreter" examples > build/hostile.out 2>&1; expect "a directory as the file" 8 $?
expect "... says it is a directory" 1 "$(grep -c 'cannot run examples: it is a directory' build/hostile.out)"
ln -sfn ../examples/hello_world.satl build/link_to_hello.satl
"$interpreter" build/link_to_hello.satl > /dev/null 2>&1; expect "a symlink to a program runs it" 0 $?
if [ "$(id -u)" != 0 ]; then
    cp examples/hello_world.satl build/unreadable.satl && chmod 000 build/unreadable.satl
    "$interpreter" build/unreadable.satl > build/hostile.out 2>&1; expect "an unreadable file" 8 $?
    expect "... says why" 1 "$(grep -c 'cannot read file: build/unreadable.satl (Permission denied)' build/hostile.out)"
    rm -f build/unreadable.satl
fi
# A machine code an exit status cannot hold exits 255, never cut to 8 bits (256 would exit 0).
build/exit_status_cases > build/exit_status.out 2> build/exit_status.err; code=$?
expect "exit statuses for 0, 1, 255, 256, -1, 4294967298 ... ($(grep -c '^ok' build/exit_status.out) cases)" 0 $code
expect "... a code that does not fit is written in full on stderr" 1 \
       "$(grep -c 'machine code 4294967298 does not fit an exit status (0 to 254 exit as themselves), so satl exits 255' build/exit_status.err)"
expect "... and satl's main returns through exit_status_of, the one place a code becomes a status" 1 \
       "$(grep -cF 'return satellite004::exit_status_of(run_satl(argc, argv));' satellite/structured-library.cpp)"
# A config row may never hold a name satl fills in, however many words a run has.
build/arguments_cases > build/arguments_cases.out 2>&1; code=$?
expect "names satl fills in are refused as rows, and gather adds no other ($(grep -c '^ok' build/arguments_cases.out) cases)" 0 $code
# Every header satl and satl-term are compiled from is a build input, or an edit to it
# makes a different binary under the same build number (review of M0.5).
expect "every header the compiler reads is in the build fingerprint" "" "$(make -s --no-print-directory build-inputs | python3 -c "
import glob, os, sys
inputs = set(line.strip() for line in sys.stdin)
missing = set()
for d in glob.glob('build/objects/**/*.d', recursive=True):
    for word in open(d).read().split():
        if word.endswith(('.hpp', '.h')) and not word.startswith('/') and os.path.normpath(word) not in inputs:
            missing.add(os.path.normpath(word))
print(' '.join(sorted(missing)))")"
"$interpreter" tests/missing_include.satl > /dev/null 2>&1; expect "missing include" 10 $?
"$interpreter" tests/missing_main.satl > /dev/null 2>&1; expect "missing main" 11 $?
"$interpreter" tests/missing_return.satl > /dev/null 2>&1; expect "missing return" 12 $?
"$interpreter" tests/not_understood.satl > build/nu.out 2>&1; code=$?
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
# satellite.statement.if and .else (2026-09-17). The author: "satellite.statement.if
# is just (condition) { call_to_whatever_runs_code } which we have kinda just built
# the thing that runs code" -- so it is run_while without the loop, and the checks
# are while's: the condition through the same evaluator, the body through
# run_statements, sharing this body's variables.
expect "if, else, else-if, and an if inside a while" "five|not six|more than four|6|one|0|1" \
       "$("$interpreter" tests/if_else.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/if_no_body.satl > build/if.out 2>&1; expect "an if with no body is refused" 13 $?
expect "... by the CHECK, with nothing run before it" "" "$(grep -x before build/if.out)"
"$interpreter" tests/else_with_no_if.satl > build/if.out 2>&1; expect "an else with no if before it is refused" 13 $?
expect "... by the check too, and says so" 1 \
       "$(grep -c 'satellite.statement.else with no satellite.statement.if before it' build/if.out)"
# A CONDITION'S TYPE IS A RUN-TIME FACT, for an if exactly as for a while: the
# checker does not evaluate, so `if(5)` is refused where it runs, after the line
# above it has printed. Pinned here so the two never drift apart.
"$interpreter" tests/if_not_a_condition.satl > build/if.out 2>&1; expect "if(5) is refused: a number is not a condition" 27 $?
expect "... and says what it was given" 1 \
       "$(grep -c 'satellite.statement.if was given a number and needs a true or false' build/if.out)"
expect "\"some\" + \"str\" joins them" "somestr" "$("$interpreter" tests/two_strings.satl 2>/dev/null)"
# `.find(` -- period + the method's own 16-bit code + `(` (the author, 2026-09-16).
# A quoted argument and an object argument both work; an undeclared one is 25.
expect "s.find() on a literal and on an object" "6|0|6" \
       "$("$interpreter" tests/find.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/find_no_object.satl > /dev/null 2>&1; expect "s.find(no object) is refused" 25 $?
# Aliases collapse at the lexer (to_string/str/string are ONE code); conversions
# go through satellite_number as the hub; and a chain is a loop, so `s.bin.find(x)`
# is two turns of it (the author, 2026-09-16: "so we can string operations together").
wanted_chain="87|87|87|1010111|1010111|57|87|87|1010111|0|4|100|87!|87x"
expect "aliases, conversions and chained methods" "$wanted_chain" \
       "$("$interpreter" tests/chain.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
expect "a 23-digit number is held exactly" "99999999999999999999999" "$("$interpreter" tests/big_number.satl 2>/dev/null)"
# A STRING AMONG AN INCLUDE'S ARGUMENTS IS SKIPPED WHOLE on the way to its `)`. A literal of
# 515 codes has count 0x0203, which is `)`, and the scan stopped on it; the file check then
# walked the payload, where U+10901's low half is name_token, and a valid program was refused
# with 11 (the count review, 2026-09-17). The paddings reach 515 codes however many codes a
# character above U+FFFF takes, and the last string holds U+0203 itself.
mkdir -p build/include_515
echo '// a spaceship' > build/include_515/ship.satl
python3 -c "
texts = ['a' * pad + chr(0x10901) for pad in range(509, 515)] + [chr(0x203) + 'bc' + chr(0x10903)]
for n, text in enumerate(texts):
    open('build/include_515/main_%d.satl' % n, 'w', encoding='utf-8').write('''satellite.include(satellite)
satellite.include(ship(\"%s\"))

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(\"ran\")
    satellite.return(satellite)
}
''' % text)"
ran=""
for n in 0 1 2 3 4 5 6; do ran="$ran$("$interpreter" build/include_515/main_$n.satl 2>/dev/null) $?|"; done
expect "a string of 515 codes among an include's arguments" "ran 0|ran 0|ran 0|ran 0|ran 0|ran 0|ran 0|" "$ran"
# A COUNT OF EXACTLY 2314 IS 0x090A, WHICH IS long_count_token, and a literal that long
# swallowed the rest of its file: main ended in "no satellite.return", a capsule after it
# in "could not be read" (the wide-strings review, 2026-09-17). These literals are 2313,
# 2314 and 2315 codes, in main and in a capsule after it. build/count_cases proves the
# counts no program could carry -- a long count whose last chunk is 0x090A is 151 million.
python3 -c "
for codes in (2313, 2314, 2315):
    literal = 'a' * (codes - 1) + 'b'
    open('build/count_%d.satl' % codes, 'w').write('''satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.variable.string s = \"%s\"
    satellite.console.display(s.find(\"b\"))
    helper()
    satellite.return(satellite)
}

satellite.capsule helper()
{
    satellite.variable.string t = \"%s\"
    satellite.console.display(t.find(\"b\"))
}
''' % (literal, literal))"
for codes in 2313 2314 2315; do
    expect "a literal of exactly $codes codes, in main and in a capsule after it" "$((codes - 1))|$((codes - 1))" \
           "$("$interpreter" build/count_$codes.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
done
# `make` alone does not rebuild it, and an old one reports on sources it was not built from
# (the count review, 2026-09-17). make -q only asks; it builds nothing.
if [ ! -x build/count_cases ]; then expect "build/count_cases is built (make build/count_cases)" built missing
elif ! make -sq build/count_cases 2>/dev/null; then
    expect "build/count_cases is as new as its sources (make build/count_cases)" current stale
else
    build/count_cases > build/count_cases.out 2>&1; code=$?
    expect "every count put_count writes, count_at reads: $(tail -1 build/count_cases.out)" 0 $code
fi
# M0.6's terminal layer alone (satellite/prompt): keys, the editor, cells and the piped
# reader with no terminal; then the reader typed at through a real pty (pty.fork()) and
# checked on the screen a person would see. Stale harnesses are refused as above.
for harness in prompt_cases prompt_reader; do
    if [ ! -x build/$harness ]; then expect "build/$harness is built (make build/$harness)" built missing
    elif ! make -sq build/$harness 2>/dev/null; then expect "build/$harness is as new as its sources (make build/$harness)" current stale
    elif [ $harness = prompt_cases ]; then
        timeout 120 build/prompt_cases > build/prompt_cases.out 2>&1; code=$?
        expect "the prompt with no terminal: $(grep -c '^ok' build/prompt_cases.out) cases (build/prompt_cases.out)" 0 $code
    else
        timeout 600 python3 -u satellite/prompt/check_prompt.py > build/check_prompt.out 2>&1; code=$?
        expect "the prompt at a real terminal: $(grep -c '^ok' build/check_prompt.out) checks on the screen (build/check_prompt.out)" 0 $code
    fi
done

# M0.6's SESSION: `satl --repl`, satellite.directory's three words and the table.
# directory_cases checks the header all three libraries are built from, with no
# interpreter around it; check_session.py types at a real satl through a pty.
if [ ! -x build/directory_cases ]; then expect "build/directory_cases is built (make)" built missing
elif ! make -sq build/directory_cases 2>/dev/null; then
    expect "build/directory_cases is as new as its sources (make build/directory_cases)" current stale
else
    timeout 300 build/directory_cases "${TMPDIR:-/tmp}" > build/directory_cases.out 2>&1; code=$?
    expect "satellite.directory's words: $(grep -c '^ok' build/directory_cases.out) cases (build/directory_cases.out)" 0 $code
fi
SATL="$interpreter" timeout 600 python3 -u satellite/satl/check_session.py > build/check_session.out 2>&1; code=$?
expect "satl --repl at a real terminal: $(grep -c '^ok' build/check_session.out) checks (build/check_session.out)" 0 $code
# The prompt's own refusals, from a pipe: each says which spelling it refused, and the
# session goes on to the line after them.
printf 'satellite.include(satellite)\n{\nsatellite.capsule satellite.main()\nsatellite.return(satellite)\nsatellite.help(x)\nsatellite.console.display("after them all")\n' | \
    "$interpreter" --repl > build/repl_refusals.out 2>&1
expect "the five spellings and help are refused by name, and the line after them still runs" "1|1|1|1|1|1" \
       "$(grep -c 'a session has already taken satellite in' build/repl_refusals.out)|$(grep -c 'a block has nowhere to live' build/repl_refusals.out)|$(grep -c 'a capsule belongs to a program' build/repl_refusals.out)|$(grep -c 'nothing here to return from' build/repl_refusals.out)|$(grep -c 'satellite.help is not built yet' build/repl_refusals.out)|$(grep -c '^after them all$' build/repl_refusals.out)"
# A brace inside a string is text and not a block: the refusals are read from the CODES.
printf 'satellite.console.display("{ not a block }")\n' | "$interpreter" --repl 2>/dev/null | grep -q '{ not a block }'
expect "a brace inside a string literal is not a block" 0 $?

# A PAYLOAD'S CODES ARE NEVER READ AS TOKENS. A character's own number can be any 16 bits,
# so a string's last code can equal a word's or a token's (the payload sweep, 2026-09-17).
# Each case has neighbours one character either side that never collided, and a character
# above U+FFFF whose low half is the same code.
#
# U+1002 is 0x1002, satellite.include's code: load_program read "ဂ"("include_code_other"),
# in a capsule nothing calls, as an include, loaded build/include_code_other.satl and ran
# ITS main.
python3 -c "
open('build/include_code_other.satl', 'w').write('''satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(\"the wrong program\")
    satellite.return(satellite)
}
''')
for code in (0x1001, 0x1002, 0x1003, 0x11002):
    open('build/include_code_%x.satl' % code, 'w', encoding='utf-8').write('''satellite.include(satellite)

satellite.capsule never_called()
{
    satellite.console.display(\"%s\"(\"include_code_other\"))
}

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(\"mine\")
    satellite.return(satellite)
}
''' % chr(code))"
for code in 1001 1002 1003 11002; do
    output=$("$interpreter" build/include_code_$code.satl 2>/dev/null); code_run=$?
    expect "a string ending in U+$code before ( is not satellite.include" "mine|0" "$output|$code_run"
done
# U+1006 is 0x1006, satellite.capsule's code: capsules_in made the while body after
# "ဆ"satellite.main a second main, the only one checked and the one that ran ("never",
# 0); "ဆ" greet replaced the capsule the user wrote. Both lines are refused as they run.
python3 -c "
for tag, ch in (('1005', 'စ'), ('1006', 'ဆ'), ('1007', 'ဇ'), ('11006', '\U00011006')):
    open('build/capsule_word_%s.satl' % tag, 'w', encoding='utf-8').write('''satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(\"before\")
    satellite.console.display(\"%s\"satellite.main)
    satellite.statement.while(1 == 2)
    {
        satellite.console.display(\"never\")
    }
    satellite.return(satellite)
}
''' % ch)
    open('build/capsule_name_%s.satl' % tag, 'w', encoding='utf-8').write('''satellite.include(satellite)

satellite.capsule greet()
{
    satellite.console.display(\"hello\")
}

satellite.capsule satellite.main()
{
    satellite.variable.string greet = \"x\"
    greet()
    satellite.console.display(\"%s\" greet)
    satellite.statement.while(1 == 2)
    {
        satellite.console.display(\"never\")
    }
    satellite.return(satellite)
}
''' % ch)"
for tag in 1005 1006 1007 11006; do
    output=$("$interpreter" build/capsule_word_$tag.satl 2>/dev/null); code_run=$?
    expect "a word touching a string ending in U+$tag is not satellite.capsule" "before|13" "$output|$code_run"
    expect "a name touching a string ending in U+$tag does not replace a capsule" "hello" \
           "$("$interpreter" build/capsule_name_$tag.satl 2>/dev/null)"
done
# The same two scans, reached by a STRAY character outside a string, which satl accepts:
# it is error_token with the character as its payload. `ဆ satellite.main` on a line of its
# own made the next capsule main ("OTHER", 0), and `ဂ("stray_theirs")` in main loaded a file
# nothing includes, whose helper replaced the program's own (the payload sweep's fuzzing,
# 2026-09-17). é and the neighbours U+1003 and U+1007 never collided.
python3 -c "
open('build/stray_theirs.satl', 'w').write('''satellite.capsule helper()
{
    satellite.console.display(\"THEIRS\")
}
''')
for tag, ch in (('e9', 'é'), ('1003', 'ဃ'), ('1002', 'ဂ'), ('11002', '\U00011002'),
                ('1007', 'ဇ'), ('1006', 'ဆ'), ('11006', '\U00011006')):
    open('build/stray_include_%s.satl' % tag, 'w', encoding='utf-8').write('''satellite.include(satellite)

satellite.capsule satellite.main()
{
    %s(\"stray_theirs\")
    helper()
    satellite.return(satellite)
}

satellite.capsule helper()
{
    satellite.console.display(\"MINE\")
}
''' % ch)
    open('build/stray_main_%s.satl' % tag, 'w', encoding='utf-8').write('''satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(\"MAIN\")
    satellite.return(satellite)
}
%s satellite.main
satellite.capsule other()
{
    satellite.console.display(\"OTHER\")
}
''' % ch)"
for tag in e9 1003 1002 11002 1007 1006 11006; do
    output=$("$interpreter" build/stray_include_$tag.satl 2>/dev/null); code_run=$?
    expect "a stray U+$tag before (\"stray_theirs\") includes nothing" "MINE|0" "$output|$code_run"
    output=$("$interpreter" build/stray_main_$tag.satl 2>/dev/null); code_run=$?
    expect "a stray U+$tag before satellite.main does not make the next capsule main" "MAIN|0" "$output|$code_run"
done
# U+0704 is 0x0704, method_token: the lexer looked back at the last CODE, so after "܄" a
# name became a method code the check never sees -- "before" printed and the run died on
# 13. Each is refused by the check, as U+0703's is, before anything runs.
python3 -c "
for tag, literal in (('0703', '\"܃\"str'), ('0704', '\"܄\"str'), ('0704_spaced', '\"܄\" str'),
                     ('0704_find', '\"܄\"find'), ('0704_stray', '܄str'), ('10704', '\"\U00010704\"str')):
    open('build/period_in_a_payload_%s.satl' % tag, 'w', encoding='utf-8').write('''satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(\"before\")
    satellite.console.display(%s)
    satellite.return(satellite)
}
''' % literal)"
for tag in 0703 0704 0704_spaced 0704_find 0704_stray 10704; do
    output=$("$interpreter" build/period_in_a_payload_$tag.satl 2>/dev/null); code_run=$?
    expect "a name after U+$tag is still a name, refused before anything runs" "|25" "$output|$code_run"
done
# A character above U+FFFF is 40000 and two codes, in a string as in the bytecode (the author,
# 2026-09-17: "40000 is not a smile... its the 16-bit value for wide"). Positions count characters.
wanted_wide=$(python3 -c "
s = 'a\U0001F600b\u9C40c\U00019C40d'
for line in (s, s.find('b'), s.find('\u9C40'), s.find('\U00019C40'), s.find('d'), s + '\U0001F680', 'true', 'true',
             '\U0001F600a'.find('a'), '\U0001F600ba'.find('a')):
    print(line)")
expect "wide characters in a string: held, found by character, joined, compared" "$wanted_wide" \
       "$("$interpreter" tests/wide_characters.satl 2>/dev/null)"
# In the bytecode too: the program's .sate holds U+1F600 as 40000, 0x0001, 0xF600, and no
# code is the retired wide_run_32_token (the wide-strings review, 2026-09-17).
expect "a wide character in the bytecode is 40000 and two codes" "True False" \
       "$(python3 -c "
codes = open('tests/wide_characters.sate').read().split()
print('1001110001000000 0000000000000001 1111011000000000' in ' '.join(codes), '0000100100001001' in codes)")"
"$interpreter" tests/find_wide_high_half.satl > /dev/null 2>&1; expect "find never matches a wide character's high half" 15 $?
"$interpreter" tests/find_wide_low_half.satl > /dev/null 2>&1; expect "find never starts at a wide character's low half" 15 $?
# D3.1 retired wide_run_32_token: nothing writes it and nothing reads it, so nothing names it.
expect "nothing names the retired wide_run_32_token" "" \
       "$(grep -rln 'token::wide_run_32_token' satellite experiments --include='*.cpp' --include='*.hpp')"
# A string minus a string takes away the FIRST occurrence of the right one (the author,
# 2026-09-17: "minus takes away the smallest string", "first occurrence"). Python's
# str.replace(right, '', 1) is the same rule, and writes every line.
wanted_minus=$(python3 -c "
s = 'a\U0001F600b鱀c\U00019C40d\U0001F600'
for left, right in (('dfksjghjfff', 'fff'), ('abcab', 'ab'), ('banana', 'an'), ('fff', 'dfksjghjfff'), ('abc', '')):
    print(left.replace(right, '', 1))
print('[' + 'abc'.replace('abc', '', 1) + ']')
print(('a' + 'b').replace('b', '', 1))
print('x-y-z'.replace('-', '', 1))
for right in ('\U0001F600', '鱀', '\U00019C40'):
    print(s.replace(right, '', 1))
for left, right in (('\U0001F600a', 'a'), ('\U0001F600ba', 'a'), ('\U0001F600', 'a'), ('\U00029C40a鱀', '\U00019C40')):
    print(left.replace(right, '', 1))")
expect "a string minus a string takes away the first occurrence" "$wanted_minus" \
       "$("$interpreter" tests/string_minus.satl 2>/dev/null)"

# THE SIX FAST PATHS, REACHED THROUGH THEIR TOKENS. The arithmetic itself is
# proven against Python over 482,465 cases (check_numbers.py); what this proves
# is that a `+` in a program reaches it, with 003 DESIGN §6.6's precedence.
# Python is the authority for every line, and writes them here itself.
wanted_math=$(python3 -c "
for n in (34587, 2+3, 10-4, 6*7, 20//3, 20%3, 2**10, 2+3*4, 10-3-2, 2**(3**2), -3, -1,
          99999999999999999999999+1, 2**200, 0b1100+0xFF, sum(range(1000))): print(n)")
expect "the six fast paths through their tokens" "$wanted_math" "$("$interpreter" tests/arithmetic.satl 2>/dev/null)"
"$interpreter" tests/arithmetic.satl > /dev/null 2>&1; expect "tests/arithmetic.satl runs" 0 $?
"$interpreter" tests/divide_by_zero.satl > /dev/null 2>&1; expect "a divisor of zero" 22 $?
"$interpreter" tests/negative_exponent.satl > /dev/null 2>&1; expect "2 ^ -1 is not a whole number" 24 $?
"$interpreter" tests/undeclared.satl > build/un.out 2>&1; expect "a name nothing declared" 25 $?
expect "nothing ran before THAT refusal" "" "$(grep -x before build/un.out)"
# A STRAY CHARACTER AT A LINE'S START IS ONE CODE, and the check and the run agree on what
# follows it. A no-break space pasted as indentation has no code (error_token): the run
# stepped over it and ran the rest of the line, while the check skipped the whole line --
# so a declaration after it was never seen (25 for a declared n), and `undeclared = 5`
# after it ran past the check and failed with "before" on the screen (the payload sweep,
# 2026-09-17). A stray `)` split the same way.
python3 -c "
head = 'satellite.include(satellite)\n\nsatellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n{\n'
tail = '    satellite.return(satellite)\n}\n'
for name, body in (('declared', ' satellite.variable.number n = 5\n    satellite.console.display(n)\n'),
                   ('undeclared', '    satellite.console.display(\"before\")\n undeclared = 5\n'),
                   ('paren', '    satellite.console.display(\"before\")\n    ) undeclared = 5\n')):
    open('build/stray_%s.satl' % name, 'w', encoding='utf-8').write(head + body + tail)"
output=$("$interpreter" build/stray_declared.satl 2>/dev/null); code_run=$?
expect "a no-break space before a declaration does not hide it" "5|0" "$output|$code_run"
for name in undeclared paren; do
    "$interpreter" build/stray_$name.satl > build/stray.out 2>&1; code_run=$?
    expect "a stray character before an undeclared name ($name) is refused by the check, before anything runs" \
           "25|1|" "$code_run|$(grep -c 'satl(check).*undeclared has no satellite.variable line' build/stray.out)|$(grep -x before build/stray.out)"
done
"$interpreter" tests/declared_twice.satl > /dev/null 2>&1; expect "a name declared twice" 26 $?
"$interpreter" tests/kinds_do_not_meet.satl > /dev/null 2>&1; expect "\"n = \" + 4 converts nothing" 27 $?
# ERROR.md: an expression that stops early is refused, not half-stored. `&` has no
# meaning yet, and `n = 1 & 2` used to store 1 and `while(n < 3 & 1)` ran as `n < 3`.
"$interpreter" tests/unread_assignment.satl > build/unread.out 2>&1; expect "n = 1 & 2 is refused, not stored as 1" 13 $?
expect "... and nothing was displayed" "" "$(grep -x 1 build/unread.out)"
"$interpreter" tests/unread_while.satl > build/unread.out 2>&1; expect "while(n < 3 & 1) is refused, not run as n < 3" 13 $?
expect "... and the loop never counted to 3" "" "$(grep -x 3 build/unread.out)"
expect "a trailing comment still ends a value" 8 "$("$interpreter" tests/comment_after_value.satl 2>/dev/null)"
"$interpreter" tests/unread_trailing.satl > build/unread.out 2>&1; expect "n = 5 6 is refused, not stored as 5" 13 $?
"$interpreter" tests/compound_assign.satl > build/unread.out 2>&1; expect "n += 1 is refused until += is built, not skipped" 14 $?
expect "... before anything runs, and says how to write it" "1|" \
       "$(grep -c 'n += ... is not built yet -- write n = n + ...' build/unread.out)|$(grep -x before build/unread.out)"
# satellite.variable.binary (the author, 2026-09-16): written with its b, and shown
# exactly as written -- b and leading zeros -- because the width is part of the value.
expect "satellite.variable.binary my_number = b10101010" \
       "b10101010|b0010|b0000|170|b10101010|0000|AA|171|-b0101|3|false|true|3|true" \
       "$("$interpreter" tests/binary.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/binary.satl > /dev/null 2>&1; expect "tests/binary.satl runs" 0 $?
# "if the user doesn't enter "b" ... spit out an ERROR: expected "b"+whatever they entered"
"$interpreter" tests/binary_without_b.satl > build/binary_b.out 2>&1; expect "a binary with no b" 27 $?
expect "a binary with no b says ERROR: expected b10101010" 1 "$(grep -c 'ERROR: expected b10101010 ' build/binary_b.out)"
expect "nothing ran before the missing b" "" "$(grep -x before build/binary_b.out)"
"$interpreter" tests/binary_assigned_without_b.satl > build/binary_b.out 2>&1; expect "a binary given a value with no b" 27 $?
expect "... and says ERROR: expected b1111" 1 "$(grep -c 'ERROR: expected b1111 ' build/binary_b.out)"
"$interpreter" tests/binary_digits_not_binary.satl > build/binary_b.out 2>&1; expect "a binary given 12" 27 $?
expect "... is told 12 is not binary, not to write b12" 1 "$(grep -c 'ERROR: 12 is not binary' build/binary_b.out)"
"$interpreter" tests/binary_b_not_binary.satl > build/binary_b.out 2>&1; expect "a binary given b12" 27 $?
expect "... is told b12 is not binary" 1 "$(grep -c 'ERROR: b12 is not binary' build/binary_b.out)"
"$interpreter" tests/binary_in_brackets.satl > build/binary_b.out 2>&1; expect "a binary with no b inside brackets" 27 $?
expect "... is still ERROR: expected b10101010, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected b10101010 ' build/binary_b.out)|$(grep -x before build/binary_b.out)"
"$interpreter" tests/binary_0b.satl > build/binary_b.out 2>&1; expect "0b10101010, the C spelling" 27 $?
expect "... is told ERROR: expected b10101010, not b0" 1 "$(grep -c 'ERROR: expected b10101010 ' build/binary_b.out)"
# A binary keeps a sign (the author, 2026-09-17: "give it a different number and keep a
# sign with all of these things"): -b0101 is a binary, worth -5, width kept.
expect "a binary below zero: shown, worth, converted, compared, turned over, given" \
       "-b0101|-5|-0101|-b0101|-5|-4|true|false|true|b0101|b0101|b0000|-b0011|-3" \
       "$("$interpreter" tests/binary_negative.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/binary_negative_without_b.satl > build/binary_b.out 2>&1; expect "a binary given -1010" 27 $?
expect "... says ERROR: expected -b1010, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -b1010 ' build/binary_b.out)|$(grep -x before build/binary_b.out)"
"$interpreter" tests/binary_negative_assigned_without_b.satl > build/binary_b.out 2>&1; expect "a binary given (- 1111)" 27 $?
expect "... says ERROR: expected -b1111, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -b1111 ' build/binary_b.out)|$(grep -x before build/binary_b.out)"
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
       "$("$interpreter" tests/percentage.satl 2>/dev/null)"
"$interpreter" tests/percentage.satl > /dev/null 2>&1; expect "tests/percentage.satl runs" 0 $?
"$interpreter" tests/percentage_without_percent.satl > build/percentage.out 2>&1; expect "a percentage with no %" 27 $?
expect "... says ERROR: expected 50%, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected 50% ' build/percentage.out)|$(grep -x before build/percentage.out)"
"$interpreter" tests/percentage_not_whole.satl > build/percentage.out 2>&1; expect "3 * 50% is not whole" 24 $?
expect "... and says so" 1 "$(grep -c '3 \* 50% is not a whole number' build/percentage.out)"
"$interpreter" tests/percentage_number_second.satl > build/percentage.out 2>&1; expect "50% + 5 is refused" 27 $?
expect "... and says to put the number first" 1 "$(grep -c 'the number goes first -- 5 + 50%' build/percentage.out)"
expect "a percentage below zero: -50%, (- 25%) and -(-12.5%)" "-50%|-25%|12.5%" \
       "$("$interpreter" tests/percentage_negative.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/percentage_negative_without_percent.satl > build/percentage.out 2>&1; expect "a percentage given -50" 27 $?
expect "... says ERROR: expected -50%, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -50% ' build/percentage.out)|$(grep -x before build/percentage.out)"
"$interpreter" tests/percentage_negative_assigned_without_percent.satl > build/percentage.out 2>&1
expect "a percentage given (- 25)" 27 $?
expect "... says ERROR: expected -25%, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -25% ' build/percentage.out)|$(grep -x before build/percentage.out)"
expect "a declaration inside a loop runs every turn" "0|1|10|11|20|21" \
       "$("$interpreter" tests/loop_declaration.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" examples/hello_world.satl > /dev/full 2> /dev/null; expect "output refused (/dev/full)" 2 $?

mkdir -p build/alone && cp "$interpreter" build/alone/satl
build/alone/satl examples/hello_world.satl > /dev/null 2>&1; expect "no libraries beside the interpreter" 5 $?
# A satl deeper than the kernel can name (ERROR #8): refused with 5 and the reason, and
# never the libraries of the folder it was started in -- this folder holds a working set.
rm -rf build/deep && mkdir -p build/deep
deep_result=$(top=$PWD; libraries=$(dirname "$interpreter")/satellite-numbers; name=$(printf 'd%.0s' $(seq 100))
    cd build/deep && for i in $(seq 45); do mkdir "$name" && cd "$name" || exit; done
    cp "$interpreter" satl && cp -r "$libraries" satellite-numbers && cp "$top/examples/hello_world.satl" .
    timeout 30 ./satl hello_world.satl > run.out 2>&1; echo "$?|$(grep -c 'satl cannot read its own path (/proc/self/exe: File name too long)' run.out)")
expect "a satl deeper than 4,096 bytes refuses, and loads nothing from the current folder" "5|1" "$deep_result"
rm -rf build/deep

"$interpreter" --debug examples/hello_world.satl > build/debug.out 2>&1; code=$?
expect "--debug runs" 0 $code
expect "--debug shows the index being defined" 1 "$(grep -c 'vector.number.index(defined) (machine_code: 6 number_vector_defined)' build/debug.out)"
expect "--debug shows memory as a size" 1 "$(grep -cE '^\[satellite\] arguments.memory.total = [0-9.]+ (kilo|mega|giga|tera)?bytes' build/debug.out)"
expect "--debug shows arguments.threads_startup from the config" 1 "$(grep -c "^\[satellite\] arguments.threads_startup = $(config_row threads_startup) " build/debug.out)"
# Every config number is a satellite_number (the author, 2026-09-16), and infinity's
# two digit counts are rows: 4096 held, 32 shown, "both digits configurable".
expect "--debug shows arguments.infinity from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity = $(config_row infinity) " build/debug.out)"
expect "--debug shows arguments.infinity_display from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity_display = $(config_row infinity_display) " build/debug.out)"
# A negative row must reach C++ negative (the review of b68d1a7, 2026-09-17): -1ULL and
# -4u are unsigned there and -9223372036854775808 has no literal, so the reader the
# build uses refuses all three before anything compiles. -1 and the quoted row still read.
row_read() { python3 -c "
import sys; sys.path.insert(0, 'satellite/config'); import build_number
try: print(build_number.live_rows('arguments_vector.push_back({\"arguments.magic\", %s, false, false});' % sys.argv[1])[0]['number'])
except SystemExit as refusal: print('refused:', str(refusal).split('; ')[-1])" "$1" 2>/dev/null; }
expect "a config row written -1ULL is refused" "refused: write it without the u" "$(row_read -1ULL)"
expect "a config row written -4u is refused" "refused: write it without the u" "$(row_read -4u)"
expect "a config row written -9223372036854775808 is refused" 'refused: write it in quotes: "-9223372036854775808"' \
       "$(row_read -9223372036854775808)"
expect "a config row written -1, -9223372036854775807 or quoted still reads" "-1|-9223372036854775807|-9223372036854775808" \
       "$(row_read -1)|$(row_read -9223372036854775807)|$(row_read '"-9223372036854775808"')"
# words_004.tsv is typed by hand, so make_words.py refuses a row it cannot trust --
# checked through the real script and 003's real satl, which is gitignored.
python3 words/check_make_words.py > build/check_make_words.out 2>&1; code=$?
if [ $code = 2 ]; then echo "  skip  make_words.py's checks: no 003 satl at old_versions/second_satellite/satl"
else expect "make_words.py against rows typed by hand: $(tail -1 build/check_make_words.out) (build/check_make_words.out)" 0 $code; fi
expect "the start-up threads are warm" 1 "$(grep -cE "^\[satellite\] threads.startup\(warm\): $(config_row threads_startup) threads parked in [0-9.]+ ms" build/debug.out)"

echo "$passed passed, $failed failed"
[ "$failed" = 0 ]
