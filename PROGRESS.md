# satellite-004 — PROGRESS.md

**satellite 004 revision 04** (the build number is in `satellite/config/satellite_config.hpp`,
and `satl --version` shows it). Where the work stands, 2026-09-16. Read this first
after a break; then **MILESTONES.md (everything not built, one milestone each)**,
PLAN.md (the order of work), DESIGN.md (the standards and every measurement) and
ERROR.md (every known error).

**2026-09-15: satellite 004 is the top of the repository.** satellite 003 revision 07
moved, unchanged, to `old_versions/second_satellite/` (tag `satellite-003-revision-07`,
branch `archive/satellite-003-revision-07`). Since then: revision 04 with build
numbers, the author's config rows, 256 warm threads, every source under
`satellite/`, and satellite_number and satellite_string (section 5 says what is
owed on them). Then PLAN M0.5: the build port, the installer and satl-term.

---

## 1. What is built and checked

| piece | files | checked by |
|---|---|---|
| **The prototype runner** — loads a .satl, checks include/main/return, runs `satellite.console.display` of a string, number or bool | `satellite/structured-library.cpp`, `satellite/satl/`, `satellite/arguments/`, `satellite/machine/`, `satellite/version/` | `./check.sh` — **41 passed, 0 failed** (§6.6 says how the four were retired) |
| **The author's config** — every `return_arguments_vector()` row loaded into `arguments`; the title lines (VERSION 004 REVISION 04 BUILD nnnn) on `--version`, `--help` and every start; the build number raised by every build | `satellite/config/satellite_config.hpp`, `satellite/config/build_number.py` | check.sh |
| **256 warm threads** — started from `arguments.threads_startup` and parked before the program runs (a requirement for later, the author) | `satellite/threads/startup_threads.*` | check.sh; about 12 ms and 1.7 MB a run |
| **Arithmetic** — `satellite.variable.number n = 34587`, `+ - * / % ^`, comparisons, assignment, `while`, per-capsule frames. Every math sign needs a space on both sides; a touching `/` between digits is a fraction | `satellite/bytecode/expression.*`, `program_check.cpp`, `program_walk.cpp`, `satellite/satellite_variable_number/number_arithmetic.hpp` | `./check.sh` 41/41; 482,465 cases against Python (§6.7) |
| **The 16-bit tokens** — `REGISTRY.satellite` is the 16-bit list; `bytecode_registry` is the program as `std::vector<std::vector<std::bitset<16>>>`, **one row a FILE** — the main `.satl` and every spaceship it includes — built on the warm threads in batches of lines. **The first thing the interpreter builds out of a program** | `REGISTRY.satellite`, `satellite/bytecode/` (`make_token_codes.py` → `token_codes.hpp`, `bytecode_registry.*`), called at `structured-library.cpp:155` | check.sh (32); `experiments/bytecode_registry_checks.cpp` — 19 checks; `--debug` shows `bytecode_registry(built)` on every run |
| **satellite_number** — sign bool + `unsigned long long` limbs, one-limb fast path (no allocation), + - * / %, text, digits, bytes | `satellite/satellite_variable_number/` | `python3 .../check_numbers.py build/number_cases` — 482,465 cases against Python (power and both radixes added 2026-09-16) |
| **satellite_string 16/32-bit** — the author's character table; 16 bits a character, 32 only when one is above U+FFFF | `satellite/satellite_variable_string/` | `.../check_strings16.py` — every case agrees with Python; `build/string_table_check` proves the table |
| **The number index** — every compiled library loaded once at start-up | `satellite-numbers/call_number.*`, `number_row.hpp` | loads all 24 libraries |
| **004's word numbers** — 364 words, first-available numbering, frozen (DESIGN §3.2) | `words/make_words.py` → `words/words.tsv`, `words/satellite_words.hpp` | matched 003's words.def row for row; no duplicates; every parent's children exactly 1..n |
| **satellite_string as char32_t** — strict UTF-8 ↔ char32_t ↔ .sati bit text, `bits_to_cxx_str` | `strings/satellite_string.*` | `python3 strings/check_strings.py` — 30,055 cases agree with Python's UTF-8 codec |
| **satellite_string's 23 words, one library each** (`1 6 1 0` .. `1 6 1 22`), behaviour ported from 003 06 | `strings/string_method.hpp`, `satellite-numbers/<exact word>/<exact word>.satellite.cpp` | `python3 strings/check_string_methods.py` — 44 cases match 003's real satl, plus 4 Unicode |
| **The library builder** — names each .so by its numbers from words.tsv | `satellite-numbers/build_libraries.py` | 24 libraries, 0 warnings |
| `satellite.console.display` library `1 5 1` | `satellite-numbers/satellite.console.display/` | `make race` — ×1.065 of std::cout today; ×1.002 with `write`/`put` (confirmed, not yet applied: ERROR.md / PLAN M1) |

