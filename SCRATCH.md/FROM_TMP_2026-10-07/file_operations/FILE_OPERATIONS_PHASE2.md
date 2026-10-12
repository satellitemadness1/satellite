# FILE OPERATIONS -- PHASE 2: the interpreter's side, and the author's words (2026-10-07)

Read FILE_OPERATIONS_BRIEF.md first: its section 0 (GROUND RULES) binds every step here too, and
its "facts that are easy to get wrong" still hold. Phase 1 (the class: read-only, binary as a list
of bytes, no limit on a file; the race; the help pages) is MERGED into the tree your copy is made
from -- read the merged `satellite/satellite_variable_file/satellite_file.hpp` and
`satellite/bytecode/file_calls.cpp` as they now are before anything else, and
`satellite.help/satellite.file/help_text.txt` and `satellite.help/satellite.variable.file/help_text.txt`.

THE AUTHOR'S ANSWERS, 2026-10-07, to the five questions put to him (his words: "A. yes B. yes (we
simply need to build a position object, and maintain position objects for current_source_file, for
all files that are open, what else? C. yes D. yes E. yes"):

    A. `.write(x)` with no position = append; after `.position(n)`, `.write(x)` goes in at line n
       and the next write goes under it (n + 1).                                         -- YES
    B. `.read()` = the line at the position, then the position moves to the next line;
       `.read(n)` = line n.                                                               -- YES,
       and the position is an object of EVERY open file ("position objects ... for all files
       that are open"); a program's own running source, when it opens itself, is one of them.
    C. `.replace("a", "b", n)` = inside line n only; `.replace("a", n)` = line n becomes "a"
       (today that means text a -> "n"; that meaning goes).                              -- YES
    D. A byte of a binary file = a `satellite.variable.binary` of 8 digits (b01000001).   -- YES
    E. Install when it is all built and checked.                                          -- YES

His model, 2026-09-24, verbatim: *"for file_objects I want to have .write("str") (by default it
just appends onto the bottom of the file, but I also want to be able to position the writer, and
also have a "replace" ... returns true if it found "some_str" and false if it did not"*; the forms
*1) .replace("str","str") 2) .replace("str","str",number) 3) .replace("str",number) 4)
.position(number)* -- *"all writes will push that line and all other lines down"*; *.read() and
.read(number) "will return a satellite_string of that line"*. He also said a text file object is
also a list<string>, and text is the default when only a filename is given (already so).

--------------------------------------------------------------------------------------------------
## JOB D -- the interpreter: FO-5's word, FO-8's values, FO-10's lists, the author's words
--------------------------------------------------------------------------------------------------

One builder, one copy (`job_d`). The class may be extended where a piece below needs it (phase 1's
builder has finished with it); keep its public names and the file split phase 1 made.

### D1. FO-5 -- `satellite.file.open(path, kind, "read")`, word `1 8 7`

