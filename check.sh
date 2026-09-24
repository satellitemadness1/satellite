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

# A HOME OF ITS OWN, SO THE SUITE DOES NOT READ THE PERSON'S SETTINGS.
#
# satl reads $HOME/.satl/config.ini at start-up (satellite/config/config_file.hpp)
# and everything in it changes what a run does -- a feature turned on prints a
# profile, a missing file prints S010, a stale register prints S011. All three
# land on stderr, and this suite counts stderr lines.
#
# FOUND BY BREAKING IT, 2026-09-18. `word_counts = true` was left on in the
# author's own config.ini and "the start-up block is five lines" failed with 11 --
# the six extra being the per-word profile. The suite was reading the developer's
# machine, so it could pass here and fail there, or fail for a reason that had
# nothing to do with the change being tested.
#
# --rebuild MAKES THE FILE, rather than this script writing one. The register's
# width grows every time a feature is added, so a config.ini written here as fixed
# text would go stale and raise the very S011 this is avoiding; the binary is the
# thing that knows how wide its register is. It also runs BEFORE the missing-file
# notice by design, so this first call is quiet on a home with nothing in it.
CHECK_HOME=$PWD/build/check-home
rm -rf -- "$CHECK_HOME"
mkdir -p -- "$CHECK_HOME/.satl"
export HOME=$CHECK_HOME

# AND NO WINDOW OF satl'S OWN, FOR THE WHOLE SUITE (WIN-9, ruled 2026-09-22):
# satl opens its own console when it has no terminal, its output is going
# nowhere anybody reads, and there is a display. A harness with no controlling
# terminal -- CI, cron, an editor's shell -- that sends a row's stdout to
# /dev/null on a desktop is exactly that, and would put a window on the
# screen of whoever ran the suite and wait for them. The rows that test the rule
# unset this on purpose, with no display to reach.
export SATL_NO_WINDOW=1
"$interpreter" --rebuild > build/check-home-rebuild.out 2>&1 ||
    echo "  note  --rebuild could not write $CHECK_HOME/.satl/config.ini; the suite may see S010"

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
# --version IS THE THREE TITLE LINES AND THEN THE LICENCE BLOCK, since 2026-09-20.
# `head -3` still gives exactly what it always gave -- the two checks above still
# assert that -- so anything parsing the version is unaffected. What follows is
# eight lines naming the licence families, pointing at satl --license, and carrying
# the one credit sentence the FreeType Licence makes MANDATORY (FTL.TXT section 3:
# "This credit MUST appear in the documentation and/or other materials"). That is an
# obligation, not decoration, which is why it is in the binary and asserted here.
expect "--version is the title lines and the licence block" 11 "$("$interpreter" --version | wc -l)"
expect "--version carries the mandatory FreeType credit" 1 "$("$interpreter" --version | grep -c 'The FreeType Project')"
expect "--version still gives the three title lines to head -3" 3 "$("$interpreter" --version | head -3 | wc -l)"

# --version POINTS AT THE COMMAND, NOT AT A FOLDER, since 2026-09-21. It used to end
# "Full texts: licenses/ in the satellite distribution, one folder per project" --
# written before `satl --license` existed and stale from the moment it did. It sent
# somebody who is HOLDING every text off to look for a directory that a shipped
# binary does not come with.
expect "--version names the command that shows the texts" 1 "$("$interpreter" --version | grep -c 'satl --license')"

# AND THE TWO COUNTS AGREE. --version said "24 other projects" while --license all
# said 26, because one was typed and the other is licence_rows().size(). Only the
# generated one could be right, and it was not the one a person reads first.
# licence_lines() now takes the count, so this row is what keeps them together.
carried=$("$interpreter" --license all | sed -n '1s/.*binary, \([0-9]*\) of them.*/\1/p')
named=$("$interpreter" --version | sed -n 's/.*carries \([0-9]*\) other projects.*/\1/p')
expect "--version's count is every licence less satellite's own" "$carried" "$((named + 1))"

# EVERY SPELLING ANSWERS. --licenses was refused until 2026-09-21 -- "is not a word
# satl takes" -- over one letter, for a command that shows 26 of them. NOT > /dev/null:
# writing to it is what triggers the console handover, so a /dev/null sweep can report
# a pass that never ran.
for licence_word in --license --licence --licenses --licences; do
    "$interpreter" "$licence_word" all > build/licence.out 2> build/licence.err
    expect "$licence_word is accepted" 0 $?
done
expect "every spelling gives the same text" 1 "$(grep -c 'Every licence in this binary' build/licence.out)"
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
# THE SHAPE CHANGED 2026-09-20 AND THE PROPERTY DID NOT. main used to be one
# line, `return exit_status_of(run_satl(...))`; the window needs the code before
# main returns, so that run is now named and then passed. The third field is the
# one that keeps this honest: run_satl's value must never be returned DIRECTLY,
# which is the only way a code could become a status without passing through.
expect "... and satl's main returns through exit_status_of, the one place a code becomes a status" "1|1|0" \
       "$(grep -cF 'const signed long long int code = run_satl(argc, argv);' satellite/structured-library.cpp)|$(grep -cF 'return satellite004::exit_status_of(code);' satellite/structured-library.cpp)|$(grep -cE 'return [^;]*run_satl' satellite/structured-library.cpp)"
# A config row may never hold a name satl fills in, however many words a run has.
build/arguments_cases > build/arguments_cases.out 2>&1; code=$?
expect "names satl fills in are refused as rows, and gather adds no other ($(grep -c '^ok' build/arguments_cases.out) cases)" 0 $code
# Every header satl is compiled from is a build input, or an edit to it
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
# THE INFINITY FAMILY'S LETTERED LEVELS ARE NOT BUILT (SATELLITE_INFINITY.md, INF-6).
# These two held satellite.variable.infinity and satellite.infinity() until INF-2 built
# them, and were rebased then, as their comments say -- red on purpose again at INF-6.
"$interpreter" tests/infinity_not_built.satl > build/inf.out 2>&1; code=$?
expect "satellite.variable.aasat, a lettered level, is not built yet" 13 $code
expect "... refused before it runs: satellite.variable is not a call" 1 \
       "$(tr '\n' ' ' < build/inf.out | grep -c 'satellite.variable is not a call')"
expect "... with nothing run before it" "" "$(grep -x before build/inf.out)"
"$interpreter" tests/infinity_constructor_not_built.satl > build/inf.out 2>&1; code=$?
expect "satellite.aasat(), a lettered level's constructor, is not built yet" 13 $code
expect "... refused before it runs: no capsule named aasat" 1 "$(tr '\n' ' ' < build/inf.out | grep -c 'no capsule named aasat')"
expect "... with nothing run before it" "" "$(grep -x before build/inf.out)"
# satellite.variable.infinity, INF-2 (SATELLITE_INFINITY.md): arm 12, satellite.infinity(),
# the display and the order, through a program. check_infinity.py, further down, holds
# the same display and order to infinity_oracle.py over every value the tables print.
expect "INF-2: an infinity displays, orders, copies, sorts, and a family name takes a number" \
       "(infinity)|(infinity)|(-infinity)|true|true|true|true|true|true|true|true|true|5|(infinity)|{(infinity), 5, (-infinity)}|(infinity)|{(-infinity), 0, 5, (infinity)}|true|3" \
       "$("$interpreter" tests/infinity.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/infinity.satl > /dev/null 2>&1; expect "tests/infinity.satl runs" 0 $?
# EVERY REFUSAL INF-2 OWES IS BY NAME -- its done-when, and the milestones that build
# the rest. Answers the code, whether the reason was said, and how many "before" lines
# ran first: 0 for a refusal the checker makes, 1 for one made while running.
infinity_says() {
    printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n%s\n}\n\nsatellite.return(satellite)\n' "$1" > build/infinity_probe.satl
    "$interpreter" build/infinity_probe.satl > build/infinity_probe.out 2>&1; code=$?
    printf '%s|%s|%s' "$code" "$(tr '\n' ' ' < build/infinity_probe.out | grep -c -- "$2")" "$(grep -cx before build/infinity_probe.out)"
}
expect "a satellite.variable.number name refuses an infinity" "27|1|1" \
       "$(infinity_says '    satellite.variable.number n = satellite.infinity()' 'n was declared satellite.variable.number, and it holds an infinity')"
expect "an infinity name refuses a string" "27|1|1" \
       "$(infinity_says '    satellite.variable.infinity x = "text"' 'x was declared satellite.variable.infinity, and it holds a string')"
expect "an infinity name refuses a binary: besides its family it takes a plain number (Q24)" "27|1|1" \
       "$(infinity_says '    satellite.variable.infinity x = b1010' 'and it holds a binary')"
expect ".reverse() on an infinity is refused by name" "27|1|1" \
       "$(infinity_says '    satellite.variable.infinity x = satellite.infinity()
    satellite.console.display(x.reverse())' 'x.reverse() was written on an infinity, and an infinity has no digits to turn round')"
expect ".number on an infinity is refused by name" "27|1|1" \
       "$(infinity_says '    satellite.console.display(satellite.infinity().number)' 'an infinity is larger than every number -- there is no number to make of it')"
expect "satellite.infinity(x) is INF-5's, refused before anything runs" "14|1|0" \
       "$(infinity_says '    satellite.console.display(satellite.infinity(2))' 'satellite.infinity(x) -- infinity to the power of x -- is not built yet')"
for pair in power_of:INF-4 to_the_power_of:INF-4 power:INF-4 nines:INF-7 resize:INF-7; do
    written=${pair%%:*}; builds=${pair##*:}
    expect "x.$written(...) on an infinity names $builds, before anything runs" "14|1|0" \
           "$(infinity_says "    satellite.variable.infinity x = satellite.infinity()
    satellite.console.display(x.$written(2))" "SATELLITE_INFINITY.md $builds")"
done
expect "+ on an infinity names INF-3" "14|1|1" \
       "$(infinity_says '    satellite.console.display(satellite.infinity() + 1)' "an infinity's + is not built yet (SATELLITE_INFINITY.md, INF-3)")"
expect "* by a percentage names INF-3 and INF-4, not the percentage's pair" "14|1|1" \
       "$(infinity_says '    satellite.console.display(satellite.infinity() * 50%)' "an infinity's \\* is not built yet (SATELLITE_INFINITY.md, INF-3 and INF-4)")"
expect "** on two infinities names INF-4 and INF-5" "14|1|1" \
       "$(infinity_says '    satellite.console.display(satellite.infinity() ** satellite.infinity())' "(SATELLITE_INFINITY.md, INF-4 and INF-5)")"
# THE INFINITY'S THREE METHOD TOKENS (INF-1): every spelling lexes to its token and is
# refused BY NAME on a type that does not have it -- the registry's own name, from the
# generated method_name_of(), where a hand-kept table used to say "that method".
for pair in power_of:power_of to_the_power_of:power_of power:power_of nines:nines resize:resize; do
    written=${pair%%:*}; named=${pair##*:}
    printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n    satellite.variable.number n = 5\n    satellite.console.display(n.%s(1))\n}\n\nsatellite.return(satellite)\n' "$written" > build/method_probe.satl
    "$interpreter" build/method_probe.satl > build/method_probe.out 2>&1; code=$?
    expect "n.$written(1) on a number is not built yet" 14 $code
    expect "... and is named n.$named, which no type has yet" 1 \
           "$(tr '\n' ' ' < build/method_probe.out | grep -c "n.$named is not built for satellite.variable.number yet -- so far no type has it")"
done
# ...and a container's method on a number names the container, not "no type".
printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n    satellite.variable.number n = 5\n    satellite.console.display(n.sort())\n}\n\nsatellite.return(satellite)\n' > build/method_probe.satl
"$interpreter" build/method_probe.satl > build/method_probe.out 2>&1
expect "n.sort() on a number: so far it is a container's" 1 \
       "$(tr '\n' ' ' < build/method_probe.out | grep -c "n.sort is not built for satellite.variable.number yet -- so far it is a container's")"
# A TOUCHING ** OUTSIDE A for IS NAMED TOO, with the caret on it: the generic "a space
# on both sides" answer would send a person to write 2 * * 3.
printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display(2**3)\n}\n\nsatellite.return(satellite)\n' > build/method_probe.satl
"$interpreter" build/method_probe.satl > build/method_probe.out 2>&1; code=$?
expect "2**3 is refused" 13 $code
expect "... by name: power is written with a space on both sides" 1 \
       "$(tr '\n' ' ' < build/method_probe.out | grep -c 'power is written with a space on both sides -- 2 \*\* 3 or 2 ^ 3')"
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
# FLATTENED FIRST: the refusal is a full report now and a report wraps at eighty
# columns, so this sentence arrives split across two lines. The assertion is
# about the words, not where the report chose to break them.
# satellite.feedback CANNOT BE USED TO FLOOD ANYBODY, and the proof is run rather
# than argued: 50,000 identical calls in a loop must cost one line.
#
# The author asked for "a system that you cannot BOMB with
# satellite.feedback({"something_in_a_loop"})". The answer is that the word never
# touches a network -- so the last row here asserts that satl links NOTHING that
# could reach one, which is the property the whole design rests on.
cat > build/feedback_bomb.satl <<'BOMB_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.feedback("a real thing a person typed")
    satellite.variable.number i = 0
    satellite.statement.while(i < 50000)
    {
        satellite.feedback("BOMB")
        i = i + 1
    }
    satellite.console.display("done")
}
satellite.return(satellite)
BOMB_EOF
rm -f "$CHECK_HOME/.satl/feedback.txt"
HOME="$CHECK_HOME" "$interpreter" build/feedback_bomb.satl > build/feedback.out 2>&1
expect "50,002 feedback calls run, and the loop is not refused" "0|done" \
       "$?|$(tail -1 build/feedback.out)"
expect "... and 50,000 identical ones cost ONE line" 2 \
       "$(wc -l < "$CHECK_HOME/.satl/feedback.txt" 2>/dev/null || echo 0)"
expect "... the book holds what was typed and nothing about the machine" "1|0|0" \
       "$(grep -c 'a real thing a person typed' "$CHECK_HOME/.satl/feedback.txt")|$(grep -c "$(whoami)" "$CHECK_HOME/.satl/feedback.txt")|$(grep -c 'feedback_bomb.satl' "$CHECK_HOME/.satl/feedback.txt")"
# WHOLE SYMBOL NAMES, NOT SUBSTRINGS (2026-09-20). This row used to grep for
# `connect` anywhere in nm's output, and the moment satl linked GTK it matched
# `g_signal_connect_data` -- GLib connecting a SIGNAL, which reaches no network
# at all. A guard that cries wolf is a guard somebody turns off, so it now asks
# for the symbol itself and still catches every real way in: a raw socket, a
# resolver, OpenSSL, libcurl, and gio's own network classes.
expect "satl imports no network entry point of its own" 0 \
       "$(nm -D "$interpreter" 2>/dev/null | awk '{print $NF}' | grep -cE '^(socket|socketpair|connect|bind|listen|accept|accept4|send|sendto|recv|recvfrom|getaddrinfo|gethostbyname)$|^(SSL_|curl_|g_socket|g_network|g_resolver|g_inet|g_tls|g_proxy)')"
# AND THE THING GTK COST, SAID OUT LOUD RATHER THAN LOST. Since satl links GTK
# it also links libgio, and gio CAN open a socket -- so "satl links nothing that
# can reach a network" stopped being true on 2026-09-20 and this is what replaced
# it: satl calls none of it. The row above is the proof; this one names the
# library so nobody reads the row above as the old, stronger claim.
# Written to pass with or without GTK: on a machine with no gtk4 there is no
# libgio either, and the count is 0.
expect "... and libgio, which CAN reach one, is linked by GTK and never called" "yes" \
       "$(gio_linked=$(readelf -d "$interpreter" 2>/dev/null | grep -c 'libgio-2.0'); gio_called=$(nm -D "$interpreter" 2>/dev/null | awk '{print $NF}' | grep -cE '^g_(socket|network|resolver|inet|tls|proxy)'); if [ "$gio_called" = 0 ] && [ "$gio_linked" -le 1 ]; then echo yes; else echo "linked=$gio_linked called=$gio_called"; fi)"
# THE RUN-TIME FLOOR, WRITTEN DOWN AS A NUMBER (GTK_AND_NO_DEPENDENCIES.md DEP-3
# and DEP-4, 2026-09-22). satl and every word library beside it ask the machine's
# libc and libstdc++ for these symbol versions and nothing newer: GLIBC_2.38 (the
# __isoc23_strtol family, strlcat, fmod -- clang 24 against glibc 2.39's headers)
# and GLIBCXX_3.4.32 (gcc 13's std::ios_base_library_init, one symbol). AlmaLinux
# 10 ships 2.39 and 3.4.33 and runs this binary on its own /lib64, proved in an
# empty root by prove-bare-machine.sh; AlmaLinux 9 (2.34, 3.4.29) cannot. The day
# a change moves either number, in either direction, this row fails and somebody
# decides whether the floor moves -- it must not move quietly.
expect "the run-time floor is glibc 2.38 and GLIBCXX 3.4.32, satl and its words alike (DEP-4: EL10, not EL9)" "GLIBC_2.38|GLIBCXX_3.4.32" \
       "$(objdump -T "$interpreter" "$(dirname "$interpreter")"/satellite-numbers/*.so 2>/dev/null | grep -oE 'GLIBC_2\.[0-9]+' | sort -V | tail -1)|$(objdump -T "$interpreter" "$(dirname "$interpreter")"/satellite-numbers/*.so 2>/dev/null | grep -oE 'GLIBCXX_3\.4\.[0-9]+' | sort -V | tail -1)"

# THE BRACED LIST -- `{a, b}` WHERE A VALUE BELONGS (the author, 2026-09-18: "we
# need to build satellite object definitions to be this: = {series_of_objects,
# another_object}").
#
# THE FIRST ROW IS THE ONE THAT MATTERS AND IT IS NOT THE OBVIOUS ONE: a list
# given to satellite.feedback must become TWO ENTRIES, not one entry reading
# {"a", "b"}. It did exactly that during the build -- the word has two spellings
# (`1 25` and `1 25 1`, because the lexer matches the longest plain path), the
# list scenario was added to one of them, and the other silently went on taking
# the text path. Nothing errored; the only way to see it was to read the file.
# That is why both bodies now live in feedback_book.hpp and why this asserts on
# the STORE rather than on the exit status.
cat > build/braced_list.satl <<'LIST_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.feedback({"the caret is great", "the prompt eats my tabs"})
    satellite.console.display({"one", "two"})
    satellite.console.display({1, 2, 3})
    satellite.console.display({})
    satellite.console.display({1, "two", {3, 4}})
    satellite.console.display({1 + 1, 2 * 3})
    satellite.console.display({1, 2,})
    satellite.return(satellite)
}
LIST_EOF
rm -f "$CHECK_HOME/.satl/feedback.txt"
HOME="$CHECK_HOME" "$interpreter" build/braced_list.satl > build/braced_list.out 2>&1
expect "a braced list runs" 0 $?
expect "a list of two reaches feedback as TWO entries, not one line of {a, b}" "2|0" \
       "$(wc -l < "$CHECK_HOME/.satl/feedback.txt")|$(grep -c '{' "$CHECK_HOME/.satl/feedback.txt")"
expect "a list prints as what was typed, nested and empty and all" \
       '{"one", "two"}|{1, 2, 3}|{}|{1, "two", {3, 4}}|{2, 6}|{1, 2}' \
       "$(tail -6 build/braced_list.out | tr '\n' '|' | sed 's/|$//')"

# A BLOCK'S BRACE IS UNTOUCHED. This is the row that would catch the whole idea
# being wrong: `{` was the block opener long before it was a list, and if the two
# ever collide it is here that it shows.
cat > build/braced_block.satl <<'BLOCK_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.number n = 3
    satellite.statement.if(n > 2)
    {
        satellite.console.display({"a list inside a block", "still a list"})
    }
    satellite.return(satellite)
}
BLOCK_EOF
HOME="$CHECK_HOME" "$interpreter" build/braced_block.satl > build/braced_block.out 2>&1
expect "a block's brace and a list's brace do not collide" '0|{"a list inside a block", "still a list"}' \
       "$?|$(tail -1 build/braced_block.out)"

# THE REFUSALS. An unclosed list names the `{` that opened it, and a list handed
# to a name of another type is the ordinary type refusal, reading "a list".
cat > build/braced_bad.satl <<'BAD_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display({1, 2)
    satellite.return(satellite)
}
BAD_EOF
HOME="$CHECK_HOME" "$interpreter" build/braced_bad.satl > build/braced_bad.out 2>&1
expect "an unclosed { says so" "13|1" \
       "$?|$(grep -c 'opened with { and never closed' build/braced_bad.out)"

cat > build/braced_type.satl <<'TYPE_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.string s = {"a", "b"}
    satellite.return(satellite)
}
TYPE_EOF
HOME="$CHECK_HOME" "$interpreter" build/braced_type.satl > build/braced_type.out 2>&1
expect "a list given to a string name is refused, and is CALLED a list" "27|1" \
       "$?|$(grep -c 'it holds a list' build/braced_type.out)"

# A LIST IN A LOOP STILL CANNOT FLOOD ANYBODY -- the author's own worry, written
# with the shape he actually wrote it in: satellite.feedback({...}) in a loop.
cat > build/braced_bomb.satl <<'LBOMB_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.statement.for(satellite.variable.number i = 0; i < 25000; i++)
    {
        satellite.feedback({"something_in_a_loop", "and another thing"})
    }
    satellite.console.display("done")
    satellite.return(satellite)
}
LBOMB_EOF
rm -f "$CHECK_HOME/.satl/feedback.txt"
HOME="$CHECK_HOME" "$interpreter" build/braced_bomb.satl > build/braced_bomb.out 2>&1
expect "50,000 feedback calls through a LIST cost two lines" "0|done|2" \
       "$?|$(tail -1 build/braced_bomb.out)|$(wc -l < "$CHECK_HOME/.satl/feedback.txt")"

# `a[n]` AND `a[n] = v`, INCLUDING LISTS INSIDE LISTS (the author, 2026-09-18:
# "we must build it to be able to access lists inside of lists").
#
# COUNTING FROM 1, because a file's lines already do and one bracket cannot have
# two rules.
cat > build/list_index.satl <<'IDX_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list names = {"one", "two", "three"}
    satellite.console.display(names[1])
    names[2] = "CHANGED"
    satellite.console.display(names)

    satellite.container.list grid = {{1, 2}, {3, 4}}
    satellite.console.display(grid[2][1])
    grid[2][1] = 99
    satellite.console.display(grid)

    satellite.variable.number k = 1
    names[k + 1] = "computed"
    satellite.container.list p = {2, 1}
    satellite.console.display(names[p[1]])
    satellite.return(satellite)
}
IDX_EOF
HOME="$CHECK_HOME" "$interpreter" build/list_index.satl > build/list_index.out 2>&1
expect "a[n], a[n] = v, a[i][j] = v, and an index that is itself an index" \
       'one|{"one", "CHANGED", "three"}|3|{{1, 2}, {99, 4}}|computed' \
       "$(tail -5 build/list_index.out | tr '\n' '|' | sed 's/|$//')"

# `b = a` COPIES. A LIST IS A VALUE, not a reference -- 003 §12's ruling, and the
# answer to red note 8, which `a[n] = v` is what made askable.
cat > build/list_copy.satl <<'COPY_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list a = {"first", "second"}
    satellite.container.list b = a
    b[1] = "b changed this"
    satellite.console.display(a[1])
    satellite.console.display(b[1])
    satellite.return(satellite)
}
COPY_EOF
HOME="$CHECK_HOME" "$interpreter" build/list_copy.satl > build/list_copy.out 2>&1
expect "b = a COPIES a list: writing through b leaves a alone" 'first|b changed this' \
       "$(tail -2 build/list_copy.out | tr '\n' '|' | sed 's/|$//')"

# COPY-ON-WRITE IS ACTUALLY FIRING, AND THIS IS THE ROW THAT CANNOT BE REPLACED
# BY READING THE CODE.
#
# 003 wrote this same fast path and it was DEAD for months: `use_count() == 1`
# was never true, because a copy of the handle was always live, so every write
# quietly copied the whole list and only a benchmark ever said so (`19526c9`).
# Nothing about the OUTPUT differs between the two -- a copying implementation is
# perfectly correct and perfectly quadratic.
#
# So this measures SCALING rather than seconds: the same 50,000 writes into a
# 1,000-item list and an 8,000-item list. In place, both take the same time. If
# the write ever starts copying again, the second is eight times the first, and
# this row fails on a machine of any speed under any load.
python3 - <<'GEN_EOF'
for n in (1000, 8000):
    items = ", ".join(str(i) for i in range(1, n + 1))
    open(f"build/list_cow_{n}.satl", "w").write(f'''satellite.include(satellite)
satellite.capsule satellite.main()
{{
    satellite.container.list big = {{{items}}}
    satellite.statement.for(satellite.variable.number i = 0; i < 50000; i++)
    {{
        big[{n // 2}] = i
    }}
    satellite.console.display(big[{n // 2}])
    satellite.return(satellite)
}}
''')
GEN_EOF
cow_small=$( { TIMEFORMAT=%R; time HOME="$CHECK_HOME" "$interpreter" build/list_cow_1000.satl > /dev/null 2>&1; } 2>&1 )
cow_big=$(   { TIMEFORMAT=%R; time HOME="$CHECK_HOME" "$interpreter" build/list_cow_8000.satl > /dev/null 2>&1; } 2>&1 )
# Four times, against a list eight times bigger, is still far under what copying
# costs and far over the noise of a loaded machine.
expect "a write into a list does not copy it (8x the items, not 8x the time: ${cow_small}s vs ${cow_big}s)" \
       "in place" \
       "$(awk -v s="$cow_small" -v b="$cow_big" 'BEGIN { print (b < s * 4 + 0.05) ? "in place" : "COPYING: " b "s vs " s "s" }')"

# THE REFUSALS. Each one is a sentence a person can act on, and the index rules
# are the file's: count from 1, and say how many there are.
list_says() {
    cat > build/list_bad.satl <<BAD_EOF
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list a = {"one", "two"}
$1
    satellite.return(satellite)
}
BAD_EOF
    HOME="$CHECK_HOME" "$interpreter" build/list_bad.satl > build/list_bad.out 2>&1
    # THE REPORT WRAPS AT 80 COLUMNS, so a sentence is matched with the
    # newlines flattened -- otherwise a message that is right fails a test
    # because of where it happened to break.
    printf '%s|%s' "$?" "$(tr '\n' ' ' < build/list_bad.out | grep -c "$2")"
}
expect "reading past the end names the size and says counting from 1" "47|1" \
       "$(list_says '    satellite.console.display(a[5])' 'the list holds 2 items, counting from 1')"
expect "writing past the end says the list does not grow" "47|1" \
       "$(list_says '    a[5] = "x"' 'Writing past the end does not make the list longer')"
expect "item 0 is past the end, because items count from 1" "47|1" \
       "$(list_says '    satellite.console.display(a[0])' 'counting from 1')"
expect "a negative index says items count from 1" "19|1" \
       "$(list_says '    satellite.console.display(a[-1])' 'items count from 1')"
expect "an index that is not a number says so" "27|1" \
       "$(list_says '    satellite.console.display(a["two"])' 'takes an item number, and was given a string')"
expect "[ ] on something with no items says what it is" "27|1" \
       "$(list_says '    satellite.variable.number n = 4
    satellite.console.display(n[1])' 'is a number, and \[ \] reads a line of a file')"
expect "a list given to a number name is refused" "27|1" \
       "$(list_says '    satellite.variable.number bad = {1, 2}' 'it holds a list')"

# WHAT 004 TOOK FROM 003 FOR THE LIST, 2026-09-23: the constructor
# satellite.container.list() -- 473 lines of the author's programs are written with it,
# many as a spacesuit's field -- and .sum .max .min .join(separator) .reserve(n), which
# 003 numbered and gave meanings at M16. The program walks all of it, and the
# author's my_list[x][y] on a list of lists the constructor made.
"$interpreter" tests/list_from_003.satl > build/list_from_003.out 2>&1; code=$?
expect "list(): in a capsule and as a spacesuit field; grid[x][y]; sum max min join reserve; truncate past the end keeps all" \
       "0|{{7}, {8, 9}}|8|16|25|6|3|1|zoo|apple|0|2.0|1.0|{1.0, 1, 2, 2.0}|home/madness/code|3, 1, 2|0|{5}|{3, 1, 2}|42|zoe,al|1 dna, grid[2][1] = 30, total 70" \
       "$code|$(grep -v -e '^THE SATELLITE' -e '^VERSION' -e '^CLANG' -e '^G++' -e '^---' -e '^$' build/list_from_003.out | tr '\n' '|' | sed 's/|$//')"
expect "satellite.container.list(1, 2) is refused before anything runs: a list that holds something is braces" "13|1" \
       "$(list_says '    satellite.container.list b = satellite.container.list(1, 2)' 'satl(check): in satellite.main, satellite.container.list() makes a list of nothing and takes nothing')"
expect "the constructor's list given to a number name is refused" "27|1" \
       "$(list_says '    satellite.variable.number n = satellite.container.list()' 'it holds a list')"
expect ".sum adds numbers, and points a list of strings at .join" "27|1" \
       "$(list_says '    satellite.console.display(a.sum)' 'a.sum adds numbers, and item 1 is a string -- .join(separator) makes one string of them')"
expect ".max of a number and a string says they have no order between them" "27|1" \
       "$(list_says '    satellite.container.list m = {1, "b"}
    satellite.console.display(m.max)' 'max finds the largest by what each item is worth, and item 1 and item 2 have no order between them')"
expect ".min of an empty list has nothing to answer" "47|1" \
       "$(list_says '    satellite.container.list e = {}
    satellite.console.display(e.min)' 'the list is empty, so nothing in it is the smallest')"
expect ".join's separator must be a string" "27|1" \
       "$(list_says '    satellite.console.display(a.join(5))' 'puts a string between the items, and was given a number')"
expect ".reserve(-1) is not a count" "19|1" \
       "$(list_says '    a.reserve(-1)' 'a count is 0 or more')"
expect ".reserve past what any machine holds is out_of_memory, not a crash" "48|1" \
       "$(list_says '    a.reserve(100000000000000000000000000000)' 'no machine holds that many items')"
expect ".reserve needs a name to make room in" "13|1" \
       "$(list_says '    satellite.console.display({1}.reserve(3))' 'changes a container, and this one has no name to change')"
expect "an index is told to say which half: .values.sum or .keys.sum" "27|1" \
       "$(list_says '    satellite.container.index<satellite.variable.string, satellite.variable.number> s
    s["k"] = 1
    satellite.console.display(s.sum)' 'say which: s.values.sum or s.keys.sum')"
# THE FRESH READER'S FINDINGS, 2026-09-23, each pinned. Every pair of KINDS present is
# asked whether it has an order -- asking item 1 against the rest let {1, b10, 9.5}.max
# answer b10 with exit 0 -- and the refusal carries compare's own code (14 for a pair
# decided and not built, as `b10 < 9.5` says). A method's argument count, an index's
# which-half, and a container method on a literal are refused before anything runs.
expect ".max over three kinds finds the one pair with no order (item 2 and 3), not built yet" "14|1" \
       "$(list_says '    satellite.container.list m = {1, b10, 9.5}
    satellite.console.display(m.max)' 'item 2 and item 3 have no order between them')"
expect ".sort().by_value() refuses the same list rather than leave it unsorted" "14|1" \
       "$(list_says '    satellite.container.list m = {1, b10, 9.5}
    satellite.console.display(m.sort().by_value())' 'by_value orders by what each item is worth, and item 2 and item 3')"
expect "a container method given the wrong count is refused before the run" "13|1" \
       "$(list_says '    a.reserve()' 'satl(check): in satellite.main, a.reserve takes 1 argument, and was given 0')"