**Waiting on other types** (answer machine code 14 `not_built_yet`):
`to_number`, `number`, `binary`, `hex`, and `string(x)` for anything but a string.

**Build and check everything:**

```
make                                   # interpreter + every numbered library; raises the build number
./check.sh                             # 41 checks, all pass since 2026-09-16 (§6.7)
make build/string_cases && python3 strings/check_strings.py   # char32_t conversion against Python
make build/string_methods && python3 strings/check_string_methods.py   # against 003's satl
build/satellite-004 --version          # THE SATELLITE PROGRAMMING LANGUAGE / VERSION 004 REVISION 04 BUILD nnnn
python3 words/make_words.py            # regenerate the word table (needs old_versions/second_satellite/satl)
```

## 2. Decided (by the author unless marked)

- **Version 004 revision 04, build numbers from 0050** (2026-09-15), raised by every
  build. 003 07 is archived in `old_versions/second_satellite/`.
- **The sources live under `satellite/`,** one folder a subject (2026-09-15).
- **Numbering:** 003 06's words minus the 7 GUI words, renumbered once
  first-available, frozen since. Next free: `satellite` 1 25, `satellite.variable`
  1 6 16, string methods 1 6 1 23. (Author delegated the choice.)
- **Every word is a library** in a folder named by the exact word, arguments
  included (`satellite.variable.string.find(x)`), built as `<numbers>.so`.
- **A string is 16 bits a character** (the author, 2026-09-15), and 32 only when it
  holds a character above U+FFFF. Codes 0–127 are every ASCII character once, in
  the author's order. An 8-bit path is still open. Translating satellite into
  other languages: dropped.
- **Machine codes 0–23** in `satellite/machine/machine_codes.hpp`; a code is added
  to the list before it is used.
- **Config values are rows** of `return_arguments_vector()`: a name, a number, a
  flag, and whether it is a flag (the author, 2026-09-15; DESIGN §1 still says
  quoted text). Maximums are used as ceilings.
- **Threads:** 256 started at start-up — 64 search for batches, 192 run them
  (the author wrote 196; 64 + 192 = 256). Target: 1,000,000 threads running.
- **Machine limits raised by the author** on this machine: `nproc unlimited`,
  `TasksMax=infinity`, `threads-max` and `max_map_count` 4,194,304 (not yet saved
  to /etc/sysctl.d, so a reboot resets those two).
- **Files:** `.satc` (numbers) → `.satb` (batch marks) → `.sati` (strings as bits).
- **Testing method:** each milestone's entries; done when a run writes no
  `[entry]` to satellite.log.

## 3. Measured, and what it means (details in DESIGN §12–14)

- 004's `display` is **~119× faster than 003 06's** (4,433 ns vs 37 ns a line).
- A thread costs 34.3 KB (8.3 KB program + 26 KB kernel); **1,000,000 threads =
  32.7 GB**. 240,000 ran math at once with 0 wrong results.
- **Math stops speeding up at 24 threads**; waiting work scales to 10,000+.
- **One recall = 12,486 ns**, one command = 28 ns: hand threads BATCHES, never
  single commands (break-even ~450 commands).
- **TBB's `parallel_for` should run the batches:** equal to our pool on one huge
  loop (×14.1 vs ×13.6), **9× faster than our pool** on a small loop repeated
  10,000 times, where our pool was slower than one thread. oneTBB 2023.1 is
  installed at `/opt/intel/oneapi/tbb/2023.1`. *(Recommended; not yet decided.)*