- words/words_004.tsv gains ONE row at its end: `1 8 7<TAB>satellite.file.open(path, kind, access)`
  (the row's parameter names are placeholders, as `satellite.file.open(path, mode)` 1 8 2 shows).
  Regenerate: `python3 words/make_words.py` (your copy has 003's satl under old_versions/, which it
  walks), then `python3 satellite/bytecode/make_word_codes.py`. Check `words/words.tsv` gained
  exactly that row at the end and no other code moved (the row is the code: 4097 + row).
- file_calls.cpp: is_file_word knows 1 8 7; file_word_arity answers 3 for it; file_word_takes says
  `satellite.file.open takes a path, a path and "text" or "binary", or those and "read"`; the
  lexer picks the 3-parameter row by argument count (bytecode_registry.cpp shaped_word_code), so a
  3-argument open lexes to 1 8 7 on its own -- check it does. call_file_word: the third argument
  must be the STRING "read"; anything else is refused (types_do_not_meet): `satellite.file.open's
  third argument is "read", and it was given "write"` / `... a number`. Then
  `open_existing(path, kind, true)`. The two-argument open given "read" AS THE KIND is refused by
  kind_of today with `"read"` named; make that sentence say the way: `"read" is not a kind -- write
  satellite.file.open(path, "text", "read") to open a file to read`.
- On a read-only handle the class answers false with file_opened_to_read for every changing
  method; the interpreter passes the bool through, `f.error` says why; nothing stops. `save` and
  `close` answer true. The end-of-body close writes nothing (the class does that).
- tests/file_read_only.satl + check.sh rows beside the other file rows: a fixture file; every
  changing word answers false; size/f[n]/search/read_all answer; save and close true; the bytes
  and the mtime of the file unchanged after the run (`stat -c %Y` before and after); the program
  exits 0. And a row for `satellite.file.open("x.se", "text", "write")` refused (exit code pinned)
  and one for the two-argument `open(path, "read")` refusal naming the way.
- satellite.help/satellite.file/help_text.txt: the read-only open is built -- replace phase 1's
  "not built yet" words with the real thing and an example that runs.

### D2. FO-8 -- what a byte is as a value (the author: a satellite.variable.binary of 8 digits)

OUT of a binary file: `f[n]`, `f.first`, `f.last`, `f.read()`, `f.read(n)` answer
`Value::of_binary(satellite_binary_number{byte, 8})` -- `b01000001` for 0x41, width 8 always.
`f.read_all` on a binary file answers ONE satellite.variable.binary of width 8 x size (every
byte's digits in order): a big file makes a big value, which is what was asked for; say so in the
help. (satellite_binary_number holds the bits as a satellite_number: build it from the bytes with
the cheapest route the number type offers -- from_radix_text on the digits is fine for a few
thousand bytes; if there is a from-bytes/base-256 route, use it; measure 1 MB.)

IN to a binary file (append, insert, write, replace in every shape, index_of, search, contains,
remove), by what the value IS -- the file's kind decides, as it decides what f[n] answers:

    a satellite.variable.binary    its bytes, the width rounded UP to whole bytes with leading
                                   zeros: b1000001 is 0x41, b100000001 is 0x01 0x01. A NEGATIVE
                                   binary is refused: a sign has no byte.
    a number                       ONE byte, 0 to 255; anything else refused ("a byte is 0 to 255").
    a string                       its UTF-8 bytes (so "AB" is 0x41 0x42, and a text with a wide
                                   character is its UTF-8 bytes).

The refusals share one NEW machine code `not_a_byte` (next free number, on the list before use)
and one report code `S507 NOT_A_BYTE` in s_codes.hpp ("a binary file takes whole bytes: a binary
value in whole bytes, a number from 0 to 255, or text as its UTF-8 bytes; this value was none of
those"). The exit code of a program that trips it is pinned by a row. On a TEXT file nothing
changes: a number where text is expected is its digits with the S020 warning, as today.

tests/file_binary.satl + rows: the author's Part 1 example done in bytes
(`satellite.file.new("demo.bin", "binary")`, append, insert, replace in both shapes, f[n] printing
b01000001-style values, size, index_of/search/contains with a binary, a number and a string,
remove, truncate, read_all of a 3-byte file printing 24 digits); a text file opened as "binary"
reads bytes; a binary (non-UTF-8) file opened as "text" is a handle that is not ok and the program
goes on; `open(path, "binary", "read")` works; a 0x00 and a 0xFF byte round-trip; a negative binary
and a number 256 refused (exit codes pinned, own tiny programs). Help: the satellite.variable.file
page gets its binary section (the three ways in, the one way out, read_all's size), and the
satellite.file page's "binary is not built yet" words go.

### D3. FO-10 -- `index_of`, `search` and `contains` given a LIST

The first line (text file) or byte position (binary file) that matches ANY item of the list:
items are taken as D2 says for a binary file and as text (a number is its digits, S020) for a text
file. An empty list answers 0 / false. A list holding a list is refused (types_do_not_meet,
`f.index_of takes text or a list of text, and the list held a list`). One pass over the file (add
`index_of_any` / `search_any` taking a `std::vector<std::string>` to the class; never N passes over
a big file). `f.remove(list)` is NOT asked for: leave it refused as today (a list is not text).
tests/file_list_search.satl + rows: a list that matches at line 7 and not before; a list with no
match answers 0; `contains` true/false; the empty list; a nested list refused (exit pinned); on a
binary file with binaries. Help: one paragraph with an example.

### D4. THE AUTHOR'S WORDS: `.write`, `.position`, `.read`, and the third and fourth `.replace`

TOKENS. REGISTRY.satellite: two new [METHOD] rows in the free range that starts at
0000101101110000 -- `position_token` (position) and `read_token` (read), described the way the
rows around them are, with "(SATELLITE_FILE_OPERATIONS, the author 2026-09-24 / 2026-10-07)";
then `python3 satellite/bytecode/make_token_codes.py` (never edit token_codes.hpp). The file's
`.write` reuses `write_token` (0000101101000010, canvas.write): a method token is per type, as
`.text` and `.on` are. Update the free-range row below them.

THE POSITION, in the class (one per handle -- every name for the file shares it, as it shares the
file; a program that opens its own .satl has one for it like any other):

    std::size_t position() const;          1 .. size() + 1; size() + 1 is THE END -- where append writes.
                                           A file just made or opened is at the end.
    bool set_position(std::size_t n);      1 .. size() + 1 is taken; 0 is counts_from_one (false);
                                           above size() + 1 is line_past_the_end (false), the position unchanged.
    bool write(const std::string &text);   the text goes in AT the position (the line there and
                                           every line below move down -- the author: "all writes
                                           will push that line and all other lines down"), and
                                           the position moves to just after what went in, so the
                                           next write follows it. At the end it is append, and the
                                           position stays the end. Text with \n is that many lines (R5).
    bool read(std::string &out);           the line AT the position, then the position moves on by
                                           one. At the end there is no line: false, line_past_the_end.
    bool read_at(std::size_t n, std::string &out);   line n, and the position becomes n + 1.

    THE POSITION FOLLOWS ITS LINE (the orchestrator's choice, overrulable): an insert above or at
    it moves it down by the lines added; a remove_at above it moves it up one; removing the very
    line it is on leaves it where it is (the next line takes that number); truncate below it, and
    clear, put it at the end; append leaves it alone (and the end stays the end). close forgets
    it; reopen is at the end. For a binary file the same, in bytes.

THE INTERPRETER (file_calls.cpp, program_check.cpp): file_method_arity becomes a least/most pair,
as program_method_least/arity are (program_calls.hpp) -- position 0..1, read 0..1, replace 2..3,
write 1..1, everything else as it was -- and the checker's file branch in method_judged judges
with it (find where file_method_arity is read in program_check.cpp and make both ends agree; keep
the existing rows green: `f.replace(1)` is still refused before anything runs, `f.size = 3` too).
`f.read` with no brackets is read(); `f.position` with none is the number; `f.write` with none is
refused (takes one). The methods:

    f.position              the number                      f.position(n)       bool (as the class)
    f.write(x)              bool
    f.read()                the line as a string; at the end it STOPS the program as f[n] past the
                            end does (S501, exit 47): "f.read(): the position is past the end --
                            the file has N lines, and the position is N + 1". A walk is
                            `while (f.position <= f.size) { ... f.read() ... }` -- say so in help.
    f.read(n)               line n (as f[n]), and the position is n + 1 after it.
    f.replace(n, text)      line n becomes text                                 (as today)
    f.replace(old, new)     the first old in the file becomes new, one line     (as today)
    f.replace(old, new, n)  the first old IN LINE n becomes new; false when old is not in line n
                            (text_not_found) -- new class method replace_text_in(n, from, to)
    f.replace(text, n)      line n becomes text = replace_line(n, text)           (the author's form 3)
    f.replace(n, m)         both numbers: line n becomes the digits of m, S020 warning (as today)
    anything else           refused before the run where the checker can see it (a 3-argument
                            shape whose third is not a number; a 4th argument), else at the run:
                            "replace takes (line, text), (text, line), (old, new) or (old, new, line)"

The old meaning of `f.replace("a", 1)` -- text a becomes "1" -- is GONE (the author's C: yes). Note
it in the report; search check.sh and tests/ for a use of that shape (there should be none).

On a binary file: write/read/position in bytes; read answers a byte as D2; replace's forms with
byte values as D2 takes them.

TESTS: tests/file_position.satl + rows: the author's Part 1 brief done in HIS words --
`my_file.write("line_1")`, `my_file.write("line_2")` (the file: line_1 line_2), `my_file.position(2)`,
`my_file.write("line_2")` (line_1 line_2 line_2), `my_file.replace("line_2", "line_3", 2)` (line_1
line_3 line_2), `my_file.replace("line_2", "line_4")` (line_1 line_3 line_4), `replace("top", 1)`
(top line_3 line_4), the position after each step printed, a read() walk from position 1 printing
every line, read(2) then read() giving lines 2 and 3, position past the end false, the position
following an insert above it, write at the end after a position(size + 1); and tiny programs whose
exit codes are pinned: read() at the end (47), position(0) (S413's exit), read(-1) (S410's),
replace("a", "b", "c") (the checker's 13 or the run's code -- pin what it is). Every row in check.sh's
file block style; fixtures in the scratch room.

HELP: satellite.variable.file's page gets the author's words (quote them as above, with the date),
the position, write, read, the four replace shapes and the walk; examples that run and print what
the page says; check_help_examples.py's FAIL list stays exactly the eight.

### D5. Run everything

`make INSTALL_AFTER_BUILD=no -j8` in your copy (you are the only builder now), file_cases (the
class's new cases for position/read/write/replace_text_in/index_of_any -- add them), the full
check.sh (1281 + phase 1's rows + yours, 0 failed), check_help_examples. Report as the schema
says: the patch path, files, counts, the codes and rows taken (machine codes, S507, 1 8 7, the two
tokens' codes), every choice the author might rule on, every finding. The documents
(SATELLITE_FILE_OPERATIONS.md, PROGRESS.md, the memory) are the orchestrator's: do not edit them.
