# satellite-004 — DESIGN.md

Checked against satl BUILD 0058 on 2026-10-09.

Started 2026-09-14. The fourth start of satellite, and in the author's words
"as we rebuild satellite for the last time". PLAN.md was the order of work when
this file was written; the author's current plan is NEW_MILESTONES.md (since
2026-10-06). This file is the standards every line of satellite-004 is written to.

Everything marked **(author)** was decided by the author on 2026-09-14.
Everything marked **(measured)** was run on this machine that day; §12 is a
table of the main results, and the commands that made them are not kept here.
Anything else is a recommendation and says so.

**How to read it now.** Each section keeps its text of 2026-09-14: the design as
it was written that day, with the author's words. A paragraph headed **Today
(build 0058)** says what satl does now, checked on 2026-10-09 by a run or in the
source. Where the two differ, Today is what satl does, and the 2026-09-14 text is
the record.

---

## 1. The standards

1. **The fastest C++ path, always.** (author) Every satellite word is a small
   piece of compiled C++ of *likely scenarios*, built into satl (§3.4). The most likely scenario is
   the fast path; rarer shapes may be slower, but never slow down the common
   one.
2. **No built-in limits.** (author) Nothing in the code may assume what the
   operating system allows today. Every maximum (threads, memory, digits,
   nesting, the length of a command) is a `satellite_number` read from
   `satellite/config/satellite_config.hpp`. The default maximum for everything is
   **999,999,999,999,999,999,999,999,999** (27 digits, nine hundred ninety-nine
   septillion), and a larger machine adds nines. The code is written so that
   no other number stands in the way.

   **Every value in the config is quoted text**, turned into a
   `satellite_number` at start-up. (measured) Written as a C++ integer,
   `999999999999999999999999999` only draws a warning and silently holds
   `11515845246265065471`; as a string literal, 1,000,000 nines compiled in
   0.10 s with every digit kept.

   **A maximum is a ceiling, never an instruction.** The interpreter uses the
   smaller of the config value and what the machine actually offers. This
   machine runs 24 threads at once; its limits and the measured cost of a
   thread are in §13.
3. **Every function answers a machine code.** (author) 0 is success. The list
   lives in `machine_codes.hpp`, and **no code may be used before it is added
   to that list.**
4. **An error reaches a person.** A failure is written to stderr with its code,
   whether or not debug mode is on, and the exit status says which code
   stopped the program (§8).
5. **Names are safe.** (author) A name holds only `a–z A–Z 0–9 _`. Every
   milestone throws hostile names and bytes at the executable (§9).
6. **No name ever clashes.** (author) Every name lives in a
   `std::vector<std::string>` by kind, and a clash is written to
   `satellite.log` as an entry (§7).
7. **Ported, not copied.** (author) 003 06's parser and resolver are the
   starting point, but "we cannot just copy 003 over into 004": each part is
   rewritten as it comes across, with its fast paths. The author writes the
   pseudocode for each milestone and Claude writes C++ that compiles clean
   and passes the milestone's entries.

**Today (build 0058):**

- **Standard 1:** every word is compiled into satl, and nothing is loaded beside
  the binary (§3.4).