expect "a container method on a literal is refused before the run: Python's \", \".join(list)" "14|1" \
       "$(list_says '    satellite.console.display(", ".join(a))' 'satl(check): in satellite.main, that string.join is not built for a string yet')"
expect "a name declared an index is told which half before the run, join with its (separator)" "27|1" \
       "$(list_says '    satellite.container.index s
    satellite.console.display(s.join(","))' 'satl(check): in satellite.main, s.join -- an index holds keys and values, so say which: s.values.join(separator) or s.keys.join(separator)')"
expect "a multiple<list, number> name appends anything, as its [n] = x already did" "0|1" \
       "$(list_says '    satellite.container.multiple<satellite.container.list, satellite.variable.number> m = {1}
    m.append("text")
    satellite.console.display(m)' '{1, "text"}')"
expect "sum max min join reserve are registry rows 0x0B52 to 0x0B56, and token_codes.hpp agrees" "1|1|1|1|1|1|1|1|1|1" \
       "$(for pair in sum:0B52:0000101101010010 max:0B53:0000101101010011 min:0B54:0000101101010100 join:0B55:0000101101010101 reserve:0B56:0000101101010110; do
              n=${pair%%:*}; rest=${pair#*:}; hex=${rest%%:*}; bits=${rest#*:}
              printf '%s|%s|' "$(grep -c "^$bits  ${n}_token " REGISTRY.satellite)" "$(grep -c "Code ${n}_token = 0x$hex;" satellite/bytecode/token_codes.hpp)"
          done | sed 's/|$//')"

# satellite.container.index -- A PYTHON DICT, NOT A std::map (the author,
# 2026-09-18: "an index is a python dictionary, but I think it's just a std::map,
# but I could be wrong").
#
# THE FIRST ROW IS THE WHOLE DIFFERENCE, and it is the reason this is not a
# std::map: the keys come back IN THE ORDER THEY WERE PUT IN. A std::map would
# print alice, bob, zoe -- sorted -- from the same program. Writing an existing
# key keeps its place rather than moving it to the end, which is what Python does
# and what anybody reading the output expects.
cat > build/index_order.satl <<'IDX_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.index<satellite.variable.string, satellite.variable.number> scores
    scores["zoe"] = 10
    scores["alice"] = 20
    scores["bob"] = 30
    satellite.console.display(scores)
    satellite.console.display(scores["alice"])
    scores["zoe"] = 99
    satellite.console.display(scores)
    satellite.return(satellite)
}
IDX_EOF
HOME="$CHECK_HOME" "$interpreter" build/index_order.satl > build/index_order.out 2>&1
expect "an index keeps INSERTION order, not sorted order (a std::map would say alice first)" \
       '{"zoe": 10, "alice": 20, "bob": 30}|20|{"zoe": 99, "alice": 20, "bob": 30}' \
       "$(tail -3 build/index_order.out | tr '\n' '|' | sed 's/|$//')"

# ANY CONTAINER WITH ANY CONTAINER (the author's words), and `[a][b]` across two
# DIFFERENT kinds: an index by key, then the list it holds by position -- reading
# and writing. `>>` closing two type parameters is the C++ wart, split here.
cat > build/container_mix.satl <<'MIX_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.multiple<satellite.variable.string, satellite.variable.number> either = "text"
    satellite.console.display(either)
    either = 42
    satellite.console.display(either)

    satellite.container.index<satellite.variable.string, satellite.container.list> teams
    teams["red"] = {"ann", "bo"}
    teams["blue"] = {"cy"}
    satellite.console.display(teams["red"][2])
    teams["red"][1] = "ANN"
    satellite.console.display(teams)

    satellite.container.list<satellite.container.list<satellite.variable.number>> grid = {{1, 2}, {3, 4}}
    satellite.console.display(grid[2][2])
    satellite.return(satellite)
}
MIX_EOF
HOME="$CHECK_HOME" "$interpreter" build/container_mix.satl > build/container_mix.out 2>&1
expect "multiple holds either type; [key][position] reads and writes across two container kinds; >> closes two" \
       'text|42|bo|{"red": {"ANN", "bo"}, "blue": {"cy"}}|4' \
       "$(tail -5 build/container_mix.out | tr '\n' '|' | sed 's/|$//')"

# THE TYPE BETWEEN < AND > IS ENFORCED ON ONE ITEM, NOT ONLY ON THE WHOLE
# CONTAINER. This shipped broken for an hour: `index<string, number> s` took
# `s[1] = 5` -- a number key -- because the shape was checked when a whole
# container was assigned and ignored when one item was written. A type that holds
# until you use it is worse than no type, because a person believes it.
index_says() {
    cat > build/index_bad.satl <<BAD_EOF
satellite.include(satellite)
satellite.capsule satellite.main()
{
$1
    satellite.return(satellite)
}
BAD_EOF
    HOME="$CHECK_HOME" "$interpreter" build/index_bad.satl > build/index_bad.out 2>&1
    printf '%s|%s' "$?" "$(tr '\n' ' ' < build/index_bad.out | grep -c "$2")"
}
expect "a key of the wrong type is refused ON THE WRITE, not just on assignment" "27|1" \
       "$(index_says '    satellite.container.index<satellite.variable.string, satellite.variable.number> s
    s[1] = 5' 'the key does not fit')"
expect "a value of the wrong type is refused on the write" "27|1" \
       "$(index_says '    satellite.container.index<satellite.variable.string, satellite.variable.number> s
    s["a"] = "not a number"' 'the value does not fit')"
expect "list<number> refuses a string written into an item" "27|1" \
       "$(index_says '    satellite.container.list<satellite.variable.number> n = {1, 2}
    n[1] = "text"' 'does not fit')"
expect "the shape is walked DOWN: index<string, list<number>> refuses t[key][n] = text" "27|1" \
       "$(index_says '    satellite.container.index<satellite.variable.string, satellite.container.list<satellite.variable.number>> t
    t["a"] = {1, 2}
    t["a"][1] = "text"' 'does not fit')"
expect "a missing key says so rather than answering nothing" "47|1" \
       "$(index_says '    satellite.container.index<satellite.variable.string, satellite.variable.number> s
    s["a"] = 1
    satellite.console.display(s["nope"])' 'there is no such key in it')"
expect "a list cannot be a key, because a key must not be able to change" "27|1" \
       "$(index_says '    satellite.container.index s
    s[{1, 2}] = 1' 'a key must be a number, a string, a bool, a binary or a percentage')"
expect "multiple<a, b> refuses a third type" "27|1" \
       "$(index_says '    satellite.container.multiple<satellite.variable.string, satellite.variable.number> x = {1, 2}' 'this name takes')"
expect "multiple with one type is refused as saying nothing" "13|1" \
       "$(index_says '    satellite.container.multiple<satellite.variable.string> x = "a"' 'takes two or more types')"
# THE CONTAINER METHODS (the author, 2026-09-18): .append, .size, .contains and
# .contain as an alias, and sort as `.sort().by_name()` / `.sort().by_value()`
# with `.reverse()` in place of the old .sort("up") / .sort("down").
#
# `.sort().by_value()` ORDERS BY WHAT THINGS ARE WORTH, not by how they read:
# {30, 4, 100, 2} sorts to {2, 4, 30, 100}, where sorting the TEXT would give
# {100, 2, 30, 4}. That row is here because the wrong one looks plausible.
#
# `.sort()` DOES NOT REORDER THE NAME -- the last row checks names is untouched
# after being sorted, which is what makes
# `satellite.console.display(names.sort().by_name())` safe to write.
cat > build/list_methods.satl <<'MET_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list names = {"zoe", "alice", "bob"}
    names.append("carol")
    satellite.console.display(names)
    satellite.console.display(names.size)
    satellite.console.display(names.contains("bob"))
    satellite.console.display(names.contain("nope"))
    satellite.console.display(names.sort().by_name())
    satellite.console.display(names.sort().by_name().reverse())
    satellite.container.list nums = {30, 4, 100, 2}
    satellite.console.display(nums.sort().by_value())
    satellite.console.display(nums.sort().by_value().reverse())
    satellite.console.display(names)
    satellite.return(satellite)
}
MET_EOF
HOME="$CHECK_HOME" "$interpreter" build/list_methods.satl > build/list_methods.out 2>&1
expect "append changes the name; size, contains and its .contain alias; by_name is A-Z and by_value is by worth" \
       '{"zoe", "alice", "bob", "carol"}|4|true|false|{"alice", "bob", "carol", "zoe"}|{"zoe", "carol", "bob", "alice"}|{2, 4, 30, 100}|{100, 30, 4, 2}|{"zoe", "alice", "bob", "carol"}' \
       "$(tail -9 build/list_methods.out | tr '\n' '|' | sed 's/|$//')"

# `.reverse()` ON EVERY TYPE THAT HAS AN ORDER (the author: "add .reverse()
# anywhere we can reverse something, strings, numbers, binary numbers hex").
#
# THE SECOND ROW IS THE ONE WORTH KEEPING: "hello" with accents reverses BY
# CHARACTER, not by byte. Reversing the UTF-8 bytes would answer something that
# is not text at all, and that is the whole reason the language carries its own
# 16/32-bit string rather than a std::string of bytes.
cat > build/reverse_all.satl <<'REV_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.string s = "hello"
    satellite.console.display(s.reverse())
    satellite.variable.string u = "héllo→"
    satellite.console.display(u.reverse())
    satellite.variable.number n = 12345
    satellite.console.display(n.reverse())
    satellite.variable.number neg = -123
    satellite.console.display(neg.reverse())
    satellite.variable.number z = 120
    satellite.console.display(z.reverse())
    satellite.variable.binary b = b1010
    satellite.console.display(b.reverse())
    satellite.console.display(s.reverse().reverse())
    satellite.return(satellite)
}
REV_EOF
HOME="$CHECK_HOME" "$interpreter" build/reverse_all.satl > build/reverse_all.out 2>&1
expect "reverse: a string BY CHARACTER, a number's digits, a sign that stays, a binary's width kept" \
       'olleh|→olléh|54321|-321|21|b0101|hello' \
       "$(tail -7 build/reverse_all.out | tr '\n' '|' | sed 's/|$//')"

# APPEND IS NOT QUADRATIC, AND THIS ROW EXISTS BECAUSE IT WAS.
#
# The first version of .append took 0.588s for 5,000 appends and 7.647s for
# 20,000 -- THIRTEEN times the work for four times the appends. The cause was one
# line, `Value receiver = start;` at the top of call_method: copy-on-write asks
# `use_count() == 1`, and that copy made the answer no every single time, so
# every append duplicated the whole list. It is exactly the bug 003 shipped for
# months, rebuilt here by accident and found only by measuring.
#
# NOTHING ABOUT THE OUTPUT DIFFERS between the two. After the fix, 100,000
# appends take 0.26s.
for size in 25000 100000; do
  {
    echo 'satellite.include(satellite)'
    echo 'satellite.capsule satellite.main()'
    echo '{'
    echo '    satellite.container.list big = {}'
    echo "    satellite.statement.for(satellite.variable.number i = 0; i < $size; i++)"
    echo '    {'
    echo '        big.append(i)'
    echo '    }'
    echo '    satellite.console.display(big.size)'
    echo '    satellite.return(satellite)'
    echo '}'
  } > "build/list_append_$size.satl"
done
ap_small=$( { TIMEFORMAT=%R; time HOME="$CHECK_HOME" "$interpreter" build/list_append_25000.satl > build/ap_small.out 2>&1; } 2>&1 )
ap_big=$(   { TIMEFORMAT=%R; time HOME="$CHECK_HOME" "$interpreter" build/list_append_100000.satl > build/ap_big.out 2>&1; } 2>&1 )
expect "100,000 appends all landed" "100000" "$(tail -1 build/ap_big.out)"
# Four times the appends inside eight times the time is linear with room to
# spare, and nowhere near the sixteen times a copying append costs.
expect "append is linear, not quadratic (4x the appends: ${ap_small}s -> ${ap_big}s)" \
       "linear" \
       "$(awk -v s="$ap_small" -v b="$ap_big" 'BEGIN { print (b < s * 8 + 0.05) ? "linear" : "QUADRATIC: " b "s vs " s "s" }')"

expect "an index says how to fill it rather than refusing .append bare" "27|1" \
       "$(index_says '    satellite.container.index s
    s.append(1)' 'an index has keys and not positions')"
# THE REST OF THE CONTAINER METHODS (the author, 2026-09-18: "ready 21.43 to
# finish the methods?"). Semantics settled against the FILE versions, which
# already had all of these: a method spelled the same on a file and on a list
# takes the same arguments and means the same thing.
#
# THE MUTATORS ANSWER THE CONTAINER, not a bool and not the removed item, so they
# read left to right like .append already did. To have the item, read it on the
# line before -- `gone = names.first` then `names.remove_first()`.
#
# `.insert(size + 1, x)` IS AN APPEND, which is what satellite_file::insert does
# in its own first line. It is the last step of every loop that fills a list in
# order, and refusing it would make the file's promise false in the one place it
# is tested hardest.
cat > build/list_rest.satl <<'REST_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list n = {"a", "b", "c", "d"}
    satellite.console.display(n.first)
    satellite.console.display(n.last)
    satellite.console.display(n.empty)
    satellite.console.display(n.index_of("c"))
    satellite.console.display(n.index_of("zz"))
    n.insert(2, "NEW")
    satellite.console.display(n)
    n.insert(6, "ATEND")
    satellite.console.display(n)
    n.remove_at(1)
    n.remove("c")
    satellite.console.display(n)
    n.remove_first()
    n.remove_last()
    satellite.console.display(n)
    n.truncate(1)
    satellite.console.display(n)
    n.clear
    satellite.console.display(n)
    satellite.console.display(n.empty)
    satellite.container.list s = {12, 345, 67}
    satellite.console.display(s.search(4))
    satellite.return(satellite)
}
REST_EOF
HOME="$CHECK_HOME" "$interpreter" build/list_rest.satl > build/list_rest.out 2>&1
expect "first, last, empty, index_of, insert (incl. at size+1 = append), remove_at, remove, remove_first/last, truncate, clear, search" \
       'a|d|false|3|0|{"a", "NEW", "b", "c", "d"}|{"a", "NEW", "b", "c", "d", "ATEND"}|{"NEW", "b", "d", "ATEND"}|{"b", "d"}|{"b"}|{}|true|2' \
       "$(tail -13 build/list_rest.out | tr '\n' '|' | sed 's/|$//')"

# AN INDEX'S OWN METHODS. .keys and .values line up entry for entry and both come
# back IN INSERTION ORDER, which is the only order a dict has -- and the order it
# prints in, so what you read is what you saw. Removing a key keeps the rest in
# order, which is the part a rebuilt lookup table gets wrong.
cat > build/index_rest.satl <<'IREST_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.index<satellite.variable.string, satellite.variable.number> k
    k["zoe"] = 1
    k["al"] = 2
    k["bo"] = 3
    satellite.console.display(k.keys)
    satellite.console.display(k.values)
    satellite.console.display(k.first)
    satellite.console.display(k.last)
    satellite.console.display(k.empty)
    k.remove("al")
    satellite.console.display(k)
    satellite.console.display(k["bo"])
    k.remove_first()
    satellite.console.display(k)
    k.clear
    satellite.console.display(k.empty)
    satellite.return(satellite)
}
IREST_EOF
HOME="$CHECK_HOME" "$interpreter" build/index_rest.satl > build/index_rest.out 2>&1
expect "an index: keys and values aligned in insertion order, first, last, remove by key keeps the order, clear" \
       '{"zoe", "al", "bo"}|{1, 2, 3}|zoe|bo|false|{"zoe": 1, "bo": 3}|3|{"bo": 3}|true' \
       "$(tail -9 build/index_rest.out | tr '\n' '|' | sed 's/|$//')"

# TWO CONTAINERS ARE EQUAL WHEN THEY HOLD EQUAL THINGS.
#
# THIS ARM SAID "THE SAME LIST OR NOT THE SAME LIST" UNTIL RED NOTE 8 CLOSED, and
# the note it carried gave the reason: while nothing had decided whether `b = a`
# shares or copies, a deep compare would have answered a question still open.
# It is answered now -- a list is a VALUE -- and identity became WRONG the moment
# it was: `{1,2} == {1,2}` was refused, and `g.contains({1,2})` answered false on
# a list that plainly held it, because copy-on-write clones on every write by
# design and a clone is a different address.
#
# AN INDEX IGNORES ORDER FOR ==, as python's dict does: insertion order is what an
# index REMEMBERS, not what it IS. It still PRINTS in that order, so two equal
# indexes can print differently -- exactly as python's do.
cat > build/container_equal.satl <<'EQ_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display({1, 2} == {1, 2})
    satellite.console.display({1, 2} == {1, 3})
    satellite.console.display({1, 2} == {1, 2, 3})
    satellite.console.display({{1}, {2}} == {{1}, {2}})
    satellite.container.list g = {{1, 2}, {3}}
    satellite.console.display(g.contains({1, 2}))
    satellite.console.display(g.index_of({3}))
    satellite.container.index a
    a["x"] = 1
    a["y"] = 2
    satellite.container.index b
    b["y"] = 2
    b["x"] = 1
    satellite.console.display(a == b)
    satellite.console.display(a)
    satellite.console.display(b)
    satellite.return(satellite)
}
EQ_EOF
HOME="$CHECK_HOME" "$interpreter" build/container_equal.satl > build/container_equal.out 2>&1
expect "two containers are equal by what they HOLD; an index ignores order for == and still prints in it" \
       'true|false|false|true|true|2|true|{"x": 1, "y": 2}|{"y": 2, "x": 1}' \
       "$(tail -9 build/container_equal.out | tr '\n' '|' | sed 's/|$//')"

# THE TYPE BETWEEN < AND > IS ENFORCED ON EVERY DOOR INTO THE CONTAINER.
#
# `.append` WAS THE DOOR THAT WAS LEFT OPEN. For an hour,
# `satellite.container.list<satellite.variable.number> a` took `a.append("zoe")`
# while refusing `a[1] = "zoe"` -- the same value, the same name, two opposite
# answers. Worse, it was self-contradicting: the program built a list its own
# declaration rejects, so a later innocent `a = a` refused and pointed at the
# WRONG statement.
say_no() {
    cat > build/ctype.satl <<CT_EOF
satellite.include(satellite)
satellite.capsule satellite.main()
{
$1
    satellite.return(satellite)
}
CT_EOF
    HOME="$CHECK_HOME" "$interpreter" build/ctype.satl > build/ctype.out 2>&1
    printf '%s|%s' "$?" "$(tr '\n' ' ' < build/ctype.out | grep -c "$2")"
}
expect "append is refused by the declared item type, like a[n] = v is" "27|1" \
       "$(say_no '    satellite.container.list<satellite.variable.number> a = {1, 2}
    a.append("zoe")' 'it holds a string')"
expect "insert is refused by it too" "27|1" \
       "$(say_no '    satellite.container.list<satellite.variable.number> a = {1, 2}
    a.insert(1, "zoe")' 'it holds a string')"
expect "a multiple<a, b> does not read its types as a container's items" "0|1" \
       "$(say_no '    satellite.container.multiple<satellite.container.list, satellite.variable.number> m = {1, 2}
    m[1] = "text"
    satellite.console.display(m)' '{"text", 2}')"

# THE REFUSALS THE REST OF THE METHODS MAKE.
expect "a position method on an index says an index has keys, not positions" "27|1" \
       "$(say_no '    satellite.container.index s
    s.remove_at(1)' 'an index has keys and not positions')"
expect "insert past size+1 names the range a new item may go in" "47|1" \
       "$(say_no '    satellite.container.list a = {1}
    a.insert(9, 2)' 'a new one goes in at 1 to 2')"
expect "remove of something absent says so instead of doing nothing" "15|1" \
       "$(say_no '    satellite.container.list a = {1}
    a.remove(2)' 'there is no such item in it')"
expect "first on an empty list says the list is empty" "47|1" \
       "$(say_no '    satellite.container.list a = {}
    satellite.console.display(a.first)' 'the list is empty')"
expect "a mutator with no name to change is refused" "13|1" \
       "$(say_no '    satellite.console.display({1, 2}.clear)' 'has no name to change')"
expect "a position larger than anything says THAT, not a 20-digit number" "47|1" \
       "$(say_no '    satellite.container.list a = {1}
    satellite.console.display(a[99999999999999999999])' 'more items than anything could hold')"
expect "ordering two lists is refused; only == and != compare them" "27|1" \
       "$(say_no '    satellite.console.display({1} < {2})' 'there is no order between two containers')"

# A METHOD MAY FOLLOW A LITERAL, NOT ONLY A NAME.
#
# `"abc".reverse()` FAILED WITH "could not be read to the end of" until this was
# built, and that is the first thing a person would type after being told the
# language has .reverse() for strings. Every literal branch in one_operand
# answered its value and returned, so the `.` after it was a token nothing
# expected -- invisible while every test called methods on names.
cat > build/literal_methods.satl <<'LIT_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("abc".reverse())
    satellite.console.display(123.reverse())
    satellite.console.display(b1010.reverse())
    satellite.console.display({3, 1, 2}.size)
    satellite.console.display({3, 1, 2}.sort().by_value())
    satellite.console.display({1, 2}.contains(2))
    satellite.console.display((1 + 2).reverse())
    satellite.console.display("abc".reverse().reverse())
    satellite.return(satellite)
}
LIT_EOF
HOME="$CHECK_HOME" "$interpreter" build/literal_methods.satl > build/literal_methods.out 2>&1
expect "a method reads off a literal: a string, a number, a binary, a braced list and a bracketed sum" \
       'cba|321|b0101|3|{1, 2, 3}|true|3|abc' \
       "$(tail -8 build/literal_methods.out | tr '\n' '|' | sed 's/|$//')"
# ...and a literal still has no name, so nothing that CHANGES one may be called
# on it -- the append would be correct and then thrown away.
expect "a mutator on a literal is refused, because there is nothing to change" "13|1" \
       "$(say_no '    satellite.console.display({1, 2}.clear)' 'has no name to change')"
# A MUTATOR REACHES INTO A NESTED CONTAINER (the author's "any container with
# any container"). `grid[1].append(3)` was refused -- "this one has no name to
# change" -- so a list inside a list could never be appended to at all.
#
# THE READ PATH HANDS A METHOD A COPY, which is why refusing was right and the
# refusal was the symptom. A chain that ENDS IN A MUTATOR is now walked a second
# time by reference; a chain that does not is left on the copy, so reading an
# item never clones a shared list.
cat > build/nested_mutate.satl <<'NM_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list g = {{1, 2}, {3}}
    g[1].append(99)
    satellite.console.display(g)
    g[2].insert(1, 0)
    satellite.console.display(g)
    g[1].remove_last()
    satellite.console.display(g)
    satellite.container.index<satellite.variable.string, satellite.container.list> teams
    teams["red"] = {"ann"}
    teams["red"].append("bo")
    satellite.console.display(teams)
    satellite.console.display(teams["red"].size)
    satellite.container.list a = {{1}}
    satellite.container.list b = a
    b[1].append(2)
    satellite.console.display(a)
    satellite.console.display(b)
    satellite.return(satellite)
}
NM_EOF
HOME="$CHECK_HOME" "$interpreter" build/nested_mutate.satl > build/nested_mutate.out 2>&1
expect "append, insert and remove reach a list inside a list and inside an index -- and a copy is still a copy" \
       '{{1, 2, 99}, {3}}|{{1, 2, 99}, {0, 3}}|{{1, 2}, {0, 3}}|{"red": {"ann", "bo"}}|2|{{1}}|{{1, 2}}' \
       "$(tail -7 build/nested_mutate.out | tr '\n' '|' | sed 's/|$//')"

# A NESTED APPEND IS NOT QUADRATIC EITHER, AND IT WAS -- for the THIRD time in
# this file, by a third route.
#
# The read walk left a live handle on the very item about to be changed, so
# copy-on-write saw use_count() == 2 and cloned the whole inner list on every
# append: 10,000 nested appends 1.569s, 40,000 appends 23.667s. Fifteen times the
# work for four times the appends. One line -- letting go of the read copy before
# walking to the slot -- took 40,000 to 0.090s.
#
# NOTHING ABOUT THE OUTPUT DIFFERS between the two versions. This row is the only
# thing that would say so.
for size in 10000 40000; do
  {
    echo 'satellite.include(satellite)'
    echo 'satellite.capsule satellite.main()'
    echo '{'
    echo '    satellite.container.list g = {{}}'
    echo "    satellite.statement.for(satellite.variable.number i = 0; i < $size; i++)"
    echo '    {'
    echo '        g[1].append(i)'
    echo '    }'
    echo '    satellite.console.display(g[1].size)'
    echo '    satellite.return(satellite)'
    echo '}'
  } > "build/nested_append_$size.satl"
done
na_small=$( { TIMEFORMAT=%R; time HOME="$CHECK_HOME" "$interpreter" build/nested_append_10000.satl > build/na_small.out 2>&1; } 2>&1 )
na_big=$(   { TIMEFORMAT=%R; time HOME="$CHECK_HOME" "$interpreter" build/nested_append_40000.satl > build/na_big.out 2>&1; } 2>&1 )
expect "40,000 nested appends all landed" "40000" "$(tail -1 build/na_big.out)"
expect "a nested append is linear too (4x the appends: ${na_small}s -> ${na_big}s)" "linear" \
       "$(awk -v s="$na_small" -v b="$na_big" 'BEGIN { print (b < s * 8 + 0.05) ? "linear" : "QUADRATIC: " b "s vs " s "s" }')"




# THE EXAMPLE IN `satl --help` IS EXTRACTED FROM THE REAL OUTPUT AND RUN.
#
# There are no users yet -- satellite is pre-release -- so the first program a
# person ever writes will be the one they copy out of --help, and an example that
# does not run is the worst possible first impression. 003 learned this the
# expensive way: HELP.md's examples were run once and FIVE of them were wrong.
#
# Taken from the binary's own output, never re-typed here, so the test cannot
# agree with a copy while the real one rots.
"$interpreter" --help > build/help.out 2>&1
sed -n '/YOUR FIRST PROGRAM/,/satellite.return(satellite)/p' build/help.out \
    | sed '1,2d' | sed 's/^    //' > build/help_example.satl
expect "the example in --help is a whole program" "1|1|1" \
       "$(grep -c 'satellite.include(satellite)' build/help_example.satl)|$(grep -c 'satellite.capsule satellite.main()' build/help_example.satl)|$(grep -c 'satellite.return(satellite)' build/help_example.satl)"
expect "... and it runs, exactly as printed" "hello|0" \
       "$("$interpreter" build/help_example.satl 2>/dev/null | tail -1)|$(? 2>/dev/null; "$interpreter" build/help_example.satl >/dev/null 2>&1; echo $?)"
expect "--help does not promise a feature that is missing" 0 \
       "$(grep -c 'not built yet' build/help.out)"

expect "... by the check too, and says so" 1 \
       "$(tr '\n' ' ' < build/if.out | grep -c 'satellite.statement.else with no satellite.statement.if before it')"
# A CONDITION'S TYPE IS A RUN-TIME FACT, for an if exactly as for a while: the
# checker does not evaluate, so `if(5)` is refused where it runs, after the line
# above it has printed. Pinned here so the two never drift apart.
"$interpreter" tests/if_not_a_condition.satl > build/if.out 2>&1; expect "if(5) is refused: a number is not a condition" 27 $?
expect "... and says what it was given" 1 \
       "$(grep -c 'satellite.statement.if was given a number and needs a true or false' build/if.out)"
# satellite.statement.for (2026-09-17), MILESTONES M20.A and the author's own line:
# three parts divided by semicolons, and a third part written with no `=` because
# the loop's own name goes in front of it -- `my_int + 1` MEANS `my_int = my_int + 1`.
# `++` and `--` are the one spelling that exists nowhere else in the language, and
# they are not tokens: the lexer already writes `i++` as the name and two TOUCHING
# pluses, which is not an operation anywhere, so reading the pair costs no registry row.
wanted_for="0|1|2|0|1|2|3|2|1|1|2|4|8|16|10|0|1|same|different|different|same|0|1|9|0|1"
expect "for: + 1, ++, --, * 2, an empty step, nesting, and a call in the condition" "$wanted_for" \
       "$("$interpreter" tests/for_loop.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/for_loop.satl > /dev/null 2>&1; expect "... and it ends cleanly" 0 $?
# EVERY SHAPE MISTAKE IS THE CHECKER'S, and each is asserted to have printed
# nothing: a loop that half-runs and then stops is the one thing program_check.cpp
# exists to prevent, and a for has three more places to get that wrong.
# THE STEP IS THE ONE PART THAT RUNS AFTER THE BODY, so a step the walker cannot
# use is a loop that prints a turn and then stops -- or, for `--i` and a bare `i`,
# one that runs forever saying nothing (9 million lines in five seconds, the
# review, 2026-09-17). Every one of these is a SHAPE and is refused by the checker.
for case in for_no_body for_no_semicolons for_no_declaration for_empty_condition \
            for_power_stars_touching for_step_other_name for_step_payload \
            for_step_prefix_minus for_step_is_the_name for_step_two_operators \
            for_step_doubled_then_more for_step_undecided_sign; do
    timeout 10 "$interpreter" "tests/$case.satl" > build/for.out 2>&1
    expect "$case is refused" 13 $?
    expect "... with nothing run before it" "" "$(grep -x before build/for.out)"
done
# A SPACED `**` IS POWER IN A for STEP TOO (SATELLITE_INFINITY.md, INF-1): the file
# that used to be refused here for `i ** 2` now runs, from 2 so that it ends.
timeout 10 "$interpreter" tests/for_power_stars_touching.satl > build/for.out 2>&1
expect "for(...; i**2): the touching ** is refused by name" 1 \
       "$(tr '\n' ' ' < build/for.out | grep -c 'power is written with a space on both sides -- write i \*\* ... or i ^ ...')"
expect "for(...; i ** 2) runs: ** is the second spelling of ^" "before|2|4|16" \
       "$(timeout 10 "$interpreter" tests/for_power_stars.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
# THE ROWS ABOVE ARE ONLY WORTH SOMETHING IF THE FIXTURES REALLY PRINT FIRST: each
# one opens with display("before"), so a refusal that let the program start would
# show it. This proves the "nothing run before it" rows are not vacuous.
"$interpreter" tests/for_not_a_condition.satl > build/for.out 2>&1
expect "the fixtures do print before their for -- so the rows above mean something" "before" \
       "$(grep -x before build/for.out)"
# THE for's NUMBER DIES WITH THE LOOP (M20.A: it "exists while the for loop is
# running", and then belongs to satellite.history, which is M20.B and unbuilt).
# The CHECKER forgets it where the walker erases it, so a later `i` is refused
# before the loop has printed -- and tests/for_loop.satl declares `i` twice, in
# two loops that do not overlap, which is the same rule seen from the other side.
"$interpreter" tests/for_counter_after.satl > build/for.out 2>&1
expect "the for's number is gone after the loop" 25 $?
expect "... by the check, with nothing run before it" "" "$(grep -x before build/for.out)"
# A CONDITION'S TYPE AND A DECLARATION'S VALUE ARE RUN-TIME FACTS, for a for
# exactly as for an if: the checker does not evaluate, so both are refused after
# the line above them has printed. Pinned so the two never drift apart.
"$interpreter" tests/for_not_a_condition.satl > build/for.out 2>&1
expect "for(...; 5; ...) is refused: a number is not a condition" 27 $?
expect "... and says what it was given" 1 \
       "$(grep -c 'satellite.statement.for was given a number and needs a true or false' build/for.out)"
