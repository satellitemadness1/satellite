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
expect "satl links nothing that can reach a network" 0 \
       "$(nm -D "$interpreter" 2>/dev/null | grep -icE 'socket|connect|getaddrinfo|SSL_|curl_')"

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
            for_power_stars for_step_other_name for_step_payload \
            for_step_prefix_minus for_step_is_the_name for_step_two_operators \
            for_step_doubled_then_more for_step_undecided_sign; do
    timeout 10 "$interpreter" "tests/$case.satl" > build/for.out 2>&1
    expect "$case is refused" 13 $?
    expect "... with nothing run before it" "" "$(grep -x before build/for.out)"
done
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
# A binary keeps a sign (the author, 2026-09-17: "give it a different number and keep a
# sign with all of these things"): -b0101 is a binary, worth -5, width kept.
expect "a binary below zero: shown, worth, converted, compared, turned over, given" \
       "-b0101|-5|-0101|-b0101|-5|-4|true|false|true|b0101|b0101|b0000|-b0011|-3" \
       "$("$interpreter" tests/binary_negative.satl 2>/dev/null | tr '\n' '|' | sed 's/|$//')"
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