- **The author's parallel design works as a model** (`experiments/parallel_demo.cpp`):
  5.5× faster, byte-identical output, the monitor unmarked a group that did not
  pay, and the guard caught a hidden shared write at iteration 1,700,001.

`experiments/` holds every program those numbers came from, copied from the
session's scratchpad so they survive it.

## 4. Open decisions (the author's)

- ~~**D0.1**~~ and ~~**D1.1**~~ **ANSWERED 2026-09-16: DEAD** -- *"we threw away satc and
  satb in favor of all 16-bit"* (MILESTONES.md).
- ~~**D3.1**~~ **ANSWERED 2026-09-16:** *"32-bits only when we use the number 40000 as a
  16-bit code"*. Everything is 16 bits; `wide_token` (40000) says the next two codes are
  one 32-bit integer. Owed: the lexer's `wide_run_32_token` path gives way to it.
- **D9.1–D9.2** polymorph: what "re-included into the individual capsules" means,
  what `args` are passed to. ~~**D9.3**~~ **ANSWERED 2026-09-16:** *"a class declared
  twice is an ERROR: name collision"*.
- **D11.1** infinity's arithmetic.
- **D12.1** the parallel-group syntax in the numbered file.
- **Adopt TBB for the runners?** and record it in DESIGN §13.
- One more thing to remove from 004's words that the author could not remember.

## 5. Next

**Owed on satellite_number and satellite_string (2026-09-15).** Both types are
built and check against Python, but the session's limit stopped the rest:
- **nobody has tried to break them.** Four adversarial reviewers and the
  migration never ran (workflow `wf_2846936f-fc6`, agents killed by the limit).
- **the 23 string-method libraries still use the 32-bit `strings/` string.** They
  move onto the new one, checked against 003's satl, in the same step. `strings/`
  then moves into `satellite/satellite_variable_string/`.
- ~~the tokens move to 16 bits~~ **DONE 2026-09-16** — see §6.
- **an 8-bit string holds ASCII only** (decided 2026-09-16); 16-bit holds up to
  U+FFFF, 32-bit everything. The 8-bit path is not built yet: the committed
  string chooses between 16 and 32.
- **the races miss ×1.05 in places** (ERROR.md §5), and the four string words
  waiting on satellite_number (`to_number`, `number`, `binary`, `hex`) are not
  built.

Then the prompt (PLAN M0.5 → M0.6 → M0.7). The prototype's defects (ERROR.md §1)
are PLAN M1.

## 6. The 16-bit tokens, 2026-09-16 — and the nineteen rows the author still owes a ruling on

**The layout is the author's, and it is settled.** Codes 0–127 are the characters,
ASCII only, in his order; **128–255 are kept back and never issued**; **256 is the
first token**. So the HIGH BYTE says which — zero is a character, anything else is
a token — which is the 8-bit registry's one-test rule widened, and an 8-bit ASCII
string widens into it by zero-extending. The high byte also names the family
(`0000001100000000` up is always arithmetic), so a family grows into its own 255
spare codes and nothing is ever renumbered. **69 tokens**, in eleven families.

**Two tokens the author added the same day:** `binary_token` (`b11001100`) and
`hexadecimal_token` (`xFFAAC2985765`), which replace the single `bits_token` — the
radix stops being a field and becomes the token.

**EVERY `satellite.something.something` HAS ITS OWN 16-BIT CODE, 4096-8191** (the
author, 2026-09-16). 4096 is reserved and no word has it; the words run from 4097,
364 of them today, leaving 3731 for the ones a user defines. **4096 and not lower
because the eleven token families reach 3071** — a range starting at 1024 would have
moved seven of them.

**TWO NUMBERS, ONE TABLE — and the author named the risk himself:** *"so really
everything has two different numbers, this is a new set we must maintain… the number
inside of the interpreter, and the number inside of a .sat file."* They cannot be the
same number (a path is a tree of any depth, a code is flat and fixed width), but only
one is maintained: **a word's code is 4097 plus its ROW in `words/words.tsv`**, which
is already the authority (DESIGN §3.2). `make_word_codes.py` generates the header, so
nothing is hand-assigned and the two cannot disagree. Appending a word is safe;
inserting one shifts every code after it, and SATC.md §2's word-list digest is what
makes that loud rather than silent.