# A STRING IN THE STEP is refused for not beginning with the number -- it is in
# the loop above. U+0308 is 0x0308, which is tight_times_token, so this is the
# shape that would fool a scan reading a payload's codes as tokens; the rule reads
# only the code after the name, so there is no scan left to fool.
"$interpreter" tests/for_declares_a_string.satl > build/for.out 2>&1
expect "a for that declares a number and is given a string" 27 $?
expect "... says both the type and the name" 1 \
       "$(grep -c 'declares satellite.variable.number i, and it was given a string' build/for.out)"

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
# satellite_infinity AGAINST infinity_oracle.py (SATELLITE_INFINITY.md, INF-2): every value
# the spec's tables print is shown as the oracle shows it, every pair is ordered as the
# oracle orders it, and a chain 100,000 exponents deep goes through every walk.
if [ ! -x build/infinity_cases ]; then expect "build/infinity_cases is built (make)" built missing
elif ! make -sq build/infinity_cases 2>/dev/null; then
    expect "build/infinity_cases is as new as its sources (make build/infinity_cases)" current stale
else
    timeout 600 python3 satellite/satellite_variable_infinity/check_infinity.py > build/check_infinity.out 2>&1; code=$?
    expect "satellite_infinity against the oracle: $(tail -1 build/check_infinity.out) (build/check_infinity.out)" 0 $code
fi
# satellite.variable.file's handle with no interpreter around it (SATELLITE_FILE_OPERATIONS
# Part 3): lines from 1, the endings a file had, strict UTF-8, and the two saves, in a
# folder it makes inside $TMPDIR and removes.
if [ ! -x build/file_cases ]; then expect "build/file_cases is built (make)" built missing
elif ! make -sq build/file_cases 2>/dev/null; then
    expect "build/file_cases is as new as its sources (make build/file_cases)" current stale
else
    timeout 300 build/file_cases "${TMPDIR:-/tmp}" > build/file_cases.out 2>&1; code=$?
    expect "satellite.variable.file's handle: $(grep -c '^ok' build/file_cases.out) cases (build/file_cases.out)" 0 $code
fi
# satellite.variable.file IN A PROGRAM (2026-09-18, SATELLITE_FILE_OPERATIONS FO-1 to
# FO-4 and 3.8). Each program is copied into a scratch folder and run there, because a
# relative path is the folder of the .satl that wrote it -- so what they make lands
# there and nowhere in the tree.
expect "a method call standing alone runs, and its answer is let go" "line_1" \
       "$("$interpreter" tests/method_statement.satl 2>/dev/null)"
file_room=$(mktemp -d "${TMPDIR:-/tmp}/satl_files.XXXXXX")
cp tests/file_list.satl tests/file_self_edit.satl tests/file_snapshot.satl tests/file_past_the_end.satl \
   tests/file_word_chain.satl tests/file_not_open.satl tests/file_wrong_count.satl tests/file_unsaved.satl "$file_room/"
expect "a file is a list of lines counting from 1 (the author's example)" \
       "line_1|line_3|line_4|3|2|3|0|true|true|line_1|line_4|line_3|2|false|false|false|true" \
       "$("$interpreter" "$file_room/file_list.satl" 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
expect "... and the handle still open at the end was saved: line 1 removed" "line_3|line_4" \
       "$(tr '\n' '|' < "$file_room/demo.se" | sed 's/|$//')"
expect "a program raises the number in its own source: this run says 1" "RUN 1" \
       "$("$interpreter" "$file_room/file_self_edit.satl" 2>/dev/null)"
expect "... the next run says 2" "RUN 2" "$("$interpreter" "$file_room/file_self_edit.satl" 2>/dev/null)"
expect "... and the source on the disk now says 3" 1 \
       "$(grep -c '^    satellite.variable.number ds_build = 3$' "$file_room/file_self_edit.satl")"
"$interpreter" "$file_room/file_snapshot.satl" > build/file_snapshot.out 2>&1
expect "a program that rewrote its own line 10 and then failed on it stops there" 22 $?
expect "... and the report quotes line 10 as it was LOADED" 1 \
       "$(grep -c 'syntax: satellite.console.display(1 / 0)' build/file_snapshot.out)"
expect "... while the disk has the rewritten line" 1 \
       "$(grep -cx '    satellite.console.display(2)' "$file_room/file_snapshot.satl")"
"$interpreter" "$file_room/file_past_the_end.satl" > build/file_past.out 2>&1
expect "reading a line past the end stops the program" 47 $?
expect "... after the line that was there printed" 1 "$(grep -cx 'the only line' build/file_past.out)"
# THE REVIEW'S FINDINGS, 2026-09-18, each pinned so it cannot come back.
expect "a method after a word's call runs: new(...).append, open(...).append, open(...).size" 2 \
       "$("$interpreter" "$file_room/file_word_chain.satl" 2>/dev/null)"
"$interpreter" "$file_room/file_not_open.satl" > build/file_not_open.out 2>&1
expect "a question to a file that did not open stops the program (S503)" 45 $?
expect "... after saying it is not ok, and without answering 0" "false" "$(grep -x -e false -e 0 build/file_not_open.out)"
"$interpreter" "$file_room/file_wrong_count.satl" > build/file_count.out 2>&1
expect "a file method given the wrong number of arguments is refused" 13 $?
expect "... by the CHECK, with nothing run before it" "" "$(grep -x before build/file_count.out)"
printf 'kept\n' > "$file_room/read_only.se"; chmod 444 "$file_room/read_only.se"
if [ "$(id -u)" = 0 ]; then expect "a save that cannot land at the end is said (skipped: root writes anyway)" 0 0
else
    "$interpreter" "$file_room/file_unsaved.satl" > build/file_unsaved.out 2>&1
    expect "a save that cannot land at the end of the run is said, and the run exits 44" 44 $?
    expect "... and the file is as it was" "kept" "$(cat "$file_room/read_only.se")"
fi
rm -rf -- "$file_room"
SATL="$interpreter" timeout 600 python3 -u satellite/satl/check_session.py > build/check_session.out 2>&1; code=$?
expect "satl --repl at a real terminal: $(grep -c '^ok' build/check_session.out) checks (build/check_session.out)" 0 $code
# The prompt's own refusals, from a pipe: each says which spelling it refused, and the
# session goes on to the line after them.
printf 'satellite.include(satellite)\n{\nsatellite.capsule satellite.main()\nsatellite.return(satellite)\nsatellite.help(no_such_topic)\nsatellite.console.display("after them all")\n' | \
    "$interpreter" --repl > build/repl_refusals.out 2>&1
expect "the five spellings and a help with no topic are refused by name, and the line after them still runs" "1|1|1|1|1|1" \
       "$(grep -c 'a session has already taken satellite in' build/repl_refusals.out)|$(grep -c 'a block has nowhere to live' build/repl_refusals.out)|$(grep -c 'a capsule belongs to a program' build/repl_refusals.out)|$(grep -c 'nothing here to return from' build/repl_refusals.out)|$(grep -c 'there is no help on no_such_topic yet' build/repl_refusals.out)|$(grep -c '^after them all$' build/repl_refusals.out)"
# satellite.help() AND satellite.help(topic) READ THE TEXT FILES IN satellite.help/ (the
# author, 2026-09-22: "you just need to write text files ... then a function that reads
# them"): the list, a topic by its last part, and the same topic by its whole name.
printf 'satellite.help()\nsatellite.help(include)\nsatellite.help(satellite.include)\n' | "$interpreter" --repl > build/repl_help.out 2>&1; code=$?
expect "satellite.help() lists the topics, and include and satellite.include both find one" "0|1|2" \
       "$code|$(grep -c 'THE SATELLITE HELP SYSTEM' build/repl_help.out)|$(grep -c '^SATELLITE 004: satellite.include()$' build/repl_help.out)"
# A SECOND SPELLING FINDS ITS WORD'S TOPIC, AND A NAME TWO TOPICS SHARE NAMES BOTH.
printf 'satellite.help(double)\nsatellite.help(bin)\nsatellite.help(window)\n' | "$interpreter" --repl > build/repl_help2.out 2>&1
expect "satellite.help(double) is the float, (bin) the binary, and (window) names both window topics" "1|1|1" \
       "$(grep -c '^SATELLITE 004: satellite.variable.float$' build/repl_help2.out)|$(grep -c '^SATELLITE 004: satellite.variable.binary$' build/repl_help2.out)|$(grep -c 'window is more than one topic -- write satellite.help(satellite.variable.window) or satellite.help(satellite.window)' build/repl_help2.out)"
# A brace inside a string is text and not a block: the refusals are read from the CODES.
printf 'satellite.console.display("{ not a block }")\n' | "$interpreter" --repl 2>/dev/null | grep -q '{ not a block }'
expect "a brace inside a string literal is not a block" 0 $?
# `interpret <file>` AND `run <file>` (the author, 2026-09-22: "In 003, I would always
# type "interpret file.satl" into the prompt"): the prompt's own words, as `exit` is, and
# the program runs as `satl <file>` runs it -- its output, its refusals, its status. From
# a pipe the session's status is the first failing line's, so a failing program is it.
printf 'interpret examples/hello_world.satl\nrun "tests/missing_main.satl"\ninterpret\nsatellite.console.display("still here")\n' | \
    "$interpreter" --repl > build/repl_interpret.out 2>&1; code=$?
expect "interpret runs a program, run runs one that fails, and the session goes on" "11|1|1|1|1" \
       "$code|$(grep -c '^Hello, World!$' build/repl_interpret.out)|$(grep -c 'S102' build/repl_interpret.out)|$(grep -c '`interpret` needs a file after it' build/repl_interpret.out)|$(grep -c '^still here$' build/repl_interpret.out)"

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
# `**` spaced is `^` (SATELLITE_INFINITY.md, INF-1): the same token, so the same answers
# and the same right-to-left grouping, mixed with `^` too.
expect "** is the second spelling of ^: 2 ** 3 ** 2, 2 ^ 3 ^ 2, 2 ** 10, 3 ** 2 + 1, 2 ** 3 ^ 2" \
       "512|512|1024|10|512" "$("$interpreter" tests/power_stars.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/arithmetic.satl > /dev/null 2>&1; expect "tests/arithmetic.satl runs" 0 $?
"$interpreter" tests/divide_by_zero.satl > /dev/null 2>&1; expect "a divisor of zero" 22 $?
# 2 ^ -1 WAS REFUSED FOR WANT OF A FLOAT, AND IS ONE SINCE 2026-09-22 (SATELLITE_INFINITY.md
# FLT-2 and Q13): an answer refused for want of a float becomes a float.
expect "2 ^ -1 is the float 0.5 (FLT-2)" 0.5 "$("$interpreter" tests/negative_exponent.satl 2>/dev/null)"
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
expect "a binary with no b says ERROR: expected b10101010" 1 "$(grep -c 'ERROR: expected b10101010$' build/binary_b.out)"
expect "nothing ran before the missing b" "" "$(grep -x before build/binary_b.out)"
"$interpreter" tests/binary_assigned_without_b.satl > build/binary_b.out 2>&1; expect "a binary given a value with no b" 27 $?
expect "... and says ERROR: expected b1111" 1 "$(grep -c 'ERROR: expected b1111$' build/binary_b.out)"
"$interpreter" tests/binary_digits_not_binary.satl > build/binary_b.out 2>&1; expect "a binary given 12" 27 $?
expect "... is told 12 is not binary, not to write b12" 1 "$(grep -c 'ERROR: 12 is not binary' build/binary_b.out)"
"$interpreter" tests/binary_b_not_binary.satl > build/binary_b.out 2>&1; expect "a binary given b12" 27 $?
expect "... is told b12 is not binary" 1 "$(grep -c 'ERROR: b12 is not binary' build/binary_b.out)"
"$interpreter" tests/binary_in_brackets.satl > build/binary_b.out 2>&1; expect "a binary with no b inside brackets" 27 $?
expect "... is still ERROR: expected b10101010, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected b10101010$' build/binary_b.out)|$(grep -x before build/binary_b.out)"
"$interpreter" tests/binary_0b.satl > build/binary_b.out 2>&1; expect "0b10101010, the C spelling" 27 $?
expect "... is told ERROR: expected b10101010, not b0" 1 "$(grep -c 'ERROR: expected b10101010$' build/binary_b.out)"
# A binary keeps a sign -- our reading of words the author said about percentages and
# infinities (2026-09-17), which he kept on 2026-09-22: "I just didn't think of having
# negative binary numbers, but thats better than not having them anyways". -b0101 is a
# binary, worth -5, width kept.
expect "a binary below zero: shown, worth, converted, compared, turned over, given" \
       "-b0101|-5|-0101|-b0101|-5|-4|true|false|true|b0101|b0101|b0000|-b0011|-3" \
       "$("$interpreter" tests/binary_negative.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
# "we'll have to test it" (the same day): the sign through .string, .hex, order against a
# positive binary and against 0, .reverse, and arithmetic -- which answers a NUMBER today.
expect "a negative binary through every conversion, order, reverse and sum" \
       "-b0101|-5|-b0101|-5|-0101|false|true|true|-b0101|-b1010|b0000|0|-5" \
       "$("$interpreter" tests/binary_sign.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/binary_negative_without_b.satl > build/binary_b.out 2>&1; expect "a binary given -1010" 27 $?
expect "... says ERROR: expected -b1010, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -b1010$' build/binary_b.out)|$(grep -x before build/binary_b.out)"
"$interpreter" tests/binary_negative_assigned_without_b.satl > build/binary_b.out 2>&1; expect "a binary given (- 1111)" 27 $?
expect "... says ERROR: expected -b1111, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -b1111$' build/binary_b.out)|$(grep -x before build/binary_b.out)"
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
       "$(grep -c 'satl(check).*ERROR: expected 50%$' build/percentage.out)|$(grep -x before build/percentage.out)"
"$interpreter" tests/percentage_not_whole.satl > build/percentage.out 2>&1; expect "3 * 50% is not whole" 24 $?
expect "... and says so" 1 "$(grep -c '3 \* 50% is not a whole number' build/percentage.out)"
"$interpreter" tests/percentage_number_second.satl > build/percentage.out 2>&1; expect "50% + 5 is refused" 27 $?
# NEWLINES FLATTENED BEFORE THE GREP. The refusal is a full report now (S301,
# with the file, the line and a caret), and a report WRAPS at eighty columns --
# so a sentence this used to find on one line can arrive split across two. The
# assertion is about the words, not about where the report chose to break them.
expect "... and says to put the number first" 1 \
       "$(tr '\n' ' ' < build/percentage.out | grep -c 'the number goes first -- 5 + 50%')"
expect "a percentage below zero: -50%, (- 25%) and -(-12.5%)" "-50%|-25%|12.5%" \
       "$("$interpreter" tests/percentage_negative.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" tests/percentage_negative_without_percent.satl > build/percentage.out 2>&1; expect "a percentage given -50" 27 $?
expect "... says ERROR: expected -50%, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -50%$' build/percentage.out)|$(grep -x before build/percentage.out)"
"$interpreter" tests/percentage_negative_assigned_without_percent.satl > build/percentage.out 2>&1
expect "a percentage given (- 25)" 27 $?
expect "... says ERROR: expected -25%, before anything runs" "1|" \
       "$(grep -c 'satl(check).*ERROR: expected -25%$' build/percentage.out)|$(grep -x before build/percentage.out)"
expect "a declaration inside a loop runs every turn" "0|1|10|11|20|21" \
       "$("$interpreter" tests/loop_declaration.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
"$interpreter" examples/hello_world.satl > /dev/full 2> /dev/null; expect "output refused (/dev/full)" 2 $?

# ---------------------------------------------------------------------------
# THE WINDOW (SATELLITE_WINDOW.md WIN-3, 2026-09-20).
# ---------------------------------------------------------------------------
#
# EVERY ROW HERE RUNS WITH NO DISPLAY, ON PURPOSE, AND THAT IS THE LIMIT OF WHAT
# THIS FILE CAN PROVE. A row that opened a real window would put one on the
# screen of whoever ran check.sh -- and would then have to close it, or hang the
# suite. So what is asserted here is the shape: the words lex, the checker knows
# their arguments, a window's methods are known before anything runs, and a
# machine with no screen is REFUSED rather than left waiting. That a window
# actually appears was proved by running it and photographing the screen, which
# is not a thing a shell script can assert.
#
# NO DISPLAY MEANS NO XDG_RUNTIME_DIR EITHER. Unsetting WAYLAND_DISPLAY is not
# enough: libwayland falls back to $XDG_RUNTIME_DIR/wayland-0, so a run meant to
# be headless opens a window on the real desktop instead. That cost a window on
# the author's own screen on 2026-09-20, and this comment is why the line below
# is as long as it is.
rm -rf build/no_display && mkdir -p build/no_display
headless() { env -u DISPLAY -u WAYLAND_DISPLAY -u XAUTHORITY XDG_RUNTIME_DIR="$PWD/build/no_display" \
                 timeout 30 "$interpreter" "$@"; }

cat > build/window_new.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("a title", 800, 600)
    w.append(satellite.window.button("press me"), 400, 300)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_new.satl > build/window.out 2>&1
expect "a window on a machine with no screen is refused, not hung" 50 $?
expect "... with S730 NO_DISPLAY and the reason" "1|1" \
       "$(grep -c 'S730: NO_DISPLAY' build/window.out)|$(tr '\n' ' ' < build/window.out | grep -c 'there is no display to draw on')"

# THE AUTHOR'S 800x600 DOES NOT LEX AND IS NOT QUIETLY ACCEPTED (WIN-4): satl
# reads x600 as a hex literal, so `800x600` is two numbers side by side. The
# spelling built is three arguments, which is 003's own -- its removed word table
# has satellite.window.console.new(title, width, height).
cat > build/window_nxm.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("a title", 800x600)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_nxm.satl > build/window_nxm.out 2>&1
expect "800x600 does not lex, and is refused before anything runs (WIN-4)" "13|" \
       "$?|$(grep -x before build/window_nxm.out)"

# A WRONG ARGUMENT COUNT IS THE CHECKER'S, not the window's: nothing runs first.
cat > build/window_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("a title", 800)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_arity.satl > build/window_arity.out 2>&1
expect "satellite.window.new with two arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_arity.out)"

# A METHOD A WINDOW DOES NOT HAVE, also before anything runs.
cat > build/window_method.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("a title", 800, 600)
    satellite.console.display(w.read_all)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_method.satl > build/window_method.out 2>&1
expect "a window has no .read_all, and the checker says so first" "27|" \
       "$?|$(grep -x before build/window_method.out)"
expect "... and names what a window DOES have" 1 \
       "$(tr '\n' ' ' < build/window_method.out | grep -cF 'a window has .append(piece, across, down), .close(), .focus()')"

# THE WORDS ARE IN THE TABLE, at the numbers WIN-3 minted. 003 had these paths
# and they were REMOVED and their numbers REASSIGNED, so a row here that read
# 003's numbers would be a word pointing at the wrong library.
expect "satellite.window is 1 27, and its two calls 1 27 1 and 1 27 2" "1|1|1|1" \
       "$(grep -cP '^1 27\tsatellite.window\t' words/words.tsv)|$(grep -cP '^1 27 1\tsatellite.window.new\(title, width, height\)\t' words/words.tsv)|$(grep -cP '^1 27 2\tsatellite.window.button\(text\)\t' words/words.tsv)|$(grep -cP '^1 6 18\tsatellite.variable.window\t' words/words.tsv)"

# ---------------------------------------------------------------------------
# A BUTTON THAT TALKS BACK (SATELLITE_WINDOW.md WIN-11, 2026-09-21).
# ---------------------------------------------------------------------------
#
# WHAT THESE ROWS CAN AND CANNOT PROVE, and the line is the same as above: every
# row here is headless, so what is asserted is that the CHECKER knows `.pressed`
# and refuses the two ways of writing it wrongly BEFORE anything runs. That a
# real click really runs a real capsule was proved by pressing one --
# satellite/satellite_variable_window/press-a-button.sh opens a window on a
# compositor of its own, clicks the button three times and reads three lines
# back. It is not run from here because it would need mutter, a session bus and
# twenty seconds, and because check.sh must not open windows.
cat > build/window_pressed.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.variable.window b = satellite.window.button("press me")
    b.pressed(when_pressed)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_pressed.satl > build/window_pressed.out 2>&1
expect "a button wired to a capsule passes the checker, and stops only for want of a screen" 50 $?

# A CAPSULE NOBODY WROTE IS REFUSED WITH THE PROGRAM, not when somebody presses
# the button -- which is the whole reason `.pressed` takes a name and not text.
cat > build/window_nocapsule.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("press me")
    b.pressed(nobody_wrote_this)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_nocapsule.satl > build/window_nocapsule.out 2>&1
expect ".pressed naming a capsule nobody wrote is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_nocapsule.out)"
expect "... and says which capsule is missing" 1 \
       "$(tr '\n' ' ' < build/window_nocapsule.out | grep -cF 'no capsule named nobody_wrote_this')"

# AND TEXT IS NOT A NAME. `b.pressed("when_pressed")` would lex, run, and fail at
# the moment the button was pressed -- with the window already up, which is
# exactly the refusal the checker exists to move earlier.
cat > build/window_pressedtext.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("press me")
    b.pressed("when_pressed")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_pressedtext.satl > build/window_pressedtext.out 2>&1
expect ".pressed given text instead of a name is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_pressedtext.out)"
expect "... and shows the spelling it wanted" 1 \
       "$(tr '\n' ' ' < build/window_pressedtext.out | grep -cF 'takes the NAME of a capsule, written as it is written: .pressed(when_pressed)')"

# EVERY SPELLING, AND NOT JUST THE EASY ONE. The "takes a NAME" rule first lived
# beside the RECEIVER's own check, which sees only the FIRST method of a chain on
# a DECLARED name -- so both rows below drew a window and failed at the moment
# somebody pressed the button, which is the exact failure WIN-11 says it moved
# earlier. A fresh reader found it by running it (2026-09-21). The rule now lives
# where a whole statement is walked, and these two are why.
cat > build/window_chain1.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.append(satellite.window.button("y").pressed("no_such"), 400, 300)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_chain1.satl > build/window_chain1.out 2>&1
expect ".pressed with text on a word's ANSWER is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_chain1.out)"

cat > build/window_chain2.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("y")
    b.title("t").pressed("no_such")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_chain2.satl > build/window_chain2.out 2>&1
expect ".pressed with text as a chain's SECOND method is refused too" "27|" \
       "$?|$(grep -x before build/window_chain2.out)"

# ONE NAME AND NOTHING ELSE. Looking only at the code after the `(` let a second
# argument through to be refused at run time.
cat > build/window_twoargs.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("y")
    b.pressed(when_pressed, 5)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_twoargs.satl > build/window_twoargs.out 2>&1
expect ".pressed given a name and then junk is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_twoargs.out)"

# ---------------------------------------------------------------------------
# .press() -- THE PROGRAM PRESSING IT (the author, 2026-09-21: "It's just a mouse
# click, so it's not like there's any arguments to clicking on something").
# ---------------------------------------------------------------------------
#
# `.press` and `.pressed` are ONE LETTER APART, so each refuses the other's
# argument by naming the other out loud.
cat > build/window_press_arg.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("y")
    b.press(when_pressed)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_press_arg.satl > build/window_press_arg.out 2>&1
expect ".press given an argument is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_press_arg.out)"
expect "... and names .pressed(a_capsule), which is what was meant" 1 \
       "$(tr '\n' ' ' < build/window_press_arg.out | grep -cF 'To name what a press RUNS, that is .pressed(a_capsule)')"

cat > build/window_press_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.variable.window b = satellite.window.button("y")
    b.pressed(when_pressed)
    b.press()
    satellite.return(satellite)
}
WIN_EOF
headless build/window_press_ok.satl > build/window_press_ok.out 2>&1
expect "a button pressing itself passes the checker, and stops only for want of a screen" 50 $?

expect "press is a method token at 0000101100101010, generated into token_codes.hpp" "1|1" \
       "$(grep -c '^0000101100101010  press_token ' REGISTRY.satellite)|$(grep -c 'Code press_token = 0x0B2A;' satellite/bytecode/token_codes.hpp)"

# THE TOKEN IS MINTED ONCE, in the registry, and the header is GENERATED from it
# -- so a header edited by hand is a header this row catches.
expect "pressed is a method token at 0000101100101001, and token_codes.hpp was generated from it" "1|1" \
       "$(grep -c '^0000101100101001  pressed_token ' REGISTRY.satellite)|$(grep -c 'Code pressed_token = 0x0B29;' satellite/bytecode/token_codes.hpp)"

# ---------------------------------------------------------------------------
# A LABEL, AND THE PIECE TABLE UNDER IT (GTK_AND_NO_DEPENDENCIES.md GTK-1,
# 2026-09-21). The second piece satellite ever had, and the first one added
# through GTK-0's recipe rather than by hand.
# ---------------------------------------------------------------------------
#
# SAME LINE AS EVERY WINDOW ROW ABOVE: these run headless, so what is asserted is
# the word, the checker and the refusals that happen BEFORE a display is asked
# for. That a label really draws, reads its words back and has them written was
# proved on a compositor of its own -- an 800x600 window with the label appended,
# `.text` read, `.text("...")` written and read again, a button pressed and the
# window closed, exit 0.

expect "satellite.window.label is 1 27 3, the next free number under satellite.window" "1|1" \
       "$(grep -cP '^1 27 3\tsatellite.window.label\(text\)\t' words/words.tsv)|$(grep -c '1 27 3 -- satellite.window.label(text)' satellite/bytecode/word_codes.hpp)"

expect "text is a method token at 0000101100101011, and token_codes.hpp was generated from it" "1|1" \
       "$(grep -c '^0000101100101011  text_token ' REGISTRY.satellite)|$(grep -c 'Code text_token = 0x0B2B;' satellite/bytecode/token_codes.hpp)"

# THE ARGUMENT COUNT IS THE CHECKER'S, and it comes out of the ONE word table in
# window_calls.cpp -- a widget added to that table is refused correctly here
# without a second list being kept true.
cat > build/window_label_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window a = satellite.window.label("one", "two")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_label_arity.satl > build/window_label_arity.out 2>&1
expect "satellite.window.label with two arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_label_arity.out)"
expect "... and the sentence is the word table's own" 1 \
       "$(tr '\n' ' ' < build/window_label_arity.out | grep -cF 'satellite.window.label takes the text it shows')"

# A WINDOW'S WORDS ARE ITS TITLE, and a person who wrote .text on one meant
# .title -- so the refusal names the word they wanted rather than the one they
# did not get. Refused at the RUN and not the checker: `.text` is a real method
# on a real piece, and which piece the receiver is is not known until it is made.
cat > build/window_text_on_window.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.text("no")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_text_on_window.satl > build/window_text_on_window.out 2>&1
expect "a window asked for .text stops for want of a screen, not for want of the method" 50 $?
expect "... and .text is a method the CHECKER knows a window has" 0 \
       "$(tr '\n' ' ' < build/window_text_on_window.out | grep -cF 'is not built for')"

# THE SENTENCE THAT LISTS WHAT A PIECE DOES IS GENERATED, not typed -- it comes
# from window_calls.cpp, which is the file that knows. program_check.cpp kept its
# own copy of a list like this once and it was stale by the afternoon.
expect "a window has no .read_all, and the sentence naming .text comes from window_calls.cpp" 1 \
       "$(tr '\n' ' ' < build/window_method.out | grep -cF 'a piece in one has .text')"

