# FILE OPERATIONS -- the builders' brief (2026-10-07)

The author asked to FINISH `SATELLITE_FILE_OPERATIONS.md`: the milestones FO-5 to FO-10 that the
document still lists as not built. Read that document first (Part 3 = the model, Part 4 = the
rulings, Part 5 = the milestones, Part 7 = what was built on 2026-09-18 and the choices made), then
`SCRATCH.md/PLAN_DOCS_STATUS.md` section "SATELLITE_FILE_OPERATIONS.md" (what the code REALLY has,
checked against build 0043), then the code:

    satellite/satellite_variable_file/satellite_file.hpp     the class: a text file as a list of lines
    satellite/satellite_variable_file/satellite_file.cpp     how (1,097 lines)
    satellite/satellite_variable_file/file_cases.cpp         222 cases, no interpreter around them
    satellite/bytecode/file_calls.hpp / file_calls.cpp       the interpreter's side (words, methods, f[n])
    check.sh lines ~1936-1990                                the file rows (file_cases + tests/file_*.satl)
    DESIGN.md section 1 (the standards) and section 9 (the hostile inputs)

Facts that are easy to get wrong:

- LINES COUNT FROM 1 (the author: "all line counts all start at 1"). 0 is never a line; index_of
  and search answer 0 for "no such line"; f[0] is S413 (counts_from_one), a negative number is
  S410 (not_a_position), past the end is S501 (line_past_the_end, exit 47).
- The report codes are S501 LINE_PAST_THE_END, S502 FILE_HAS_NO_LINES, S503 FILE_NOT_OPEN, S504
  FILE_NOT_SAVED, S410, S413 (satellite/machine/s_codes.hpp) -- NOT the S09xx the document says.
- `f.path` answers the ABSOLUTE path (the handle keeps it, so satellite.directory.change cannot
  move a save), not "the path as written" the document says. Keep that.
- A file that would not open is a VALUE, not an error: ok() false, code()/error() say why. Only
  reading a line that is not there, or asking a question of a file that is not open, stops the
  program -- and that is the interpreter's decision (file_calls.cpp), never the class's.
- FO-3 is marked BUILT but its line index was never built: open reads the WHOLE file into
  `lines_` (satellite_file.hpp:36 admits it). That is what FO-9 replaces.
- The author's bar for speed: within x1.05 of the same work in plain C++ (DESIGN 1.1). His rule
  on slower: "time old vs new first; slower means stop and tell him" -- report numbers, never hide one.
- Binary files today: `satellite.file.new("b.bin", "binary")` gives a handle that is not ok with
  "binary files are not built yet (SATELLITE_FILE_OPERATIONS FO-8)" (satellite_file.cpp:313, 359, 1082).
- `satellite.file.open(path, kind, "read")` (read-only, R7, FO-5) is refused by the checker today:
  there is no `1 8 7` row and file_word_arity answers 1 or 2.

--------------------------------------------------------------------------------------------------
## 0. GROUND RULES -- every agent, no exceptions
--------------------------------------------------------------------------------------------------