`word::code_of(1, 5, 1)` → `4163` is the conversion, and `word::numbers_of(code)` is
the way back for whatever writes a `.sat` file out again. **A `.sat` file stores a
word as `int int int int`** (the author) and the interpreter holds the one code.
`code_of()` answers `0` — the registry's own *nothing* — for a word with no code, and
the caller then writes `word_number_token` and the numbers, which has no ceiling. So
the range is a fast path and never a limit.

**ONE ROW A FILE, NOT A LINE** (the author, 2026-09-16): *"the reason for the second
vector is we have to take in other satellite files, like other includes"*. Row 0 is the
main `.satl`; every spaceship taken in by `satellite.include()` gets a row behind it,
and the row's INDEX is which file it is. Statements are found INSIDE a row by
`line_end_token`, which is the reason that token exists at all. Nothing records a
file's NAME yet — a name belongs beside the registry, not inside it, and PLAN M1 is
where that gets settled.

**EVERY [COUNTED] TOKEN CARRIES A COUNT, and this is the one thing the 8-bit design
could not carry over.** At 8 bits a run of characters ended when the high bit
flipped. That test is *arithmetically false* at 16 bits: above 127 a character is
its own Unicode number (`satellite_string.cpp:53`), so a string holding one wide
character carries codes indistinguishable from token codes. QUAD's `view.hpp:84`
writes `" ∞  "` as one literal and would have ended its own string early. So a
count follows every literal marker, the counted codes are SKIPPED and never
classified, and `long_count_token` continues a count that does not fit in 16 bits
so no literal has a ceiling (DESIGN §1.2).

**Which tokens are counted is NOT a family test, and making it one was a defect the
checks caught.** A family says what a token MEANS; carrying a count is what it does
STRUCTURALLY, and the two do not line up — `error_token` is in the stream family and
is counted, `comment_token` is in the text family and is a marker only. Testing the
family let a reader walk into a payload's length as though it were a token. The rows
marked `[COUNTED]` in the registry are the list, and `carries_a_count()` is generated
from them. (A comment's text is not stored at all, which is 003 DESIGN §5.6 exactly:
`//` is discarded in the lexer. PLAN M1.5 already says comments do not round-trip.)

**Measured, 2026-09-16, and both numbers went against the first guess:**

- `std::bitset<16>` **is 8 bytes, not 2** — libstdc++ rounds it to an
  `unsigned long`. A pass over a stored program costs **3.5–5×** the same tokens
  as `uint16_t` (0.67 ms against 0.18 ms at a million tokens), and 4× the memory.
  **The author chose it knowing that** — "8 bytes for a 16 bit token is fine" —
  because `.to_string()` is the sixteen binary digits the registry's own column is
  written in, and what PLAN M1.5–M3.6's converters print.
- **One thread per line is 5.6× SLOWER than one thread doing all of it** (227 ms
  against 40 ms for 100,000 lines, through the real `StartupThreads`): a line
  costs ~402 ns to tokenise and a recall costs ~12,486 ns. The same 256 threads
  given **256 batches of lines** run it 27× faster than one thread. This is PLAN
  M1's "about 400 times slower", now measured rather than quoted.

**THE NINETEEN ROWS MARKED `QUESTION` IN THE REGISTRY.** A review on 2026-09-16
(eight agents, every claim checked to a file and line) found that of the 57 tokens
first written down, **nineteen name forms this language cannot emit** — and the
author named several of them on the same day, so they keep their codes until he
rules. Each row carries its reason beside it. The list, by why:

| rows | why it is questioned |
|---|---|
| `<<` `>>` | 003 DESIGN §5.5: *"There is no `<<` and no `>>`, ever"* — permanent policy, and the property that makes `list<list<string>>` safe. The author asked for both on 2026-09-16 |
| `/*` `*/` | §5.6: no block comment, *"a form that can be left unclosed is a form that can swallow a file"* |
| `+=` `-=` `*=` `/=` `%=` | §5.5: the greedy two-character operators are exactly `== <= >= !=`; §6.6: assignment is a statement, not an operator. QUAD wants `+=` (MISSING #12) |
| `&&` `\|\|` `!` `&` `\|` `^` `~` | §13 leaves open what `&` means **and** whether `!` is the negation — and says the answer may be **words**, not symbols. QUAD wants `&&`/`\|\|` (MISSING #9, promised at M28) |
| `::` `->` `?` `'` | No grammar production accepts any of them, none appears in any of the 22 example programs. QUAD's 574 `::` all become dotted words; its ~35 ternaries all become `satellite.statement.if` |
| `\` `/` (path) `.` (decimal) `~` (home) | The registry's **own** rule: a token is *"never a character and never inside a string's value"*, and all four only ever occur inside a literal. 003 spells `$HOME` as a live **character** code, not a token |

**Two additions the review found that the language had already decided and never
got codes for**, both now in: `bit_run_join_token` (`!!` — DESIGN §6.6, *"A FIFTH
LEVEL IS DECIDED AND UNBUILT"*, 2026-09-09) and `option_token` (`0#down` —
SATC.md §1.1, *"a third kind of token since M19.6"*).

**One idea the review killed, and it was mine:** separate `type_open`/`type_close`
codes for a generic's `<` `>`. Four of five lenses proposed it; `parser_types.cpp:122-127`
records that the collision it would solve is already unreachable, and §5.5 solved
it by subtraction years ago.

**Still open, and the author has to take it before the converters are built:** is
the stored program a token transcription that is re-lexed, or a 16-bit code
stream? PROGRESS §5 and SATC.md §4 currently say different things, and the answer
governs whether the writer may record what the parser knew.

## 6.5 The bytecode path RUNS a whole program (2026-09-16, later the same day)

`752d4e6`, `378e13f`, `878b63b`, `d8eeab2`, `2118359`. `experiments/program_run.cpp`
on `test_programs/hello_world.satl`:

```
loaded 5 files, one row each: hello_world 223 codes, test_file 54, another_test_file 59,
                              test_dir/test_file 34, test_dir/final_test_file 57
5 capsules, found by NAME       satellite.main row 0 body 128, some_capsule row 1 body 20, ...
running satellite.main:
hello, world! / some test complete! / another test completed! / final test completed!
```

- **`FunctionTable`** — `table[code - 4096]`, one subtraction and one load. 4096 pointers,
  32 KB, read-only after start-up so any number of threads dispatch without a lock.
  24 of 364 words have a library; `NumberIndex::find()` is now start-up only, which is
  what `call_number.hpp` always promised.
- **`CapsuleTable`** — a user's capsule by NAME, because a name a user invents has no
  number. **THERE IS NO BODY** (the author saw it first): a `CapsuleSite` is two
  integers, `{which row, where the brace was}`. Calling one moves the walker's position.
  Nothing is allocated to run a line — the whole difference from 003's tree.
- **`include_shape`** — the author's five spellings, told apart by the first code after
  the `(`. The **unquoted path** `include(dir/file)` is new in 004 (003 required quotes)
  and is whitespace-sensitive: `dir / file` with spaces is *division*.
- **A cwd for every file** — each file's includes resolve against **its own** directory.
  `experiments/nested_include/` proves it with a decoy `two.satl` that must lose.
- **`file_can_run`** — refuses a file with no `include(satellite)`, no `main`, or no
  `return`, codes 10/11/12, matching the prototype's `check_satl` so the two cannot
  disagree. **There are no globals** (the author): execution begins and ends inside main,
  and the only globals are the includes. A *string* saying `satellite.main` does not
  count — a payload's codes are skipped, never classified.
- **A C++ file carries byte for byte** — six real sources round-tripped, including
  `bytecode_registry.cpp` itself (braces, quotes, `'\\'`, `//`). This settles
  POLYMORPH/M6's *"needs a closing marker nothing in C++ can contain"*: a **count** needs
  no marker, so `foreign_text_token` needs no code.

**RECOMMENDED, NOT YET APPLIED TO PLAN.md — three files collapse to one.** `.satc` is
superseded (the bytecode IS the numbered program, one code a word); `.sati` is superseded
(strings are 16-bit codes inline, counted — **D3.1 dies with it**); `.satb` survives as
*work* but not as a *file*, because combine's marks live in the bytecode and the registry
already reserves `batch_start`/`batch_end`/`wait`/`batch_size`. M1/M2/M3 + M1.5–M3.6 →
about two milestones. **Keep SATC.md §2's header**: a word's code is its row in
`words.tsv`, so the word-list digest matters *more* now, not less.

**Owed, smallest first:**
1. **The leading-slash rule is a FALLBACK, not a rule.** `find_file()` tries the
   filesystem root, then the main file's directory, because the author wrote
   `include("/test_dir/final_test_file.satl")` and put the file under
   `test_programs/test_dir/`. Either `/` is the filesystem root (003's rule) or it is the
   program's own root (a new rule). Nothing else in the include path is a guess.
2. **Two implementations of "runnable".** `satl_file.cpp:118` and `file_can_run()` both
   decide it, with the same codes. Two will drift.
3. **The value type.** `display(42)` still has nowhere to put its argument. `Call`'s
   `kind`/`text`/`count`/`flag` wants to become one type over `satellite_number` and
   `satellite_string`. This is the real next piece.

## 6.6 The interpreter runs the bytecode, and check.sh is 28/32 ON PURPOSE

`ca8d060`, 2026-09-16, the author: "we specifically build this into the interpreter...
Can you just make it work?" `structured-library.cpp` is now `load_program` →
`file_can_run` → `capsules_in` → `run_main`. `satl test_programs/hello_world.satl`
prints four lines; `satl examples/hello_world.satl` prints all four of its own,
including `42` and `true`. `std::vector<Call>` no longer runs anything.

**THE FOUR FAILING CHECKS ARE A NAMED DEBT, NOT AN UNNOTICED REGRESSION:**

```
FAIL  a line with no scenario -> wanted 13, got 0
FAIL  nothing ran before the refusal -> wanted , got before
FAIL  "some" + "str" (no scenario yet) -> wanted 4, got 0
FAIL  a number too large -> wanted 3, got 0
```

All four are one gap: **the prototype checked the whole program before running any
of it; this path discovers as it goes.** `compile_satl` cannot be borrowed for the
verdict — tried, and it refused `test_programs/hello_world.satl` outright with 13,
because it is the prototype's checker and knows neither a user's own capsule nor any
include spelling past the first. **THE NEXT PIECE IS A PRE-PASS OVER THE BYTECODE**,
walking every capsule body before main is entered; it buys all four back and retires
the duplicate path at the same time.

**The argument chooses the scenario** — `string_token` → `text`, `number_token` →
`count`, and `satellite.bool.true/false` arrive as word codes `1 17 2` / `1 17 1` and
pick `flag`. That is the smallest thing that is not a value type, and the seam a real
one goes into.

**Four include spellings, no probing** (the author): `include(test)` and
`include(/test)` are both `./test` — a leading slash is RELATIVE; `include(~/test)` is
the home directory; `include(/root/x)` reaches the machine's root, asked for. The
rejected alternative was "try the cwd, then the root", whose cost is that the same
program run from two directories loads two different files, silently. `//` would have
been unambiguous but the lexer reads it as a comment. `include(/test)` did not parse
at all before — a leading slash is a `path_separator_token`.

**Two real defects the checks caught, both fixed:** the library's answer was thrown
away (a refused write answered `display_error` and the walker ignored it), and a
refused write is only refused **at the flush** — `/dev/full` succeeds line by line and
fails once at the end, so without the flush check the program exited 0 having printed
nothing.

## 6.7 ARITHMETIC RUNS, 2026-09-16 — the tokens reach satellite_number

`satellite.variable.number my_number = 34587` declares, `+ - * / % ^` all run, and
the author's own `test_programs/hello_world.satl` — the while loop counting to
99,999 — runs end to end. **check.sh is 41/41**, the first time it has been green:
the four failing "on purpose" are retired, two by the new pre-pass checker and two
because `satellite_number` abolished the thing they asserted.

| piece | file |
|---|---|
| **power**, by squaring over the exponent's LIMBS, and base 2 / base 16 text | `satellite/satellite_variable_number/satellite_number_power.cpp` |
| **the six fast paths the tokens call** — add, subtract, multiply, divide, power, modulus | `.../number_arithmetic.hpp` |
| **the conversion fast paths** — number ↔ text in all three radixes | `.../number_conversions.hpp` |
| **the value type PROGRESS §6.5 said was owed** — holds a `satellite_number`, not a 64-bit count | `satellite/bytecode/value.hpp` |
| **the token → fast path wiring**, precedence climbing over 003 §6.6 | `satellite/bytecode/expression.cpp/.hpp` |
| **the pre-pass checker** — every capsule body judged before main is entered | `satellite/bytecode/program_check.cpp` |
| declarations, assignment, `while`, per-capsule frames | `satellite/bytecode/program_walk.cpp` |

**The author's four rulings, all 2026-09-16, all in the registry:**

- **EVERY math operation is written `space sign space`.** *"Literally every math
  operation, ANY math operation has to have spacebar(sign)spacebar"*. This was
  already the rule for `/` alone; generalising it is what frees the touching
  spelling of each sign.
- **A touching `/` between two digits is a FRACTION.** `5/4` now answers
  `fraction_token` and refuses by name (`not_built_yet`) instead of meaning
  anything. `dir/file` is still a path — the digits on both sides are the test.
- **`^` is power**, and it has its own `power_token` in the arithmetic family.
  `bit_exclusive_or_token` keeps its code and no spelling reaches it.
- **`.power()` is an alias for `^`** — *"so .power() becomes something you can
  call on that object"*.

**OWED, AND THE FIRST THING TO PICK UP:**

1. **`satellite/bytecode/number_methods.cpp/.hpp` IS WRITTEN AND NOT WIRED.** It is
   in the tree, in no Makefile line, referenced by nothing — dead code, on purpose
   and committed so it is not lost. It holds `.power()` and the rest of
   `1 6 4 n` as names bound to the fast paths. What it still needs: the Makefile
   rows, and a branch in `one_operand` that reads `name . name (` off a declared
   variable. **Either finish it or delete it — do not leave it a third session.**
2. **`n = 1 & 2` silently answers 1** (ERROR.md, verified). `run_assignment` and
   `run_while` need the same three lines `call_word` already has.
3. **`satellite.statement.if`** is the missing half of `while`.

**MEASURED, AND THE FIRST TWO NUMBERS I GAVE THE AUTHOR WERE WRONG — both are
corrected here.** Against **CPython 3.12**, which is the fair comparison (both are
bytecode interpreters; `python3` on this machine is **PyPy**, a JIT, and racing it
means nothing):

| | 004 | CPython 3.12 |
|---|---|---|
| start-up | **14.7 ms** | 15.1 ms |
| display, a line | 648 ns | 559 ns |
| `i = i + 1`, an iteration | 458 ns | 87 ns |

**004 beats CPython to start** — while parking 256 threads and `dlopen`ing 24
libraries — and ties it on display. The whole gap is arithmetic, and it is one
thing: CPython resolves a local to an array slot (`LOAD_FAST`); 004 rebuilds a
`std::string` from 16-bit codes for every name and hashes it into a map, on every
evaluation. **That is the object model's work** (the author: *"the next thing we
are doing after this is the satellite object model"*), and it is where the ~5×
is.

*Two corrections worth keeping so they are not repeated: 003's start-up is
**2.2 ms**, not the 268 ms first reported — that was the prompt waiting on stdin,
and every timing needs `stdin` closed. And "004 displays 6.9× faster than 003"
was measured on a file of 100,000 separate `display` statements, so it was mostly
003's PARSE cost, not its display. A display race must put ONE statement in a
loop.*

## 6.8 THE OBJECT MODEL RUNS THE INTERPRETER, 2026-09-16 — and the three words

`e21522b` built the classes, `d0cce17` wired them in. **check.sh is 41/41**, and
the language's own 16/32-bit string reaches the interpreter for the first time:
`"wide: ∞ é 日本語"` and emoji past U+FFFF round-trip through `satellite_string`
instead of `std::string`.

**THE AUTHOR CAUGHT A NAMING ERROR THAT WAS THREE THINGS UNDER ONE NAME** —
*"this is our spacesuit… this is satelliteSpacesuit not satelliteObject!"* He was
right, and 003 had already settled it. The language's own vocabulary:

| the word | what it is | the type |
|---|---|---|
| **spacesuit** | THE CLASS. `satellite.spacesuit` is word `1 10` in words.tsv; `satellite.class` is the author's second spelling (2026-09-09). DESIGN §13: *"Reference semantics"* | `satelliteSpacesuit` (003's `Layout`) |
| **object** | ONE INSTANCE. **Not a word of the language** — in neither words.tsv nor REGISTRY.satellite — but the author's own prose: DESIGN §13's *"what a constructor produces is the object"*, M26's `object_name.pointer()` | `satelliteObject` (003's `SuitObject`) |
| **spaceship** | a FILE that is included, `1 1 2`. A different thing again | — |