# ONLY A BUTTON IS PRESSED, and a label is refused by the sentence that already
# refused a window -- one rule, not one a widget.
cat > build/window_label_press.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed()
{
    satellite.console.display("pressed")
}
satellite.capsule satellite.main()
{
    satellite.variable.window a = satellite.window.label("x")
    a.pressed(when_pressed)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_label_press.satl > build/window_label_press.out 2>&1
expect "a label wired to a capsule passes the checker, and stops only for want of a screen" 50 $?

# EVERY Piece HAS A NAME AND THE COMPILER PROVES IT. kPieceNames is sized by the
# enum with a static_assert, and window_pieces.cpp switches over every enumerator
# with no `default` -- so a widget added to the enum and left out of either one
# does not build. This row is what stops the two guards being quietly deleted.
expect "a Piece cannot be added without a name or a widget" "1|yes" \
       "$(grep -c 'static_assert(sizeof(kPieceNames)' satellite/satellite_variable_window/satellite_window.hpp)|$([ "$(grep -c 'case satellite_window::how_many_pieces: break;' satellite/satellite_variable_window/window_pieces.cpp)" -ge 1 ] && echo yes || echo no)"

# ---------------------------------------------------------------------------
# A PERSON TYPES (GTK_AND_NO_DEPENDENCIES.md GTK-2, 2026-09-21). A text box is
# one line and a text area is many, and they are the first pieces whose words
# belong to GTK rather than to satellite.
# ---------------------------------------------------------------------------
#
# WHAT THESE ROWS CAN PROVE IS THE CHECKER, as every window row here can. What
# was proved on a compositor of its own: a text box and a text area appended into
# an 800x600 window, `.text` read back through GtkEditable and through a
# GtkTextBuffer, `.text("...")` written and read again, and then -- after the
# window closed -- a closed LABEL still answering its words while a closed TEXT
# BOX refuses with S505 and says to read it while the window is open.

expect "text_box is 1 27 4 and text_area is 1 27 5, the next free numbers" "1|1" \
       "$(grep -cP '^1 27 4\tsatellite.window.text_box\(text\)\t' words/words.tsv)|$(grep -cP '^1 27 5\tsatellite.window.text_area\(text\)\t' words/words.tsv)"

cat > build/window_textbox_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window t = satellite.window.text_box("a", "b")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_textbox_arity.satl > build/window_textbox_arity.out 2>&1
expect "satellite.window.text_box with two arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_textbox_arity.out)"
expect "... and the sentence tells a person what an empty one is written as" 1 \
       "$(tr '\n' ' ' < build/window_textbox_arity.out | grep -cF 'satellite.window.text_box("")')"

# A TEXT AREA PASSES THE CHECKER AND STOPS ONLY FOR WANT OF A SCREEN -- which is
# what says the word, its arity and `.text` on it are all known before a line
# runs, and that the only thing missing here is a display.
cat > build/window_textarea_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window m = satellite.window.text_area("line one")
    m.text("line two")
    satellite.console.display(m.text)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_textarea_ok.satl > build/window_textarea_ok.out 2>&1
expect "a text area written and read passes the checker, and stops only for want of a screen" 50 $?

# `.append`'s REFUSAL NAMES EVERY PIECE THERE IS, and it is written out of the
# one word table. THE SENTENCE ITSELF CANNOT BE PROVED HERE: reaching `.append`
# needs a window, and a window needs a screen -- headless, satellite.window.new
# refuses first and the line never runs. It was proved on a compositor of its
# own, where `w.append(5, 400, 300)` answered
#
#     takes a piece to put in the window -- satellite.window.button("text"),
#     .label, .text_box or .text_area makes one -- and was given a number
#
# What IS proved here is that the list is GENERATED rather than typed, which is
# the thing that would silently rot: a hand-written list would keep passing the
# compositor test with the widget added this morning missing from it.
expect "the words that make a piece are generated, not typed into the refusal" "1|0" \
       "$(grep -c 'the_words_that_make_a_piece() + \" makes one' satellite/bytecode/window_methods.cpp)|$(grep -c 'satellite.window.button(.\"text.\") makes one' satellite/bytecode/window_calls.cpp)"

# WHAT WAS TYPED CANNOT BE RESCUED AT TEARDOWN, and the measurement that settled
# it is in window_desk.cpp: gtk_entry_dispose clears the text and
# gtk_text_view_dispose drops the buffer BEFORE either chains up to the dispose
# that emits `destroy`. This row is what stops the rescue being attempted a third
# time, and what keeps `text` single-writer.
expect "the desk does not read a piece's words while a window is being torn down" "0|1" \
       "$(grep -c 'window_keeps_what_was_typed' satellite/satellite_variable_window/window_desk.cpp)|$(grep -c 'no moment in a teardown' satellite/satellite_variable_window/window_desk.cpp)"

# ---------------------------------------------------------------------------
# ON AND OFF (GTK_AND_NO_DEPENDENCIES.md GTK-3, 2026-09-21). A checkbox and a
# switch, and `.on` read and written.
# ---------------------------------------------------------------------------
#
# PROVED ON A COMPOSITOR: both appended, `.on` false for each, `.on(1)` turning
# both on, `.on(0)` turning one off, a checkbox's `.text` answering its label, a
# switch displaying as (switch) because it has no words, and `.on` on a LABEL
# refused by name rather than answered false.

expect "checkbox is 1 27 6 and switch is 1 27 7" "1|1" \
       "$(grep -cP '^1 27 6\tsatellite.window.checkbox\(text\)\t' words/words.tsv)|$(grep -cP '^1 27 7\tsatellite.window.switch\t' words/words.tsv)"

expect "on is a method token at 0000101100101100, and token_codes.hpp was generated from it" "1|1" \
       "$(grep -c '^0000101100101100  on_token ' REGISTRY.satellite)|$(grep -c 'Code on_token = 0x0B2C;' satellite/bytecode/token_codes.hpp)"

# A SWITCH'S WORD TAKES NOTHING, which is the first piece of that shape -- so the
# table's own arity is what drives it, and giving it something is refused before
# anything runs.
cat > build/window_switch_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window s = satellite.window.switch("on")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_switch_arity.satl > build/window_switch_arity.out 2>&1
expect "satellite.window.switch given words is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_switch_arity.out)"
expect "... and says a switch is only on or off, in one argument and not 1 arguments" "1|1" \
       "$(tr '\n' ' ' < build/window_switch_arity.out | grep -cF 'a switch says nothing, it is only on or off')|$(tr '\n' ' ' < build/window_switch_arity.out | grep -cF 'was given 1 argument')"

# A WORD THAT TAKES NOTHING IS TWO ROWS, the name and the call -- which is
# satellite.infinity's own shape. With only the call registered,
# satellite.window.switch("on") matched no word at all and was refused as "no
# capsule named switch", which tells a person nothing.
expect "a word that takes nothing registers its NAME as well as its call" "1|1" \
       "$(grep -cP '^1 27 7\tsatellite.window.switch\t' words/words.tsv)|$(grep -cP '^1 27 7 0\tsatellite.window.switch\(\)\t' words/words.tsv)"

cat > build/window_on_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.window.checkbox("I agree")
    c.on(1)
    satellite.console.display(c.on)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_on_ok.satl > build/window_on_ok.out 2>&1
expect "a checkbox turned on and read back passes the checker, and stops only for want of a screen" 50 $?

# .on TAKES 1 OR 0, AND SAYS SO. satellite has no `true` to type (GTK-3 leaves
# that to the author), so a number is how a checkbox is turned on today -- and
# anything that is neither is refused with the sentence that explains it.
expect ".on says it takes 1 to turn it on and 0 to turn it off" 1 \
       "$(grep -c 'takes 1 to turn it on and 0 to turn it off' satellite/bytecode/window_readers.cpp)"

# A PIECE WITH NOTHING TO SAY DISPLAYS AS JUST ITSELF -- (switch), never
# (switch "").
expect "a wordless piece displays with no empty quotes" 1 \
       "$(grep -c 'which->text.empty()' satellite/satellite_object/satellite_object.cpp)"

# ---------------------------------------------------------------------------
# THE RADIO (GTK_AND_NO_DEPENDENCIES.md GTK-3's leftover, 2026-09-22), BUILT AS
# THE RECOMMENDATION and still reversible: `satellite.window.one_of(a_list)` is
# ONE word that draws MANY check buttons, each after the first given the first
# as its group (gtk_check_button_set_group) so that exactly one is ticked, and
# asked `.chosen` as a choice is. The other spelling in the plan --
# `.group(other_checkbox)` on a checkbox -- stays the author's to ask for.
#
# PROVED ON A COMPOSITOR (prove-canvas-tabs-menus.sh): three words, .chosen
# reading the first from the start, .chosen("large") and reading it back, and a
# REAL POINTER CLICK on the middle button running the .changed capsule with
# .chosen answering "medium".
# ---------------------------------------------------------------------------
expect "one_of is 1 27 22, made from a list" 1 \
       "$(grep -cP '^1 27 22\tsatellite.window.one_of\(items\)\t' words/words.tsv)"
expect "a one-of is one piece: every button grouped to the first, and the first ticked" "1|1" \
       "$(grep -c 'gtk_check_button_set_group(GTK_CHECK_BUTTON(button), first)' satellite/satellite_variable_window/window_pieces.cpp)|$(grep -c 'gtk_check_button_set_active(first, TRUE)' satellite/satellite_variable_window/window_pieces.cpp)"
expect "a one-of answers .chosen and .changed, as a choice does" "1|1" \
       "$(grep -c 'which.piece == satellite_window::one_of;' satellite/satellite_variable_window/window_state.cpp)|$(grep -c 'case satellite_window::one_of:' satellite/satellite_variable_window/window_answers.cpp)"

cat > build/window_oneof_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window g = satellite.window.one_of()
    satellite.return(satellite)
}
WIN_EOF
headless build/window_oneof_arity.satl > build/window_oneof_arity.out 2>&1
expect "satellite.window.one_of given nothing is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_oneof_arity.out)"
expect "... and says it takes a list of what a person may pick one of" 1 \
       "$(tr '\n' ' ' < build/window_oneof_arity.out | grep -cF 'takes a list of what a person may pick one of')"

cat > build/window_oneof_empty.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list nothing = {}
    satellite.variable.window g = satellite.window.one_of(nothing)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_oneof_empty.satl > build/window_oneof_empty.out 2>&1
expect "a one-of of nothing is refused, and NOT as a machine with no screen" 13 $?
expect "... and says a one-of needs something to choose from" 1 \
       "$(tr '\n' ' ' < build/window_oneof_empty.out | grep -cF 'a one-of needs something to choose from')"

cat > build/window_oneof_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_picked(satellite.variable.window the_group)
{
    satellite.console.display(the_group.chosen)
}
satellite.capsule satellite.main()
{
    satellite.variable.window g = satellite.window.one_of({"small", "medium", "large"})
    g.chosen("large")
    satellite.console.display(g.chosen)
    g.changed(when_picked)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_oneof_ok.satl > build/window_oneof_ok.out 2>&1
expect "a one-of, .chosen read and written and .changed pass the checker, and stop only for want of a screen" 50 $?

# A CHOICE OR A ONE-OF WITH NO SCREEN TO DRAW IT ON IS THE MACHINE'S ANSWER AND
# NOT THE PROGRAM'S (found 2026-09-22 by running the one-of headless). The
# items branch of call_window_word marked EVERY failure as the program's, so a
# valid choice on a machine with no display printed S110 LINE_NOT_UNDERSTOOD
# over a sentence saying there was no display -- the code and the sentence
# disagreeing, which that file has now fixed four times. Only an EMPTY list is
# the program's, and that is what is tested now, as the factory tests it.
cat > build/window_choice_nodisplay.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.window.choice({"red", "green"})
    satellite.return(satellite)
}
WIN_EOF
headless build/window_choice_nodisplay.satl > build/window_choice_nodisplay.out 2>&1
expect "a choice of two on a machine with no screen is S730 NO_DISPLAY, not S110" "50|1" \
       "$?|$(grep -c 'S730: NO_DISPLAY' build/window_choice_nodisplay.out)"

# ---------------------------------------------------------------------------
# A NUMBER A PERSON CHOOSES (GTK_AND_NO_DEPENDENCIES.md GTK-4, 2026-09-21): a
# slider, a number box and a progress bar, and `.value` read and written.
# ---------------------------------------------------------------------------
#
# PROVED ON A COMPOSITOR, and the percentage round trip is the part worth saying:
# `getting_on.value(12.5%)` read back as exactly 12.5%, and 100% as 100%. A
# percentage is held as itself times 10^32 and the desk speaks millionths, so one
# millionth of the whole is exactly 10^28 -- both conversions are one multiply or
# one divide and NOTHING rounds on satellite's side. Also proved: a number on a
# progress bar refused by name, a percentage on a slider refused by name, and a
# slider whose least is not smaller than its most refused before a window opens.

expect "slider is 1 27 8, number_box 1 27 9, progress 1 27 10 and its call 1 27 10 0" "1|1|1|1" \
       "$(grep -cP '^1 27 8\tsatellite.window.slider\(least, most\)\t' words/words.tsv)|$(grep -cP '^1 27 9\tsatellite.window.number_box\(least, most\)\t' words/words.tsv)|$(grep -cP '^1 27 10\tsatellite.window.progress\t' words/words.tsv)|$(grep -cP '^1 27 10 0\tsatellite.window.progress\(\)\t' words/words.tsv)"

expect "value is a method token at 0000101100101101, and token_codes.hpp was generated from it" "1|1" \
       "$(grep -c '^0000101100101101  value_token ' REGISTRY.satellite)|$(grep -c 'Code value_token = 0x0B2D;' satellite/bytecode/token_codes.hpp)"

cat > build/window_slider_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window s = satellite.window.slider(100)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_slider_arity.satl > build/window_slider_arity.out 2>&1
expect "satellite.window.slider with one number is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_slider_arity.out)"
expect "... and says it takes the least and the most" 1 \
       "$(tr '\n' ' ' < build/window_slider_arity.out | grep -cF 'takes the least and the most it runs between')"

cat > build/window_value_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window s = satellite.window.slider(0, 100)
    s.value(50)
    satellite.console.display(s.value)
    satellite.variable.window p = satellite.window.progress()
    p.value(12.5%)
    satellite.console.display(p.value)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_value_ok.satl > build/window_value_ok.out 2>&1
expect "a slider and a progress bar written and read pass the checker, and stop for want of a screen" 50 $?

# THE MILLIONTH IS 10^28 AND IT IS NOT A MAGIC NUMBER: a percentage is held as
# itself times 10^32, so the whole of it is 10^34 and a millionth of that is
# 10^28. This row is what catches somebody "tidying" the two factors.
expect "a millionth of the whole is 10^16 times 10^12" 1 \
       "$(grep -c 'satellite_number(10000000000000000ull) \* satellite_number(1000000000000ull)' satellite/bytecode/window_readers.cpp)"

# A SLIDER'S VALUE IS NOT PIXELS. place_of is borrowed for it, and it was saying
# "takes a number of pixels" in the one place a person reads carefully.
expect "the borrowed number reader names what the number is OF" 1 \
       "$(grep -c 'const char \*units = \"a number of pixels\"' satellite/bytecode/window_readers.hpp)"

# A WORD WHOSE ONLY ROW TAKES TWO ARGUMENTS USED TO FALL OUT OF THE LEXER
# ENTIRELY (found by GTK-4, 2026-09-21). shaped_word_code had two answers -- a
# row with exactly as many parameters as the call, and the ONE-PARAMETER row as
# a fallback -- and `satellite.window.slider(100)` matched neither, so a word
# that exists was refused as "no capsule named slider".
#
# satellite.window.new HAD THE SAME HOLE SINCE WIN-3 and nobody had seen it,
# because the row that tested it only ever asserted the exit code: "no capsule
# named new" and "takes a title, a width and a height" are both 13.
expect "a window word called with the wrong number of arguments is named, not denied" "1|1" \
       "$(tr '\n' ' ' < build/window_arity.out | grep -cF 'satellite.window.new takes a title, a width and a height')|$(tr '\n' ' ' < build/window_slider_arity.out | grep -cF 'satellite.window.slider takes the least and the most')"
expect "... and neither is called a capsule nobody wrote" "0|0" \
       "$(grep -c 'no capsule named new' build/window_arity.out)|$(grep -c 'no capsule named slider' build/window_slider_arity.out)"

# A CODE AND A SENTENCE MUST AGREE. A slider on a machine with no screen exited
# 13 while printing "there is no display to draw on", because the refusal asked
# whether the WORD takes numbers instead of whether the NUMBERS were wrong.
expect "a piece that fails for want of a screen exits 50, whatever its word takes" 50 \
       "$(headless build/window_value_ok.satl > build/window_value_ok.out 2>&1; echo $?)"

# ---------------------------------------------------------------------------
# A LIST TO CHOOSE FROM (GTK_AND_NO_DEPENDENCIES.md GTK-5, 2026-09-21). The
# first piece made out of a satellite CONTAINER rather than out of words.
# ---------------------------------------------------------------------------
#
# PROVED ON A COMPOSITOR: a choice made from {"red", "green", "blue"}, `.chosen`
# answering "red", `.chosen("blue")` picking it, and `.chosen("purple")` REFUSED
# -- gtk_drop_down_set_selected on a position that is not there simply picks
# nothing, and a program that named an item this choice does not offer has said
# something untrue about itself.

expect "choice is 1 27 11" 1 \
       "$(grep -cP '^1 27 11\tsatellite.window.choice\(items\)\t' words/words.tsv)"

expect "chosen is a method token at 0000101100101110, and token_codes.hpp was generated from it" "1|1" \
       "$(grep -c '^0000101100101110  chosen_token ' REGISTRY.satellite)|$(grep -c 'Code chosen_token = 0x0B2E;' satellite/bytecode/token_codes.hpp)"

# A CHOICE TAKES A LIST AND NOTHING ELSE, and the refusal names the kind it got.
cat > build/window_choice_kind.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.window.choice("red")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_choice_kind.satl > build/window_choice_kind.out 2>&1
expect "satellite.window.choice given text and not a list is refused" 27 $?
expect "... and says it takes a list" 1 \
       "$(tr '\n' ' ' < build/window_choice_kind.out | grep -cF 'satellite.window.choice takes a list, and was given a string')"

# AN EMPTY LIST IS A CONTROL A PERSON CAN DO NOTHING WITH, and it is refused
# where it is written rather than drawn as an empty box.
cat > build/window_choice_empty.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.container.list nothing = {}
    satellite.variable.window c = satellite.window.choice(nothing)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_choice_empty.satl > build/window_choice_empty.out 2>&1
expect "a choice of nothing is refused, and NOT as a machine with no screen" 13 $?
expect "... and says it needs something to choose from" 1 \
       "$(tr '\n' ' ' < build/window_choice_empty.out | grep -cF 'a choice needs something to choose from')"

# A DOING-METHOD THAT FAILS WITH THE WINDOW WIDE OPEN MUST NOT SAY S505. Every
# doing could once fail for one reason -- the window had gone -- and the generic
# tail said so for all of them. `a_choice.chosen("purple")` broke that.
expect "the refusal code follows what happened, not what the tail used to assume" 1 \
       "$(grep -c 'context.refuse(window->widget == nullptr ? window_is_closed : types_do_not_meet,' satellite/bytecode/window_methods.cpp)"

# ---------------------------------------------------------------------------
# ROWS, COLUMNS AND A GRID (GTK_AND_NO_DEPENDENCIES.md GTK-7, 2026-09-21) -- the
# first pieces that HOLD other pieces, and the first change to `.append`.
# ---------------------------------------------------------------------------
#
# PROVED ON A COMPOSITOR: a row of two buttons and a column of two labels and a
# grid of three labels all appended into one window; `a_row.append(one)` with no
# coordinates; `a_grid.append(label, 1, 1)` by CELL counting from 1; and -- the
# part worth having -- a button pressed INSIDE A ROW handing its capsule the
# WINDOW, not the row.
#
# THAT LAST ONE WAS A REAL BUG NESTING WOULD HAVE INTRODUCED. `inside_of` names
# the immediate parent, so a press in a row would have handed a capsule a row
# where it declared a window, and `its_window.close()` would have answered "only
# a window can be closed" -- a refusal about a line that is right.
# the_window_holding() walks the rest of the way.

expect "row is 1 27 12, column 1 27 13 and grid 1 27 14, each with its call" "1|1|1|1|1|1" \
       "$(grep -cP '^1 27 12\tsatellite.window.row\t' words/words.tsv)|$(grep -cP '^1 27 12 0\tsatellite.window.row\(\)\t' words/words.tsv)|$(grep -cP '^1 27 13\tsatellite.window.column\t' words/words.tsv)|$(grep -cP '^1 27 13 0\tsatellite.window.column\(\)\t' words/words.tsv)|$(grep -cP '^1 27 14\tsatellite.window.grid\t' words/words.tsv)|$(grep -cP '^1 27 14 0\tsatellite.window.grid\(\)\t' words/words.tsv)"

# `.append` TAKES ONE ARGUMENT **OR** THREE, and the checker lets both through
# because which is right depends on what the receiver turned out to BE -- a
# satellite.variable.window name may hold a window or a row, and that is not
# decided until the line that makes it runs.
cat > build/window_append_one.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window r = satellite.window.row()
    r.append(satellite.window.button("a"))
    satellite.return(satellite)
}
WIN_EOF
headless build/window_append_one.satl > build/window_append_one.out 2>&1
expect ".append with one argument passes the checker, and stops only for want of a screen" 50 $?

cat > build/window_append_two.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window r = satellite.window.row()
    r.append(satellite.window.button("a"), 10)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_append_two.satl > build/window_append_two.out 2>&1
expect ".append with TWO arguments is still refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_append_two.out)"

# A CONTAINER NESTS, SO THE TEARDOWN HAS TO WALK. GTK frees the whole tree with
# the window; walking only the window's direct children would leave a button in
# a row holding a GtkWidget * that has been freed -- and it would read as a live
# button right up until something touched it.
# AND SINCE A MENU CAN HOLD MENUS (GTK-12's submenus, 2026-09-22) the walk asks
# `pieces` itself and not holds_pieces(): a menu refuses .append and holds
# pieces all the same, and holds_pieces() would have walked past a submenu and
# left it open with its window gone.
expect "a window going away lets go of every piece, however deep" "1|1" \
       "$(grep -c 'let_go_of_every_piece(\*open_windows\[at\])' satellite/satellite_variable_window/window_desk.cpp)|$(grep -c 'if (!piece->pieces.empty())' satellite/satellite_variable_window/window_desk.cpp)"

# AND A PRESS REPORTS THE WINDOW, NEVER THE ROW.
expect "a press walks up to the window it happened in" "1|0" \
       "$(grep -c 'the_window_holding(\*piece)' satellite/satellite_variable_window/window_desk.cpp)|$(grep -c 'piece->inside_of.lock()});' satellite/satellite_variable_window/window_desk.cpp)"

# A LOOP IN `inside_of` WOULD BE A HANG INSIDE A PRESS, which is the one place a
# hang looks exactly like satl locking up. .append refuses to make one, and
# the_window_holding caps its walk anyway.
expect "a piece cannot be put inside something it already holds" "1|1" \
       "$(grep -c 'a piece cannot be put inside something it already holds' satellite/satellite_variable_window/satellite_window.cpp)|$(grep -c 'steps < 4096' satellite/satellite_variable_window/satellite_window.cpp)"

# ---------------------------------------------------------------------------
# A PICTURE (GTK_AND_NO_DEPENDENCIES.md GTK-6, 2026-09-21), and the four
# vendored projects it makes reachable.
# ---------------------------------------------------------------------------
#
# gdk-pixbuf, libpng, libjpeg-turbo and libtiff are ~2.6 MB of satl and had been
# carried since the first vendored build with NO satellite word able to touch a
# line of them (Part 00, Table B). satellite.window.picture is what earns them.
#
# PROVED ON A COMPOSITOR with real files written by GdkPixbuf: a 64x48 PNG
# loaded and shown, `.path` reading it back, `.path("...jpg")` swapping it for a
# JPEG -- which is libpng and libjpeg-turbo both reached from a satellite
# program -- and `.text` on a picture refused and sent to `.path`.

expect "picture is 1 27 15" 1 \
       "$(grep -cP '^1 27 15\tsatellite.window.picture\(path\)\t' words/words.tsv)"

# A FILE THAT IS NOT THERE IS REFUSED WHERE IT IS WRITTEN, and with a FILE's
# machine code. gtk_picture_new_for_filename would have handed back a widget
# that draws nothing and says nothing -- a person seeing an empty space where
# their logo should be, with no way to find out why. gdk_texture_new_from_filename
# is used instead because it is the one with a GError.
cat > build/window_picture_missing.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    satellite.variable.window p = satellite.window.picture("build/no_such_picture.png")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_picture_missing.satl > build/window_picture_missing.out 2>&1
expect "a picture of a file that is not there stops for want of a SCREEN when headless" 50 $?

# ... and the file's own codes are the ones it uses when there IS a screen. The
# row above cannot reach them: reading a file needs GDK started, so a headless
# run says NO_DISPLAY first and is right to.
expect "a bad picture file answers file_not_found or file_unreadable, never line_not_understood" "1|1" \
       "$(grep -c '? file_not_found' satellite/bytecode/window_calls.cpp)|$(grep -c ': file_unreadable' satellite/bytecode/window_calls.cpp)"

# A PICTURE'S WORDS ARE ITS FILE, and `.text` says so rather than answering
# something else -- the same pairing a window has with `.title`.
expect "a picture asked for .text is sent to .path" "2" \
       "$(grep -cF 'a picture'"'"'s words are the file it shows -- write .path' satellite/satellite_variable_window/window_asks.cpp)"

# A FAILED .path LEAVES THE OLD PICTURE AND THE OLD ANSWER. Writing the new path
# before checking would have left a piece saying it shows a file it does not.
expect "a picture that could not be swapped still says what it actually shows" 1 \
       "$(grep -cF 'if (!swapped)' satellite/satellite_variable_window/window_pieces.cpp)"

# ---------------------------------------------------------------------------
# EVERY PIECE TALKS BACK (GTK_AND_NO_DEPENDENCIES.md GTK-9, 2026-09-21).
# ---------------------------------------------------------------------------
#
# WIN-11 built the queue for one signal. GTK-9 is two more methods and a rename:
# `APress` became `AnEvent` and `the_desk_saw_a_press` became
# `the_desk_saw_something`, because a text box typed in, a slider moved, a
# checkbox ticked and a window closed all arrive on that same queue now, and a
# name saying "press" for all five would be a comment that lies.
#
# PROVED ON A COMPOSITOR: `.changed` on a text box reading what it now says;
# `.changed` on a slider; `.closed` on a window running AFTER it went away and
# still answering `.title`; and three `s.value()` calls in a row collapsing into
# ONE capsule run that read 44 -- the latest.

expect "changed and closed are method tokens at 0000101100101111 and 0000101100110000" "1|1|1|1" \
       "$(grep -c '^0000101100101111  changed_token ' REGISTRY.satellite)|$(grep -c 'Code changed_token = 0x0B2F;' satellite/bytecode/token_codes.hpp)|$(grep -c '^0000101100110000  closed_token ' REGISTRY.satellite)|$(grep -c 'Code closed_token = 0x0B30;' satellite/bytecode/token_codes.hpp)"

# EXTENDING ONE PREDICATE EXTENDED THE WHOLE CHECKER. Every rule WIN-11 wrote
# for `.pressed` -- the name is read as written and not as text, only one name
# may stand there, the capsule must exist, it may declare at most the piece and
# its window -- holds for `.changed` and `.closed` without a line changing.
cat > build/window_changed_nocapsule.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window a = satellite.window.text_box("x")
    a.changed(nobody_wrote_this)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_changed_nocapsule.satl > build/window_changed_nocapsule.out 2>&1
expect ".changed wired to a capsule nobody wrote is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_changed_nocapsule.out)"
expect "... and the sentence names .changed and not .pressed" 1 \
       "$(tr '\n' ' ' < build/window_changed_nocapsule.out | grep -cF '.changed(...) names a capsule to run')"