- **Standard 2:** three rows of `satellite/config/satellite_config.hpp` are named as maxima:
  `arguments.threads_max` 1000000, `arguments.object_bytes_max` 34359738368 (32 GB)
  and `arguments.file_size_max_bytes` 549755813888 (512 GB). Other rows are ceilings
  too: `arguments.display.buffer` 131072 (§8), and a float's digits,
  `arguments.float.whole` 4096 and `arguments.float.decimal` 128 (§4). No row holds the
  27-nines default, and no row is a maximum for a whole number's digits, for nesting or
  for the length of a command. A row's number is written as a plain integer, and in quotes only when it
  is longer than a C++ integer can hold; either way it is read into a
  `satellite_number` at start-up (the author, 2026-09-16: *"just make everything a
  satellite number, satellite numbers can hold any amount of digits"*). Only
  `threads_max` is read today: the warm threads are never more than it, and a thread
  the machine refuses is reported while the others stay warm (§13).
  `object_bytes_max` and `file_size_max_bytes` are read by nothing yet
  (`satellite/arguments/argument_settings.hpp`).
  Ruled 2026-10-09: standard 2 stays as written, and those rows are work still owed
  (the author, answering the editors' questions: "looks good").
  Owed: a row that holds the 27-nines default; maximum rows for a whole number's
  digits, for nesting and for the length of a command; and code that reads
  `object_bytes_max` and `file_size_max_bytes`.
- **Standard 3:** the list is `satellite/machine/machine_codes.hpp` (§2).
- **Standard 4:** a refusal is a report on stderr, and satl exits with its machine
  code. A name declared twice printed its S202 report on stderr, nothing on stdout,
  and exited 26 (run on 0058).
- **Standard 5:** a name is `a–z A–Z 0–9 _` and does not start with a digit.
  `my$name`, `café`, and a Cyrillic `а` (U+0430) written in place of an `a`, are each
  refused before anything runs (S110). A one-letter name and a 5,000-letter name both
  run.
- **Standard 6:** names are not kept in vectors by kind. Each place -- a capsule,
  or the prompt -- has its names in one master list, satellite.legal
  (`satellite/bytecode/satellite_legal.hpp`, NEW_MILESTONES.md NM-2a), and since
  2026-10-07 (build 0047) the checker reads that list itself. A name declared twice
  in one capsule is refused before anything runs (S202 NAME_DECLARED_TWICE, machine
  code 26) and is written to satellite.log as an entry (§7).
- **Standard 7:** 003 06's parser was not ported, because there is no parser. Since
  2026-09-16 a `.satl` is lexed straight into 16-bit codes, checked, and walked
  (PLAN.md, M6).

---

## 2. Machine codes

`machine_codes.hpp` is the list. On 2026-09-14: 0 success, 1 error, 2 display_error,
3 int_error, 4 string_error, 5 vector_loading_error, 6 number_vector_defined,
7 satellite_loading_successful, 8 missing_satl_file,
9 successfully_loaded_satl_file,
10 satl_file_missing_satellite_include_satellite,
11 satl_file_missing_satellite_main,
12 satl_file_missing_satellite_return_satellite,
13 satl_line_not_understood.

**The type is `signed long long int`, but an exit status is 8 bits.** Code 256
exits as 0 (success). Measured by the 2026-09-14 review. So `main` must map any
code outside 1–255 to one reserved exit status, and the full code always goes
to stderr (PLAN M1).

**Today (build 0058):** the list (`satellite/machine/machine_codes.hpp`) runs from 0
to 83, and 130 is `interrupted`: Ctrl-C, 128 + SIGINT, the number a shell gives. 77
is never given, on purpose: test drivers read an exit of 77 as "skipped". The mapping
is built (`satellite/machine/exit_status.hpp`, PLAN M0.5): 0 exits 0, 1 to 254 exit
as themselves, and any other code exits 255 with the whole code on stderr, so 256 can
no longer exit 0 (check.sh has a row for it). 255 is never given to a code (PLAN
D0.5.2, a recommendation that was taken; the author may rule otherwise). A person
also sees an S-code: each critical error report names one (S110, S202 and so on),
while a one-line report -- a directory given as the program, a flag satl does not
take -- names only its machine code. The S-codes are in
`satellite/machine/s_codes.hpp`, and the exit status is the machine code.

---

## 3. The number vector and the words (libraries until 2026-10-07)

### 3.1 A row

The author's sketch was
`std::vector<std::map<std::string, std::string, unsigned long long int, … ×6>>`.
**A `std::map` holds a key and one value**, so that does not compile
(measured: *"wrong number of template arguments (5, should be at least 2)"*).
What it describes is a row (the 2026-09-14 design):

```cpp
struct SatelliteNumberRow {
    std::string name;                         // "satellite.console.display"
    std::string function;                     // the C++ function / library it calls
    unsigned long long int numbers[6];        // 1 5 1 0 0 0 -- the common case, no allocation
    unsigned long long int depth;             // how many of numbers[] are used
    std::shared_ptr<satellite_number> longer; // only for a command deeper than 6 (§3.3)
    Scenarios scenarios;                      // the likely-scenario functions
};
std::vector<SatelliteNumberRow> satellite_number_vector;
```

**Today (build 0058):** the number vector is `NumberIndex`'s `std::vector<NumberRow>`
(`satellite-numbers/call_number.hpp`):

```cpp
struct NumberRow {
    std::string name;                               // "satellite.console.display"
    std::vector<unsigned long long int> numbers;    // {1, 5, 1}
    Scenarios scenarios;
    std::string file;                               // "(built into satl)" -- every row, since 2026-10-07
};
```

Each word describes itself by filling in a `LibraryRow`
(`satellite-numbers/number_row.hpp`): its name, at most 16 numbers (`kMaxDepth`) and
its scenarios. The index copies that into a `NumberRow` at start-up. The 33 arguments words skip
that step: each is filed straight into a `NumberRow`, its numbers read from
`words/words.tsv` (`satellite-numbers/call_number.satellite.cpp`). There is no
`function` field and no `longer`: the numbers are a vector in the index, and a fixed
16 in a word's own description.

### 3.2 Every word, and 004's numbers

**Settled 2026-09-14.** The author: "Just settle on a particular number for
each of the commands, just remove GUI commands", and "it has to be
first_available_number.first_available_number.first_available_number, that's
all the numbering scheme is."

- **The words:** 003 06's 371, minus the 7 GUI words (`satellite.window` and
  everything under it, and `satellite.variable.window`), so **364**.
- **The numbers:** first available at every level, in 003's order, with a bare
  shape keeping 0. Renumbered once, before 004 wrote any number anywhere, so
  only three changed: `satellite.variable.capsule` 1 6 16 → **1 6 15**, and
  `satellite.constructor` 1 25 → **1 24** (and its bare shape 1 25 0 → 1 24 0).
- **Frozen from now on.** A word removed later leaves its number empty forever,
  and a new word takes the next free number: under `satellite` that is
  **1 25**, under `satellite.variable` **1 6 16**, and under
  `satellite.variable.string` **1 6 1 23**.
- **Generated, never typed:** `words/make_words.py` walks 003's own registry
  (`satl --words`) and writes `words/words.tsv`, with each word's 003 number
  beside its 004 number, and `words/satellite_words.hpp`. Verified: 364 rows, no
  duplicate numbers, every parent's children exactly 1..n.
- **Question still open:** the author mentioned one more thing to remove and
  could not remember it.

**Today (build 0058):** `words/words.tsv` has 548 rows: the 364 from 003 first, then
184 words 004 added, in the order they were added. Those 184 are typed by hand in
`words/words_004.tsv` and have no 003 number. `make_words.py` still reads the 364
from 003's own satl (`old_versions/second_satellite/satl --words`); it is run by
hand, check.sh checks it, and it refuses a new word that is not the next free number
under its parent.
The rule held, with no gap and no number used twice under any parent. The GUI words
came back as 004 words with new numbers: `satellite.variable.window` 1 6 18 and
`satellite.window` 1 27. 1 6 16 became `satellite.variable.percentage`, the first word
004 added (2026-09-17), and 1 25 became `satellite.feedback`. The next free numbers are
now 1 34 under `satellite`, 1 6 25 under `satellite.variable`, and still 1 6 1 23 under
`satellite.variable.string`. The three renumbered words keep their numbers (1 6 15,
1 24, 1 24 0). The question above is still open.

### 3.3 Commands deeper than six numbers

(author) "is there a way to program it so that the maximum command is like, 999
ints long?" **Yes:** six numbers are held inline, so the common case never
allocates, and a deeper command keeps the rest in `longer`. Its depth is a
`satellite_number`, so it has no maximum. (author) A command longer than six
numbers takes the C++-library path; test it with a 90-word command.

**Today (build 0058):** there is no `longer`, and a command's depth is not a
`satellite_number`. A word in a program is lexed from its spelling straight into one
16-bit code, 4097 plus its row in words.tsv (`satellite/bytecode/word_codes.hpp`), so a
program never carries a word's numbers. The codes run to 8191, which is room for 4,095
words. A word built into satl describes itself in at most 16 numbers (`kMaxDepth`,
§3.1), and the index refuses a deeper one at start-up
(`satellite-numbers/call_number.satellite.cpp`). A second way to write a word -- a
count, then its numbers (`word_number_token`, `satellite/bytecode/token_codes.hpp`) --
is reserved, and nothing writes it yet. The deepest word today has 6 numbers, and
check.sh has no row for a command deeper than six. A fixed 16 is a maximum, which
standard 2 rules out. Ruled 2026-10-09: the 16 stays for now, since the deepest word
has 6, and taking it away, as standard 2 asks, is work still owed
(the author, answering the editors' questions: "looks good").

### 3.4 A word (a library until 2026-10-07)

**Since 2026-10-07 every word is built into satl** (the author: *"We were supposed to
have built the satellite-numbers directly into the interpreter, so they are not external
... make a table that calls a certain function for each number, and for each number, just
copy over that source code into the function that corresponds to each number"*):

- **Source:** `satellite-numbers/words/<n.n.n>.cpp`, named by its numbers, compiled into
  satl like any other source (040-sources.mk `WORD_SOURCES`). The 33 arguments words were
  built in the same way on 2026-10-03 (`satellite/arguments/argument_words.hpp`).
- **Ends in one function:** `describe_<n_n_n>(LibraryRow *)`, which fills in its name,
  numbers and scenario pointers -- the shape the library's one export had.
- **The table:** `satellite-numbers/word_table.cpp`, one row for each of the 260 word
  sources, is what the index walks at start-up (`call_number.satellite.cpp`), holding
  each row against its function and against `words/words.tsv`. With the 33 arguments
  words, each filed at two places, the index holds 326 rows (`satl --debug` says
  `326 of 692 words have a library`). The other 366 words have no row there: the
  checker and the walker in `satellite/bytecode/` run the ones satl has by their
  16-bit codes.
- Nothing is opened beside the binary, so satl alone is the whole language, and the rules
  that guarded the folder (below) are history.

**Until then a word was a library:** `satellite-numbers/<word>/<word>.satellite.cpp` with
no `main`, built as `build/satellite-numbers/<n.n.n>.so` and dlopened. Rules the prototype
review proved necessary for that: load only regular files named `<numbers>.so`; refuse a
library whose file name disagrees with the numbers it describes; refuse an empty index
(code 5, not 13); never load from the current directory, and stop with code 5 when the
executable's own path cannot be read; check the C++ ABI at load (a library built with
`-D_GLIBCXX_USE_CXX11_ABI=0` loaded and wrote 82 KB of heap memory to stdout). **Dynamic
linking** was on purpose then: the interpreter and every library had to share one
libstdc++, or each had its own `std::cout`; with the words inside satl that reason is gone,
and since 2026-10-22 a clang build links its C++ library into satl statically -- LLVM's
libc++ (`make_support/010-compiler.mk`) -- while a g++ build links the machine's libstdc++.

---

## 4. satellite_number

(author) "satellite_numbers are made by laying unsigned long long ints beside
each other, and the sign is carried as a bool with the object… keep the number
of digits in each number, keep the size of the number in bytes always as a
satellite_number, so a satellite_number will be made out of a std::shared_ptr to
another satellite_number."

The 2026-09-14 sketch of it:

```cpp
struct satellite_number {
    bool negative = false;
    std::vector<unsigned long long int> limbs;   // laid side by side
    std::shared_ptr<satellite_number> digits;    // how many digits this number has
    std::shared_ptr<satellite_number> bytes;     // how many bytes it occupies
};
```

**Two things the representation needs, recommended:**

- **The chain has to stop.** A `digits` that is itself a `satellite_number`
  with its own `digits` never ends. The count of a small number is held
  inline, and the `shared_ptr` is used only when the count itself needs more
  than one `unsigned long long int`. The author's maximum needs 90 bits, which
  is two limbs (measured), so in practice the chain is one level deep.
- **Small numbers must not allocate.** A loop counter that allocates on every
  `i + 1` cannot be within ×1.05 of C++. 003 keeps a small form inline in its
  40-byte value for exactly this reason. So: one inline limb for numbers that
  fit, and the vector and pointers only beyond that.

The largest allowed number of digits comes from `satellite/config/satellite_config.hpp`
(author).

**Today (build 0058):** `satellite_number`
(`satellite/satellite_variable_number/satellite_number.hpp`) is a class with three
private fields: `small_`, one `unsigned long long int` held inline; `large_`, a
pointer to a vector of two or more limbs, null while the number fits in one; and
`negative_`. `digits()` and `bytes()` are `satellite_number`s counted when asked, not
stored, so there is no chain. Both recommendations were taken: a number that fits one
limb never allocates, and it comes back to the inline form as soon as it fits again.
A whole number has no upper limit but memory, and no config row holds a largest
number of digits. The floats have their own two rows, `arguments.float.whole` 4096
and `arguments.float.decimal` 128.
Ruled 2026-10-09: a row for the largest number of digits a whole number may have is
work still owed, as standard 2 in §1 asks
(the author, answering the editors' questions: "looks good").

---

## 5. Likely scenarios and dispatch

- **Choose the function before running.** (measured) Looking `"1.1.1"` up on
  every call took 0.053 s per million calls; looking it up once took 0.026 s.
- **When the kinds are only known while running, use a table.**
  `table[left kind][right kind]` measured **3.9 ns** per `+`, against
  **12.2 ns** for "check the most likely first, then search a vector" and
  3.5 ns for a direct call. (author) "we can have a very most likely function,
  then all of the other likely functions" — the most likely function is chosen
  before running when the types are written in the file (`"some" + "str"`),
  and the table covers the rest.
- **One scenario per likely shape.** (author) `+` gets string + string,
  string + number, and so on. Each keeps 003's rule that the left side
  decides: `"n = " + 4` is `"n = 4"`, `4 + "2"` is `6`, `4 + "abc"` is
  `"4abc"`.

**Today (build 0058):** a word becomes its 16-bit code before the program runs, and
a word with a library reaches its function through `table[code]`, one subtraction and
one load, so no name is looked up while a program runs
(`satellite/bytecode/function_table.hpp`). When
two values meet, `satelliteObject::add` and its siblings
(`satellite/satellite_object/satellite_object.cpp`) try number with number first,
the commonest pair. Every other pair is one `switch` on the two kinds made into one
number (`pair_of`), and each case calls the one file named after that pair, such as
`number_and_string_add.hpp`. The left side still decides: `"n = " + 4` displays
`n = 4`, `4 + "2"` displays `6`, and `4 + "abc"` displays `4abc` (run on 0058). The
timings above are the 2026-09-14 measurements and were not re-run.

---

## 6. Start-up, threads, and reaching `satellite.main`

*This design was not built as written. Today, at the end of this section, says
what satl does.*

**(author) The design:**

1. Start the configured number of threads first (24 on this machine).
2. Read every spaceship (include) as a vector of strings, with one thread per
   spaceship.
3. Each thread converts every `satellite.something` in its file into numbers.
4. The main thread converts only the first line of `satellite.main` and runs
   it at once, while other threads convert the lines after it. "There's
   always another thread looking for the next satellite.something."
5. Another thread looks for `satellite.return(satellite)`. If it is missing,
   that thread reports a critical error and the program stops.
6. Start-up is not what 004 is built for: "if it starts fast, great".

**(measured) What a thread costs:** starting and joining one takes
**28,254 ns**, and converting `satellite.console.display` to `1 5 1` takes
**25.8 ns**. So a thread costs about as much as 1,100 conversions. Threads
already running in a pool avoid the start cost, but every hand-off still costs
far more than a conversion.

**The pool, batches and combine** are in §13; the files are in §14.

**Three questions for PLAN M1 and M7,** each to be answered by measuring:
- On a 10-line program and a 100,000-line program, does the threaded start
  reach line 1 sooner than converting on the main thread?
- **Running line 1 before the whole file is converted means a mistake on
  line 50 is found after lines 1–49 have run.** 003 checks everything first.
  Which does 004 want?
- The return check runs beside the program. What happens to output already
  shown when it reports the return is missing?

**Today (build 0058):**

- **Steps 1 to 3:** the warm threads start first, before the program is read: the
  machine's cores × 2, 24 here (§13). The program and each file it includes are read
  one after another, each into a list of lines, and the lines of each file are cut
  into one chunk a warm thread. Each thread turns its chunk into 16-bit codes, a word
  into one code, in any order, and the chunks are laid back in the program's order
  (`satellite/bytecode/cascade_convert.hpp`).
- **Step 4:** nothing runs until every line is converted and the whole program has
  been checked (`satellite/bytecode/program_check.cpp`). The author, 2026-09-16:
  *"let's create 1024 threads, and have 1024 convert every line of the program into
  16-bits before a single line runs, that makes it so much easier to program, and
  there's nothing to test that way"* (quoted in `cascade_convert.hpp`).
- **Step 5:** a missing `satellite.return(satellite)` is found by that check, on the
  main thread, before anything runs: S103 FILE_HAS_NO_RETURN, machine code 12, and
  nothing is shown (run on 0058).
- **Question 1:** not answered in words. The 100,000-line case was measured on
  2026-09-16 (`satellite/bytecode/bytecode_registry.hpp`, and PLAN.md M1): 227 ms
  with one job a line, 40 ms on one thread, and 1.5 ms in 256 batches on 256
  threads. The 10-line case is not measured there.
- **Question 2:** answered. The author's words under step 4 speak to it:
  every line is converted before a single line runs. The checker then reads the whole program
  before anything runs, as 003 does. A misspelled word on line 7 stopped a run
  before lines 5 and 6 printed (run on 0058).
  Ruled 2026-10-09: question 2 is answered by the author's 2026-09-16 words under
  step 4, *"convert every line of the program into 16-bits before a single line
  runs"*: 004 converts the whole program before line 1 runs
  (the author, answering the editors' questions: "looks good").
- **Question 3:** not answered in words. It does not come up today, because the
  return is checked before anything is shown.

---

## 7. Names and satellite.log

(author) Every kind of name has its own vector:
`std::vector<std::string> object_name_vector, class_name_vector,
command_name_vector`, and so on. A new name is checked against all of them.
A clash is written to `satellite-004/satellite.log` as an entry, and "we have to
clear the entries by fixing the C++".

The author's `write_entry`, corrected to compile (measured):

```cpp
signed long long int write_entry(const std::vector<std::string> &text_input_vector)
{
    std::ofstream log_file("satellite.log", std::ios::out | std::ios::app);
    if (!log_file)
        return(1);

    log_file << "\n\n[entry]\n\n";
    for (unsigned long long int vector_index = 0; vector_index < text_input_vector.size(); vector_index++)
        log_file << text_input_vector[vector_index] << "\n";
    log_file << "\n\n[/entry]\n\n";

    if (!log_file)
        return(1);
    return(0);
}
```

**Two things to settle when it is built:**
- `"satellite.log"` is relative to wherever the program is run. The author
  wants it in `satellite-004/`, so the path comes from config.
- With 24 threads, two entries written at once can interleave. It needs one
  mutex around the whole entry.

(author) **Reaching a class made inside `polymorph`:** `satellite.library.poly.class_name`
or `poly.class_name`, and plain `class_name` when that name is used for
nothing else.

**Today (build 0058):**

- Names are not kept in vectors by kind: each place has its names in
  satellite.legal (§1, standard 6).
- `write_entry` is built (`satellite/machine/satellite_log.hpp`; MILESTONES.md M5,
  2026-09-25): the same name, the same argument, the same `[entry]` and `[/entry]`
  around one line a vector element, and the same 0 or 1. Each entry starts with a
  heading line (the time, the process id, the program's whole path and the folder
  it ran in), and every report satl makes goes in, not only a clash.
- The two things are settled. The path comes from config: `arguments.log_path`,
  `~/.satl/satellite.log`, and one machine may set `log_path = ...` in config.ini. That is not the
  `satellite-004/` the author named above: the file shared with 003 in `~/.satl`, and
  every report going in, not only warnings, were chosen at M5 (MILESTONES.md, M5).
  Ruled 2026-10-09: the log stays at `~/.satl/satellite.log`, not in `satellite-004/`:
  one file a person, beside the installed satl
  (the author, answering the editors' questions: "looks good").
  Every report going in, not only warnings, is still the author's to overrule.
  One mutex is held around the whole entry, and the entry goes out in one `write`
  on a file opened to append, so two threads, or two satls, never mix inside an
  entry.
- A name declared twice wrote its entry to `$HOME/.satl/satellite.log` (run on
  0058, in a throwaway home): the heading line, then the S202 report, between
  `[entry]` and `[/entry]`.
- Reaching a class made inside `polymorph` is not built: there is no polymorph word
  in words.tsv, and a spacesuit declared inside a capsule is refused as not built
  yet (S210, POLYMORPH M1).

---

## 8. Output

- **Writing:** `std::cout.write(data, size)` and `put('\n')`, not `<<`.
  (measured) `display` went from ×1.065 to **×1.002** of `std::cout <<`, with
  error detection kept (confirmed independently, three runs). Against a
  baseline that also uses `write`, the call through the index still costs
  about 5%. `-fno-plt` made it slower (×1.111).
- **Error detection:** `if (!std::cout) return(display_error);` is one bit
  test. A refused write is only seen when the ~4 KB buffer is actually
  written, so after the final flush the stream is checked again.
- **Pipes:** ignore SIGPIPE, so `| head -1` becomes display_error (2) with a
  message instead of a silent exit 141 (measured).
- **Line numbers:** a refused write names a line thousands of bytes after the
  one that failed. The error must say "at or before line N".

**Today (build 0058):** display goes through the printing satellite
(`satellite/display/printing_satellite.hpp`; SCRATCH.md/FAST_PRINTING.md, step 3,
2026-09-26). The interpreter hands each display's finished value to a ring and goes
on; a display thread turns it into text and writes it to stdout with `::write`.
`std::cout` writes into the same ring, so satl's own words keep their place among the
program's lines. The `std::cout.write` path measured above no longer exists; its
numbers stay as the record of 2026-09-14. At most `arguments.display.buffer` (131072)
displays may wait for the screen; past that the program is stopped with S840. A write
the output refuses is `display_error` (2), reported as S820 DISPLAY_REFUSED. SIGPIPE
is still ignored: 200,000 lines into `| head -1` ended with S820 and exit 2, not 141
(run on 0058). "At or before line N" is not built: no report uses those words.

---

## 9. Security: what every milestone throws at the executable

(author) "we mean to build this to be slightly secure environment."
**The entries list for every milestone includes:**

- **Names that are not `a–z A–Z 0–9 _`:** `!@#$%^&*()`,
  `poly.#@($*&$&$`, empty, one character, very long, and Unicode look-alikes.
- **Raw bytes where text belongs:** NUL, a UTF-8 byte-order mark, CR-only
  line endings, and a binary file.
- **Machine code as a name or value.** Compile a C++ function call, take its
  bytes (`objdump`), and feed them as a name, a string and a number. The
  interpreter must refuse them as names and treat them only as data. Nothing
  that arrives as text is ever jumped to.
- **Paths:** a directory as the `.satl` file, an unreadable file, a symlink,
  and a very long current directory.

**Today (build 0058):** check.sh has rows for an escape sequence and invalid UTF-8
as words given to a program, an escape sequence as the file's name, a directory as
the file (code 8), a symlink to a program (it runs), an unreadable file (code 8), a
byte-order mark (it runs, and the mark is dropped), and CR-only and CR LF line
endings (they run). Run on 0058 for this update: `$`, `é` and a Cyrillic look-alike
`а` (U+0430) in a name are refused before anything runs (S110); a one-letter name and
a 5,000-letter name run; a binary file as the program is refused before anything runs -- random bytes with
S110 (code 13), their bytes shown escaped, and the first 300 bytes of satl's own
binary with S101 (code 10), as a file with no include; a NUL inside a string is kept as data and displayed. Not built:
no row feeds machine-code bytes (`objdump`) in as a name, a string or a number, and
no row runs satl from a very long current directory. An empty name was not tried.

---

## 10. satellite.infinity

(author) `satellite.variable.infinity my_infinity_name = satellite.infinity.new()`,
with its fast path in `satellite/infinity.cpp` and the rest in
`satellite/infinity/special.<name>.satellite.cpp`.

**What must be decided before building it:**
- **The arithmetic:** infinity + 1, infinity − infinity, infinity × 0, and
  what `display` prints.
- **Order:** is infinity equal to itself, and larger than every
  `satellite_number`?
- **Every other word:** what `list.size()`, a loop bound or `sleep(infinity)`
  do when handed one.

**Today (build 0058):** the spelling is `satellite.variable.infinity my_infinity_name
= satellite.infinity()` (the word 1 26 0); `satellite.infinity.new()` is refused before
anything runs (S110, "no capsule named new"). The C++ is in
`satellite/satellite_variable_infinity/`, not in `satellite/infinity.cpp`. Built
(SATELLITE_INFINITY.md, Part 13, INF-1 and INF-2): `display` prints `(infinity)`, and
`i == i` and `i > 5` are both `true`. The arithmetic is refused by name: `i + 1` is
S210 NOT_BUILT_YET, "an infinity's + is not built yet (SATELLITE_INFINITY.md, INF-3)"
(run on 0058). SATELLITE_INFINITY.md (2026-09-18) holds the design, and puts this
section's questions to the author in its Part 12, among them Q2 (`inf - inf`,
`inf * 0` and others), Q21 (the order), Q27 (a word that wants a whole number: a loop
count, `list.satellite.size()`, `sleep`) and Q28 (where the C++ lives), each with what
is built if he says nothing. None of the four is marked answered there.

---

## 11. The prototype's defects, found 2026-09-14

A three-lens review, with every finding independently reproduced, proved
these in the first prototype. PLAN M1 was to fix all of them.

1. The order of include, main and return is not checked. Code outside `main`
   runs, a return on line 1 drops the program, and braces are never balanced.
2. A directory as the file reports "loaded" and then 10; read errors are never
   checked.
3. SIGPIPE kills the process with 141 and no report.
4. A refused write names a line far past the one that failed.
5. In debug mode, a refused state line is never reported on early-return paths.
6. An empty numbers folder gives 13 instead of 5.
7. Exit codes are cut to 8 bits: 256 exits 0.
8. Libraries load from the current directory when `/proc/self/exe` cannot be
   read.
9. `race.sh` prints a passing ratio when a run fails; `race.cpp` uses a
   cwd-relative library path.
10. Any `*.so` directory entry is loaded: directories, dangling symlinks,
    half-written files.
11. There is no ABI check (§3.4).
12. A library writing with `printf` prints out of order, because of
    `sync_with_stdio(false)`.
13. A UTF-8 byte-order mark or CR-only line endings give a misleading 10.
14. Whitespace inside the brackets is rejected: `display( 42 )` gives 13.
15. An unterminated string followed by a `//` comment gives 13 instead of 4.
16. Unknown escape sequences are accepted: `"a\qb"` displays `aqb`.
17. A library's file name is not checked against the numbers it describes.

**Today (build 0058)**, each re-checked on 2026-10-09 by a run or in the source.
ERROR.md keeps the full record of each: section "1. The prototype runner", and for
each one fixed, its section "Fixed".

- **Fixed:** 1 -- the checker refuses, before anything runs, a line outside every
  capsule (S110), a return that is not main's last line (S103, code 12), a `{` never
  closed (S110), and a file with no include (S101, code 10). 2 -- a directory is
  refused with code 8. 3 -- SIGPIPE is ignored, and a closed pipe is S820, exit 2
  (§8). 7 -- a code that does not fit exits 255 (§2). 13 -- a byte-order mark and
  CR-only line endings run. 14 -- `display( 42 )` prints 42. 16 -- an unknown escape
  is refused before anything runs (S110, naming the six escapes), on the author's
  ruling *"refuse an escape that is unknown"*.
- **15:** refused before anything runs, with a report that the string is never
  closed -- but still with code 13, not 4.
  Ruled 2026-10-09: the refusal stays at code 13
  (the author, answering the editors' questions: "looks good").
  So 15 counts as fixed, as ERROR.md has it.
- **Gone with the libraries on 2026-10-07:** 6, 8, 10, 11, 12 and 17. Nothing is
  loaded beside the binary, no word writes with `printf`, and each word built into
  satl is held against its table row and words.tsv at start-up (§3.4).
- **9:** `race.cpp` opens no library now, but `race.sh` still reads no exit status,
  so a run that fails still ends in a ratio (read in the source, not run).
- **4 and 5:** not re-checked.

---

## 12. Measurements, 2026-09-14

| what | result |
|---|---|
| satl 003 06 `display` alone, 1M lines, fastest of 14 | 4,433 ns per line, ×128.9 of C++ |
| satellite-004 prototype `display` (`<<`) | ×1.065 (37.22 vs 34.95 ns) |
| with `write` + `put` | ×1.002, ×1.003, ×1.003 |
| type table vs likely-first + vector search | 3.9 ns vs 12.2 ns per `+` |
| thread start + join | 28,254 ns |
| one word → numbers (hash lookup) | 25.8 ns |
| `long double` | 18 exact digits; every 64-bit byte count exact, bytes → terabytes → bytes exact |
| satl `display` `write()` calls per 1M lines | 699,262 (C++: 2,933 per 2M) |

---

## 13. Threads: the pool, batches and combine

*The batches and combine were not built, and the pool is no longer 256. Today, at
the end of this section, says what satl does.*

**(author) Targets:** 1,000,000 threads running, and 256 threads started first
and recalled whenever they are needed.

**(measured) This machine:**
- **24 hardware threads** (12 cores × 2).
- **Limits raised by the author:** `nproc unlimited`, `TasksMax=infinity`, and
  `threads-max` = `max_map_count` = 4,194,304. `pid_max` is 4,194,304, the top
  for 64-bit Linux.
- **A thread costs 34.3 KB:** 8.3 KB of program memory and 26 KB of the
  kernel's. So 1,000,000 threads need 32.7 GB, against 55.9 GB available.
- **240,000 threads were alive at once,** each doing 1,000 math steps: 0 wrong
  results, 6.2 s to start them, 2.9 s for the math and join.
- **Math stops getting faster at 24 threads** (10.7× on 3 billion steps).
  10,000 threads were slower (7.5×). Waiting work does scale: 10,000 threads
  did 10,000 seconds of waiting in 1.26 s.
- **Starting 256 threads takes 7.15 ms.** One recall — handing a job to a
  sleeping thread and waiting for it — takes 12,486 ns. A command done where
  it is takes about 28 ns.

**The rule these numbers give: recall a thread for a batch, never for one
command.** Over 10,000,000 commands:
- all on the main thread: 0.280 s;
- one recall per command: 111.8 s, 399× slower;
- 24 or 256 batches, one recall each: 0.021–0.022 s, 13× faster, same answer.

Break-even is about 450 commands per recall.

**How a batch keeps order.** A batch is a `std::vector<Call>` in written order,
and one thread runs it front to back. Batches are cut at loop-iteration
boundaries, each writes into its own buffer, and the buffers are printed in
batch order. Measured: 1,000,000 iterations of three calls, split into 24
batches, gave all 11,888,890 bytes identical to one thread.

**What cannot be split: a step that needs the step before it.** Examples are a
shared running total, reading a file where each line decides the next, and
input. (author) "If you have a list of commands that run in order … that is a
single thread." Only independent parts go faster, which is Amdahl's law. So:

- **The main thread** runs the program in order, at full speed, and never waits
  for analysis.
- **satellite combine** (author) runs on 24 threads at start-up. It works out
  what each statement reads and writes and marks, in the `.satb`, where the
  main thread must wait and which batches can run in parallel. Anything it
  cannot prove independent stays in order. Its hard cases are in PLAN M2.
- **The 256 pool threads** run marked batches, work that waits (files, the
  network, `sleep`), the interpreter's own jobs, and `satellite.thread.new`.
- **The correctness test:** combine on and off give byte-identical output.

**Today (build 0058):**

- **The warm threads** are the machine's cores × 2. The author, 2026-10-03: *"set
  the arguments.threads thing to be how many cores the machine has x2, so it checks
  how many cores the machine has, then starts double that amount, so 24 threads on
  this machine"*. Before that they were 256 (the author, 2026-09-15) and then 1024
  (2026-09-16). They are never more than `arguments.threads_max` (1000000). Here:
  12 cores, 24 threads, all parked before the program is read (`satl --debug`, run
  on 0058).
- **Their one job** is turning a program's lines into 16-bit codes before anything
  runs (§6), for a file and for each line typed at the prompt. Nothing else is
  handed to them.
- **A program's own threads** (`satellite.thread.new`, then `.start()`) each get a
  new operating-system thread, not a warm one: a pool of N threads deadlocks the
  moment N + 1 program threads wait on each other (003's M23;
  `satellite/bytecode/thread_calls.hpp`).
- **satellite combine is not built**, and there is no `.satb` (§14). The bytecode
  keeps four codes for its marks -- `batch_start`, `batch_end`, `wait` and
  `batch_size` (`satellite/bytecode/token_codes.hpp`) -- and nothing writes them
  yet. PLAN.md M2 is the plan, now marking the bytecode instead of a file. The
  correctness test, combine on and off, has nothing to compare yet.
- The measurements above are the record of 2026-09-14 and were not re-run.

---

## 14. The files: `.satc`, `.satb`, `.sati`

*None of these three files was built. Today, at the end of this section, says
what satl does.*

(author) Built in this order. With `arguments.satc` / `arguments.satb` at `1`
each is built before running, at `0` while running, and a special value turns
it off (PLAN M0).

**`.satc` — every word as its numbers.** 003's format (`old_versions/second_satellite/SATC.md`), ported:

```
satc 1
words 003.06 <hash of the word list>
source hello_world.satl <mtime> <size>

#1.1.1                                   // satellite.include(satellite)

#1.2 #1.3(#1.4.2<#1.6.1> arguments)      // satellite.capsule satellite.main satellite.container.list satellite.variable.string
{
    #1.5.1("Hello, World!")              // satellite.console.display
    #1.15.1                              // satellite.return(satellite)
}
```

- **The `#` is part of the format:** `1.5` alone is also the number
  one-and-a-half (SATC.md §1.1.1).
- **The header decides whether a `.satc` can be believed:** a different word
  list or a changed source means it is rebuilt.
- **A method written on a variable stays un-numbered** until names are resolved.
- **003 writes this file only for `satl --satc`, and an ordinary run never
  reads it** (measured: 0 accesses in a traced run). 004 was to use it on every run.

**`.satb` — the `.satc` with batch marks** (§13): wait here, run these in
parallel, and a batch size for each. A likely scenario can have its own number
under its word. (author) `display` of a string: `satellite.console.display` is
`1 5 1` and has no children, so `1 5 1 1` is free.

**`.sati` — strings as bits, 32 per character.** Every string literal becomes a
bit sequence, and `bits_to_cxx_str.cpp` turns one back into a C++ string for a
library. 32 bits a character is UTF-32 (`char32_t`, `std::u32string`): every
character has the same width, and ASCII text becomes 4× larger. To keep ×1.05,
a string is turned back once when the `.sati` loads, never per call.

**The numbers in the author's first sketch, checked against WORD_NUMBERS**
(003's `satl --words`, 2026-09-14; WORD_NUMBERS.md is now `old_versions/second_satellite/WORD_NUMBERS.md`):

| word | number |
|---|---|
| `satellite.include(satellite)` | 1 1 1 |
| `satellite.capsule` | 1 2 |
| `satellite.main` | 1 3 |
| `satellite.container.list` | 1 4 2 |
| `satellite.console.display` | 1 5 1 |
| `satellite.variable.string` | 1 6 1 |
| `satellite.variable.hex` | 1 6 11 |
| `satellite.random` | 1 7 |
| `satellite.return(satellite)` | 1 15 1 |

**Today (build 0058):** none of the three files exists, and there is no
`arguments.satc` or `arguments.satb` row. The author, 2026-09-16 (PLAN.md, D0.1):
*"we threw away satc and satb in favor of all 16-bit"*. A `.satl` is lexed straight
into 16-bit codes, checked, and walked, with no file in between (§6). The `.sate`
that took their place, the codes written beside the program, was dropped on
2026-10-03; the author: *"skip .sate altogether"*
(`satellite/config/satellite_config.hpp`; MILESTONES.md M31). A string is not 32 bits
a character: `satellite_string` holds one 16-bit code a character, and a character
that needs more is stored as three: 40000, then its number in the next two (the
author's D3.1, 2026-09-16; `satellite/satellite_variable_string/satellite_string.hpp`).
All that is left of the `.sati` is one reserved code (`string_index_token`,
`satellite/bytecode/token_codes.hpp`), and `bits_to_cxx_str.cpp` does not exist.
`--words` is 003's flag, and 004 refuses it (exit 23). The number table is still
right: all nine numbers are the same in words.tsv on 2026-10-09, and `1 5 1` still has
no children, so `1 5 1 1` is still free.
