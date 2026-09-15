# satellite-004 — DESIGN.md

Started 2026-09-14. The fourth start of satellite, and in the author's words
"as we rebuild satellite for the last time". PLAN.md is the order of work; this
file is the standards every line of satellite-004 is written to.

Everything marked **(author)** was decided by the author on 2026-09-14.
Everything marked **(measured)** was run on this machine that day; the command
is in §12. Anything else is a recommendation and says so.

---

## 1. The standards

1. **The fastest C++ path, always.** (author) Every satellite word is a small
   compiled C++ library of *likely scenarios*. The most likely scenario is
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

---

## 2. Machine codes

`machine_codes.hpp` is the list. Today: 0 success, 1 error, 2 display_error,
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

---

## 3. The number vector and the libraries

### 3.1 A row

The author's sketch was
`std::vector<std::map<std::string, std::string, unsigned long long int, … ×6>>`.
**A `std::map` holds a key and one value**, so that does not compile
(measured: *"wrong number of template arguments (5, should be at least 2)"*).
What it describes is a row:

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

### 3.3 Commands deeper than six numbers

(author) "is there a way to program it so that the maximum command is like, 999
ints long?" **Yes:** six numbers are held inline, so the common case never
allocates, and a deeper command keeps the rest in `longer`. Its depth is a
`satellite_number`, so it has no maximum. (author) A command longer than six
numbers takes the C++-library path; test it with a 90-word command.

### 3.4 A library

- **Source:** `satellite-numbers/<word>/<word>.satellite.cpp`, with no `main`.
- **Built:** `build/satellite-numbers/<n.n.n>.so`, named by its numbers.
- **Exports one function:** `satellite_number_describe(LibraryRow *)`, which
  fills in its name, numbers and scenario pointers. It is called once at
  start-up.

**Rules the prototype review proved necessary:**
- Load only regular files whose name is `<numbers>.so`.
- Refuse a library whose file name disagrees with the numbers it describes.
- Refuse an empty index (code 5, not 13).
- Never load from the current directory. When the executable's own path
  cannot be read, stop with code 5.
- Check the C++ ABI at load. A library built with
  `-D_GLIBCXX_USE_CXX11_ABI=0` loaded and wrote 82 KB of heap memory to
  stdout.

**Dynamic linking on purpose:** the interpreter and every library must share
one libstdc++, or each has its own `std::cout`.

---

## 4. satellite_number

(author) "satellite_numbers are made by laying unsigned long long ints beside
each other, and the sign is carried as a bool with the object… keep the number
of digits in each number, keep the size of the number in bytes always as a
satellite_number, so a satellite_number will be made out of a std::shared_ptr to
another satellite_number."

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

---

## 6. Start-up, threads, and reaching `satellite.main`

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

---

## 11. The prototype's defects, found 2026-09-14

A three-lens review, with every finding independently reproduced, proved
these in the first prototype. PLAN M1 fixes all of them.

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

---

## 14. The files: `.satc`, `.satb`, `.sati`

(author) Built in this order. With `arguments.satc` / `arguments.satb` at `1`
each is built before running, at `0` while running, and a special value turns
it off (PLAN M0).

**`.satc` — every word as its numbers.** 003's format (SATC.md), ported:

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
  reads it** (measured: 0 accesses in a traced run). 004 uses it on every run.

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
(`satl --words`, 2026-09-14):

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