**A VALUE IS NOT AN OBJECT**, which is why the variant is `satelliteValue` and
not `satelliteObject`: a number is a value and it is an instance of nothing. 003
called this exact type `Value` (`src/satellite_value/value.hpp`, sixteen arms
since M26); this is that type rebuilt on 004's number and string.

**THE CLASS AND THE INSTANCE WERE ONE STRUCT, AND THAT WAS THE REAL DEFECT** —
not the name. Every instance carried its own copy of its class's field NAMES.
Split the way 003 splits them: the spacesuit holds the layout and is shared, the
object holds a bare pointer to it and a flat `vector<satelliteValue>` indexed by
slot. **That is what keeps a field read an integer index** instead of the string
hash that already costs 004 ×5 against CPython (§6.7).

**The object arm is a `shared_ptr`, and that is the author's ruling and not a C++
workaround.** DESIGN §7.4 says a spacesuit is a reference type; M26 built it that
way — a capsule handed one mutates it and the caller sees the mutation. Storing
the instance inline would make `b = a` copy it, deciding the opposite by
accident. *(It also does not compile: `std::variant` needs complete types.)*

**ONE FUNCTION, ONE FILE, NAMED FOR ITS PAIR** (the author): nineteen headers —
`number_and_number_add.hpp`, `number_and_string_add.hpp`,
`bytecode_and_bytecode_join.hpp`, `number_to_string.hpp`. `expression.cpp` gave
up its token → fast path table; `satelliteValue::add` decides on the PAIR of
kinds through a `case pair_of(number, string):` that reads like the filename.
**That file is the grammar now; the object model is the meaning.**

