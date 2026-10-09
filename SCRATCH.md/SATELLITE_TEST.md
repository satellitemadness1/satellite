# satellite.test -- full(), speed(), loss(), all()

The author's design, 2026-10-06 (his words are in satellite/bytecode/test_calls.hpp), A-D all "yes":
all() runs the three; speed() runs the same work in C++ built into satl and prints both; build it
before NM-1; loss() is a CHAIN -- 547311173 goes from one type's test into the next.

## What is in it

| test | what runs | how long here |
|---|---|---|
| full()  | 149 programs (25 answers programs, 124 refused programs) = 789 checks, the whole set repeated to fill the minute | one round 10.6 s |
| speed() | 28 sections, each a satl program and its C++ twin, ~2.1 s of satl each | ~61 s |
| loss()  | one program: the chain through all 16 loss tests of official_test/loss_test/ | ~60 s |
| all()   | the three in turn | ~3 min |

The programs are in satellite/satellite_test/programs/ (full/, speed/, loss/, manifest.tsv), and
make_test_programs.py builds them into satl (test_programs.cpp, generated; check.sh compares it).
Written 2026-10-06 by seven writers, each proved on build 0039 (sealed): the writers' notes, proofs
and probes were in session be807508's scratchpad, st/<group>/; the minimal programs for every
finding below are copied into SATELLITE_TEST/findings/.

- full_structure_1..30: include, capsules, main, return, statements, comments, legal, delete,
  access, library, the arguments variable, info, return on a thread; 25 refusals.
- full_numbers_1..51: number, float, infinity, binary, hex, fraction, percentage, colour, random;
  42 refusals.
- full_text_1..52: string, bool and conversions, list, map, multiple, nested containers; 46 refusals.
- full_objects_1..16: spacesuits, threads, files, running programs, the console into a pipe;
  11 refusals.
- speed: the author's own speed test, its six loops one a section (from
  official_test/speed_test/official_speed_test.satl, his spacesuit byte for byte), then loops,
  statements, numbers of every size, float, percent, hex/binary, bool, conversions, fractions,
  capsules, objects, lists, maps, nested containers and strings.

## How it runs -- my choices, each one can be overruled

1. **Each program is a child satl**, `satl --run <name>`, started in an empty folder of its own
   (its working directory), with **a private HOME whose config.ini is satl's built-in defaults**
   (made by `satl --rebuild`, as check.sh makes its own), one home for the whole test, and
   SATL_NO_WINDOW=1. So a person's own settings cannot make a right satl look wrong (the review
   found infinity_display = 8 turning a right answer WRONG), none of their rows reaches a child
   (an absolute log_path sent the refusals into their log), the refusals full() exists to cause
   land in the private home's satellite.log and go with it, and nothing can write their
   config.ini. Built by giving program_spawn's start_a_program an optional ProgramPlace (folder +
   environment); satellite.variable.program and .bash are unchanged.