cat > build/window_changed_text.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_changed()
{
    satellite.console.display("changed")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window a = satellite.window.text_box("x")
    a.changed("when_changed")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_changed_text.satl > build/window_changed_text.out 2>&1
expect ".changed given TEXT where a name goes is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_changed_text.out)"

# A CAPSULE WIRED TO A PIECE NOTHING A PERSON CAN CHANGE WOULD NEVER RUN, which
# is the quietest possible way for a program to be wrong. A BUTTON is refused
# too and sent to `.pressed` -- a button is not changed, it is pressed.
cat > build/window_changed_label.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule c()
{
    satellite.console.display("ran")
}
satellite.capsule satellite.main()
{
    satellite.variable.window a = satellite.window.label("x")
    a.changed(c)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_changed_label.satl > build/window_changed_label.out 2>&1
expect ".changed on a label passes the checker, and stops only for want of a screen" 50 $?
expect "... and the sentence that WOULD refuse it says a capsule there would never run" 1 \
       "$(grep -c 'so a capsule here would never run' satellite/satellite_variable_window/window_answers.cpp)"
expect "... and a button is sent to .pressed rather than told it cannot change" 1 \
       "$(grep -c 'a button is not changed, it is pressed' satellite/satellite_variable_window/window_answers.cpp)"

# A DRAG IS ONE CHANGE AND THREE CLICKS ARE THREE PRESSES. Consecutive identical
# events collapse; a press never does, and press-a-button.sh counts on it.
expect "a change may collapse and a press may not" "1|1" \
       "$(grep -c 'the_desk_saw_something(piece->when_changed, piece->shared_from_this(), true)' satellite/satellite_variable_window/window_answers.cpp)|$(grep -c 'the_desk_saw_something(button->when_pressed, button->shared_from_this())' satellite/satellite_variable_window/window_answers.cpp)"

# THE QUEUE IS DRAINED BEFORE THE WINDOWS ARE TESTED, which is what guarantees a
# window's own `.closed` capsule runs at all: that one is queued BY the handler
# that empties open_windows.
expect "the queue is tested before the windows, so .closed is never lost" 1 \
       "$(grep -c 'THE QUEUE FIRST, AND THAT ORDER IS THE POINT' satellite/satellite_variable_window/window_desk.cpp)"

# ---------------------------------------------------------------------------
# THE WINDOW'S OWN SHAPE (GTK_AND_NO_DEPENDENCIES.md GTK-8, 2026-09-21).
# ---------------------------------------------------------------------------
#
# PROVED ON A COMPOSITOR: `w.width` answering 800 before anything was mapped --
# the size it ASKED for, because gtk_widget_get_width is 0 until a compositor
# has given it one, and 0 is not what a program that wrote new("t", 800, 600)
# means; `b.width` answering 105 for a button that is in NO window, which is its
# measured natural size and the very number `.append` uses to centre it; and
# `w.width` still answering 800 after `w.resize(1024, 768)` -- the compositor
# declined, and `.width` reports what IS rather than what was asked for. A
# window that reported its wish would be the failure this project keeps naming.

expect "width, height and fullscreen are method tokens 0000101100110001..0011" "1|1|1|1|1|1" \
       "$(grep -c '^0000101100110001  width_token ' REGISTRY.satellite)|$(grep -c 'Code width_token = 0x0B31;' satellite/bytecode/token_codes.hpp)|$(grep -c '^0000101100110010  height_token ' REGISTRY.satellite)|$(grep -c 'Code height_token = 0x0B32;' satellite/bytecode/token_codes.hpp)|$(grep -c '^0000101100110011  fullscreen_token ' REGISTRY.satellite)|$(grep -c 'Code fullscreen_token = 0x0B33;' satellite/bytecode/token_codes.hpp)"

# `.resize` IS A TOKEN THAT ALREADY EXISTED -- infinity.resize(n), 0x0B25. One
# name, one meaning, many kinds of thing, which is the shape `.append` has had
# since a file and a list shared it.
expect ".resize is the token infinity already had, not a second one" 1 \
       "$(grep -c 'Code resize_token = 0x0B25;' satellite/bytecode/token_codes.hpp)"

cat > build/window_shape_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.resize(1024, 768)
    satellite.console.display(w.width)
    satellite.console.display(w.fullscreen)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_shape_ok.satl > build/window_shape_ok.out 2>&1
expect ".resize, .width and .fullscreen pass the checker, and stop only for want of a screen" 50 $?

# ONLY A WINDOW IS GIVEN A SIZE. A piece inside one is sized by what holds it,
# and a button told to be 300 wide in a row is a button arguing with the row.
expect "a piece is not given a size, and the sentence says why" 1 \
       "$(grep -c 'a piece inside one is sized by what holds it' satellite/satellite_variable_window/satellite_window.cpp)"

# GTK4 HAS NO WINDOW ICON FROM A FILE, so `.icon(path)` is NOT built and is not
# pretended. A Wayland compositor takes a window's icon from the .desktop file
# it matches by app id; gtk_window_set_icon_name takes a THEME name, not a path.
# Written down so nobody adds a word that quietly does nothing.
expect "no .icon method was minted for a window" 0 \
       "$(grep -c 'icon_token' satellite/bytecode/token_codes.hpp)"

# ---------------------------------------------------------------------------
# THE LOOK (GTK_AND_NO_DEPENDENCIES.md GTK-10, 2026-09-21): colour, what is
# behind a piece, and the font its words are drawn in.
# ---------------------------------------------------------------------------
#
# GTK4 HAS NO PER-WIDGET COLOUR SETTER AT ALL -- everything is CSS -- so this is
# really "satellite generates a stylesheet". One css class and one provider a
# piece: a provider added to the DISPLAY would style every window, and the class
# is what keeps a rule to the piece that asked for it.
#
# PROVED ON A COMPOSITOR: a window given a background, a label given #00ff88 and
# IBM Plex Mono at 24, a button given `red` through the `.color` spelling and
# rgb(30, 30, 40) behind it -- no GTK warning and exit 0 -- and then
# `.colour("notacolour")` REFUSED, which is what says the good ones parsed.

expect "colour has two spellings and background and font have one each" "1|1|1" \
       "$(grep -c 'if (spelling == \"color\") return colour_token;' satellite/bytecode/token_codes.hpp)|$(grep -c 'Code background_token = 0x0B35;' satellite/bytecode/token_codes.hpp)|$(grep -c 'Code font_token = 0x0B36;' satellite/bytecode/token_codes.hpp)"

# A RULE GTK CANNOT READ IS **DROPPED** and gtk_css_provider_load_from_string
# answers nothing at all -- the piece stays as it was and the program carries on
# believing it asked for something. The `parsing-error` signal is the only way to
# hear about it, and without this row somebody could delete the handler and every
# test here would still pass.
expect "a stylesheet GTK cannot read is a refusal, not a silent no-op" "1|2" \
       "$(grep -c 'g_signal_connect(wearing, \"parsing-error\"' satellite/satellite_variable_window/window_look.cpp)|$(grep -c 'GTK could not read' satellite/satellite_variable_window/window_look.cpp)"

# AND A REFUSED COLOUR LEAVES THE PIECE WEARING WHAT IT WAS WEARING, not a rule
# with a hole in it where the bad line was dropped.
expect "a refused colour puts the old one back" 1 \
       "$(grep -c 'THE OLD ONE IS PUT BACK IF GTK WILL NOT HAVE THE NEW ONE' satellite/satellite_variable_window/window_look.cpp)"

cat > build/window_look_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.background("#202028")
    satellite.variable.window a = satellite.window.label("x")
    a.colour("#00ff88")
    a.font("IBM Plex Mono", 12)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_look_ok.satl > build/window_look_ok.out 2>&1
expect ".colour, .background and .font pass the checker, and stop only for want of a screen" 50 $?

# THE FONT satl CARRIES IS FINDABLE BY THE NAME A PROGRAM WOULD WRITE. Measured
# 2026-09-21 through satl's OWN spilled fontconfig, not the machine's:
#
#     FONTCONFIG_FILE=$SPILL/fonts.conf fc-match "IBM Plex Mono"
#     IBMPlexMono-Regular.ttf: "IBM Plex Mono" "Regular"
#
# AND fc-match "NoSuchFamilyAtAll" ANSWERS THE SAME FILE, which is the asymmetry
# worth knowing: a bad COLOUR is refused and a bad FONT FAMILY cannot be,
# because fontconfig always answers something. This row keeps the family name
# the spill provides matching the one the documentation tells people to write.
expect "the carried font family is spelled the way a program would write it" "1|1" \
       "$(grep -c 'IBM Plex Mono' satellite/satellite_variable_window/satellite_window.hpp)|$(ls vendor/fonts/ibm-plex-mono/IBMPlexMono-Regular.ttf >/dev/null 2>&1 && echo 1 || echo 0)"

# ---------------------------------------------------------------------------
# MORE THAN ONE SCREENFUL (GTK_AND_NO_DEPENDENCIES.md GTK-16, 2026-09-21): a
# scroll, a frame and a split. TABS ARE NOT BUILT and the reason is below.
# ---------------------------------------------------------------------------
#
# PROVED ON A COMPOSITOR: a frame with words on its edge holding a label, a
# scroll holding a text area, a split holding a label either side, all three
# appended into one window -- and a second `.append` into the scroll REFUSED.
#
# THE REFUSAL IS THE POINT. gtk_scrolled_window_set_child on a scroll that
# already has one silently DROPS the first: the piece is still a piece, the
# program still holds it, and it is simply not on the screen any more and
# nothing said so.

expect "scroll is 1 27 16, frame 1 27 17 and split 1 27 18" "1|1|1" \
       "$(grep -cP '^1 27 16\tsatellite.window.scroll\t' words/words.tsv)|$(grep -cP '^1 27 17\tsatellite.window.frame\(title\)\t' words/words.tsv)|$(grep -cP '^1 27 18\tsatellite.window.split\t' words/words.tsv)"

expect "a holder that holds a fixed number refuses the one too many" 1 \
       "$(grep -c 'and it already has' satellite/satellite_variable_window/satellite_window.cpp)"

cat > build/window_holders_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window f = satellite.window.frame("edge")
    f.append(satellite.window.label("inside"))
    satellite.variable.window s = satellite.window.scroll()
    s.append(satellite.window.text_area(""))
    satellite.variable.window p = satellite.window.split()
    p.append(satellite.window.label("left"))
    p.append(satellite.window.label("right"))
    satellite.console.display(f.text)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_holders_ok.satl > build/window_holders_ok.out 2>&1
expect "a frame, a scroll and a split pass the checker, and stop only for want of a screen" 50 $?

# TABS ARE BUILT (2026-09-22), AND THE RECOMMENDATION IS WHAT WAS BUILT: the
# PIECE carries its tab's name -- `the_piece.title("Open files")`, then
# `a_tabs.append(the_piece)` -- so `.append` kept its two shapes and grew no
# third. Until then a row here asserted that no tabs word existed, so that
# nobody minted one without answering that question; it was answered the way
# GTK-16 recommended, and the plan keeps the question as still reversible.
#
# PROVED ON A COMPOSITOR: two pages named First and Second; .chosen read First,
# .chosen("Second") brought the second forward and .chosen read it back; a page
# renamed with .title while in the set was relabelled in place; and a REAL
# POINTER CLICK on the renamed tab ran the .changed capsule, whose .chosen read
# the new name, and closed the window -- exit 0.
expect "tabs is 1 27 21, made from nothing, and canvas is 1 27 20, made from a size" "1|1|1" \
       "$(grep -cP '^1 27 21\tsatellite.window.tabs\t' words/words.tsv)|$(grep -cP '^1 27 21 0\tsatellite.window.tabs\(\)\t' words/words.tsv)|$(grep -cP '^1 27 20\tsatellite.window.canvas\(width, height\)\t' words/words.tsv)"
expect "a tab is named by its piece's .title, and an unnamed piece is refused at the tabs" "1|1" \
       "$(grep -c 'gtk_notebook_set_tab_label_text' satellite/satellite_variable_window/satellite_window.cpp)|$(grep -c 'a tab is named by its piece' satellite/satellite_variable_window/satellite_window.cpp)"
expect "a set of tabs answers .chosen and .changed, as a choice does" "1|1" \
       "$(grep -c 'piece == satellite_window::choice || which.piece == satellite_window::tabs' satellite/satellite_variable_window/window_state.cpp)|$(grep -c 'g_signal_connect(widget, "notify::page"' satellite/satellite_variable_window/window_answers.cpp)"

cat > build/window_tabs_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window t = satellite.window.tabs("x")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_tabs_arity.satl > build/window_tabs_arity.out 2>&1
expect "satellite.window.tabs with an argument is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_tabs_arity.out)"
expect "... and says a tab is named by its piece's .title" 1 \
       "$(tr '\n' ' ' < build/window_tabs_arity.out | grep -cF "named by that piece's .title")"

cat > build/window_tabs_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_switched(satellite.variable.window the_tabs)
{
    satellite.console.display(the_tabs.chosen)
}
satellite.capsule satellite.main()
{
    satellite.variable.window t = satellite.window.tabs()
    satellite.variable.window first = satellite.window.label("the first page")
    first.title("First")
    t.append(first)
    t.chosen("First")
    t.changed(when_switched)
    satellite.console.display(t.chosen)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_tabs_ok.satl > build/window_tabs_ok.out 2>&1
expect "tabs, a titled page, .chosen and .changed pass the checker, and stop only for want of a screen" 50 $?

# ---------------------------------------------------------------------------
# A CANVAS (GTK_AND_NO_DEPENDENCIES.md GTK-15, 2026-09-22): satellite draws it
# itself, and IT IS A DISPLAY LIST AND NOT A DRAW CAPSULE. .line, .box, .circle
# and .write append a stroke to the piece and ask for a redraw; GTK's draw
# function replays the list and waits on nobody -- so the deadlock GTK-15 was
# ordered last to avoid (the desk blocked on the interpreter while the
# interpreter is blocked on the desk) cannot be built out of it. The first time
# satellite's own code calls cairo, and pango.
#
# PROVED ON A COMPOSITOR, AND READ BACK BY EYE: a 400x300 canvas drew two
# lines, a box, a circle and words in IBM Plex Mono 18, and .save wrote them to
# a PNG; a REAL POINTER CLICK on the canvas ran its .clicked capsule, which drew
# a red circle and red words AFTER the canvas was on the screen, saved a second
# PNG, and closed the window -- exit 0. The second picture is the first plus
# the red. THE PICTURE ALSO FOUND A DEFECT: with the pen read off the widget's
# computed style, a canvas told .colour twice before it was in a window drew
# the second batch in the FIRST colour -- reloading a CSS provider does not
# reach a widget with no root. The pen is the canvas's own string now.
# ---------------------------------------------------------------------------
expect "a canvas is a display list: the draw function replays it and waits on nobody" "1|1|0" \
       "$(grep -c 'gtk_drawing_area_set_draw_func' satellite/satellite_variable_window/window_canvas.cpp)|$(grep -c '^void replay(' satellite/satellite_variable_window/window_canvas.cpp)|$(grep -c 'the_desk_saw_something\|the_desk_waits_for_something' satellite/satellite_variable_window/window_canvas.cpp)"
expect "the pen is the canvas's own colour, parsed at the stroke, and not the widget's computed style" 1 \
       "$(grep -c 'gdk_rgba_parse(&colour, canvas.a_colour.c_str())' satellite/satellite_variable_window/window_strokes.cpp)"
expect "line, box, circle, write and separator are 0x0B3F to 0x0B43, in that order" "1|1|1|1|1" \
       "$(grep -c 'line_token = 0x0B3F' satellite/bytecode/token_codes.hpp)|$(grep -c 'box_token = 0x0B40' satellite/bytecode/token_codes.hpp)|$(grep -c 'circle_token = 0x0B41' satellite/bytecode/token_codes.hpp)|$(grep -c 'write_token = 0x0B42' satellite/bytecode/token_codes.hpp)|$(grep -c 'separator_token = 0x0B43' satellite/bytecode/token_codes.hpp)"

cat > build/window_canvas_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.window.canvas(400)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_canvas_arity.satl > build/window_canvas_arity.out 2>&1
expect "satellite.window.canvas with one argument is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_canvas_arity.out)"

cat > build/window_canvas_method.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.line(0, 0, 100)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_canvas_method.satl > build/window_canvas_method.out 2>&1
expect "c.line with three arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_canvas_method.out)"
expect "... and says .line takes 4 arguments" 1 \
       "$(tr '\n' ' ' < build/window_canvas_method.out | grep -cF 'c.line takes 4 arguments, and was given 3')"

cat > build/window_canvas_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.colour("#40c8ff")
    c.line(0, 0, 399, 299)
    c.box(20, 20, 80, 50)
    c.circle(100, 200, 30)
    c.font("IBM Plex Mono", 18)
    c.write(150, 40, "hello from satellite")
    c.save("build/canvas.png")
    c.clear()
    satellite.console.display(c.width)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_canvas_ok.satl > build/window_canvas_ok.out 2>&1
expect "a canvas, its five strokes, .clear and .save pass the checker, and stop only for want of a screen" 50 $?

# A CONSOLE (GTK-17, built 2026-09-22): `satellite.console.new("a title", 800, 600)`,
# a window whose whole inside is a VTE terminal with a pty of its own, UNDER
# satellite.console and not satellite.window -- the author's own spelling. It is
# `1 5 10`, the first word 004 has put under satellite.console (003 ended at 1 5 9).
# satellite.console.display keeps writing to stdout; a console is a piece a program
# makes by name (GTK-17's reading 2, the recommendation). Five method tokens, 0x0B4A
# to 0x0B4E in that order: .display("words"), .typed(a_capsule) -- and .typed read
# bare is the last line a person finished -- .home(); .columns and .rows are
# questions. .clear() is the canvas's token, answered by the receiver.
#
# AND THE CONSOLE satl LAUNCHES FOR ITSELF: `satl --console [file]`, the same window
# with satl's own stdin, stdout and stderr on its pty, IN THIS PROCESS -- so the
# code a run stops on is still the exit status, which is what 003's handover to
# satl-term lost and why 004 removed it. prove-console.sh drives both on a
# compositor; these rows are what a machine with no screen can assert.
#
# AND satl OPENS IT ON ITS OWN WHEN NOBODY GAVE IT A CONSOLE (WIN-9, the author,
# 2026-09-22: "satl has to, when it's not ran in a console, take you to it's
# prompt"), and satl-term is gone. The four reasons it does NOT are in
# window_run.cpp; the row below takes all four away but the display -- no
# controlling terminal (setsid), stdout on /dev/null, SATL_NO_WINDOW unset --
# and points WAYLAND_DISPLAY at a socket that is not there, in a runtime folder
# of the suite's own, so the real desktop cannot be reached. satl tries, cannot,
# and runs the program where it was pointed: the file it writes says it ran.
expect "satellite.console.new is 1 5 10, under satellite.console" 1 \
       "$(grep -cP '^1 5 10\tsatellite.console.new\(title, width, height\)\t' words/words.tsv)"
expect "display, typed, home, columns and rows are 0x0B4A to 0x0B4E, in that order" "1|1|1|1|1" \
       "$(grep -c 'display_token = 0x0B4A' satellite/bytecode/token_codes.hpp)|$(grep -c 'typed_token = 0x0B4B' satellite/bytecode/token_codes.hpp)|$(grep -c 'home_token = 0x0B4C' satellite/bytecode/token_codes.hpp)|$(grep -c 'columns_token = 0x0B4D' satellite/bytecode/token_codes.hpp)|$(grep -c 'rows_token = 0x0B4E' satellite/bytecode/token_codes.hpp)"

cat > build/console_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.console.new("a console", 800)
    satellite.return(satellite)
}
WIN_EOF
headless build/console_arity.satl > build/console_arity.out 2>&1
expect "satellite.console.new with two arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/console_arity.out)"
expect "... and says what it takes" 1 \
       "$(tr '\n' ' ' < build/console_arity.out | grep -cF 'satellite.console.new takes a title, a width and a height')"

cat > build/console_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_typed(satellite.variable.window the_console)
{
    satellite.console.display(the_console.typed)
    satellite.console.display(the_console.columns)
    satellite.console.display(the_console.rows)
    the_console.clear()
    the_console.home()
}
satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.console.new("a console", 800, 600)
    c.display("hello")
    c.display(42)
    c.typed(when_typed)
    c.title("renamed").font("IBM Plex Mono", 12).colour("#000000").background("#90D5FF")
    satellite.return(satellite)
}
WIN_EOF
headless build/console_ok.satl > build/console_ok.out 2>&1
expect "a console and every one of its methods pass the checker, and stop only for want of a screen" 50 $?
expect "... with S730 NO_DISPLAY and the reason" "1|1" \
       "$(grep -c 'S730: NO_DISPLAY' build/console_ok.out)|$(tr '\n' ' ' < build/console_ok.out | grep -c 'satellite.console.new could not open a window -- there is no display to draw on')"

cat > build/console_method_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.console.new("a console", 800, 600)
    c.display()
    satellite.return(satellite)
}
WIN_EOF
headless build/console_method_arity.satl > build/console_method_arity.out 2>&1
expect "c.display with no argument is refused before anything runs" "13|" \
       "$?|$(grep -x before build/console_method_arity.out)"
expect "... and says .display takes 1 argument" 1 \
       "$(tr '\n' ' ' < build/console_method_arity.out | grep -cF 'c.display takes 1 argument, and was given 0')"

cat > build/console_typed_text.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.console.new("a console", 800, 600)
    c.typed("when_typed")
    satellite.return(satellite)
}
WIN_EOF
headless build/console_typed_text.satl > build/console_typed_text.out 2>&1
expect "c.typed given text and not a capsule's name is refused before anything runs" "27|" \
       "$?|$(grep -x before build/console_typed_text.out)"

cat > build/console_typed_nobody.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.console.new("a console", 800, 600)
    c.typed(nobody)
    satellite.return(satellite)
}
WIN_EOF
headless build/console_typed_nobody.satl > build/console_typed_nobody.out 2>&1
expect "c.typed naming a capsule nobody wrote is refused before anything runs" "13|" \
       "$?|$(grep -x before build/console_typed_nobody.out)"
expect "... and names the capsule" 1 "$(grep -c 'no capsule named nobody' build/console_typed_nobody.out)"

mkdir -p build/no_display build/auto_console
rm -f build/auto_console/auto_console.se
cat > build/auto_console/auto_console.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.file.new("auto_console.se").append("it ran where it was pointed")
    satellite.return(satellite)
}
WIN_EOF
env -u SATL_NO_WINDOW -u DISPLAY -u XAUTHORITY WAYLAND_DISPLAY=satl-no-such-display XDG_RUNTIME_DIR="$PWD/build/no_display" \
    setsid -w timeout 30 "$interpreter" build/auto_console/auto_console.satl < /dev/null > /dev/null 2> build/auto_console.err
expect "with no terminal and no screen to reach, satl runs the program where it was pointed and exits 0" "0|it ran where it was pointed" \
       "$?|$(cat build/auto_console/auto_console.se 2>/dev/null)"
# THE START-UP BLOCK IS ON stderr AT EVERY RUN, by design; what must not be
# there is a word about a console that satl asked for itself and could not have.
expect "... and says nothing about the console it could not have" 0 "$(grep -ci 'console' build/auto_console.err)"
expect "the rule's four reasons are asked in order, and bare satl with no console is the prompt in one" "1|1|1|1" \
       "$(grep -c 'const char \*off = std::getenv("SATL_NO_WINDOW");' satellite/bytecode/window_run.cpp)|$(grep -c 'open("/dev/tty", O_RDONLY | O_NOCTTY | O_CLOEXEC)' satellite/bytecode/window_run.cpp)|$(grep -c 'S_ISFIFO(out.st_mode) || S_ISREG(out.st_mode)' satellite/bytecode/window_run.cpp)|$(grep -c 'if (on_its_own && asked == Command::opening)' satellite/structured-library.cpp)"
expect "satl-term is gone: no folder, no build rule, and the launcher starts satl --console" "0|0|1" \
       "$([ -d satl-term ] && echo 1 || echo 0)|$(grep -c 'satl-term:' make_support/050-build.mk)|$(grep -cx 'Exec=satl --console %f' satellite_enterprise/icons/org.satellite.terminal.desktop)"
expect "satl's own console carries satl-term's id, File menu and priority, and leaves F10 to the program" "1|1|1|1" \
       "$(grep -c 'g_set_prgname("org.satellite.terminal");' satellite/satellite_variable_window/console_launch.cpp)|$(grep -c 'setpriority(PRIO_PROCESS, 0, 19);' satellite/satellite_variable_window/console_launch.cpp)|$(grep -c 'gtk_window_set_handle_menubar_accel(GTK_WINDOW(window), FALSE);' satellite/satellite_variable_window/console_menu.cpp)|$(grep -c '{"New window", "satl.new-window"}' satellite/satellite_variable_window/console_menu.cpp)"

# THE FOLDER THE PROMPT STARTS IN (the author, 2026-09-22): the row is words,
# "~" by default, and config.ini sets it for one machine by its name without
# `arguments.`. --debug lists every argument, so it is read back there.
"$interpreter" --debug build/auto_console/auto_console.satl > build/directory_default.out 2>&1
expect "arguments.directory.default is a row of words, ~ by default" 1 \
       "$(grep -cF 'arguments.directory.default = ~ (machine_code' build/directory_default.out)"
mkdir -p build/directory_home/.satl
printf 'directory.default = /somewhere/else\n' > build/directory_home/.satl/config.ini
HOME="$PWD/build/directory_home" "$interpreter" --debug build/auto_console/auto_console.satl > build/directory_set.out 2>&1
expect "... and config.ini sets it for one machine, as directory.default" 1 \
       "$(grep -cF 'arguments.directory.default = /somewhere/else (machine_code' build/directory_set.out)"
expect "... and only the prompt in satl's own console goes there" 1 \
       "$(grep -c 'if (satls_own_console_is_open() && command_line.command == Command::repl)' satellite/structured-library.cpp)"

# THE CONSOLE satl LAUNCHES: refused for want of a screen BEFORE the program runs,
# with the machine's code and not the build's; and refused by the command line
# beside anything that prints and exits, because a window that shows a licence
# and vanishes has shown nothing.
headless --console > build/console_launch.out 2>&1
expect "satl --console on a machine with no screen is refused with no_display, not hung" 50 $?
expect "... and says the console could not be opened" 1 "$(grep -c 'the console could not be opened' build/console_launch.out)"
cat > build/console_launch_file.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.return(satellite)
}
WIN_EOF
headless --console build/console_launch_file.satl > build/console_launch_file.out 2>&1
expect "satl --console <file> with no screen is refused before the program runs" "50|" \
       "$?|$(grep -x before build/console_launch_file.out)"
"$interpreter" --console --version > /dev/null 2>&1; expect "--console --version is refused: --version is the whole command line" 23 $?
"$interpreter" --console --rebuild > /dev/null 2>&1; expect "--console --rebuild is refused: it prints and exits" 23 $?
"$interpreter" --console --license > /dev/null 2>&1; expect "--console --license is refused the same way" 23 $?
expect "--help names --console" 1 "$("$interpreter" --help | grep -c 'satl --console \[file.satl\]')"

# THE SHAPE, PINNED IN THE SOURCE: a console is a window everywhere a window is
# one, holds no pieces, is written into and not .text'ed; the run never waits on
# satl's own console; a person closing that console hangs up on the interpreter;
# a finished line travels on the event as a key does; and the pty's slave is
# opened by hand -- vte_pty_child_setup() is for a forked child and would _exit
# THIS process when setsid() fails.
# THE THREE `== window` LEFT ARE NAMED, so a fourth is noticed: .append's own
# a_window (satellite_window.cpp), and .text written and read on a window
# (window_asks.cpp), each of which branches the console off just above it.
expect "a console answers to every window method through is_a_window(): no `!= window` refusal is left, and exactly three `== window` remain" "0|3" \
       "$(grep -c 'piece != satellite_window::window' satellite/satellite_variable_window/*.cpp | awk -F: '{s+=$2} END {print s+0}')|$(grep -c 'piece == satellite_window::window' satellite/satellite_variable_window/*.cpp | awk -F: '{s+=$2} END {print s+0}')"
# WHAT A FRESH READER FOUND (2026-09-22), each pinned so it cannot come back: the
# desk's C streams stay off the pty (the deadlock); a frame is refused as a
# piece; Ctrl-D at the start of a line keeps the reader; the reader is re-armed
# only on the desk's own view of the console; the copy chords copy at the hold.
expect "the desk's stdout and stderr are kept off satl's own console pty, before the dup2" "1|1" \
       "$(grep -c 'keep_the_c_streams_off_the_pty();' satellite/satellite_variable_window/console_launch.cpp)|$(grep -c '        stderr = err;' satellite/satellite_variable_window/console_launch.cpp)"
expect "a frame is refused as a piece to append, and Ctrl-D does not stop a console's reader" "1|1" \
       "$(grep -c 'is a frame of its own and goes inside nothing' satellite/satellite_variable_window/satellite_window.cpp)|$(grep -c 'if (got == 0)' satellite/satellite_variable_window/window_console.cpp)"
# CTRL-C AT THE HOLD COPIES WHAT IS HIGHLIGHTED, satl-term's rule, ported when
# satl-term went (Shift allowed, so Ctrl-Shift-C copies too); with nothing
# highlighted it closes, as any key does.
expect "the reader is armed on the desk's own view of the console, and Ctrl-C at the hold copies what is highlighted" "1|1" \
       "$(grep -c 'raw->typed_watch = g_unix_fd_add(raw->slave, G_IO_IN, a_line_was_finished, raw);' satellite/satellite_variable_window/window_console.cpp)|$(grep -c 'vte_terminal_copy_clipboard_format(terminal, VTE_FORMAT_TEXT);' satellite/satellite_variable_window/console_launch.cpp)"
expect "a console refuses .append by name, and .text sends to .display" "1|1" \
       "$(grep -c 'a console holds nothing but its terminal' satellite/satellite_variable_window/satellite_window.cpp)|$(grep -c 'a console is written into a line at a time' satellite/satellite_variable_window/window_asks.cpp)"
expect "the run never waits on satl's own console, and a person closing it hangs up" "1|1" \
       "$(grep -c 'return !presses.empty() || !a_program_window_is_open();' satellite/satellite_variable_window/window_desk.cpp)|$(grep -c 'kill(getpid(), SIGHUP);' satellite/satellite_variable_window/window_console.cpp)"
expect "a finished line travels on the event, and the interpreter copies it onto the piece" "1|1" \
       "$(grep -c 'AnEvent::a_line);' satellite/satellite_variable_window/window_console.cpp)|$(grep -c 'happened.piece->last_typed = happened.said' satellite/bytecode/window_run.cpp)"
expect "the pty's slave is opened by hand from the master's name, with no controlling terminal taken" 1 \
       "$(grep -c 'ptsname_r(vte_pty_get_fd(pty)' satellite/satellite_variable_window/window_console.cpp)"
# TWO MESSAGES END IN "press any key to close": a run that stopped, and the
# prompt when it ends. A file that finished has no message; it closes.
expect "satl's own console is closed on purpose with SIGHUP ignored first, and a stopped run or the prompt holds it" "1|2" \
       "$(grep -c 'std::signal(SIGHUP, SIG_IGN);' satellite/satellite_variable_window/console_launch.cpp)|$(grep -c 'press any key to close' satellite/bytecode/window_run.cpp)"

# THE FOUR LEFTOVERS (GTK-15, 2026-09-22), each built as the recommendation and
# still reversible: WHERE A CLICK LANDED -- `.across` and `.down`, carried on
# the event as a key's name is and copied onto the piece by the interpreter,
# so no string or number has two threads on it; an OUTLINE -- `.outline(1)`,
# a pen setting like `.colour`; a line's WIDTH -- `.thickness(3)`, the pen
# again, and an even width gets no half-pixel; and an ARC --
# `.arc(across, down, radius, from_degrees, to_degrees)`, clockwise from three
# o'clock, a slice when filled and the curve alone when outlined.
#
# PROVED ON A COMPOSITOR (prove-canvas-tabs-menus.sh): a real click at the
# canvas's centre answered .across 200 and .down 150 inside the capsule, and
# the saved PNGs show a four-pixel outlined box, circle and arc beside the
# filled ones.
expect "across, down, outline, thickness and arc are 0x0B44 to 0x0B48, in that order" "1|1|1|1|1" \
       "$(grep -c 'across_token = 0x0B44' satellite/bytecode/token_codes.hpp)|$(grep -c 'down_token = 0x0B45' satellite/bytecode/token_codes.hpp)|$(grep -c 'outline_token = 0x0B46' satellite/bytecode/token_codes.hpp)|$(grep -c 'thickness_token = 0x0B47' satellite/bytecode/token_codes.hpp)|$(grep -c 'arc_token = 0x0B48' satellite/bytecode/token_codes.hpp)"
expect "where a click landed travels on the event, and the interpreter copies it onto the piece" "1|1" \
       "$(grep -c 'AnEvent::a_place, static_cast<long long int>(std::floor(x))' satellite/satellite_variable_window/window_answers.cpp)|$(grep -c 'happened.piece->last_across = happened.across' satellite/bytecode/window_run.cpp)"
expect "an arc's angles are positions on the face: brought into [0, 360), and round to itself is a circle" "1|1" \
       "$(grep -c 'const long long int from = ((from_degrees % 360) + 360) % 360;' satellite/satellite_variable_window/window_strokes.cpp)|$(grep -c 'if (to <= from)' satellite/satellite_variable_window/window_strokes.cpp)"
# A NEGATIVE NUMBER WHERE A PLACE IS EXPECTED (found 2026-09-22 by drawing an
# arc from -90): place_of borrowed fits_a_count(), which refuses every negative
# number because a count is never negative -- so from 2026-09-20 a slider from
# -50, the reader's own example, was refused as "further than any screen
# reaches". It reads the magnitude now, whatever the sign.
cat > build/window_negative_place.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.window s = satellite.window.slider(-50, 50)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_negative_place.satl > build/window_negative_place.out 2>&1
expect "a slider from -50 passes the checker and stops only for want of a screen, not as a bad position" 50 $?
expect ".across and .down refuse a button and a menu by name, which are never clicked" 1 \
       "$(grep -c 'is pressed and never clicked, so no click lands on it' satellite/bytecode/window_questions.cpp)"
expect "the thickness and the outline are copied onto each stroke, as the colour is" "1|1" \
       "$(grep -c 'stroke.thickness = static_cast<double>(canvas.pen_thickness)' satellite/satellite_variable_window/window_strokes.cpp)|$(grep -c 'stroke.outline = canvas.pen_outline' satellite/satellite_variable_window/window_strokes.cpp)"

cat > build/window_canvas_arc_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.arc(200, 150, 60, 0)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_canvas_arc_arity.satl > build/window_canvas_arc_arity.out 2>&1
expect "c.arc with four arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_canvas_arc_arity.out)"
expect "... and says .arc takes 5 arguments" 1 \
       "$(tr '\n' ' ' < build/window_canvas_arc_arity.out | grep -cF 'c.arc takes 5 arguments, and was given 4')"

cat > build/window_canvas_pen_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_clicked(satellite.variable.window the_canvas)
{
    satellite.console.display(the_canvas.across)
    satellite.console.display(the_canvas.down)
}
satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.thickness(4)
    c.outline(1)
    c.box(20, 20, 80, 50)
    c.circle(100, 200, 30)
    c.arc(200, 150, 60, 0, 270)
    c.outline(0)
    c.arc(200, 150, 40, 270, 360)
    satellite.console.display(c.thickness)
    satellite.console.display(c.outline)
    satellite.console.display(c.across)
    c.clicked(when_clicked)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_canvas_pen_ok.satl > build/window_canvas_pen_ok.out 2>&1
expect "the pen, an arc, and .across and .down pass the checker, and stop only for want of a screen" 50 $?

# ---------------------------------------------------------------------------
# TIME (GTK_AND_NO_DEPENDENCIES.md GTK-13, 2026-09-21): a capsule on a clock.
# ---------------------------------------------------------------------------
#
# glib AND NOT gtk -- g_timeout_add() on the desk's own context is the whole of
# it, and it is the only milestone here that touches neither a widget nor a
# window's drawing. It is the SECOND producer for the queue WIN-11 built for one
# button, which is what says that queue was a general thing and not a button's
# private arrangement.
#
# MEASURED ON A COMPOSITOR: `w.every(when_it_ticks, 120)` ran the capsule 49
# times in six seconds -- 50 is what 120ms gives -- and a second program whose
# button closed the window ENDED, exit 0, rather than ticking for ever. The
# clock stops with the window, and it stops FIRST: the source holds a raw
# pointer into the satellite_window, and a tick firing between the close and the
# desk letting go would queue a capsule for a window that is already gone.

expect "every is a method token at 0000101100110111" "1|1" \
       "$(grep -c '^0000101100110111  every_token ' REGISTRY.satellite)|$(grep -c 'Code every_token = 0x0B37;' satellite/bytecode/token_codes.hpp)"