THE LIVE TREE `/home/madness/code/cxx/satellite` IS NEVER TOUCHED: no writes, no `make` in it, no
satl started from it. There is NO GIT on this project any more (the author's ruling): the files on
the disk are the only record, so a stray write there is a loss with no undo. I (the orchestrator)
merge your patch into it afterwards.

YOUR OWN COPY. `S=/tmp/claude-1000/-home-madness-code-cxx-satellite/e99fe5f0-97ca-4af1-97db-ab6b131bb528/scratchpad`
holds a pristine copy of the tree at `$S/copies/pristine` (build/ and vendor/ included; .git,
.claude, old_versions and haswell_test left out on purpose). Make yours with

    cp -a --reflink=auto $S/copies/pristine $S/copies/<your-job-name>

and work ONLY under `$S/copies/<your-job-name>`. Never edit `$S/copies/pristine`.

EVERY satl RUN (build/satl, check.sh, utility/check_help_examples.py, tests) goes with:

    mkdir -p $S/homes/<job> $S/xdg-<job> && chmod 700 $S/xdg-<job>
    env -u DISPLAY -u WAYLAND_DISPLAY -u XAUTHORITY -u GDK_BACKEND \
        HOME=$S/homes/<job> SATL_NO_WINDOW=1 XDG_RUNTIME_DIR=$S/xdg-<job> <the command>

(the -u options FIRST: env takes a -u after a NAME=VALUE as the command to run, and exits 127.)

Why: satl opens its own console window on every run unless stdout is a pipe/file or SATL_NO_WINDOW
is set, and unsetting DISPLAY alone STILL reaches the author's desktop (it happened three times);
and every refusal lands in `$HOME/.satl/satellite.log`, and `satl --rebuild` writes
`$HOME/.satl/config.ini` -- his real one must never be touched. check.sh sets HOME and SATL_NO_WINDOW
itself, but run it under the env line above anyway.

BUILD AND CHECK, in your copy only, one make at a time:

    cd $S/copies/<job> && env <the env line> make INSTALL_AFTER_BUILD=no -j6 > build.log 2>&1; echo EXIT=$?
    cd $S/copies/<job> && env <the env line> ./check.sh > check.log 2>&1; echo EXIT=$?; tail -1 check.log

`-j6`, because three builders share the 24 cores. A full build is about a minute; check.sh is 2-3
minutes and ends with "N passed, M failed" (1281 passed, 0 failed on the pristine tree). A bare
`make` would INSTALL into $HOME/.satl -- always pass INSTALL_AFTER_BUILD=no. `make` raises
arguments.build in satellite/config/satellite_config.hpp and writes `.satellite_build`: those two
are make's, not yours, and are excluded from your patch (below). Never run a build in the
background across your own turns without a log; read `EXIT=` from the log, never from a pipe.

NEW SOURCE FILES must be listed where the build looks: satl's sources in make_support/040-sources.mk
(find the variable that lists satellite_file.cpp), file_cases' in make_support/065-tests.mk
(FILE_CASES_SOURCES / FILE_CASES_HEADERS). A header is also listed in HEADERS so a change rebuilds
what includes it. check.sh checks `make -sq build/file_cases` (as new as its sources).

GENERATED TABLES are regenerated by their scripts, never edited: satellite/bytecode/token_codes.hpp
from REGISTRY.satellite (`python3 satellite/bytecode/make_token_codes.py`), word_codes.hpp and
words/words.tsv from words/words_004.tsv (`words/make_words.py`, then
`python3 satellite/bytecode/make_word_codes.py`; words/check_make_words.py needs 003's satl, which
your copy does not have -- check.sh counts that as skipped). Phase 1 (this brief) adds NO token and
NO word row: those are phase 2's.

THE PATCH you return:

    cd $S/copies && diff -ruN -x build -x '*.o' -x '*.so' -x '*.d' -x '*.tmp' -x .satellite_build \
        -x satellite_config.hpp -x __pycache__ -x build.log -x check.log -x '*.out' \
        pristine <job> > $S/patches/<job>.patch; wc -l $S/patches/<job>.patch

It must apply with `patch -p1` from the tree's root. If you really had to change
satellite_config.hpp, say so in the report (it is excluded above). Nothing generated by a run
(build/, *.out, scratch files) goes in it.

STYLE. Match the tree: comments say WHY and name the rule or the day's finding, claims in
CAPITALS where the tree does it, the author's words quoted ONLY when you have them verbatim from
this brief or the documents (never paraphrase as a quote). The aim is about 300 lines a file
("try to build for 300 lines" -- an aim, not a cap): a new piece goes in a NEW file beside the old
one, not into a 1,100-line file. Every machine code goes on the list in
satellite/machine/machine_codes.hpp BEFORE it is used, with its number in the comment column, and
a number is never reused (DESIGN 1.3, 3.2). No defensive extras beyond the design: a thing the
design leaves out is a question for the report, not code.

THE REPORT is raw data for the orchestrator (the StructuredOutput schema you are given), not a
message to a person. Put numbers in it: counts, times, exit codes, the exact FAIL lines.

--------------------------------------------------------------------------------------------------
## JOB A -- the class: FO-5 read-only, FO-8 binary, FO-9 no limit on a file
--------------------------------------------------------------------------------------------------

Everything in `satellite/satellite_variable_file/` plus the codes it needs. The PUBLIC API of
`satellite_file` keeps every method's name, signature and meaning for text, so file_calls.cpp
compiles unchanged except for the additions named here. file_calls.cpp is phase 2's; do not edit it
beyond what compiling needs (it should need nothing).

### A1. Read-only (FO-5; the author's R7, "yes")

    static std::shared_ptr<satellite_file> open_existing(const std::string &path, Kind kind, bool to_read);
    bool read_only() const;

The two-argument open_existing stays and means `to_read = false`. On a handle opened to read, every
CHANGING method -- append, insert, replace_line, replace_text, remove_at, remove, remove_first,
remove_last, truncate, clear_lines -- answers false with a NEW machine code `file_opened_to_read`
(next free number on the list) and error() "<path> was opened to read, and a file opened to read is
never changed". The questions (size, line, first, last, index_of, search, contains, read_all)
answer as always. save() answers true and writes nothing (nothing can be pending). close() closes
without writing. reopen() re-reads and the handle stays read-only. The destructor writes nothing.
Add the report code to satellite/machine/s_codes.hpp beside S501-S504: `S506 FILE_OPENED_TO_READ`
("a file opened to read was asked to change ...") -- S505 is taken by the window. The interpreter
(phase 2) will give `satellite.file.open(path, kind, "read")` to this.

### A2. Binary files (FO-8): a binary file is a LIST OF BYTES, counted from 1

The author (2026-09-24): binary files get "all of the same operations" as text files. So
`Kind::binary` opens and makes files for real (today it fails with not_built_yet), and every
method means the same thing done to BYTES, with the std::string payloads holding raw bytes:

    size()                      how many bytes            empty()
    line(n, out)                byte n, as a 1-byte string; 0 / past the end as for lines
    first / last                byte 1 / byte size
    append(run)                 the run goes on the end (an empty run adds nothing, answers true)
    insert(n, run)              the run goes in before byte n; n = size + 1 is append
    replace_line(n, run)        byte n becomes the run (any length, empty removes it)
    replace_text(from, to)      the FIRST run `from` becomes `to`; empty `from` is empty_search_text
    index_of(run) / search(run) the 1-based position where the first run starts, or 0 (both the
                                same for bytes: a byte has no inside); empty run is 0
    contains(run)               search(run) != 0
    remove_at(n)                byte n out            remove(run)   the first run out
    remove_first / remove_last  byte 1 / byte size out
    truncate(n)                 keep the first n bytes        clear_lines()   no bytes
    read_all()                  every byte

No UTF-8 check, no \r rule, no byte-order mark, no endings for binary. The append fast path
(O_APPEND, the 64 KiB stream) and the whole-or-nothing save apply exactly as for text. A text
file opened as binary is bytes (fine); a binary file opened as text is refused as today
(file_not_text). `kind()` stays, so the interpreter can decide what a byte is as a value (phase 2:
a satellite.variable.binary of 8 digits is the likely answer -- the author's to rule).
`file_has_no_lines` (46) is then answered by nothing; it STAYS on the list (numbers are never
reused) with a comment saying since when it is unused. Rename nothing in the public API: the
methods are the list's words, and the names hold for bytes too.

### A3. No limit on a file (FO-9; DESIGN 1.2 "no built-in limits")

REQUIREMENT: the handle's memory does not grow with the size of the file. Opening, walking f[n]
from 1 to size, index_of/search/contains, replace_text, an insert at line 1, remove_at, truncate,
save and close must all work on a file far larger than memory (the document's done-when: a 10 GB
file with one line inserted at the top saves using memory that does not grow with the file).
Memory MAY grow with the unsaved changes, and with the size of an answer (read_all's answer is the
whole file -- the caller asked for it). And the small file -- the likely scenario (DESIGN 1.1) --
stays at least as fast as today: measure before and after (file_cases' appends_stream has 100,000
appends; time 1,000,000 appends + close, and a walk of a 1,000,000-line file by line(n), on the
pristine build and on yours; put both numbers in the report; slower on the common path = stop and
say so, do not paper over it).

RECOMMENDED DESIGN (yours to improve, the requirement is what counts): the file is a sequence of
RUNS, each either ON THE DISK (a byte range of the file as it was opened, with its line count and a
sparse index: the offset of every Kth line, K ~ 4096, found in one streaming pass that also checks
UTF-8, refuses a lone \r, finds the endings and the mark) or IN MEMORY (lines with their endings,
exactly today's `lines_`/`endings_`). At open, a file up to a threshold (say 4 MB: a named constant
with a comment -- it is a choice of path, not a limit on the user) is loaded whole as ONE memory
run, so small files are exactly today's code; a bigger one is one disk run plus its sparse index.
f[n] on a disk run: the nearest index point, then scan forward; remember where the last read ended
so a sequential walk costs one line per step. An edit splits a disk run at the line into
[disk before][memory][disk after]; adjacent edits share the memory run; appends go to a memory tail
and stream to the disk with O_APPEND exactly as today (only_appended_ and saved_lines_ keep their
meaning). save(): nothing changed -> nothing; only appended -> the tail with O_APPEND; otherwise
every run streamed into `.<name>.saving.<pid>` (disk runs copied in 1 MB blocks, memory runs
written), fsync, mode and owner kept, rename, fsync the folder -- today's guarantees to the letter
(whole or not at all, the old file untouched on any failure, a read-only file not got around by the
rename). After a save: one disk run and a fresh sparse index (build it while writing, or re-scan).
When the unsaved changes outgrow a budget (say 64 MB of held text -- a named constant), save()
runs on its own so memory never grows without bound; a failed automatic save is kept in code() and
error() the way a failed 64 KiB append write is, and said in the header. A file that something
else changed after it was opened must not crash a read: a read past the file's current end is
line_past_the_end, not a fault.

NOT A SECOND DESIGN: one structure, two ways of making the first run. If you find the whole-file
path and the streamed path diverging into two implementations, stop and rethink.

### A4. Files, build and cases

- satellite_file.cpp is 1,097 lines: put the runs and the index in a new file (names like
  file_runs.hpp/.cpp), the byte-specific bits in another if they are more than a few lines, and
  list them in 040-sources.mk and 065-tests.mk (both!). Keep satellite_file.hpp's opening comment
  true: rewrite the paragraph "THE WHOLE FILE IS IN MEMORY" to what is now true.
- ALL 222 EXISTING CASES PASS UNCHANGED (FO-4's done-when says FO-9 must keep them). Do not edit
  an existing case's expectation; if one must change, that is a finding for the report.
- NEW CASES in file_cases.cpp (or a second cases file run by the same binary): the read-only handle
  refusing every changing method with the file's bytes AND mtime unchanged after close; binary: the
  author's example done in bytes, f[n] = byte n, insert/replace/remove/truncate on bytes, a 0x00
  and a 0xFF byte, index_of/search/contains/remove(run), a text file opened as binary, a binary
  file opened as text refused, the append fast path on bytes (100,000 appends stream); FO-9: a big
  file (about 256 MB -- writing it to the given folder takes a second or two; check.sh gives
  file_cases 300 s) walked by line(n) with the process's RSS (VmRSS from /proc/self/status, or
  getrusage ru_maxrss) staying under 64 MB above where it started; the same file with a line
  inserted at line 1 and one replaced near the end, saved, the bytes checked at a few places, RSS
  flat; search for a line near the end; remove_at of the last line; truncate; a save after an
  automatic save. Keep the whole file_cases run under 60 s.
- Build file_cases once with `-fsanitize=address,undefined` by hand (the make rule's compile line
  with the flags added) and run it; the 2026-09-18 cases were clean under both and these must be.
- Run the full check.sh in your copy at the end; every file row and the satellite.test rows must
  still pass (official_test/loss_test/file_loss.satl and satellite_test's full_objects_3/4/12/13
  use files; they are run by check.sh through satellite.test.*).

### A5. The report (schema): the patch path, files added/changed, the 222 + new case counts and the
sanitizer result, the before/after timings, check.sh's last line and every FAIL line, the machine
code number you took for file_opened_to_read, and every choice you made that the author might rule
on (each in one line: what you chose, why, what changes if he rules otherwise).

--------------------------------------------------------------------------------------------------
## JOB B -- FO-6: the race, and the hostile run
--------------------------------------------------------------------------------------------------

Do NOT change the class or file_calls.cpp; you build the harness and the hostile test against the
pristine class, and report what you find. (Job A is rewriting the class at the same time; I re-run
your race when it lands, so your numbers are the "before".)

### B1. The race: `make file-race`

Pattern: satellite/race/race.cpp + race.sh (display against std::cout) and
satellite/satellite_variable_number/number_race.sh (the better one: a run that fails, or whose
answers differ, stops the race with a non-zero exit instead of counting -- ERROR.md item 9 is the
fault to avoid). Make satellite/race/file_race.cpp + file_race.sh + the `file-race` row in
make_support/065-tests.mk, next to `race` and `number-race`, and name it in the file's header
comment list. The binary takes a folder to work in (as file_cases does), makes what it needs there
and removes it.

Three legs, each satellite_file against plain C++ doing the same work on the same bytes, 1,000,000
lines of text (e.g. "line_<n>" or race.cpp's "hello, world!"):

    append     make_new + append x 1,000,000 + close        vs  fopen/fwrite (or ofstream) the same lines + fclose
    walk       open_existing + line(n) for n = 1..size      vs  ifstream + getline, the same sum of lengths
    read_all   open_existing + read_all                     vs  read the whole file into a std::string

Each leg checks its answers agree (byte count on the disk, line count, the sum of lengths) and
stops the race if not. Best of 7 runs in each order (satellite first / C++ first), the three ratios
printed with the bar: "satl is x1.023 of C++ (the bar is x1.05)". Run it and put the three ratios
and the raw ns in the report. Do not tune the class.

### B2. The hostile run: tests/file_hostile.satl + check.sh rows

DESIGN 9's inputs thrown at every file word and method, from a PROGRAM, with the program going on
after every one and exiting 0 with a fixed output that check.sh pins. The rows go after the
existing file rows (check.sh ~1990, before `rm -rf -- "$file_room"`), in that block's style
(`expect "<what>" <wanted> <actual>`; a scratch room; fixtures made by the row, not committed).
Assert on true/false/0/exit codes and on bytes read back -- not on error sentences, which Job A may
reword.

Inputs: a path of punctuation only; a path 5,000 bytes long (built in the program by a loop of
string +); a directory as the file (new, open, clear, exists); a symlink to a file and a symlink to
a directory (made by the row with ln -s); a dangling symlink; an unreadable file (chmod 000; skip
as root as the existing row does); a non-UTF-8 file opened as text (ok false, the program goes on);
a CR-only file; a file with a byte-order mark (opens; line 1 has no mark); a FIFO (mkfifo) given to
open and to new -- must not hang (timeout the row); machine-code bytes -- take real bytes from
`objdump -d build/satl | head` or just a run like \xE8\x00\x00\x00\xC3 -- appended as a LINE and
read back equal (data, never jumped to), and given as a PATH (refused or made as a strange name, the
program goes on; find out first which escapes a satellite string literal has --
satellite.help/satellite.variable.string/help_text.txt and the string tests -- and whether a NUL can
be typed; if it cannot, the NUL path case stays in file_cases where it already is); `f[0]`, `f[-1]`,
`f[1.5]`, `f[f.size + 1]` -- those STOP the program by design, so each is its own tiny program
whose exit code the row pins (47 / S410's / S413's) -- see tests/file_past_the_end.satl;
`f.replace("", "x")` false; `f.index_of("")` 0; `f.search` for a 4-byte character that must never
match half of one; `satellite.file.clear` on a directory false; `new` over an existing file leaves
its bytes untouched (cmp before and after).

If an input crashes satl, hangs it, or is taken as anything but data: do not fix it; put the
input, the file:line of the row, and what happened in the report. Run the full check.sh in your
copy; every row passes (yours included) or the report says which and why.

### B3. The report (schema): the patch path, the files, the three ratios and raw times, the list of
hostile rows added with their results, anything that crashed/hung, check.sh's last line.

--------------------------------------------------------------------------------------------------
## JOB C -- FO-7: the help pages
--------------------------------------------------------------------------------------------------

Two topics under satellite.help/, each a folder with one `help_text.txt`, written like the pages
there (read satellite.help/satellite.delete/help_text.txt, satellite.help/satellite.container.list/
help_text.txt and satellite.help/satellite.variable.string/help_text.txt first, and
satellite.help/help.txt for the index): the 80-dash header "SATELLITE 004: <topic>", plain prose
in the author's register ("a file that would not open is a value, not an error"), examples that
are whole programs.

    satellite.help/satellite.file/help_text.txt
        the words: satellite.file.new(path) / new(path, "text") / open(path) / open(path, "text") /
        exists(path) / clear(path); the kinds ("text" and "binary" -- binary is being built, say
        "not built yet" in today's words and I will update it); the path rule (no leading / =
        relative to the folder of the .satl the line is written in; a leading / = under the
        program's own folder first, then the filesystem's; at the prompt, the working directory);
        new never clobbers, open never creates; a handle that is not ok is a value -- ok, error
        say why; what f.path answers (the absolute path).

    satellite.help/satellite.variable.file/help_text.txt
        the handle: a text file is a list of lines counting from 1 (the author's words, Part 1 of
        SATELLITE_FILE_OPERATIONS.md: "we'll build it so you can iterate over the lines as if they
        were objects ... As if each line were a list of objects"); f[n]; the 23 methods grouped --
        the list's words (append, insert, replace in its two shapes chosen by the first argument's
        kind, index_of, search, contains, remove_at, remove, remove_first, remove_last, truncate,
        clear, size, empty, first, last), the disk's (save, read_all, close, open), the handle's
        (ok, error, path, exists), and .lock()/.unlock() (the author's lock: off until .lock(),
        then every method on the file holds it -- see satellite/satellite_object/object_lock.hpp
        and the thread help); saves whole or not at all, appends stream, every open file is saved
        and closed when its body ends (and at the prompt after every line); what stops a program
        (S501 exit 47 past the end, S503 exit 45 a question to a file that is not open, S410 a
        negative line, S413 line 0) and what never does (a changing word answers false, error says
        why); a failed save at the end of a run exits 44 (S504); a program may rewrite its own
        .satl while it runs (reports quote the loaded copy) -- the author's build-counter example
        from SATELLITE_FILE_OPERATIONS 3.8 is a good one; `f == g` is identity; two names for one
        handle are one file. Not built yet, in today's words: read-only open, binary, the author's
        .write/.position/.read (do not describe them as built).

    satellite.help/help.txt: two rows after 31), in the index's style: "32) satellite.file (...)"
    and "33) satellite.variable.file (...)".

EVERY EXAMPLE RUNS AND PRINTS WHAT THE PAGE SAYS. utility/check_help_examples.py (read its header)
runs every program in every page: a program starts at `satellite.include(satellite)` and ends at
the first prose line; a block after a line containing "refused" may exit non-zero, every other
must exit 0; each runs in its own temporary folder, so an example may make files by relative name.
check.sh expects the FAIL list to be EXACTLY the eight pages its row names (check.sh ~2030) -- your
pages must not appear in it. Verify by building your copy (`make INSTALL_AFTER_BUILD=no -j6`) and
running `utility/check_help_examples.py build/satl` under the env line, AND by running each example
yourself and comparing the output to the page's `// comment` and "prints:" lines. Then the full
check.sh. Also check at the pipe-fed prompt, under the env line:

    printf 'satellite.help(satellite.variable.file)\nsatellite.help(satellite.file)\nsatellite.help(file)\n' | build/satl --repl

and report what each answers (the third likely says it is more than one topic, as `window` does --
that is fine; report it). Report every behaviour you saw while writing that disagrees with
SATELLITE_FILE_OPERATIONS.md (there are several -- Part 7's "path as written", the S09xx codes).

The report (schema): the patch path, the files, the check_help_examples FAIL list, check.sh's last
line, what the three prompt lines answered, and the disagreements you found.