2. **An answers program is judged by its exact output, in order**: every line of its right run
   (its .out beside it) is a check, and a line it should not print is shown as it came. Exactly,
   because 14 lines are their own subject (a display, a thread's line, a console line) and cannot
   print WRONG. It must also write nothing to satellite.log (a warning goes only there).
   **A refused program** must stop with its exit code, its report must hold its phrase, and it
   must print nothing.
3. **full() repeats its whole set** (kFullRounds = 6 in test_calls.cpp) so it lasts about a minute.
   Half of each round is satellite.random's spin, which is by design (full_numbers_9).
4. **It can always be stopped.** The wait polls a tenth of a second at a time: a Ctrl-C (the
   prompt's, or from a file run -- then satl ends as Ctrl-C ends it, once the test's folder is
   gone and its child killed), the thread's stop(), the program's end, or a program running past
   its limit (60 s full, 120 s speed, 900 s loss: only a hang reaches them) stop it, and say so.
5. **The speed twins** were reformatted to this tree's brace style and put in namespace
   satellite004; their answers were compared with the writers' originals at passes 0, 1, 2, 3, 7,
   100 and 1000: identical.
6. **Windows are left out of full()**: a window needs a display, and the tests run without one.
7. **loss() lasts as long as the disk takes**: its file test closes (fsyncs) its file 10,000
   times. ~60 s alone here; 135-143 s while other programs fsync on the same disk.

## State at the end, 2026-10-06 22:00

- build/satl = BUILD 0045, NOT installed (~/.satl/satl is still 0039). check.sh 1281/1281.
- Measured sealed on 0043-0045: full() 4734 of 4734 right, 64.6 s; speed() 28 sections, satl 60.6 s,
  C++ 1.74 s; loss() on the new chain 547311173, 67.3 s (10,000 passes = the author's 1,000,000
  operations a test; 9,000 passes would be ~60 s -- left at his number).
- One fresh review (2026-10-06): 13 findings, all fixed (patch: backups/2026-10-06-before-test-review-fixes/).
- THE LOSS CHAIN was rebuilt from the 16 fixed tests (3,872 lines; tools in SATELLITE_TEST/chain_tools/,
  `build.sh <folder of the 16>`): 16 per-test injections and all 25 of the review's earlier injections
  change its last line. Its own fresh checker was STOPPED on the author's word before it reported: of
  its 2,436 changed copies, 178 still printed 547311173 -- NOT YET SORTED into real holes and changes
  that lose nothing. Some look real: a sign flipped every pass cancels over an even number of passes
  (col_worth_negate), a 10^-128 change rounded away in the map test (X_map_op27_minus_ulp). The record:
  SATELLITE_TEST/chain_check/all_results.txt (every copy) and still_547311173.txt (what each changed).
  The holes may be in the 16 official loss tests too, since the chain is made of them.
- Patches: backups/2026-10-06-before-satellite-test/satellite-test.patch (the whole feature),
  backups/2026-10-06-before-test-content/test-content.patch, backups/2026-10-06-before-test-review-fixes/.

## Findings for the author -- satl bugs the writers found, NOT fixed (his call)

HIGH
- A line that begins with `{` -- the list help's own refused example `{1, 2}.append(3)`, or a
  bare block -- silently ends the capsule: the rest of main never runs and satl exits 0.
  findings/brace_line.satl, bare_block.satl, brace_line_in_capsule.satl.

MEDIUM
- A `// comment` beside `satellite.statement.else` (after the if's `}`, on a line between, or after
  the else) breaks the if/else pairing: a valid program is refused (S110). finding_comment_*.satl.
- `satellite.main(satellite.variable.number n)` is accepted, and n holds the arguments index.
  finding_main_number_parameter.satl.
- A method on a negative literal runs before the minus: `-5.add(3)` is -8, `-7.string` is refused
  (ERRORS4/007, still there). f1_negative_literal_method.satl.
- A satellite.container.multiple holding a string refuses most string methods (S210) before
  anything runs, though its help says it behaves exactly as a plain string. multiple_upper.satl.

LOW
- satellite.main with two parameters starts running, then the second name is undeclared.
- `satellite.console.display(args)` shows a stale row after the bare name changed it in another
  capsule.
- A method straight after a `satellite.library.arguments.<row>` word is refused at run time.
- `satellite.help` in a program is refused as S210 NOT_BUILT_YET; its page says it is a prompt word.
- Binary `.width` is refused with "so far no type has it", but hex has .width (ERRORS4/049).
- The number help page quotes an old refusal for n.max(3) ("no capsule named max"); satl now says S210.
- Position 0 is refused as S413 COUNTS_FROM_ONE (exit 66), not the S501/S411 the help pages name.
- `f.path` answers the absolute resolved path, not the path as written (SATELLITE_FILE_OPERATIONS
  Part 7 says as written).

NOT A WRONG ANSWER, BUT SLOW: a map's `.remove(k)` costs time in proportion to the map's size
(~3.4 us on 10 keys, ~2.4 ms on 20,000); ~85% of the map section's time.

FROM THE LOSS TESTS (earlier today): a float's `.string` keeps only infinity_display places (32), so
float -> text -> float loses places 33-128.

## Questions the writers left (help pages do not settle them; not tested)

- {b0101} == {5} and {1.0} == {1} are false, while b0101 == 5 and 1.0 == 1 are true.
- The order among negative map keys (-40 comes before -5).
- 5 + "2.5" gives 7.5. `.remove(x)` with x there twice removes the first. `.insert(size + 1, x)`
  appends. "straße".upper() keeps the ß.
- console.clear() and .home() write escape codes into a pipe, where every sibling writes plain text.
- ok() on a spacesuit field that is still empty is refused at run time ("has no value yet").