# THE CAPSULE'S NAME COMES FIRST, because that is where the checker looks for it
# and where expression.cpp reads a name instead of working out a value. What may
# FOLLOW it is the method's business, asked of window_calls.hpp -- `.pressed`
# takes a name and nothing else, `.every` takes a name and then how often.
cat > build/window_every_noname.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_it_ticks()
{
    satellite.console.display("tick")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.every(1000, when_it_ticks)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_every_noname.satl > build/window_every_noname.out 2>&1
expect ".every with how often FIRST is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_every_noname.out)"

cat > build/window_every_nocapsule.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.every(nobody_wrote_this, 1000)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_every_nocapsule.satl > build/window_every_nocapsule.out 2>&1
expect ".every wired to a capsule nobody wrote is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_every_nocapsule.out)"

cat > build/window_every_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_it_ticks()
{
    satellite.console.display("tick")
}
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.every(when_it_ticks, 1000)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_every_ok.satl > build/window_every_ok.out 2>&1
expect ".every passes the checker, and stops only for want of a screen" 50 $?

# A TICK OF 0 IS NOT A RHYTHM, it is a busy loop with a capsule in it: glib
# would run it as fast as the main loop turns and the queue would fill faster
# than the interpreter could drain it. Refused where it is written.
expect "how often must be more than 0" 1 \
       "$(grep -c 'how often must be more than 0 milliseconds' satellite/satellite_variable_window/window_answers.cpp)"

# AND THE CLOCK IS REMOVED BEFORE THE DESK LETS GO OF THE WINDOW.
expect "closing a window stops its clock, and stops it first" 1 \
       "$(grep -c 'THE CLOCK STOPS WITH THE WINDOW (GTK-13), and it stops FIRST' satellite/satellite_variable_window/satellite_window.cpp)"

# ---------------------------------------------------------------------------
# THE KEYBOARD AND THE MOUSE (GTK_AND_NO_DEPENDENCIES.md GTK-14, 2026-09-21).
# ---------------------------------------------------------------------------
#
# libxkbcommon and xkeyboard-config's 293 files have been carried since WIN-1
# FOR GTK'S SAKE -- gdkkeymap-wayland.c SIGSEGVs without them before a window
# exists. This is the first time SATELLITE asks what key was pressed.
#
# PROVED BY PRESSING REAL KEYS on a headless mutter, through its own
# RemoteDesktop NotifyKeyboardKeysym -- the same route press-a-button.sh uses for
# the pointer:
#
#     sent  a  a  c  d  7          answered  a  c  d  7
#     sent  B  Escape Up Return F1 answered  shift_l  B  escape  up  return  f1
#
# A PRINTABLE KEY IS ITS CHARACTER and every other key is a NAME in lower case.
# `shift_l` is right: mutter synthesises a Shift press to type a capital, and a
# modifier IS a key press. THE FIRST KEY OF A REMOTE-DESKTOP SESSION IS SWALLOWED
# -- sending `a a c d 7` answers `a c d 7` -- and that is mutter settling, not
# satl: the second `a` arrives. Written down so nobody chases it.

expect "key and clicked are method tokens at 0000101100111000 and ...1001" "1|1|1|1" \
       "$(grep -c '^0000101100111000  key_token ' REGISTRY.satellite)|$(grep -c 'Code key_token = 0x0B38;' satellite/bytecode/token_codes.hpp)|$(grep -c '^0000101100111001  clicked_token ' REGISTRY.satellite)|$(grep -c 'Code clicked_token = 0x0B39;' satellite/bytecode/token_codes.hpp)"

# A PROGRAM NEVER SEES A KEYVAL. GDK_KEY_Escape is 0xff1b and a satellite
# program has no business knowing that.
expect "a key is a character or a name, never a number" "2|2" \
       "$(grep -c 'gdk_keyval_to_unicode' satellite/satellite_variable_window/window_answers.cpp)|$(grep -c 'gdk_keyval_name' satellite/satellite_variable_window/window_answers.cpp)"

# WHAT A KEY SAID TRAVELS ON THE EVENT AND IS COPIED ONTO THE PIECE BY THE
# INTERPRETER. Written by the desk and read by a capsule it would have been a
# std::string with two threads on it -- the very thing GTK-2 refused to add.
expect "what a key said is written on the interpreter's thread, not the desk's" "1|1" \
       "$(grep -c 'happened.piece->last_key = happened.said' satellite/bytecode/window_run.cpp)|$(grep -c 'std::string said;' satellite/satellite_variable_window/window_desk.hpp)"

# THE HANDLER ANSWERS FALSE, so the key goes on to whatever wanted it. TRUE
# would mean a program watching for Escape had silently made every text box in
# its window unusable.
expect "watching for a key does not eat it" 1 \
       "$(grep -c 'SO THE KEY GOES ON TO THE WIDGET THAT WANTED IT' satellite/satellite_variable_window/window_answers.cpp)"

cat > build/window_key_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_a_key(satellite.variable.window the_piece)
{
    satellite.console.display(the_piece.key)
}
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.key(when_a_key)
    satellite.console.display(w.key)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_key_ok.satl > build/window_key_ok.out 2>&1
expect ".key written and read passes the checker, and stops only for want of a screen" 50 $?

# A BUTTON ALREADY HAS A WORD FOR BEING CLICKED, and two names for one thing is
# what this language spends its refusals avoiding.
expect "a button asked for .clicked is sent to .pressed" 1 \
       "$(grep -c 'a button already has a word for being clicked' satellite/satellite_variable_window/window_answers.cpp)"

# ONLY A WINDOW HEARS THE KEYBOARD: a key goes to whatever has the focus, and
# the window is the thing that sees them all.
expect "a piece inside a window does not hear the keyboard on its own" 1 \
       "$(grep -c 'only a window hears the keyboard' satellite/satellite_variable_window/window_answers.cpp)"

# ---------------------------------------------------------------------------
# ASKING A PERSON SOMETHING (GTK_AND_NO_DEPENDENCIES.md GTK-11, 2026-09-21).
# ---------------------------------------------------------------------------
#
# A QUESTION IS A CAPSULE AND NOT A WAIT. GtkAlertDialog is asynchronous, so the
# answer arrives in a GAsyncReadyCallback on the desk's thread -- the press queue
# again with a different producer, and no new machinery at all. The OTHER shape,
# a satellite line that STOPS until a person answers, stays the author's and
# nothing here forecloses it.
#
# PROVED ON A COMPOSITOR: a message shown, then `w.ask(when_answered, "delete
# it?")`, then a REAL Return keypress through mutter's RemoteDesktop -- and the
# capsule ran with `its_window.answer` reading "yes". `w.answer` read before
# anybody answered is "".
#
# THE FILE DIALOG IS NOT BUILT, ON PURPOSE. GtkFileDialog can go out to
# xdg-desktop-portal, and a wedged portal is exactly Q-WIN-11a: the D-Bus call
# that hangs satl for ever with nothing printed. Building a word that can reach
# it before the author has ruled would be shipping the hang.
# Q-WIN-11a WAS DECIDED ON 2026-09-22, IN ONE WORD FROM THE AUTHOR: "defend".
# The desk turns portals off before it opens a display -- gtk_disable_portals()
# before gtk_init_check, the same bit as GDK_DEBUG=no-portals -- so satl never
# makes the synchronous D-Bus call that held it in gtk_init_check for an
# afternoon with nothing printed. What it costs: the portal's settings (dark
# mode, the font) fall back to gsettings, and a sandbox gets GTK's own chooser.
# What it bought: the file dialog, which this row used to assert did not exist.
#
# PROVED ON A COMPOSITOR (prove-canvas-tabs-menus.sh, the `file` stage): satl
# started ON dbus-run-session's bus -- the one that hung it -- opened
# .choose_a_file's chooser, real keys typed a path and Return, and the capsule
# read that path back in .answer and closed the window, exit 0.
expect "the portal is turned off BEFORE the display is opened, and in the desk" "1|1" \
       "$(grep -c 'gtk_disable_portals();' satellite/satellite_variable_window/window_desk.cpp)|$(awk '/gtk_disable_portals\(\);/ {a=NR} /gtk_init_check\(\) != FALSE/ {b=NR} END {print (a>0 && b>a) ? 1 : 0}' satellite/satellite_variable_window/window_desk.cpp)"
expect "the file dialog exists now, as GTK's own chooser in satl's process, and unrefs once in its callback" "1|1" \
       "$(grep -c 'gtk_file_dialog_open(chooses, GTK_WINDOW(widget), nullptr, they_chose, raw)' satellite/satellite_variable_window/window_asking.cpp)|$(grep -c 'gtk_file_dialog_open_finish' satellite/satellite_variable_window/window_asking.cpp)"
expect "choose_a_file is a method token at 0x0B49 that names a capsule, and has its own name on the piece" "1|1|1" \
       "$(grep -c 'choose_a_file_token = 0x0B49' satellite/bytecode/token_codes.hpp)|$(grep -c 'method == token::choose_a_file_token;' satellite/bytecode/window_shapes.cpp)|$(grep -c 'std::string when_a_file_is_chosen;' satellite/satellite_variable_window/satellite_window.hpp)"

cat > build/window_file_text.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("f", 800, 600)
    w.choose_a_file("when_chosen")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_file_text.satl > build/window_file_text.out 2>&1
expect ".choose_a_file given text and not a name is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_file_text.out)"

cat > build/window_file_nocapsule.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("f", 800, 600)
    w.choose_a_file(nobody_wrote_this)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_file_nocapsule.satl > build/window_file_nocapsule.out 2>&1
expect ".choose_a_file wired to a capsule nobody wrote is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_file_nocapsule.out)"

cat > build/window_file_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_chosen(satellite.variable.window the_window)
{
    satellite.console.display(the_window.answer)
}
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("f", 800, 600)
    w.choose_a_file(when_chosen)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_file_ok.satl > build/window_file_ok.out 2>&1
expect ".choose_a_file with a capsule and .answer pass the checker, and stop only for want of a screen" 50 $?

expect "message, ask and answer are method tokens 0000101100111010..1100" "1|1|1" \
       "$(grep -c 'Code message_token = 0x0B3A;' satellite/bytecode/token_codes.hpp)|$(grep -c 'Code ask_token = 0x0B3B;' satellite/bytecode/token_codes.hpp)|$(grep -c 'Code answer_token = 0x0B3C;' satellite/bytecode/token_codes.hpp)"

# THE CAPSULE'S NAME COMES FIRST IN EVERY METHOD THAT NAMES ONE. `.ask` reads
# less like English that way round and is spelled that way because `.pressed`,
# `.changed`, `.closed`, `.every` and `.key` all are: the checker looks for a
# name at the first argument and expression.cpp reads one there instead of
# working out a value. One rule a person can hold in their head.
cat > build/window_ask_backwards.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_answered()
{
    satellite.console.display("answered")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.ask("delete it?", when_answered)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_ask_backwards.satl > build/window_ask_backwards.out 2>&1
expect ".ask with the question first is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_ask_backwards.out)"

cat > build/window_ask_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_answered(satellite.variable.window the_piece, satellite.variable.window its_window)
{
    satellite.console.display(its_window.answer)
}
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    w.message("hello")
    w.ask(when_answered, "delete it?")
    satellite.console.display(w.answer)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_ask_ok.satl > build/window_ask_ok.out 2>&1
expect ".message, .ask and .answer pass the checker, and stop only for want of a screen" 50 $?

# "%s" AND NOT THE TEXT ITSELF. gtk_alert_dialog_new takes a PRINTF FORMAT, so a
# person's own text containing a % would be read as a conversion and GTK would
# walk off the end of an argument list with nothing in it -- a crash a program
# could cause by displaying a percentage.
expect "a person's own words are never a printf format" "2|0" \
       "$(grep -c 'gtk_alert_dialog_new(\"%s\"' satellite/satellite_variable_window/window_asking.cpp)|$(grep -c 'gtk_alert_dialog_new(saying' satellite/satellite_variable_window/window_asking.cpp)"

# DISMISSED IS NOT A FAILURE. Closing a question without choosing is a thing a
# person is entitled to do, and GTK reports it as an ERROR -- so it becomes ""
# rather than a refusal of a program that did nothing wrong.
expect "dismissing a question is an empty answer, not a refusal" 1 \
       "$(grep -c 'DISMISSED IS NOT A FAILURE' satellite/satellite_variable_window/window_asking.cpp)"

# ---------------------------------------------------------------------------
# A MENU (GTK_AND_NO_DEPENDENCIES.md GTK-12, 2026-09-21): the only milestone
# that is gio and not gtk.
# ---------------------------------------------------------------------------
#
#     satellite.variable.window file = satellite.window.menu("File")
#     file.item(when_open, "Open")
#     my_window.menu(file)
#
# A MENU IS MADE FROM ITS HEADING, AND THAT IS GTK'S RULING BEFORE IT IS OURS:
# gtkpopovermenubar.c's tracker_insert puts an item on the bar ONLY when it has
# a submenu, and drops one without a word said -- so `satellite.window.menu()`
# with nothing on the bar would have been a menu that is nowhere. The row is
# `1 27 19 satellite.window.menu(title)`, one row and not two: a word that
# takes something has no `()` row, exactly as a frame has none.
expect "menu is 1 27 19, made from its heading, and has no bare-call row" "1|0" \
       "$(grep -cP '^1 27 19\tsatellite.window.menu\(title\)\t' words/words.tsv)|$(grep -cP '^1 27 19 0\t' words/words.tsv)"

# `.add` WOULD HAVE READ BETTER AND IS `+` ALREADY. The checker's capsule-name
# rule is receiver-blind on purpose (names_in_statement walks every statement),
# so making `.add` name a capsule would have made every `x.add(...)` in the
# language a capsule. `.item` is its own token, and `.add` still adds.
expect "menu and item are method tokens 0000101100111101 and ...1110" "1|1|1|1" \
       "$(grep -c '^0000101100111101  menu_token ' REGISTRY.satellite)|$(grep -c 'Code menu_token = 0x0B3D;' satellite/bytecode/token_codes.hpp)|$(grep -c '^0000101100111110  item_token ' REGISTRY.satellite)|$(grep -c 'Code item_token = 0x0B3E;' satellite/bytecode/token_codes.hpp)"

cat > build/window_menu_add.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.variable.string s = "ab"
    satellite.console.display(s.add("cd"))
    satellite.variable.number n = 40
    satellite.console.display(n.add(2))
    satellite.return(satellite)
}
WIN_EOF
expect ".add is still +, which is why an item is .item and not .add" "abcd|42" \
       "$("$interpreter" build/window_menu_add.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"

# THE CAPSULE'S NAME COMES FIRST, as in every method that names one.
cat > build/window_item_backwards.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_open()
{
    satellite.console.display("open")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window m = satellite.window.menu("File")
    m.item("Open", when_open)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_item_backwards.satl > build/window_item_backwards.out 2>&1
expect ".item with the words first is refused before anything runs" "27|" \
       "$?|$(grep -x before build/window_item_backwards.out)"

cat > build/window_item_nocapsule.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window m = satellite.window.menu("File")
    m.item(nobody_wrote_this, "Open")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_item_nocapsule.satl > build/window_item_nocapsule.out 2>&1
expect ".item wired to a capsule nobody wrote is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_item_nocapsule.out)"

# `satellite.window.menu()` IS THE FIRST WORD WHOSE LAST SEGMENT IS ALSO A
# METHOD'S NAME, and it found a hole: with no `menu()` row the lexer answered
# nothing for the shaped call, fell back to `satellite.window` and then read
# `.menu` as the METHOD -- so the wrong count was refused at RUN time, after
# "before" had printed, with a sentence about a value that was not there. An
# empty call on a word that takes something now lexes as that word with 0
# arguments, and the checker refuses it by name before a line runs -- for
# `frame()` and `label()` as well, which used to be "no capsule named frame".
cat > build/window_menu_noheading.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window m = satellite.window.menu()
    satellite.return(satellite)
}
WIN_EOF
headless build/window_menu_noheading.satl > build/window_menu_noheading.out 2>&1
expect "satellite.window.menu() with no heading is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_menu_noheading.out)"
expect "... and says what a menu takes, and that it was given 0 arguments" "1|1" \
       "$(tr '\n' ' ' < build/window_menu_noheading.out | grep -cF 'takes the word that goes on the bar')|$(tr '\n' ' ' < build/window_menu_noheading.out | grep -cF 'was given 0 arguments')"

cat > build/window_menu_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_open(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display("open was picked on " + the_menu.text)
}
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    satellite.variable.window m = satellite.window.menu("File")
    m.item(when_open, "Open")
    w.menu(m)
    m.text("Edit")
    satellite.return(satellite)
}
WIN_EOF
headless build/window_menu_ok.satl > build/window_menu_ok.out 2>&1
expect "a menu, an item and .menu pass the checker, and stop only for want of a screen" 50 $?

# A MENU IS THE ONE PIECE THAT IS NOT A WIDGET: its `widget` holds a GMenu.
# Every gtk_widget_* caller asks is_drawn() first and refuses a menu by name --
# measuring it, dressing it, watching it for a click, putting it in a fixed --
# and the teardown gives back the two references GTK never took.
expect "everything that hands a menu's widget to GTK asks is_drawn() first" "yes" \
       "$([ "$(cat satellite/satellite_variable_window/satellite_window.cpp satellite/satellite_variable_window/window_answers.cpp satellite/satellite_variable_window/window_look.cpp satellite/satellite_variable_window/window_desk.cpp | grep -c 'is_drawn()')" -ge 5 ] && echo yes || echo no)"
expect "a menu's model and actions are given back when its window goes" "1|1" \
       "$(grep -c 'g_object_unref(G_OBJECT(piece->widget))' satellite/satellite_variable_window/window_desk.cpp)|$(grep -c 'g_object_unref(G_OBJECT(piece->actions))' satellite/satellite_variable_window/window_desk.cpp)"

# ACTIONS GO ON THE WINDOW AND NEVER ON AN APPLICATION. A GtkApplication is a
# GApplication, which registers on the session bus, and a wedged portal hangs
# gtk_init_check for ever (Q-WIN-11a). One bar a window, above the fixed in the
# column window_new() has held since this milestone.
expect "the menu's actions go on the window, and no GtkApplication was made" "1|0" \
       "$(grep -c 'gtk_widget_insert_action_group(window, menu.action_prefix.c_str()' satellite/satellite_variable_window/window_menu_bar.cpp)|$(grep -rc 'gtk_application_new\|GTK_APPLICATION_WINDOW' satellite/satellite_variable_window/ | awk -F: '{s+=$2} END {print s}')"

# A MENU INSIDE A MENU, AND A SEPARATOR (GTK-12's two leftovers, 2026-09-22,
# built as the recommendation). `.menu` on a MENU puts the second menu under
# the first, as an item with an arrow, and its heading is the word on that item
# -- satellite.window.menu("Recent") already carries it, as a tab's piece
# carries its title. A menu's model holds SECTIONS and nothing else now: a
# GMenu has no separator item, it draws the line between sections, so a menu is
# made with one and `.separator()` opens the next. The actions of every menu
# inside a menu go on the window with its parent's, recursively.
#
# PROVED ON A COMPOSITOR BY REAL KEYS: File with Open, Save, a separator and
# Recent under it, More under Recent with deep.satl in it; F10 Down Down Down
# Right Right Return picked deep.satl, and the capsule was handed the menu it
# was on and the window -- exit 0. Refused by name: a separator with nothing
# above it, a menu into itself, a menu into a menu it already holds.
expect "a menu's model holds sections, a separator opens the next, and the actions of every menu inside go on the window" "1|1|1" \
       "$(grep -c 'g_menu_append_section' satellite/satellite_variable_window/window_menu.cpp)|$(grep -c '^bool window_separator' satellite/satellite_variable_window/window_menu.cpp)|$(grep -c 'put_the_actions_on(window, \*under)' satellite/satellite_variable_window/window_menu_bar.cpp)"
expect "an item's action is named by a counter and not by the model's count, which is sections now" "1|0" \
       "$(grep -c '++items_so_far' satellite/satellite_variable_window/window_menu.cpp)|$(grep -c 'g_menu_model_get_n_items(G_MENU_MODEL(model_of(\*raw))) + 1' satellite/satellite_variable_window/window_menu.cpp)"

cat > build/window_submenu_ok.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_open(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display(the_menu.text)
}
satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("t", 800, 600)
    satellite.variable.window file = satellite.window.menu("File")
    satellite.variable.window recent = satellite.window.menu("Recent")
    recent.item(when_open, "one.satl")
    file.item(when_open, "Open")
    file.separator()
    file.menu(recent)
    w.menu(file)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_submenu_ok.satl > build/window_submenu_ok.out 2>&1
expect "a separator and a menu inside a menu pass the checker, and stop only for want of a screen" 50 $?

cat > build/window_separator_arity.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window m = satellite.window.menu("File")
    m.separator(1)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_separator_arity.satl > build/window_separator_arity.out 2>&1
expect "m.separator with an argument is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_separator_arity.out)"
expect "the window holds a column, and the bar goes above the fixed in it" "1|1" \
       "$(grep -c 'gtk_widget_set_vexpand(inside, TRUE)' satellite/satellite_variable_window/satellite_window.cpp)|$(grep -c 'gtk_box_prepend(GTK_BOX(column), bar)' satellite/satellite_variable_window/window_menu_bar.cpp)"

# ---------------------------------------------------------------------------
# A CAPSULE TAKES ARGUMENTS (2026-09-21). The walker ignored a capsule's declared
# parameters entirely until now -- every capsule was entered with a fresh empty
# VariableTable -- which is why a pressed capsule could print and write files and
# touch nothing else. These rows are the language half; the window half is above.
# ---------------------------------------------------------------------------
cat > build/capsule_args.satl <<'CAP_EOF'
satellite.include(satellite)
satellite.capsule add_them(satellite.variable.number a, satellite.variable.number b)
{
    satellite.console.display(a + b)
}
satellite.capsule satellite.main()
{
    satellite.variable.number x = 40
    add_them(2, 3)
    add_them(x, 2)
    satellite.return(satellite)
}
CAP_EOF
expect "a capsule is handed its arguments, and an argument may be an expression" "5|42" \
       "$("$interpreter" build/capsule_args.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"

cat > build/capsule_count.satl <<'CAP_EOF'
satellite.include(satellite)
satellite.capsule show(satellite.variable.number n)
{
    satellite.console.display(n)
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    show(1, 2)
    satellite.return(satellite)
}
CAP_EOF
"$interpreter" build/capsule_count.satl > build/capsule_count.out 2>&1
expect "a capsule given the wrong number of arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/capsule_count.out)"

cat > build/capsule_type.satl <<'CAP_EOF'
satellite.include(satellite)
satellite.capsule show(satellite.variable.number n)
{
    satellite.console.display(n)
}
satellite.capsule satellite.main()
{
    show("text")
    satellite.return(satellite)
}
CAP_EOF
"$interpreter" build/capsule_type.satl > build/capsule_type.out 2>&1
expect "an argument that does not fit the declared type is refused" 27 $?
expect "... and names the parameter and what it was declared" 1 \
       "$(tr '\n' ' ' < build/capsule_type.out | grep -cF "show's n was declared satellite.variable.number")"

# A HEADER THE SCAN CANNOT READ is refused by the CHECKER, which has a line to
# blame -- capsules_in() has none. IN A PASS OF ITS OWN, because the CapsuleTable
# is an unordered_map: inside the body loop, whether a person saw this sentence
# or the far more confusing "show takes 0 arguments, and was given 1" depended on
# which capsule the hash happened to put first.
cat > build/capsule_header.satl <<'CAP_EOF'
satellite.include(satellite)
satellite.capsule show(satellite.variable.number)
{
    satellite.console.display("x")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    show(1)
    satellite.return(satellite)
}
CAP_EOF
"$interpreter" build/capsule_header.satl > build/capsule_header.out 2>&1
expect "a parameter with no name is refused before anything runs" "13|" \
       "$?|$(grep -x before build/capsule_header.out)"
expect "... and blames the header, not the call" 1 \
       "$(tr '\n' ' ' < build/capsule_header.out | grep -cF 'satellite.capsule show -- satellite.variable.number declares a name')"

# satellite.main's OWN declared parameter IS BOUND SINCE 2026-09-22 -- it is the
# arguments variable (main_arguments.hpp). Until then run_main was handed nothing,
# and this row asserted it stayed a CHECKER refusal; now it asserts the program
# runs to the end and prints the arguments, "before" first.
cat > build/capsule_main_arg.satl <<'CAP_EOF'
satellite.include(satellite)
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("before")
    satellite.console.display(arguments)
    satellite.return(satellite)
}
CAP_EOF
"$interpreter" build/capsule_main_arg.satl > build/capsule_main_arg.out 2>&1
expect "satellite.main's declared parameter is bound: the program runs and shows the arguments" "0|before|1" \
       "$?|$(grep -x before build/capsule_main_arg.out)|$(grep -c '"username": ' build/capsule_main_arg.out)"

# WHAT A PRESS CAN HAND A CAPSULE is nothing, the piece, or the piece and its
# window -- so a capsule wanting anything else is refused where it is named.
cat > build/window_press3.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed(satellite.variable.window a, satellite.variable.window b, satellite.variable.window c)
{
    satellite.console.display("x")
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("x")
    b.pressed(when_pressed)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_press3.satl > build/window_press3.out 2>&1
expect "a pressed capsule wanting three arguments is refused before anything runs" "13|" \
       "$?|$(grep -x before build/window_press3.out)"

cat > build/window_presstype.satl <<'WIN_EOF'
satellite.include(satellite)
satellite.capsule when_pressed(satellite.variable.number n)
{
    satellite.console.display(n)
}
satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.window b = satellite.window.button("x")
    b.pressed(when_pressed)
    satellite.return(satellite)
}
WIN_EOF
headless build/window_presstype.satl > build/window_presstype.out 2>&1
expect "a pressed capsule wanting a number is refused -- a press hands it windows" "27|" \
       "$?|$(grep -x before build/window_presstype.out)"

mkdir -p build/alone && cp "$interpreter" build/alone/satl
build/alone/satl examples/hello_world.satl > /dev/null 2>&1; expect "no libraries beside the interpreter" 5 $?
# A satl deeper than the kernel can name (ERROR #8): refused with 5 and the reason, and
# never the libraries of the folder it was started in -- this folder holds a working set.
rm -rf build/deep && mkdir -p build/deep
deep_result=$(top=$PWD; libraries=$(dirname "$interpreter")/satellite-numbers; name=$(printf 'd%.0s' $(seq 100))
    # cd -P, NOT cd. /bin/sh is bash, and bash invoked as sh runs in POSIX mode,
    # where cd keeps a LOGICAL $PWD and refuses once that string passes PATH_MAX --
    # "cd: dddd...: File name too long" around level 40 of 45. The subshell then
    # exits before the echo and this check reads back EMPTY, which looks like satl
    # failing and is the harness failing. cd -P chdirs to the real directory and
    # reaches all 45 levels. Measured 2026-09-20: `sh check.sh` red, `bash check.sh`
    # green, same binary -- which is the sort of difference that sends somebody
    # hunting through the interpreter for a bug that is in the test.
    cd build/deep && for i in $(seq 45); do mkdir "$name" && cd -P "$name" || exit; done
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
# two digit counts are rows: 128 held (4096 until 2026-09-18), 32 shown, "both digits
# configurable". The counter (2026-09-18) is the calculations before the INFINITY WARNING.
expect "--debug shows arguments.infinity from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity = $(config_row infinity) " build/debug.out)"
expect "--debug shows arguments.infinity_display from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity_display = $(config_row infinity_display) " build/debug.out)"
expect "--debug shows arguments.infinity.counter from the config" 1 "$(grep -c "^\[satellite\] arguments.infinity.counter = $(config_row infinity.counter) " build/debug.out)"
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
# WHERE A CAPSULE LIVES: A FILE OR A satellite.namespace (capsule_scopes.hpp, 2026-09-22).
# Until then every capsule of every file was one map by bare name and the LAST file
# read won -- so a program's own greet() ran an included file's greet, and an
# included file's satellite.main ran instead of the program's. `other.greet()` was
# S201. These rows are the rules the header lists, one program each, and every
# refusal is asserted to come from the CHECK with nothing printed before it.
rm -rf build/scopes && mkdir -p build/scopes/chain build/scopes/types/people build/scopes/objects/people
cat > build/scopes/other.satl <<'SCOPE_EOF'
satellite.capsule greet()
{
    satellite.console.display("other.greet")
}

satellite.space tools
{
    satellite.capsule shout(satellite.variable.string what)
    {
        satellite.console.display("other.tools.shout: " + what)
    }
}

satellite.capsule satellite.main()
{
    satellite.console.display("the main of other.satl")
    satellite.return(satellite)
}
SCOPE_EOF
cat > build/scopes/ok.satl <<'SCOPE_EOF'
satellite.include(satellite)
satellite.include(other)

satellite.capsule greet()
{
    satellite.console.display("ok.greet")
}

satellite.namespace tools   // a comment after the name
{
    // a comment inside a space
    satellite.capsule add(satellite.variable.number a, satellite.variable.number b)
    {
        satellite.variable.number sum = a + b
        satellite.console.display("tools.add " + sum.to_string())
        twice(sum)
        greet()
    }

    satellite.capsule twice(satellite.variable.number n)
    {
        satellite.variable.number doubled = n * 2
        satellite.console.display("tools.twice " + doubled.to_string())
    }

    satellite.space inner
    {
        satellite.capsule deep()
        {
            satellite.console.display("tools.inner.deep")
            twice(21)
        }
    }
}

satellite.capsule satellite.main()
{
    greet()
    tools.add(2, 3)
    other.greet()
    other.tools.shout("through a file and a space")
    satellite.statement.if (1 < 2)
    {
        tools.inner.deep()
    }
    satellite.return(satellite)
}
SCOPE_EOF
"$interpreter" build/scopes/ok.satl > build/scopes/ok.out 2>/dev/null; code=$?
expect "a program's own greet() is its own, an included file's is other.greet(), and spaces nest" \
       "0|ok.greet|tools.add 5|tools.twice 10|ok.greet|other.greet|other.tools.shout: through a file and a space|tools.inner.deep|tools.twice 42" \
       "$code|$(tr '\n' '|' < build/scopes/ok.out | sed 's/|$//')"
expect "... and an included file's satellite.main never runs as the program's" 0 \
       "$(grep -c 'the main of other.satl' build/scopes/ok.out)"
# ONE REFUSAL A PROGRAM: $1 the file, $2 the top of it, $3 main's body.
scope_probe() {
    printf 'satellite.include(satellite)\nsatellite.include(other)\n%s\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n%s\n    satellite.return(satellite)\n}\n' \
        "$2" "$3" > "build/scopes/$1.satl"
    "$interpreter" "build/scopes/$1.satl" > "build/scopes/$1.out" 2>&1
}
scope_refused() {   # $1 the file, $2 the exit, $3 words the refusal must hold, $4 the row's name
    scope_probe "$1" "$5" "$6"; code=$?
    expect "$4" "$2|1|0" \
           "$code|$(tr '\n' ' ' < "build/scopes/$1.out" | sed 's/  */ /g' | grep -c "satl(check): .*$3")|$(grep -cx before "build/scopes/$1.out")"
}
scope_refused bare 13 "greet is a capsule of .*other.satl, and a file's capsules are reached through its name -- write other.greet" \
    "a bare name never reaches into another file, and the refusal says the spelling that does" '' '    greet()'
scope_refused space_twice 26 "tools is declared twice in this file" "a satellite.namespace declared twice is S202" \
    'satellite.space tools
{
}
satellite.namespace tools
{
}' ''
scope_refused capsule_twice 26 "twice is declared twice in this file -- it is already a capsule there" \
    "a capsule declared twice is S202, not the second silently winning" 'satellite.capsule twice()
{
}
satellite.capsule twice()
{
}' ''
scope_refused variable_in_space 13 "holds capsules, spacesuits and other spaces, not variables" \
    "a variable inside a space is refused (the author: variables belong to a capsule or a spacesuit)" 'satellite.space tools
{
    satellite.variable.number n = 5
}' ''
scope_refused space_in_capsule 14 "a space declared inside a capsule, that comes to exist when the capsule runs, is not built yet" \
    "a space declared inside a capsule is S210, not built yet" '' '    satellite.space inside
    {
    }'
scope_refused capsule_in_capsule 13 "satellite.capsule goes at the top of a file or inside a satellite.namespace" \
    "a capsule declared inside a capsule is refused by name" '' '    satellite.capsule inside()
    {
    }'
scope_refused spacesuit 13 "no capsule named hail" \
    "a spacesuit's capsules are its own, and never read as its file's (2026-09-22)" 'satellite.spacesuit ship()
{
    satellite.public
    {
        satellite.capsule hail()
        {
        }
    }
}' '    hail()'
scope_refused class 13 "no capsule named hail" \
    "satellite.class is a second spelling of satellite.spacesuit (words/aliases.tsv), and declares one" 'satellite.class ship()
{
    satellite.public
    {
        satellite.capsule hail()
        {
        }
    }
}' '    hail()'
scope_refused variable_named_like_a_file 26 "other is already the file other.satl this file includes, so a variable cannot be named other" \
    "a variable may not take the name of a file its file includes" '' '    satellite.variable.string other = "x"'
scope_refused no_such_member 13 "the file .*other.satl declares no capsule named nosuch" \
    "a name a file does not declare says which file" '' '    other.nosuch()'