**Keeping `satellite_bytecode` in the variant was the author's call and it cost
nothing** — the object is 80 bytes with the arm and 80 without, because every arm
is a handle. It also bought `bytecode_and_bytecode_join`, a fast path that had
nowhere to live before: QUAD writes satellite, and joining runs of codes without
going out to text and re-lexing is what that needs.

**`strings/string_method.hpp`'s `satellite_string` is now `satellite_string32`.**
Two string types had shared one name in one namespace since 2026-09-15 and
nothing had ever included both — the object model does, so it became a compile
error instead of a trap. §5's owed migration is unchanged; the two are just
tellable apart while it is done.

**OWED, AND THE FIRST IS THE ONE THAT MATTERS:**

1. **NOTHING CAN DECLARE A SPACESUIT YET.** The classes are built and the value
   type runs the interpreter, but `satellite.spacesuit` has no parse rule in
   004 — no lexer shape, no `capsules_in` sibling, no constructor. **The object
   model is the machine; the grammar is not written.** 003's
   `parser_declarations.cpp` and `resolve.cpp` are the port.
2. `satellite/bytecode/number_methods.cpp` **is still dead code, third session.**
   It is now one hop from real: its methods are `satelliteValue`'s pair files
   already. Either finish it or delete it.
3. **`n = 1 & 2` still silently answers 1** (ERROR.md). `run_assignment` and
   `run_while` need the three lines `call_word` has.
4. **`satellite.statement.if`** is the missing half of `while`.
5. Threads: 003's `SuitObject` carries a mutex and an access list (THREAD.md
   D1/D2) because two threads sharing one object freed a string twice. 004's
   walker is single-threaded, and 256 threads are already warm.

## 7. Other notes

- `POLYMORPH/M1.md`–`M7.md` (top folder, uncommitted) hold the earlier
  polymorph discussion; `POLYMORPH/M7.md` still says 342 words — it is 371 in 003,
  364 in 004.
- `polymorph/test.cpp` (top folder, uncommitted) is the author's timing test.
- The author wants notes at the top of every file, and wants anything
  questionable questioned.