scope_refused not_a_capsule 13 "tools.inner is a satellite.namespace and not a capsule" \
    "a space is not called" 'satellite.space tools
{
    satellite.space inner
    {
    }
}' '    tools.inner()'
scope_refused counted 13 "tools.add takes 2 arguments, and was given 1" \
    "a capsule reached through a space is given what it takes, before anything runs" 'satellite.space tools
{
    satellite.capsule add(satellite.variable.number a, satellite.variable.number b)
    {
    }
}' '    tools.add(1)'
scope_refused unclosed 13 "satellite.namespace tools is never closed" "a space the file ends inside is refused" \
    'satellite.space tools
{
    satellite.capsule x()
    {
    }' ''
# A FILE REACHES ONLY THE FILES IT INCLUDES ITSELF -- 003's rule, built narrow on
# purpose: the author is still deciding it (2026-09-22), and CapsuleTable::reach is
# the one place to widen it. This row pins today's answer and will move with his.
printf 'satellite.include(b)\n\nsatellite.capsule from_a()\n{\n    b.from_b()\n}\n' > build/scopes/chain/a.satl
printf 'satellite.capsule from_b()\n{\n    satellite.console.display("from b")\n}\n' > build/scopes/chain/b.satl
printf 'satellite.include(satellite)\nsatellite.include(a)\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n    a.from_a()\n    b.from_b()\n    satellite.return(satellite)\n}\n' > build/scopes/chain/main.satl
"$interpreter" build/scopes/chain/main.satl > build/scopes/chain.out 2>&1; code=$?
expect "a file a.satl includes is not reached from the file that includes a.satl -- the author's open question" "13|1|0" \
       "$code|$(tr '\n' ' ' < build/scopes/chain.out | sed 's/  */ /g' | grep -c 'b is a file that .*a.satl includes, and a file reaches only the files it includes itself')|$(grep -cx before build/scopes/chain.out)"
sed -i '/    b.from_b()$/d' build/scopes/chain/main.satl
expect "... and through a.satl it is" "0|from b" \
       "$("$interpreter" build/scopes/chain/main.satl > build/scopes/chain.out 2>/dev/null; echo $?)|$(grep -v '^before$' build/scopes/chain.out)"
# TWO FILES OF ONE NAME -- the author's view_forge_main.satl includes two people.satl.
printf 'satellite.capsule return_people_types()\n{\n    satellite.console.display("types/people")\n}\n\nsatellite.capsule both()\n{\n}\n' > build/scopes/types/people/people.satl
printf 'satellite.capsule return_people()\n{\n    satellite.console.display("objects/people")\n}\n\nsatellite.capsule both()\n{\n}\n' > build/scopes/objects/people/people.satl
printf 'satellite.include(satellite)\nsatellite.include("types/people/people.satl")\nsatellite.include("objects/people/people.satl")\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n    people.return_people_types()\n    people.return_people()\n    satellite.return(satellite)\n}\n' > build/scopes/people.satl
expect "two files of one name are both reached by it, each for what it declares" "0|before|types/people|objects/people" \
       "$("$interpreter" build/scopes/people.satl > build/scopes/people.out 2>/dev/null; echo $?)|$(tr '\n' '|' < build/scopes/people.out | sed 's/|$//')"
sed -i 's/    people.return_people()$/    people.both()/' build/scopes/people.satl
"$interpreter" build/scopes/people.satl > build/scopes/people.out 2>&1; code=$?
expect "... and a name BOTH declare is refused, naming both files" "13|1|0" \
       "$code|$(tr '\n' ' ' < build/scopes/people.out | sed 's/  */ /g' | grep -c 'people.both could be either of two files this file includes')|$(grep -cx before build/scopes/people.out)"
# A BUTTON NAMES A CAPSULE THROUGH A FILE. No display here, so the proof is that the
# CHECK passes and the run stops at the screen (S730); a wrong name stops at the check.
# press-a-button.sh and a press of helper.when_pressed beside main's own when_pressed
# are the real presses, run by hand on a compositor of their own.
printf 'satellite.include(satellite)\nsatellite.include(other)\n\nsatellite.capsule satellite.main()\n{\n    satellite.variable.window w = satellite.window.new("t", 200, 100)\n    satellite.variable.window b = satellite.window.button("press")\n    b.pressed(other.greet)\n    satellite.return(satellite)\n}\n' > build/scopes/press.satl
headless build/scopes/press.satl > build/scopes/press.out 2>&1; code=$?
expect "b.pressed(other.greet) names a capsule in another file, and passes the check" "50|1" \
       "$code|$(grep -c 'S730: NO_DISPLAY' build/scopes/press.out)"
sed -i 's/other.greet)/other.nosuch)/' build/scopes/press.satl
headless build/scopes/press.satl > build/scopes/press.out 2>&1; code=$?
expect "... and b.pressed(other.nosuch) is refused before a window is asked for" "13|1" \
       "$code|$(tr '\n' ' ' < build/scopes/press.out | sed 's/  */ /g' | grep -c 'the file .*other.satl declares no capsule named nosuch')"
expect "satellite.space and satellite.class are second spellings, with their words' codes" "2|2" \
       "$(grep -cE '\{"satellite\.(space|namespace)", 4514\},' satellite/bytecode/word_codes.hpp)|$(grep -cE '\{"satellite\.(class|spacesuit)", 4315\},' satellite/bytecode/word_codes.hpp)"
# SPACESUITS (2026-09-22): satellite.spacesuit, and satellite.class, declared as the
# author writes them -- satellite.protected, satellite.public and satellite.constructor
# beside them; an object made by declaring it, its constructor's arguments at the
# declaration; a spacesuit inside a spacesuit; one extending another; and capsules
# answering what satellite.return hands back, which ends the capsule from any depth.
# tests/spacesuits.satl walks all of it; each refusal below is one program, and every
# one the checker can make is asserted to come before anything ran.
"$interpreter" tests/spacesuits.satl > build/spacesuits.out 2>&1; code=$?
expect "spacesuits: objects, sections, nesting, a supertype and capsule answers" \
       "0|counter 2|8|10|more than five|1 0|7|kestrel has 7 plates, logged 2|osprey has 3 plates, logged 2|900|42|9|left the for at 3|[moon: depth 4] eclipse|11|view|view" \
       "$code|$(grep -v -e '^THE SATELLITE' -e '^VERSION' -e '^CLANG' -e '^G++' -e '^---' -e '^$' build/spacesuits.out | tr '\n' '|' | sed 's/|$//')"
rm -rf build/suits && mkdir -p build/suits
# ONE PROGRAM A REFUSAL: $1 the file, $2 the exit, $3 words the report must hold,
# $4 how many "before" lines ran first (0: the check refused it), $5 the row's name,
# $6 the top of the file after a counter spacesuit, $7 main's body.
suit_refused() {
    printf 'satellite.include(satellite)\n\nsatellite.spacesuit counter()\n{\n    satellite.protected\n    {\n        satellite.variable.number count = 0\n        satellite.capsule bump_by(satellite.variable.number n)\n        {\n            count = count + n\n        }\n    }\n\n    satellite.constructor(satellite.variable.number start)\n    {\n        count = start\n    }\n\n    satellite.public\n    {\n        satellite.capsule call_count() satellite.returns(satellite.variable.number)\n        {\n            satellite.return(count)\n        }\n        satellite.capsule call_nothing()\n        {\n            count = count + 1\n        }\n    }\n}\n%s\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n%s\n    satellite.return(satellite)\n}\n' \
        "$6" "$7" > "build/suits/$1.satl"
    "$interpreter" "build/suits/$1.satl" > "build/suits/$1.out" 2>&1; code=$?
    expect "$5" "$2|1|$4" \
           "$code|$(tr '\n' ' ' < "build/suits/$1.out" | sed 's/  */ /g' | grep -c -- "$3")|$(grep -cx before "build/suits/$1.out")"
}
suit_refused field_outside 52 "S230: MEMBER_IS_PROTECTED .*count is a field of counter and fields are reached from inside the spacesuit only" 0 \
    "obj.field is refused, public or not -- a field is reached inside its spacesuit only (003's S0517)" '' '    counter c(1)
    satellite.console.display(c.count)'
suit_refused field_written 52 "count is a field of counter" 0 "... and so is obj.field = x" '' '    counter c(1)
    c.count = 5'
suit_refused protected_outside 52 "bump_by is inside the satellite.protected part of counter" 0 \
    "a protected capsule is refused from outside its spacesuit (003's S0516)" '' '    counter c(1)
    c.bump_by(2)'
suit_refused no_member 13 "counter has no nosuch" 0 "a name the spacesuit does not have says so" '' '    counter c(1)
    c.nosuch()'
suit_refused constructor_count 13 "counter's satellite.constructor takes 1 argument, and c was given 0" 0 \
    "an object is given what its constructor takes, counted before anything runs" '' '    counter c'
suit_refused no_constructor 13 "plain p is declared with arguments, and plain has no satellite.constructor to take them" 0 \
    "arguments with no constructor to take them are refused (003's S0526)" 'satellite.spacesuit plain()
{
    satellite.public
    {
    }
}' '    plain p(3)'
suit_refused never_answers 53 "S240: CAPSULE_GAVE_NO_ANSWER .*c.call_nothing() is used where its answer would be, and it never hands one back" 0 \
    "a capsule that can never answer is refused where its answer is used" '' '    counter c(1)
    satellite.variable.number n = c.call_nothing()'
suit_refused ended_without 53 "maybe's answer is used, and it ended without handing one back" 1 \
    "a capsule that ended without one, the way it went, is refused when the answer is used" 'satellite.capsule maybe(satellite.variable.number n) satellite.returns(satellite.variable.number)
{
    satellite.statement.if (n > 0)
    {
        satellite.return(n)
    }
}' '    satellite.console.display(maybe(0))'
suit_refused wrong_answer 27 "five answers satellite.variable.number, and what this satellite.return handed back does not fit -- it holds a string" 1 \
    "an answer is measured against satellite.returns" 'satellite.capsule five() satellite.returns(satellite.variable.number)
{
    satellite.return("five")
}' '    satellite.console.display(five())'
suit_refused declared_but_empty 53 "five answers satellite.variable.number, and this satellite.return hands back nothing" 0 \
    "a capsule that declares satellite.returns must hand something back" 'satellite.capsule five() satellite.returns(satellite.variable.number)
{
    satellite.return()
}' ''
suit_refused in_a_capsule 14 "a satellite.spacesuit declared inside a capsule, that comes to exist when the capsule runs, is not built yet (POLYMORPH M1)" 0 \
    "a spacesuit inside a capsule is POLYMORPH M1, and not built" '' '    satellite.spacesuit inner()
    {
    }'
suit_refused no_such_type 25 "no spacesuit named ghost" 0 "a type nothing declares is refused by name" '' '    ghost g'
suit_refused protected_inner 25 "counter2.part is declared inside the satellite.protected part of the spacesuit counter2" 0 \
    "a spacesuit inside another one's satellite.protected is not named from outside it" 'satellite.spacesuit counter2()
{
    satellite.protected
    {
        satellite.spacesuit part()
        {
        }
    }
}' '    counter2.part p'
suit_refused field_names_field 25 "a is a field of early, and a field's value is worked out before there is an object" 0 \
    "a field's value cannot name another field (003's S0511)" 'satellite.spacesuit early()
{
    satellite.protected
    {
        satellite.variable.number a = 1
        satellite.variable.number b = a + 1
    }
}' ''
suit_refused parameter_like_field 26 "n is a field of clash, and a name is declared once" 0 \
    "a capsule's parameter cannot take a field's name" 'satellite.spacesuit clash()
{
    satellite.protected
    {
        satellite.variable.number n = 0
    }
    satellite.public
    {
        satellite.capsule call_set(satellite.variable.number n)
        {
        }
    }
}' ''
suit_refused bare_method_from_outside 13 "no capsule named call_count" 0 \
    "a spacesuit's capsule is not reached by its bare name from outside it" '' '    call_count()'
suit_refused cxx_constructor 13 "view() with a body is how C++ writes a constructor -- a satellite spacesuit's is satellite.constructor" 0 \
    "C++'s constructor spelling is named, not taken (003 refused it too, S0207)" 'satellite.spacesuit view()
{
    satellite.public
    {
        view()
        {
        }
    }
}' ''
suit_refused section_in_section 13 "satellite.public is written inside another section of the spacesuit odd" 0 \
    "a section inside a section is refused -- it means nothing" 'satellite.spacesuit odd()
{
    satellite.protected
    {
        satellite.public
        {
        }
    }
}' ''
suit_refused cycle 13 "a extends itself -- following the brackets from a leads back to a" 0 \
    "a spacesuit extending itself, however far round, is refused (003's S0520)" 'satellite.spacesuit a(b)
{
}
satellite.spacesuit b(a)
{
}' ''
suit_refused built_supertype 14 "satellite.spacesuit a(satellite.variable.string) -- a spacesuit extending a built type is MILESTONES M35" 0 \
    "a built type as a supertype is M35, still the author's to decide" 'satellite.spacesuit a(satellite.variable.string)
{
}' ''
suit_refused two_supertypes 13 "extends ONE spacesuit" 0 "a spacesuit has one supertype or none" 'satellite.spacesuit a(counter, counter)
{
}' ''
suit_refused supertype_arguments 13 "more extends counter, whose satellite.constructor takes 1 argument, and nothing can hand them over" 0 \
    "a supertype's constructor that wants arguments, under a subtype's own, is refused" 'satellite.spacesuit more(counter)
{
    satellite.constructor(satellite.variable.string word)
    {
    }
}' ''
suit_refused protected_through_a_subtype 52 "bump_by is inside the satellite.protected part of counter" 0 \
    "a supertype's protected capsule is not reached through a subtype's object from outside" 'satellite.spacesuit more(counter)
{
}' '    more m(1)
    m.bump_by(3)'
suit_refused made_in_a_line 13 "hull is a spacesuit, and an object of it is made by declaring one on a line of its own" 0 \
    "a spacesuit's name with brackets inside a line says how an object is made" 'satellite.spacesuit hull()
{
}' '    satellite.variable.number n = hull()'
# A CAPSULE A SUBTYPE DECLARES AGAIN must be called as the one it replaces is, because a
# name declared as the supertype may hold the subtype's object (the review, 2026-09-22).
suit_refused override_protected 13 "hidden's call_count replaces counter's for its objects, and it is in satellite.protected and the one it replaces is public" 0 \
    "an override may not be less reachable than what it replaces" 'satellite.spacesuit hidden(counter)
{
    satellite.protected
    {
        satellite.capsule call_count() satellite.returns(satellite.variable.number)
        {
            satellite.return(0)
        }
    }
}' ''
suit_refused override_arguments 13 "wide's call_count replaces counter's for its objects, and it takes different arguments" 0 \
    "... nor take different arguments" 'satellite.spacesuit wide(counter)
{
    satellite.public
    {
        satellite.capsule call_count(satellite.variable.number n) satellite.returns(satellite.variable.number)
        {
            satellite.return(n)
        }
    }
}' ''
suit_refused override_answer 13 "it answers satellite.variable.string and the one it replaces answers satellite.variable.number" 0 \
    "... nor answer something else" 'satellite.spacesuit worded(counter)
{
    satellite.public
    {
        satellite.capsule call_count() satellite.returns(satellite.variable.string)
        {
            satellite.return("many")
        }
    }
}' ''
suit_refused ring 13 "making a node makes a node for its field next, and that comes back round to making a node again" 0 \
    "a field that makes its own spacesuit, however far round, is refused -- it would never finish" 'satellite.spacesuit node()
{
    satellite.protected
    {
        node next(1)
    }
    satellite.constructor(satellite.variable.number n)
    {
    }
}' ''
suit_refused pressed_a_method 13 "call_tick is a capsule of the spacesuit ticker, and it runs on an object" 0 \
    "a button cannot run a spacesuit's capsule -- a press has no object to give it" 'satellite.spacesuit ticker()
{
    satellite.public
    {
        satellite.capsule call_tick()
        {
        }
        satellite.capsule call_wire()
        {
            satellite.variable.window b = satellite.window.button("x")
            b.pressed(call_tick)
        }
    }
}' ''
suit_refused list_item_member 53 "xs\[...\].call_nothing() is used where its answer would be" 0 \
    "an item's member is judged by what its list was declared to hold, before anything runs" '' '    satellite.container.list<counter> xs = {}
    satellite.console.display(xs[1].call_nothing())'
# A METHOD FOLLOWS THE OBJECT, CALLED BARE OR WITH A DOT: the supertype's capsule calls
# call_name bare, and on a circle the circle's runs (the review's question, 2026-09-22).
printf 'satellite.include(satellite)\n\nsatellite.spacesuit shape()\n{\n    satellite.public\n    {\n        satellite.capsule call_name() satellite.returns(satellite.variable.string)\n        {\n            satellite.return("a shape")\n        }\n        satellite.capsule call_describe() satellite.returns(satellite.variable.string)\n        {\n            satellite.return("I am " + call_name())\n        }\n    }\n}\n\nsatellite.spacesuit circle(shape)\n{\n    satellite.public\n    {\n        satellite.capsule call_name() satellite.returns(satellite.variable.string)\n        {\n            satellite.return("a circle")\n        }\n    }\n}\n\nsatellite.capsule satellite.main()\n{\n    circle c\n    shape s\n    satellite.console.display(c.call_describe())\n    satellite.console.display(s.call_describe())\n    satellite.return(satellite)\n}\n' > build/suits/follows.satl
expect "a bare call to a replaced capsule runs the object's own, as a dotted one does" "0|I am a circle|I am a shape" \
       "$("$interpreter" build/suits/follows.satl > build/suits/follows.out 2>/dev/null; echo $?)|$(grep -e '^I am' build/suits/follows.out | tr '\n' '|' | sed 's/|$//')"
# A CALL TO ITSELF AND THEN A satellite.return THAT HANDS BACK NOTHING, anywhere in the
# capsule, is a last call now that return ends the capsule from any depth -- on 8 MiB,
# 100,000 deep, where a real frame a level dies near 2,500 (the review, 2026-09-22).
printf 'satellite.include(satellite)\n\nsatellite.capsule down(satellite.variable.number n)\n{\n    satellite.statement.if (n > 0)\n    {\n        down(n - 1)\n        satellite.return()\n    }\n    satellite.console.display("reached the bottom")\n}\n\nsatellite.capsule satellite.main()\n{\n    down(100000)\n    satellite.return(satellite)\n}\n' > build/suits/call_then_return.satl
( ulimit -Ss 8192; ulimit -Hs 8192; "$interpreter" build/suits/call_then_return.satl > build/suits/call_then_return.out 2>/dev/null ) 2>/dev/null; code=$?
expect "a call to itself followed by satellite.return() is a last call, 100,000 deep on 8 MiB" "0|1" \
       "$code|$(grep -c 'reached the bottom' build/suits/call_then_return.out)"
# words_004.tsv is typed by hand, so make_words.py refuses a row it cannot trust --
# checked through the real script and 003's real satl, which is gitignored.
# THE TWO BYTECODE HEADERS ARE GENERATED, AND NOTHING ELSE CHECKED THAT THEY ARE STILL
# WHAT THE GENERATORS WRITE (INF-1's review, 2026-09-18). Both scripts run in a copy of
# their folders under build/, so the committed files are never rewritten, and each
# header must come out byte-identical -- which also runs make_token_codes.py's own
# checks (the free rows) on every check.sh.
rm -rf build/generators && mkdir -p build/generators/satellite/bytecode build/generators/words
cp REGISTRY.satellite build/generators/ && cp words/words.tsv words/aliases.tsv build/generators/words/
cp satellite/bytecode/make_token_codes.py satellite/bytecode/make_word_codes.py build/generators/satellite/bytecode/
python3 build/generators/satellite/bytecode/make_token_codes.py > build/generators/tokens.out 2>&1; code=$?
expect "make_token_codes.py runs clean on REGISTRY.satellite" 0 $code
cmp -s build/generators/satellite/bytecode/token_codes.hpp satellite/bytecode/token_codes.hpp; code=$?
expect "token_codes.hpp is exactly what make_token_codes.py writes" 0 $code
python3 build/generators/satellite/bytecode/make_word_codes.py > build/generators/words.out 2>&1; code=$?
expect "make_word_codes.py runs clean on words.tsv" 0 $code
cmp -s build/generators/satellite/bytecode/word_codes.hpp satellite/bytecode/word_codes.hpp; code=$?
expect "word_codes.hpp is exactly what make_word_codes.py writes" 0 $code
python3 words/check_make_words.py > build/check_make_words.out 2>&1; code=$?
if [ $code = 2 ]; then echo "  skip  make_words.py's checks: no 003 satl at old_versions/second_satellite/satl"
else expect "make_words.py against rows typed by hand: $(tail -1 build/check_make_words.out) (build/check_make_words.out)" 0 $code; fi
expect "the start-up threads are warm" 1 "$(grep -cE "^\[satellite\] threads.startup\(warm\): $(config_row threads_startup) threads parked in [0-9.]+ ms" build/debug.out)"

# THE ARGUMENTS VARIABLE (the author, 2026-09-22: "so main will become satellite.capsule
# satellite.main(satellite.variable.arguments anything_typed_in_here)"): every row satl
# holds, by its name after the variable's -- a config row, a command-line word, a fact
# gathered at start-up, and a live one (memory.used).
"$interpreter" tests/arguments_variable.satl one "two words" > build/arguments_variable.out 2>/dev/null; code=$?
expect "main's satellite.variable.arguments reads rows by name" "0|$(id -un)|one|two words|3|true|true|128|$(id -un)" \
       "$code|$(tr '\n' '|' < build/arguments_variable.out | sed 's/|$//')"
expect "... and the older list<string> spelling is the same arguments" "one" \
       "$("$interpreter" tests/arguments_old_spelling.satl one 2>/dev/null)"
"$interpreter" tests/arguments_misspelled.satl > build/arguments_bad.out 2>&1; code=$?
expect "a row that is not an argument is refused by name" "25|1" \
       "$code|$(tr '\n' ' ' < build/arguments_bad.out | grep -c 'args.usernme is not one of the arguments')"

# AND WRITTEN (the author, 2026-09-23: "satellite.variable.arguments any_name then
# any_name.some_var = some_value"): a name of the program's own is added and then
# changed, found by [] and .contains, holds a container it changes in place, and is
# shown after satl's rows (tests/arguments_written.satl says what each line is).
"$interpreter" tests/arguments_written.satl one > build/arguments_written.out 2>/dev/null; code=$?
expect "a name of the program's own is written into the arguments, and shown after satl's rows" \
       "0|5|6|text|one|$(id -un)|6|true|more|{\"first\", \"more\"}|{\"kept\"}|new|1" \
       "$code|$(head -11 build/arguments_written.out | tr '\n' '|')$(tail -1 build/arguments_written.out | grep -c ', "some_var": 6, "deep.row": "text", "saved": "new"}$')"
"$interpreter" tests/arguments_not_written.satl > build/arguments_not_written.out 2>&1; code=$?
expect "... and a row satl holds is refused before anything runs" "35|0|1" \
       "$code|$(grep -cx before build/arguments_not_written.out)|$(tr '\n' ' ' < build/arguments_not_written.out | grep -c 'args.memory.total is a row satl holds')"
# EVERY WAY A WRITE IS REFUSED (the review, 2026-09-23, found three of them untested): a
# row config.ini gave, a command-line name this run was not given, a name inside satl's
# row, a name inside the program's own row (only the walker knows those, so "before"
# prints), the same row written with brackets, and +=. <line> <wanted> <words it says>.
arguments_refuses() {
    printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main(satellite.variable.arguments args)\n{\n    satellite.console.display("before")\n    args.l = satellite.container.list()\n%s\n    satellite.return(satellite)\n}\n' "$1" > build/arguments_refused.satl
    "$interpreter" build/arguments_refused.satl one two > build/arguments_refused.out 2>&1; code=$?
    expect "... refused: ${1#    }" "$2" \
           "$code|$(grep -cx before build/arguments_refused.out)|$(tr '\n' ' ' < build/arguments_refused.out | grep -cF "$3")"
}
arguments_refuses '    args.infinity = 64' "35|0|1" 'args.infinity is a row satl holds'
arguments_refuses '    args.argument_7 = "x"' "35|0|1" 'args.argument_7 is a row satl holds'
arguments_refuses '    args.length.hex = 5' "35|0|1" 'args.length.hex is inside args.length, a name satl holds'
arguments_refuses '    args.l.size = 99' "35|1|1" "args.l.size is inside args.l, a row of the program's own"
arguments_refuses '    args["username"] = "x"' "35|1|1" 'args.username is a row satl holds'
arguments_refuses '    args.n += 1' "14|0|1" 'args.n += ... is not built yet'
# AND access, A SETTING, IS WRITTEN THROUGH -- to config.ini and to the variable's own
# copy -- in a home of its own, so the suite's config.ini is not the one changed under
# the rows after this; and it takes true or false only, by either spelling.
access_home=$PWD/build/arguments-access-home
rm -rf -- "$access_home"
mkdir -p -- "$access_home/.satl"
HOME=$access_home "$interpreter" --rebuild > build/arguments-access-rebuild.out 2>&1
printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main(satellite.variable.arguments a)\n{\n    satellite.console.display(a.access)\n    a.access = satellite.bool.false\n    satellite.console.display(a.access)\n    satellite.console.display(a)\n    a["access"] = 7\n}\n\nsatellite.return(satellite)\n' > build/arguments_access.satl
HOME=$access_home "$interpreter" build/arguments_access.satl > build/arguments_access.out 2>&1; code=$?
expect "... and access, a setting, is written through to config.ini and the copy, and takes true or false only" \
       "34|true|false|1|1|1" \
       "$code|$(grep -xE 'true|false' build/arguments_access.out | tr '\n' '|')$(grep -cx 'access = false' "$access_home/.satl/config.ini")|$(grep -c '"access": false' build/arguments_access.out)|$(tr '\n' ' ' < build/arguments_access.out | grep -c 'a.access is true or false, and was given a number')"

# WHAT THE PROCESSOR CAN RUN (the author, 2026-09-23: "arguments.cpu.architecture =
# "haswell" ... arguments.cpu.features = AVX, AVX2, 512-bit stuff, all that in a single
# list"; satellite/arguments/cpu_facts.hpp): asked against /proc/cpuinfo's own flags and
# not against the builtin satl asks -- x86-64-v3 is avx avx2 bmi1 bmi2 f16c fma abm
# movbe xsave there, and 003's word for a machine with all of it is haswell.
printf 'satellite.include(satellite)\n\nsatellite.capsule satellite.main(satellite.variable.arguments a)\n{\n    satellite.console.display(a.cpu.architecture)\n    satellite.console.display(a.cpu.features.contains("x86-64-v3"))\n    satellite.console.display(a.cpu.features.contains("avx2"))\n    satellite.console.display(a.cpu.features.contains("avx512f"))\n    satellite.console.display(a.cpu.features.contains("x86-64"))\n    satellite.return(satellite)\n}\n' > build/arguments_cpu.satl
"$interpreter" build/arguments_cpu.satl > build/arguments_cpu.out 2>/dev/null; code=$?
cpu_flags=" $(grep -m1 '^flags' /proc/cpuinfo | sed 's/^flags[^:]*://') "
cpu_flag_is() { case "$cpu_flags" in *" $1 "*) echo true;; *) echo false;; esac; }
cpu_v3=true
for flag in avx avx2 bmi1 bmi2 f16c fma abm movbe xsave; do [ "$(cpu_flag_is $flag)" = true ] || cpu_v3=false; done
cpu_word=baseline; [ $cpu_v3 = true ] && cpu_word=haswell
expect "arguments.cpu.architecture and arguments.cpu.features say what /proc/cpuinfo says" \
       "0|$cpu_word|$cpu_v3|$(cpu_flag_is avx2)|$(cpu_flag_is avx512f)|$([ "$(uname -m)" = x86_64 ] && echo true || echo false)" \
       "$code|$(tr '\n' '|' < build/arguments_cpu.out | sed 's/|$//')"

# satl-cpu-level CHOOSES BY WHAT A BUILD NEEDS (MILESTONES M37, make_support/055-cpus.mk): a
# folder of three builds -- x86-64-v2's real list, which a machine with /proc/cpuinfo's
# sse4_2 popcnt ssse3 cx16 lahf_lm runs; one needing a macro nobody can ask about; and one
# whose list is empty, as a failed compiler once left it. Neither of the last two is ever
# chosen. The lists are asserted not empty by themselves, so an empty one cannot pass.
#
# THE COMPILER THE BUILD USED, from its own record, and unquoted, since it may be two words
# (ccache clang++) -- not $CXX, which a shell may not have, and not $HOME/opt, which is the
# suite's own home by here.
cpu_compiler=$(sed 's/ \[.*//' build/.compile-flags)
cpu_macros='s/^#define \(__[A-Z0-9_]*__\) 1$/\1/p; s/^#define \(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16\) 1$/\1/p'
cpu_probe=build/cpu-level-probe
rm -rf -- "$cpu_probe"
mkdir -p -- "$cpu_probe/x86-64-v2" "$cpu_probe/unknowable" "$cpu_probe/empty"
$cpu_compiler -dM -E -x c++ /dev/null | sed -n "$cpu_macros" | LC_ALL=C sort > "$cpu_probe/base"
$cpu_compiler -march=x86-64-v2 -dM -E -x c++ /dev/null | sed -n "$cpu_macros" | \
    LC_ALL=C sort | LC_ALL=C comm -23 - "$cpu_probe/base" > "$cpu_probe/x86-64-v2/needs"
echo __NOT_A_FEATURE__ > "$cpu_probe/unknowable/needs"
: > "$cpu_probe/empty/needs"
for probe in x86-64-v2 unknowable empty; do printf '#!/bin/sh\n' > "$cpu_probe/$probe/satl"; chmod +x "$cpu_probe/$probe/satl"; done
cpu_v2=x86-64-v2
for flag in sse4_2 popcnt ssse3 cx16 lahf_lm; do [ "$(cpu_flag_is $flag)" = true ] || cpu_v2=baseline; done
build/satl-cpu-level --explain "$cpu_probe" > build/cpu-level-probe.out 2>&1
expect "satl-cpu-level chooses a build this machine can run, never one it cannot ask about or an empty list" \
       "yes|yes|$cpu_v2|1|1|1" \
       "$([ -s "$cpu_probe/base" ] && echo yes)|$(grep -qx __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16 "$cpu_probe/x86-64-v2/needs" && echo yes)|$(build/satl-cpu-level "$cpu_probe")|$(grep -c 'unknowable .*lacks 1: __NOT_A_FEATURE__ (satl-cpu-level cannot ask about it)' build/cpu-level-probe.out)|$(grep -c 'empty .*lacks 1: an empty list' build/cpu-level-probe.out)|$(build/satl-cpu-level --runs "$cpu_probe/unknowable/needs" > /dev/null; echo $?)"

# THE FOUR TYPES OF 2026-09-22 KEEP THEIR ROWS BESIDE THEIR CODE (each file says why).
for rows in satellite/satellite_variable_float/check_float.sh satellite/satellite_variable_hex/check_hex.sh \
            satellite/satellite_variable_color/check_color.sh satellite/satellite_variable_fraction/check_fraction.sh; do
    . "./$rows"
done

# THE STACK satl RUNS ON (machine/stack_share.hpp; the author, 2026-09-22: "32 kb for
# 1 megabyte of ram"). A capsule calling itself with work left after the call is a C++
# frame a level, and died at 2,526 deep on a shell's 8 MiB. 10,000 fits the 128 MiB
# floor on any machine -- unless a hard limit keeps satl from raising it at all.
hard_stack=$(ulimit -Hs)
if [ "$hard_stack" != unlimited ] && [ "$hard_stack" -lt 131072 ]; then
    echo "  skip  a capsule 10,000 deep: the hard stack limit here is $hard_stack KiB"
else
    "$interpreter" tests/recursion_in_the_middle.satl > build/recursion.out 2>/dev/null; code=$?
    expect "a capsule calling itself mid-body, 10,000 deep, on the raised stack" "0|reached the bottom|back in main" \
           "$code|$(tr '\n' '|' < build/recursion.out | sed 's/|$//')"
fi

# A CAPSULE'S LAST CALL TO ITSELF IS A LOOP (the author, 2026-09-22: "allow capsules to
# call themselves only as the last line ... and do the tail call thing, and specifically
# leave it broken -- we'll just crash the interpreter when a capsule calls itself in the
# middle"). Run on an 8 MiB stack, where a shape that is still a frame a level dies near
# 2,500 of the 100,000 each capsule goes. A call that is not last is an ordinary call and
# runs what follows it, in order; a last call still measures its arguments.
( ulimit -Ss 8192; ulimit -Hs 8192; "$interpreter" tests/tail_call.satl > build/tail_call.out 2>/dev/null ) 2>/dev/null; code=$?
expect "every shape of a capsule's last call to itself, 100,000 deep on 8 MiB" \
       "0|last in an if|last in an else|last in an else if|before a return|through its space|back in main" \
       "$code|$(tr '\n' '|' < build/tail_call.out | sed 's/|$//')"
"$interpreter" tests/tail_call_not_last.satl > build/tail_call.out 2>/dev/null; code=$?
expect "a call to itself that is not last runs what follows it" \
       "0|2|1|0|0|1|0|0|2|1|0|0|1|0|0|0|1|2|3|1|2|3" "$code|$(tr '\n' '|' < build/tail_call.out | sed 's/|$//')"
"$interpreter" tests/tail_call_wrong_type.satl > build/tail_call.out 2>&1; code=$?
expect "a last call to itself still measures its arguments" "27|1|0" \
       "$code|$(grep -c "down's n was declared satellite.variable.number, and it holds a string" build/tail_call.out)|$(grep -c 'NOT REACHED' build/tail_call.out)"

# satellite.library (the author, 2026-09-23: "we need to design it so that globals don't
# work, but satellite.library does work"): a value written once at the top of its file, one
# literal, read by every capsule of the file and through the file's name by a file that
# includes it -- and changed by nothing, or it would be a global (library_values.hpp).
"$interpreter" tests/library.satl > build/library.out 2>&1; code=$?
expect "satellite.library: every literal, another file's through its name, and read everywhere a value goes" \
       "0|25|-3|1.25|blue|b1010|x1E2A3A|50%|1/3|true|3|75|eulb|26|25|b1010|x1E2A3A|the if reads it|3|28|50|its own capsule reads settings label" \
       "$code|$(grep -v -e '^THE SATELLITE' -e '^VERSION' -e '^CLANG' -e '^G++' -e '^---' -e '^$' build/library.out | tr '\n' '|' | sed 's/|$//')"
rm -rf build/library && mkdir -p build/library/deep
printf 'satellite.include(satellite)\nsatellite.include("deep/inner")\n\nsatellite.library.majors = 50\n' > build/library/shelf.satl
printf 'satellite.include(satellite)\nsatellite.library.hidden = 7\n' > build/library/deep/inner.satl
# ONE PROGRAM A REFUSAL: $1 the file, $2 the exit, $3 words the report must hold, $4 the
# row's name, $5 lines at the top after `satellite.library.span = 25`, $6 main's body. The
# check refuses every one, so "before" never prints.
library_refused() {
    printf 'satellite.include(satellite)\nsatellite.include(shelf)\n\nsatellite.library.span = 25\n%s\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n%s\n    satellite.return(satellite)\n}\n' \
        "$5" "$6" > "build/library/$1.satl"
    "$interpreter" "build/library/$1.satl" > "build/library/$1.out" 2>&1; code=$?
    expect "$4" "$2|1|0" \
           "$code|$(tr '\n' ' ' < "build/library/$1.out" | sed 's/  */ /g' | grep -c -- "$3")|$(grep -cx before "build/library/$1.out")"
}
library_refused written 54 "S250: LIBRARY_VALUE_IS_FIXED .*satellite.library.span is written at the top of its file and never changes" \
    "a capsule writing a satellite.library value is refused -- that would be a global" '' '    satellite.library.span = 30'
library_refused added_to 54 "satellite.library.span is written at the top of its file and never changes" \
    "... and so is += on one" '' '    satellite.library.span += 1'
library_refused appended 54 "satellite.library.span is written at the top of its file and never changes" \
    "... and a method that changes it" '' '    satellite.library.span.append(1)'
library_refused made_in_a_capsule 54 "satellite.library.fresh is written at the top of its file and never changes" \
    "a capsule cannot make one either" '' '    satellite.library.fresh = 1'
library_refused only_read 13 "satellite.library.span is a value, and a line that only reads one does nothing" \
    "a line that only reads one is refused -- it does nothing" '' '    satellite.library.span'
library_refused never_written 25 "S201: NAME_NOT_DECLARED .*this file writes no satellite.library.nope" \
    "a value nothing wrote is refused by name" '' '    satellite.console.display(satellite.library.nope)'
library_refused another_files_bare 25 "satellite.library.majors is written in .*shelf.satl, and another file's values are reached through its name -- write satellite.library.shelf.majors" \
    "another file's value written bare is told the spelling that reaches it" '' '    satellite.console.display(satellite.library.majors)'
library_refused the_file_alone 25 "shelf is a file this file includes, and satellite.library.shelf names the file and no value in it" \
    "a file's name alone is not a value" '' '    satellite.console.display(satellite.library.shelf)'
library_refused not_in_the_file 25 "shelf.satl writes no satellite.library.nope" \
    "a value the included file does not write is refused" '' '    satellite.console.display(satellite.library.shelf.nope)'
library_refused not_included_here 25 "inner is a file that .*shelf.satl includes, and a file reaches only the files it includes itself" \
    "a file reaches only the values of files it includes itself, as it reaches capsules" '' '    satellite.console.display(satellite.library.inner.hidden)'
library_refused worked_out 54 "satellite.library.x is given something to work out, and a satellite.library value is written down" \
    "a value is one literal -- nothing runs outside a capsule to work one out" 'satellite.library.x = satellite.library.span + 1' ''
library_refused no_value 13 "satellite.library.x needs its value after an =" "a value with no = is refused" 'satellite.library.x' ''
library_refused two_names 13 "satellite.library.a.b has more than one name, and a satellite.library value has one" \
    "a value has one name -- the second name is a file's" 'satellite.library.a.b = 5' ''
library_refused written_twice 26 "S202: NAME_DECLARED_TWICE .*satellite.library.span is written twice in this file" \
    "a value written twice is refused" 'satellite.library.span = 26' ''
library_refused the_languages_own 13 "satellite.library.main is the language's own word" \
    "a name the language already has under satellite.library is refused" 'satellite.library.main = 5' ''
library_refused hash_colour 13 "a colour is written x000000 here, the hex it holds" \
    "003's #000000 is said by name -- 004 reads a colour's value as x000000" 'satellite.library.ground = #000000' ''
library_refused named_like_a_file 26 "this file already has a satellite.library value named shelf" \
    "a value named like a file this file includes is refused -- satellite.library.shelf.x would mean either" \
    'satellite.library.shelf = 5' ''
library_refused in_a_space 13 "a satellite.library value is written at the top of its file, not inside the satellite.namespace tools" \
    "a value inside a satellite.namespace is refused" 'satellite.namespace tools
{
    satellite.library.x = 1
}' ''
library_refused in_a_spacesuit 13 "a satellite.library value is written at the top of its file, not inside the spacesuit holder" \
    "a value inside a spacesuit is refused" 'satellite.spacesuit holder()
{
    satellite.library.x = 1
}' ''
# WHAT FOLLOWS A VALUE'S NAMES IS JUDGED BEFORE ANYTHING RUNS, as it is after a variable's
# (the review, 2026-09-23: each of these printed first and was refused running).
library_refused called 13 "satellite.library.span is a value, and a value is not called" \
    "brackets after a value are refused before anything runs" '' '    satellite.console.display(satellite.library.span(1))'
library_refused not_a_method 13 "satellite.library.span is satellite.variable.number, and y is not one of its methods" \
    "a name after a value that is no method is refused before anything runs" '' '    satellite.variable.number y = 1
    satellite.console.display(satellite.library.span.y)'
library_refused method_of_its_type 14 "satellite.library.span.size is not built for" \
    "a method is judged by the type the value's literal is, before anything runs" '' '    satellite.console.display(satellite.library.span.size)'
# A CHARACTER WITH NO CODE BEFORE A VALUE'S LINE -- a no-break space as indentation, a
# byte-order mark -- still leaves it a line of its own (the review, 2026-09-23: it was
# neither recorded nor refused).
printf '\357\273\277satellite.include(satellite)\n\302\240satellite.library.span = 25\n\302\240satellite.library.main = 5\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display(satellite.library.span)\n    satellite.return(satellite)\n}\n' > build/library/no_code.satl
"$interpreter" build/library/no_code.satl > build/library/no_code.out 2>&1; code=$?
expect "a value behind a no-break space is recorded, and a refused line behind one is refused" "13|1" \
       "$code|$(tr '\n' ' ' < build/library/no_code.out | sed 's/  */ /g' | grep -c 'satellite.library.main is the language.s own word')"
sed -i '/satellite.library.main = 5/d' build/library/no_code.satl
"$interpreter" build/library/no_code.satl > build/library/no_code.out 2>&1; code=$?
expect "... and read back" "0|25" "$code|$(grep -x 25 build/library/no_code.out)"
# A FILE NAMED LIKE A METHOD -- color.satl -- lexes as the method's code after the dot, and is
# still reached by the stem it was included by (the review, 2026-09-23).
printf 'satellite.include(satellite)\nsatellite.library.x = 5\n' > build/library/color.satl
printf 'satellite.include(satellite)\nsatellite.include(color)\n\nsatellite.capsule satellite.main()\n{\n    satellite.console.display(satellite.library.color.x)\n    satellite.return(satellite)\n}\n' > build/library/method_stem.satl
"$interpreter" build/library/method_stem.satl > build/library/method_stem.out 2>&1; code=$?
expect "a file named like a method has its values read through its stem" "0|5" "$code|$(grep -x 5 build/library/method_stem.out)"
sed -i 's/^satellite.include(color)$/satellite.include(color)\nsatellite.library.color = 9/' build/library/method_stem.satl
"$interpreter" build/library/method_stem.satl > build/library/method_stem.out 2>&1; code=$?
expect "... and a value named like it is refused as a name declared twice" "26|1" \
       "$code|$(tr '\n' ' ' < build/library/method_stem.out | sed 's/  */ /g' | grep -c 'this file already has a satellite.library value named color')"
printf 'satellite.console.display(satellite.library.span)\nsatellite.console.display("after it")\n' | "$interpreter" --repl > build/library/prompt.out 2>&1
expect "a typed line has no file, so it has no satellite.library values -- and the next line still runs" "1|1" \
       "$(grep -c 'a line typed at the prompt has no file behind it' build/library/prompt.out)|$(grep -cx 'after it' build/library/prompt.out)"


# ---------------------------------------------------------------------------
# display's NAMED OPTIONS AND satellite.console's SCREEN WORDS, WHICH 003 BUILT (its M30,
# "what ncurses does, without ncurses", and M14), AND THE COLOURS 004 ADDED (2026-09-23).
# The author, shown 003 refusing s.foreground(c), "text".foreground(c),
# satellite.console.foreground(c), satellite.terminal.foreground(c) and
# input(prompt, foreground=c): "Let's build 004 differently then, so every single one of
# those errors is legal". bytecode/console_style.hpp says what each colour means.
# ---------------------------------------------------------------------------
# INTO A FILE, EVERY LINE IS PLAIN TEXT (003's rule): no colour code anywhere, and the
# terminal's colours are not touched. home() is 003's bytes whatever stdout is.
printf 'bob\n' | "$interpreter" tests/console_from_003.satl > build/console_plain.out 2>/dev/null; code=$?
expect "into a file: display's options, end=, coloured strings, console colours, width, home, input -- all plain" \
       "0|warning|Loading... done|status: OK now|abc|sky|orange|green wins|plain|true|^[[Hname? hello bob" \
       "$code|$(cat -v build/console_plain.out | tr '\n' '|' | sed 's/|$//')"

# AT A TERMINAL, THE BYTES. display's options are 003's own bytes (ESC[1;3;38;2;...m,
# checked against 003 running under a pty); a string's colour ends with ESC[39m and an
# outer colour is re-opened after an inner one; a line's own foreground= wins over the
# console's; the terminal's background is OSC 11, and only it is put back (OSC 111) at the end.
at_a_terminal() {   # at_a_terminal <program> [typed] [--ctrl-c]: the bytes after the title, repr'd
    python3 - "$interpreter" "$@" <<'PTY_EOF'
import os, pty, select, sys
satl, program = sys.argv[1], sys.argv[2]
typed = sys.argv[3].encode() if len(sys.argv) > 3 and sys.argv[3] != "--ctrl-c" else b""
ctrl_c = "--ctrl-c" in sys.argv
pid, fd = pty.fork()
if pid == 0:
    env = dict(os.environ); env["TERM"] = "xterm-256color"; env.pop("NO_COLOR", None)
    if os.environ.get("CHECK_NO_COLOR"): env["NO_COLOR"] = "1"
    os.execve(satl, ["satl", program], env)
out, sent = b"", False
while True:
    ready, _, _ = select.select([fd], [], [], 20)
    if not ready: os.kill(pid, 9); out += b"<<TIMEOUT>>"; break
    try: chunk = os.read(fd, 65536)
    except OSError: break
    if not chunk: break
    out += chunk
    if not sent and b"? " in out:
        os.write(fd, b"\x03" if ctrl_c else typed + b"\r"); sent = True
_, status = os.waitpid(pid, 0)
text = out.decode("utf-8", "replace")
text = text[text.find("-----\r\n") + 7:] if "-----\r\n" in text else text
print(repr(text) + "|" + ("signal %d" % os.WTERMSIG(status) if os.WIFSIGNALED(status) else "exit %d" % os.WEXITSTATUS(status)))
PTY_EOF
}
expect "at a terminal: 003's bytes for display's options, and every colour 004 added, put back at the end" \
       "'\\r\\n\\x1b[1;3;38;2;255;136;0;48;2;0;0;0mwarning\\x1b[0m\\r\\nLoading... done\\r\\nstatus: \\x1b[38;2;0;255;0mOK\\x1b[39m now\\r\\n\\x1b[38;2;255;0;0ma\\x1b[38;2;0;0;255mb\\x1b[38;2;255;0;0mc\\x1b[39m\\r\\n\\x1b[48;2;135;206;235msky\\x1b[0m\\r\\n\\x1b[38;2;255;136;0morange\\x1b[0m\\r\\n\\x1b[38;2;0;255;0mgreen wins\\x1b[0m\\r\\nplain\\r\\ntrue\\r\\n\\x1b[H\\x1b]11;rgb:10/10/10\\x1b\\\\\\x1b[38;2;255;136;0mname? \\x1b[0mbob\\r\\nhello bob\\r\\n\\x1b]111\\x1b\\\\'|exit 0" \
       "$(at_a_terminal tests/console_from_003.satl bob)"

# NO_COLOR SET AND NOT EMPTY DROPS THE COLOURS AND KEEPS BOLD AND ITALIC (no-color.org),
# and leaves the terminal's own colours alone.
printf 'satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\n    satellite.terminal.foreground(xFF8800)\n    satellite.console.display("warning", foreground=xFF8800, bold=satellite.bool.true)\n    satellite.console.display("x".foreground(x00FF00))\n}\nsatellite.return(satellite)\n' > build/console_no_color.satl
expect "NO_COLOR: bold kept, every colour dropped, the terminal untouched" \
       "'\\r\\n\\x1b[1mwarning\\x1b[0m\\r\\nx\\r\\n'|exit 0" "$(CHECK_NO_COLOR=1 at_a_terminal build/console_no_color.satl)"

# CTRL-C PUTS THE TERMINAL'S COLOURS BACK before satl goes, and it still goes by SIGINT.
printf 'satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\n    satellite.terminal.foreground(xFF8800)\n    satellite.variable.string a = satellite.console.input("waiting? ")\n}\nsatellite.return(satellite)\n' > build/console_ctrl_c.satl
expect "Ctrl-C at the prompt after satellite.terminal.foreground writes OSC 110 -- only what it changed -- then dies of SIGINT" \
       "'\\r\\n\\x1b]10;rgb:ff/88/00\\x1b\\\\waiting? ^C\\x1b]110\\x1b\\\\'|signal 2" \
       "$(at_a_terminal build/console_ctrl_c.satl --ctrl-c)"

# THE END OF THE INPUT IS S830, EXIT 55 -- never an empty line answered forever.
"$interpreter" build/console_ctrl_c.satl < /dev/null > build/console_ended.out 2>&1; code=$?
expect "input() with the input ended stops with S830 INPUT_ENDED, exit 55" "55|1" \
       "$code|$(grep -c 'S830: INPUT_ENDED' build/console_ended.out)"

# EVERY WRONG SPELLING IS REFUSED BEFORE ANYTHING RUNS -- the "before" line is never
# printed -- except a see-through colour held in a variable, which is only a value running.
console_says() {   # console_says <lines> <sentence>: machine code | sentence found | "before" printed
    printf 'satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\n    satellite.console.display("before")\n%s\n}\nsatellite.return(satellite)\n' "$1" > build/console_bad.satl
    "$interpreter" build/console_bad.satl > build/console_bad.out 2>&1
    printf '%s|%s|%s' "$?" "$(tr '\n' ' ' < build/console_bad.out | sed 's/  */ /g' | grep -c "$2")" "$(grep -cx before build/console_bad.out)"
}
expect "an option display does not take is refused, naming the five it does" "13|1|0" \
       "$(console_says '    satellite.console.display("x", colour=xFF8800)' 'has no option called colour= -- it takes end=, foreground=, background=, bold= and italic=')"
expect "an option given twice is refused" "13|1|0" \
       "$(console_says '    satellite.console.display("x", end="", end="!")' 'end= is given twice')"
expect "a plain argument after a named one is refused" "13|1|0" \
       "$(console_says '    satellite.console.display(end="", "x")' 'a plain argument comes before every named one')"
expect "foreground= given text is refused before the run" "27|1|0" \
       "$(console_says '    satellite.console.display("x", foreground="red")' 'takes a colour -- six hex digits like xFF8800')"
expect "foreground= given three hex digits is refused before the run" "27|1|0" \
       "$(console_says '    satellite.console.display("x", foreground=xFFF)' 'exactly six hex digits like xFF8800, and xFFF has 3')"
expect "bold= given a number is refused before the run" "27|1|0" \
       "$(console_says '    satellite.console.display("x", bold=5)' 'bold= takes satellite.bool.true or satellite.bool.false')"
expect "a word with no options is told it takes none" "13|1|0" \
       "$(console_says '    satellite.feedback("x", end="")' 'satellite.feedback has no option called end= -- it takes no named options')"
expect "input takes the style options and not end=" "13|1|0" \
       "$(console_says '    satellite.variable.string a = satellite.console.input("a", end="")' 'it takes foreground=, background=, bold= and italic=')"
expect "a capsule given a named option is told a capsule's arguments go in order" "13|1|0" \
       "$(console_says '    satellite.console.display(twice(x=2))
}
satellite.capsule twice(satellite.variable.number x) satellite.returns(satellite.variable.number)
{
    satellite.return(x * 2)' 'x= is a named option, and only satellite.console.display and satellite.console.input take them')"
expect "satellite.console.foreground given text is refused before the run" "27|1|0" \
       "$(console_says '    satellite.console.foreground("red")' 'satellite.console.foreground takes a colour')"
expect "s.foreground() with no colour is refused before the run" "13|1|0" \
       "$(console_says '    satellite.variable.string s = "t"
    satellite.console.display(s.foreground())' 's.foreground takes one colour, in brackets')"
expect "a colour is given to text: 5.foreground(...) is refused before the run" "27|1|0" \
       "$(console_says '    satellite.console.display(5.foreground(xFF8800))' 'a colour is given to text')"
expect "a number variable has no .foreground" "14|1|0" \
       "$(console_says '    satellite.variable.number n = 5
    satellite.console.display(n.foreground(xFF8800))' 'so far it is a string.s')"
expect "a see-through colour is refused -- a terminal draws nothing see-through -- when the line runs" "27|1|1" \
       "$(console_says '    satellite.variable.color c = x000000, 50
    satellite.console.display("x", foreground=c)' 'a terminal draws nothing see-through')"
expect "input(prompt, target) is still not built" "14|1|0" \
       "$(console_says '    satellite.variable.string a = "z"
    satellite.console.input("p", a)' 'satellite.console.input(prompt, target) has no library built')"
expect "a variable named end is still a variable, and == is not an option" "0|1|1" \
       "$(console_says '    satellite.variable.string end = "e"
    satellite.console.display(end == "e")' 'before true')"
# THE FRESH READER'S FINDINGS, 2026-09-23, each pinned.
# A COLOURED STRING IS THE SAME STRING WHEREVER THE PROGRAM RUNS: .find and == answered
# one thing at a terminal and another in a pipe while the codes were only put in at a
# terminal. They are always in it now, and display is what leaves them out.
printf 'satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\n    satellite.variable.string s = "q".foreground(xFF0000)\n    satellite.console.display(s.find("q"))\n    satellite.console.display("OK".foreground(x00FF00) == "OK")\n}\nsatellite.return(satellite)\n' > build/console_same.satl
"$interpreter" build/console_same.satl > build/console_same.out 2>/dev/null
expect "a coloured string answers .find and == the same into a pipe as at a terminal" "15|false|'\\r\\n15\\r\\nfalse\\r\\n'|exit 0" \
       "$(tr '\n' '|' < build/console_same.out)$(at_a_terminal build/console_same.satl)"
# THE OPTION REFUSAL ONLY STRAIGHT AFTER A CALL'S `(`: a declaration with a comma in it
# is judged as it always was.
expect "a = 1, b = 2 is still a name nobody declared, not a named option" "25|1|0" \
       "$(console_says '    satellite.variable.number a = 1, b = 2' 'b has no satellite.variable line declaring it')"
expect "satellite.console.width() is told it is read with no brackets" "13|1|0" \
       "$(console_says '    satellite.console.display(satellite.console.width())' 'satellite.console.width is read with no brackets')"
expect "satellite.console.home(5) is told home() takes nothing" "13|1|0" \
       "$(console_says '    satellite.console.home(5)' 'satellite.console.home() takes nothing, and was given 1 argument')"
# AT THE PROMPT, input() READS THROUGH THE SESSION'S OWN READER: the next piped line
# was already in its buffer, and std::cin found the input ended.
printf 'satellite.console.display("got " + satellite.console.input("q? ", foreground=xFF8800))\nanswer\nsatellite.console.display("third line ran")\n' | \
    "$interpreter" --repl > build/console_repl.out 2>/dev/null; code=$?
expect "satl --repl: input() reads the next piped line, and the line after it still runs" "0|q? got answer|third line ran" \
       "$code|$(tail -2 build/console_repl.out | tr '\n' '|' | sed 's/|$//')"
# AND CTRL-C AT input() AT THE PROMPT stops that line with S810 and gives the prompt back.
python3 - "$interpreter" > build/console_repl_ctrl_c.out 2>&1 <<'PTY_EOF'
import os, pty, select, sys, time
pid, fd = pty.fork()
if pid == 0:
    env = dict(os.environ); env["TERM"] = "xterm-256color"
    os.execve(sys.argv[1], ["satl", "--repl"], env)
def drain(seconds):
    out = b""; end = time.time() + seconds
    while time.time() < end:
        ready, _, _ = select.select([fd], [], [], 0.2)
        if ready:
            try: out += os.read(fd, 65536)
            except OSError: break
    return out
drain(2)
os.write(fd, b'satellite.variable.string s = satellite.console.input("x? ")\r'); drain(2)
os.write(fd, b"\x03"); after = drain(3)
os.write(fd, b"exit\r"); drain(2)
_, status = os.waitpid(pid, 0)
print(int(b"S810: INTERRUPTED" in after), int(after.rstrip().endswith(b"C") and b">>" in after), os.waitstatus_to_exitcode(status))
PTY_EOF
expect "satl --repl: Ctrl-C at input() stops the line with S810 and the prompt comes back, then exit is 0" "1 1 0" \
       "$(tail -1 build/console_repl_ctrl_c.out)"
expect "foreground is registry row 0x0B57, and token_codes.hpp agrees" "1|1" \
       "$(grep -c '^0000101101010111  foreground_token ' REGISTRY.satellite)|$(grep -c 'Code foreground_token = 0x0B57;' satellite/bytecode/token_codes.hpp)"


# THREADS (2026-09-23, bytecode/thread_calls.hpp): the author's syntax,
# satellite.variable.thread t = satellite.thread.new(capsule(args)), with start(), join(),
# wait() and stop() -- and 003's M23 proofs (tests/threads.satl says what each line is).
"$interpreter" tests/threads.satl > build/threads.out 2>/dev/null; code=$?
expect "threads: new() does not run the capsule, join() and wait() answer, 8 threads x 200 = 1600 right, two names one thread, stop()" \
       "0|made|ran on its thread|joined|42|100|1600|14|true|(thread forever, not started)|(thread forever, stopped)|(thread twice, finished)|" \
       "$code|$(tr '\n' '|' < build/threads.out)"
"$interpreter" tests/threads_new_not_a_call.satl > build/threads_new.out 2> build/threads_new.err; code=$?
expect "threads: satellite.thread.new(5) is refused S721 before anything runs" "56|0|1" \
       "$code|$(wc -l < build/threads_new.out | tr -d ' ')|$(grep -c 'S721: THREAD_NEEDS_A_CAPSULE_CALL' build/threads_new.err)"
"$interpreter" tests/threads_started_twice.satl > build/threads_twice.out 2> build/threads_twice.err; code=$?
expect "threads: a second start() is S722, after the first run's line" "57|once|1" \
       "$code|$(tr -d '\n' < build/threads_twice.out)|$(grep -c 'S722: THREAD_ALREADY_STARTED' build/threads_twice.err)"
"$interpreter" tests/threads_join_before_start.satl > /dev/null 2> build/threads_early.err; code=$?
expect "threads: join() before start() is S723" "58|1" "$code|$(grep -c 'S723: JOIN_BEFORE_START' build/threads_early.err)"
"$interpreter" tests/threads_joined_twice.satl > build/threads_joined.out 2> build/threads_joined.err; code=$?
expect "threads: a second join() answers the same again, S724 said as a notice, exit 0" "0|8|8|8||1" \
       "$code|$(tr '\n' '|' < build/threads_joined.out)|$(grep -c '^\[satellite\] S724 THREAD_ALREADY_JOINED' build/threads_joined.err)"
"$interpreter" tests/threads_share_object.satl > build/threads_share.out 2>/dev/null; code=$?
expect "threads: an object handed to a thread is shared -- what the thread writes, main reads after the join" "0|0|5|" \
       "$code|$(tr '\n' '|' < build/threads_share.out)"
# THE AUTHOR'S LOCK (THREADS.md T2): off until obj.lock(), then a statement that writes the
# object holds it. Four threads on one locked object, and every count exact
# (tests/threads_lock.satl says what each line is).
"$interpreter" tests/threads_lock.satl > build/threads_lock.out 2>/dev/null; code=$?
expect "threads: .lock() on a shared object -- 4 threads x 5,000 adds = 20000, 4 x 1,000 appends = 4000, and a thread on the object's own capsule" \
       "0|20000|4000|20001|20002|" "$code|$(tr '\n' '|' < build/threads_lock.out)"
"$interpreter" tests/threads_window_on_a_thread.satl > build/threads_window.out 2> build/threads_window.err; code=$?
expect "threads: a window word on a thread is S727, and join() stops main with it" "62|0|1" \
       "$code|$(wc -l < build/threads_window.out | tr -d ' ')|$(grep -c 'S727: THREAD_CANNOT_SHARE_YET' build/threads_window.err)"
"$interpreter" tests/threads_failure_joined.satl > build/threads_fail.out 2> build/threads_fail.err; code=$?
expect "threads: a refusal on a thread is reported once, where it happened, and join() stops main with its code" "22|0|1" \
       "$code|$(wc -l < build/threads_fail.out | tr -d ' ')|$(grep -c 'SATELLITE CRITICAL ERROR REPORT' build/threads_fail.err)"
"$interpreter" tests/threads_failure_unjoined.satl > build/threads_unjoined.out 2> build/threads_unjoined.err; code=$?
expect "threads: a thread that fails and that nobody joins still fails the run, with its own code" "22|main ends|1" \
       "$code|$(tr -d '\n' < build/threads_unjoined.out)|$(grep -c 'SATELLITE CRITICAL ERROR REPORT' build/threads_unjoined.err)"
timeout 20 "$interpreter" tests/threads_never_joined.satl > build/threads_forever.out 2>/dev/null; code=$?
expect "threads: at satellite.return(satellite) a thread that would never end is stopped, and the run exits 0" \
       "0|main ends, and the thread is stopped for it" "$code|$(tr -d '\n' < build/threads_forever.out)"
"$interpreter" tests/threads_lines_whole.satl > build/threads_lines.out 2>/dev/null; code=$?
expect "threads: four threads display at once, 500 lines each, and every line comes out whole" \
       "0|500 alpha line|500 bravo line|500 charlie line|500 delta line|" \
       "$code|$(sort build/threads_lines.out | uniq -c | sed 's/^ *//' | tr '\n' '|')"
timeout 20 "$interpreter" tests/threads_empty_loop.satl > build/threads_empty.out 2>/dev/null; code=$?
expect "threads: stop() reaches a loop whose body is empty (the check is before the })" "0|(thread spin, stopped)" \
       "$code|$(tr -d '\n' < build/threads_empty.out)"
"$interpreter" tests/threads_answer_object.satl > build/threads_answer.out 2> build/threads_answer.err; code=$?
expect "threads: an answer that is an object is the same object to every join" "0|7|7|" \
       "$code|$(tr '\n' '|' < build/threads_answer.out)"
"$interpreter" tests/threads_lock_item.satl > build/threads_lock_item.out 2>/dev/null; code=$?
expect "threads: an append on an ITEM of a locked object's list field is a write -- 4 threads x 500 = 2000" \
       "0|2000" "$code|$(tr -d '\n' < build/threads_lock_item.out)"
# THE SECOND REVIEW'S DEFECTS (2026-09-24), each a test of its own.
"$interpreter" tests/threads_lock_method_name.satl > build/threads_lmn.out 2>/dev/null; code=$?
expect "threads: a line calling a capsule named like a method (add) is a write -- 4 x 5,000 = 20000" "0|20000" \
       "$code|$(tr -d '\n' < build/threads_lmn.out)"
# A JOIN LETS GO OF ITS LINE'S LOCKS WHILE IT WAITS (the author, 2026-09-24); a real circle is still S728.
timeout 20 "$interpreter" tests/threads_lock_wait_never_ends.satl > build/threads_wne.out 2>/dev/null; code=$?
expect "threads: total = w.join() lets go of the lock w needs while it waits, and answers" "0|1" \
       "$code|$(tr -d '\n' < build/threads_wne.out)"
timeout 20 "$interpreter" tests/threads_lock_field_join.satl > build/threads_lfj.out 2>/dev/null; code=$?
expect "threads: a thread kept in a locked object's field is joined from its capsule, and writes it" "0|1" \
       "$code|$(tr -d '\n' < build/threads_lfj.out)"
timeout 20 "$interpreter" tests/threads_lock_circle.satl > /dev/null 2> build/threads_circle.err; code=$?
expect "threads: two objects locked in opposite orders by two threads is S728 WAIT_NEVER_ENDS, not a frozen program" "63|1" \
       "$code|$(grep -c 'S728: WAIT_NEVER_ENDS' build/threads_circle.err)"
"$interpreter" tests/threads_override.satl > build/threads_override.out 2>/dev/null; code=$?
expect "threads: satellite.thread.new(call_speak()) in a dog runs the dog's override" "0|dog|dog|dog|" \
       "$code|$(tr '\n' '|' < build/threads_override.out)"
"$interpreter" tests/threads_lock_for.satl > build/threads_lfor.out 2>/dev/null; code=$?
expect "threads: a for's first part, condition and step read a locked list under its lock" "0|done" \
       "$code|$(tr -d '\n' < build/threads_lfor.out)"
# THE THIRD REVIEW'S TWO (2026-09-24): a join is not a write, and a reader does not queue into a circle.
"$interpreter" tests/threads_lock_list_join.satl > build/threads_llj.out 2>/dev/null; code=$?
expect "threads: workers[1].join() inside a locked object's capsule does not hold the object" "0|2" \
       "$code|$(tr -d '\n' < build/threads_llj.out)"
timeout 30 "$interpreter" tests/threads_lock_reader_join.satl > build/threads_lrj.out 2>/dev/null; code=$?
expect "threads: a line reading a locked object while it joins a reading thread ends, with a writer queued" "0|joined|ended|" \
       "$code|$(tr '\n' '|' < build/threads_lrj.out)"
expect "lock and unlock are registry rows 0x0B5B-0x0B5C, and token_codes.hpp agrees" "2|2" \
       "$(grep -c '^000010110101101[1]  lock_token \|^0000101101011100  unlock_token ' REGISTRY.satellite)|$(grep -c 'Code \(lock_token = 0x0B5B\|unlock_token = 0x0B5C\);' satellite/bytecode/token_codes.hpp)"
expect "start, stop and wait are registry rows 0x0B58-0x0B5A, and token_codes.hpp agrees" "3|3" \
       "$(grep -c '^00001011010110[01][01]  \(start_token\|stop_token\|wait_method_token\) ' REGISTRY.satellite)|$(grep -c 'Code \(start_token = 0x0B58\|stop_token = 0x0B59\|wait_method_token = 0x0B5A\);' satellite/bytecode/token_codes.hpp)"

echo "$passed passed, $failed failed"
[ "$failed" = 0 ]
