# satellite — the layout

Every file in this tree, and one line on what each is for.

**This is a map, not a declaration.** Nothing here is load-bearing: what actually
gets compiled is declared in `make_support/040-sources.mk`, and what actually gets
installed is declared in `satellite_enterprise/install_support/060-install-tree.sh`.
Those two are the authority and this file is a reader's index of the tree. Keeping
a second list that *decided* anything is how a tree rots — the day someone adds a
file to one list and not the other, one of them is silently wrong.

So: when a file is added, add a line here too, and know that forgetting costs a
reader a minute rather than costing a build a file.

Companions: [DESIGN.md](DESIGN.md) is what the language is, [PLAN.md](PLAN.md) is
how it gets built, [WORD_NUMBERS.md](WORD_NUMBERS.md) holds every number in it,
[SATC.md](SATC.md) specifies the `.satc` file those numbers get written to, and
[HELP.md](HELP.md) is what each of those numbers means to somebody using it.

---

## The root

| file | what it is |
| --- | --- |
| [README.md](README.md) | The front door: what satellite is, hello world, where the state of things is written down. An index, and deliberately holds no fact of its own. |
| [DESIGN.md](DESIGN.md) | The language: the generating rule, syntax, the numbering, scope, types, and what it refuses. Permanent. |
| [PLAN.md](PLAN.md) | The work: architecture, the build, the install, milestones, measurement discipline. Permanent. |
| [WORD_NUMBERS.md](WORD_NUMBERS.md) | **The numbering**, and the authority over every number in the language. DESIGN §4 explains it; `words.def` transcribes it; when they disagree this file is right. Permanent. |
| [SATC.md](SATC.md) | The `.satc` file format: a program with its language-owned words replaced by their numbers, cached in `$HOME/.satl/cache`. Specified before it was built, because it constrains the numbering; **built at M4.5 on 2026-08-30**, which corrected three things in it. Permanent. |
| [QUAD.md](QUAD.md) | The goal: `quad_infinity` must be expressible in satellite, what that program needs, and — after reading its source on 2026-08-27 — the one thing the language has not settled that it needs. Permanent. |
| [HELP.md](HELP.md) | **The language's own account of itself**, one entry for every node `words.def` numbers: what you type to bring it up, what the word is for, and a worked line. Generated from `help_lines/`, and **every worked line in it has been run** against the interpreter in this tree. Written for M18 and **built by it**: the same run of `help_lines/gen.py` that writes this document writes `src/satellite_help/help.def`, so what the file says and what `satellite.help` prints cannot disagree. The text went into a `.def` of its own rather than into `words.def`, which the plan had guessed — 90KB of prose in the numbering would put a document inside the registry a program's meaning depends on. Permanent. |
| [LAYOUT.md](LAYOUT.md) | This file. |
| [PLAN_ONE.md](PLAN_ONE.md) | The first draft plan, **superseded** by the two above and deletable as soon as nothing cites it. |
| [Makefile](Makefile) | An index. Includes the twelve fragments under `make_support/` in numbered order and does nothing else. |
| [LICENSE](LICENSE) | MIT (Expat) for satellite's own source, plus a third-party section for `pcg/`, which is Apache-2.0. It also records that no built binary currently contains any of it. |
| [pcg/](pcg/) | The only third-party code in the tree: three pcg-cpp 0.98 headers, its licence, and a README recording what was cut, why `-isystem`, and why a 512-bit variant was refused. |
| [.gitignore](.gitignore) | Build output, and the deliberate exclusion of `old_versions/` from this repository's history. |

## `FORMAT/` — how the code is written

| file | what it is |
| --- | --- |
| [FORMAT/CXX.md](FORMAT/CXX.md) | Everything needed to write C++ in this tree: the house style, the comment culture, how the build is edited to add a module, how a test is built, and the X-macro registry mechanism M2 ports from the first satellite. Its §9 listed the ten things that were **not** decided anywhere and blocked M2; all ten were settled when M2 was written and §9 now records each answer and where it lives. Permanent. |

The directory is separate from the root because these are documents about *writing the
code*, not about the language or the plan. The four permanent documents at the root
answer "what is satellite"; this one answers "what does a file here look like."

## `src/` — the interpreter

One folder per module, named for the job the module does rather than for the
abbreviation its files use. Every unit spells its includes from the top of `src/`
— `"system_facts/version.hpp"` and never `"../version.hpp"`.

| file | what it is |
| --- | --- |
| [src/programs/main.cpp](src/programs/main.cpp) | `satl` itself: reads the command line, answers `--version` / `--help` / `--words` / `--tokens` / `--unparse` / `--errors`, and hands every other arm to the file beside it — `--satc` to `cache_command.cpp`, `--check` to `check_command.cpp`, and **since M10 a bare filename and `--run` to `run_command.cpp`**. *What to say* when a file will not open is this program's business; **getting the bytes is `source_file.cpp`'s**. |
| [src/programs/opening.cpp](src/programs/opening.cpp) | The banner and the usage text — the words, kept in a `.cpp` because they change every milestone. |
| [src/programs/window_handover.hpp](src/programs/window_handover.hpp) · [.cpp](src/programs/window_handover.cpp) | Started with no console, `satl` hands itself to `satl-term` (DESIGN §10.4). The test is a **controlling terminal**, not `isatty(stdout)` — the obvious version opens a window instead of feeding a pipe. Six named refusals. |
| [src/programs/opening.hpp](src/programs/opening.hpp) | Declarations for the above, plus the exit-status enum so two arms cannot disagree about what a failure is worth. **M6 added `EXIT_LIMIT = 4`** and widened `EXIT_MALFORMED` from "not a satellite program" to "a file satl was given is not what it has to be", because a `satellite_config.ini` is now the second kind. |
| [src/programs/limits_command.hpp](src/programs/limits_command.hpp) · [.cpp](src/programs/limits_command.cpp) | **M6.** The command line's half of the machine limits: which flags may name a `satellite_config.ini`, and starting the limits, the pool and the watchdog before any arm runs. The same seam `cache_command` and `check_command` are on — `machine_limits/` takes a path and knows nothing about `argv`. |
| [src/programs/source_file.hpp](src/programs/source_file.hpp) · [.cpp](src/programs/source_file.cpp) | Getting a source file's bytes off the disk **unchanged** — DESIGN §5.3's rule, one layer down. Split from `main.cpp` by subject when that file reached 396 lines. **One function under a hundred lines of comment**, and the comment is the file: `<fstream>` costs 1.4 MB statically and buys nothing back, while its *good* idiom is 2.5× faster than a naive `fread` loop — for a reason that turns out not to be `fstream` at all. Both measurements are there, including the one that made the first answer look wrong. |
| [src/programs/cpu_level.cpp](src/programs/cpu_level.cpp) | `satl-cpu-level`: prints `haswell` or `baseline`. Compiled at the baseline on purpose — it runs before anything is known about the machine. |
| [src/programs/window.cpp](src/programs/window.cpp) | `satl-term`: the command line, the `GtkApplication`, and the window. Its title and size are the same string and two numbers `satellite.window.console.new` takes. |
| [src/programs/terminal.cpp](src/programs/terminal.cpp) | The VTE widget and the `satl` it spawns into a PTY. Holds the exit policy — the clean-exit arm is M1.5's and M22 deletes it. |
| [src/programs/terminal.hpp](src/programs/terminal.hpp) | One door to the above. Split from `window.cpp` by subject, not by line count. |
| [src/system_facts/version.hpp](src/system_facts/version.hpp) | The two version numbers and what a build records about itself, including both the compiler make invoked and the one that answered. |
| [src/satellite_number/bignum.hpp](src/satellite_number/bignum.hpp) | **M8.** The umbrella header and the only door a consumer opens. Carries the `sizeof` measurement PLAN §6.1 called *"the one number that could make this port not fit"* — 32 bytes with the sign flat, 40 with the magnitude cut out as its own type, and the second would put a `Value` at 48. |
| [src/satellite_number/bignum_bigint.hpp](src/satellite_number/bignum_bigint.hpp) | `BigInt` — arbitrary-precision **unsigned**, base 10⁹, little-endian limbs. **The one place `satellite_number` reaches outside itself**: it includes `satellite_random/random.hpp` for `Bits32` and `MAX_RANDOM_DIGITS` rather than carrying v1's copies, which is a correction to PLAN §6.1's *"ports alone"*. |
| [src/satellite_number/bignum_number.hpp](src/satellite_number/bignum_number.hpp) | `class Number`, and **where M8's argument lives**: DESIGN §8.1's explicit `bool positive_`, what it cost (a compare in `add`), what it paid for (no `LLONG_MIN` case, an opposite-sign add that cannot overflow, one more row of powers of ten), and the `static_assert` that keeps it at 32 bytes. |
| [src/satellite_number/bignum_internal.hpp](src/satellite_number/bignum_internal.hpp) | Private to the module. `POW10` to 10¹⁹ — nineteen rows and not v1's eighteen, because an unsigned significand reaches one power further — and `scale_u64`. |
| [src/satellite_number/limbs.cpp](src/satellite_number/limbs.cpp) | The base-10⁹ core. Came across **unchanged apart from two things**, both named in the file: `scale_ll` became `scale_u64`, and `fits_ll`/`to_ll` became `fits_u64`/`to_u64` — the one signedness opinion an unsigned magnitude class ever had. |
| [src/satellite_number/number_core.cpp](src/satellite_number/number_core.cpp) | Construction, the small form, parsing, normalisation. **Where "there is no negative zero" is enforced** — `make()` answers a zero magnitude with a default `Number`, so `-0` is a state the representation cannot reach rather than one a comparison repairs. |
| [src/satellite_number/number_query.cpp](src/satellite_number/number_query.cpp) | Sign and integrality questions, rounding, comparison, and the three M8 **added**: `max` `1 6 4 2`, `min` `1 6 4 3` and `clamp` `1 6 4 5`, which the first satellite had none of. By value and not by reference, because a reference-returning `max` is the standard library's oldest dangling temporary. |
| [src/satellite_number/number_arith.cpp](src/satellite_number/number_arith.cpp) | add, sub, mul, divide, modulo. Every one of them decides a sign, so every one changed shape at M8 without changing what it computes. Carries v1's recorded division bug — *"a non-zero remainder does not mean the division fails to terminate"* — and its two fixtures. |
| [src/satellite_number/render.cpp](src/satellite_number/render.cpp) | Print the **value**, never N significant digits. The notation boundary restated as *padding outnumbering information*, which is why 30! prints whole and 1e308 does not. |
| [src/satellite_number/random.cpp](src/satellite_number/random.cpp) | The uniform draw at arbitrary precision — the bignum half of DESIGN §11, and **what gives `satellite_random` a consumer**. Rejection and never a fold; `tests/number_test/draw.cpp` measures what the fold would cost. |
| [src/satellite_bits/bits.hpp](src/satellite_bits/bits.hpp) · [.cpp](src/satellite_bits/bits.cpp) | **M19.5, both radices.** DESIGN §8.5's types: a run of bits where **the width is part of the value**, so `b0010` and `b10` are two values and nothing in the module trims a leading zero — that one rule is what the whole file is arranged around, and it is the only clause a representation can break silently. One `std::vector<bool>`, the bit-packed specialisation, so the author's "just use bools" costs exactly one bit per bit: 125,000 bytes for a million against 40,000,000 for a `Value` apiece (measured, and the header carries the table). **`HexRun` HOLDS a `BitRun` and does not derive from one** — four bits to the digit, so the round trip is exact — and the composition is load-bearing: under inheritance a `shared_ptr<const HexRun>` would convert implicitly to `shared_ptr<const BitRun>` and a hex value could be assigned into the binary arm and silently become one. Hex's width is therefore always a multiple of four, which is what makes `to_hex()` partial and hex's expansion total. Nine free functions and no methods — the satellite rows live one module over, because this file must not know what a Machine is. |
| [src/satellite_float/satellite_float.hpp](src/satellite_float/satellite_float.hpp) | **M15.** DESIGN §8.6's type: `(positive, L, R)`, the three invariants held by construction, and the rounding rule — **half away from zero**, chosen at M15 by delegation, ratifying what `Number::divide` and `round()` had taken already. The sign is INHERITED: the four operations compose over the exact join, so M8's signed arithmetic runs the case analysis and it is written once. |
| [src/satellite_float/float_internal.hpp](src/satellite_float/float_internal.hpp) · [float_value.cpp](src/satellite_float/float_value.cpp) | **M15.** One digit-level decomposition serving places(), rendering, truncation and the rounding step, so the four cannot disagree about where the point is — through Number's PUBLIC surface only, keeping PLAN §6.1's "internally closed" true of the module one door down; the day a profile objects, the fix is a digit window beside `small_parts()`, not a friend here. |
| [src/satellite_float/float_arith.cpp](src/satellite_float/float_arith.cpp) | **M15.** The four operations and the precision rule — max of the operands', floored at `float_digits`. `+`/`-` exact and taking no digit count; `*` an exact product rounded once; `/` **correctly rounded**, its last place settled by exact cross-multiplication so a tie is a proved tie, not a guard-digit pattern trusted at sight. |
| [src/satellite_float/float_power.cpp](src/satellite_float/float_power.cpp) | **M15.** The class-3 answers. sqrt is Newton then an exact-squaring settle (proved, like divide); fractional power peels the exponent a decimal digit at a time through 10th roots and is **guard-argued** (`kGuardPlaces`), the one place the last digit is argued rather than proved — the file says so, and widening the guard is the whole fix. Integer exponents are exact, driven by the exponent's own bits. |
| [src/satellite_random/random.hpp](src/satellite_random/random.hpp) | `satellite.random`: the three tiers, the `Bits32` seam, `MAX_RANDOM_DIGITS`, and the spin. Names no PCG type, so nothing above it includes an Apache-2.0 header. |
| [src/satellite_random/random.cpp](src/satellite_random/random.cpp) | The only translation unit that names a PCG entity — and **linked into `satl` since M13**, which ended the linked-into-nothing exception 040-sources.mk documented from M2 to M12. The seed comes from the kernel through `getrandom(2)`, batched, and cannot abort — never `std::random_device`, whose default token is RDRAND and whose failure mode is a throw with no catch (M13 done-when clause 9; the receipt is in the file). |
| [src/satellite_random/tiers.hpp](src/satellite_random/tiers.hpp) · [.cpp](src/satellite_random/tiers.cpp) | **M13.** The twelve shapes' arithmetic in two layers: `draw_*`, one answer each from whatever `Bits32` is handed in — the layer tests drive with a stub, so a hundred-run distribution check costs microseconds — and `tier_*`, the same draws behind DESIGN §11's window against one generator seeded once. The throwaway is whole answers of the shape being asked for. |
| [src/satellite_random/handlers.hpp](src/satellite_random/handlers.hpp) · [.cpp](src/satellite_random/handlers.cpp) | **M13.** The dice's join, on the console's model: twelve rows, `1 7 1`–`1 7 12`, with the three `.range` spellings free because M2 built them as aliases. Three rows are refusals by design — the zero-argument shapes hold S0901's author-text — and every argument check runs **before** any spin, random.hpp's lazy rule enforced at the door. |
| [src/satellite_time/time.hpp](src/satellite_time/time.hpp) · [.cpp](src/satellite_time/time.cpp) | **M13.** The clock and the wait, under DESIGN §13's settled split: the **value** is `system_clock` on the Unix epoch, int64 nanoseconds; the **timer** is `steady_clock` — `clock_nanosleep` against an absolute `CLOCK_MONOTONIC` deadline, so an EINTR resume has no drift and Ctrl-C wakes a sleep the moment it lands. |
| [src/satellite_time/handlers.hpp](src/satellite_time/handlers.hpp) · [.cpp](src/satellite_time/handlers.cpp) | **M13.** Two rows, not four, and the gap dated: `now` `1 9 1` (a fact, asked fresh — the module-constant road, no parentheses anywhere) and `sleep(n)` `1 9 3` (n IS seconds, whole or fractional). `new` `1 9 2` is M29's, designed beside the date it constructs from. |
| [src/satellite_file/file_handle.hpp](src/satellite_file/file_handle.hpp) · [.cpp](src/satellite_file/file_handle.cpp) | **M19.** What a `satellite.variable.file` IS — DESIGN §8's "handle, reference type", and the **only arm of `Value` that is not `const` behind its handle**: two names for one open file are one open file, so `close` through either closes it for both. The four mode words in one table, and the read cursor that is the language's rather than the kernel's. |
| [src/satellite_file/file_internal.hpp](src/satellite_file/file_internal.hpp) | What the module's three sources share: the argument checks, the one constructor, and the direction and closed-handle refusals — so each of S12xx's sentences has exactly one site. |
| [src/satellite_file/handlers.hpp](src/satellite_file/handlers.hpp) · [.cpp](src/satellite_file/handlers.cpp) | **M19.** `satellite.file` `1 8`'s five rows, and **`satellite.system.delete` `1 22 1`, which is installed from here and spelled under another parent** — v1's argument is that unlink acts on a NAME so one verb covers a file and an empty directory, and its second argument shape is an open handle, so this is the only module that can read it. `1 6 2 1` is deliberately not installed. |
| [src/satellite_file/file_methods.cpp](src/satellite_file/file_methods.cpp) | **M19.** The handle's writing and lifecycle rows — `write_line(s)` and `write(x)` (the newline is the only difference and it is why there are two verbs), `open`, `close`, `exists`, `ok`, `path`, `error`. None of them `mutates` in the table's sense, which is the reference type showing through. |
| [src/satellite_file/file_reading.cpp](src/satellite_file/file_reading.cpp) | **M19.** `read_line` `1 6 2 3`, `read_all` `1 6 2 5`, and the cursor they share — the only thing M19 adds to v1's design. Every read is a **`pread` at the handle's own offset**, because "read_append" is `O_RDWR \| O_APPEND` and a write moves the shared offset to the end of the file. |
| [src/satellite_directory/handlers.hpp](src/satellite_directory/handlers.hpp) · [.cpp](src/satellite_directory/handlers.cpp) | **M19.** `satellite.directory`'s five rows. Not `satellite.terminal`: a headless run still has a working directory. `.list` is **the one failure in the module that is an error and not a value**, because the empty list already means an empty directory. **No create verb, and `1 18 6` is free** — a named hole rather than a discovered one. |
| [src/satellite_words/words.def](src/satellite_words/words.def) | **The numbering, as data** — 269 nodes since M19, 10 aliases, and five small lists beside the nodes: `SAT_PLACE` (M14, `input(prompt, target)`'s out parameter), `SAT_BUILT` and `SAT_TOPIC` (M18), and **`SAT_OPTIONS` (M19, the words WORD_NUMBERS §1.5's fold applies to — one row, `sort`, and it replaced a guess from spellings that was wrong about two words out of the three it fired on)**. A transcription of WORD_NUMBERS §2.2 and nothing else. A node's number is its **position** among its parent's children and is not a column. The one file exempt from PLAN §3's line ceiling. |
| [src/satellite_words/words.hpp](src/satellite_words/words.hpp) | The umbrella: one door over the seven parts below, in an order that compiles. Include this and you get everything. |
| [src/satellite_words/words_nodes.hpp](src/satellite_words/words_nodes.hpp) | `NodeId`, `PathId`, the node table, the aliases, and how a row's text splits into a word and a call shape. |
| [src/satellite_words/words_numbers.hpp](src/satellite_words/words_numbers.hpp) | The numbers, computed from file order at compile time, plus `number_text` and `path_text` — which is what reproduces §2.2's path column character for character. |
| [src/satellite_words/words_spellings.hpp](src/satellite_words/words_spellings.hpp) | **The spelling interner** — many nodes, one string (DESIGN §4.4) — and every node's children as a first/next pair, because a node's children are not contiguous in a depth-first file. |
| [src/satellite_words/words_walk.hpp](src/satellite_words/words_walk.hpp) | `walk()`: a path read against the trie. A failed walk returns which segment failed and **which node it failed under**, which is what DESIGN §4.6's "did you mean" needs and what M5 will be built from. |
| [src/satellite_words/words_invariants.hpp](src/satellite_words/words_invariants.hpp) | The `static_assert`s, in a header so every consumer inherits them. Says which of PLAN M2's four properties are enforced here and which the encoding already made unrepresentable. |
| [src/satellite_words/words_digest.hpp](src/satellite_words/words_digest.hpp) | The numbering's identity as one 64-bit number, for a `.satc` header. Over the **numbering** and not the file's bytes — SATC §6 asked and this answers. |
| [src/satellite_words/words_runtime.hpp](src/satellite_words/words_runtime.hpp) | The user's half: the live child counter, and names numbered as they are met. PLAN §8.1's second table, and the half no `static_assert` can reach. |
| [src/satellite_words/dump.hpp](src/satellite_words/dump.hpp) · [dump.cpp](src/satellite_words/dump.cpp) | `satl --words`. The **registry's consumer, in the milestone that wrote it** — which the first satellite did not have for three commits, and four defects accumulated in that window. The only part of the module that prints. |
| [src/satellite_string/satellite_string.hpp](src/satellite_string/satellite_string.hpp) | **The language's alphabet** (DESIGN §5): a 16-bit code table where letters, digits and punctuation are ranges, everything unassigned round-trips through a raw area, and codes 95–100 are *live values*. `encode_raw` is the one the lexer must use; `encode` expands escapes and would rewrite a program's text. |
| [src/satellite_string/satellite_string.cpp](src/satellite_string/satellite_string.cpp) | The two directions, plus the `static_assert` that no escape name is a prefix of a later one — v1 held that by hand-ordering the table. **Two `decode()`s over one walk**: the lexer's, where the six live codes are placeholders, and M9's, where the caller hands in what they are. |
| [src/satellite_value/value.hpp](src/satellite_value/value.hpp) · [.cpp](src/satellite_value/value.cpp) | **M9.** DESIGN §8's value, at §8.2's forty bytes with a `static_assert` on it — **twelve arms since M19.5**, append-only, and nothing wider than a `Number` goes inline, which is why §8.6's float cannot and why `Flo`, `Lst`, `Map`, `Fil`, `Bin` and `Hex` all arrive behind sixteen-byte handles. The assert has not moved once across six appends, which is the whole point of measuring each one against `Str` rather than against the budget. Plus the three things every arm shares: what a value is CALLED in a sentence, whether it is true (only a bool is — there is no truthiness), and whether two are equal. |
| [src/satellite_value/render.hpp](src/satellite_value/render.hpp) · [.cpp](src/satellite_value/render.cpp) | **M9.** A value as characters, and **the one place in the tree that answers DESIGN §5's six live codes** — PLAN §6.1's "six calls", made here and not in the alphabet. A string with no live code in it never asks the machine anything. |
| [src/lexical_analyzer/lexer.hpp](src/lexical_analyzer/lexer.hpp) | `Token` and `TokenKind`. The field to read first is `spelling`: it is a **`SpellingId` and not a `PathId`**, and the header carries the argument for why, because the two are the same 32 bits and an earlier draft got it wrong and compiled. |
| [src/lexical_analyzer/lexer_chars.hpp](src/lexical_analyzer/lexer_chars.hpp) | What a character *is*, written against the code table and never against `ctype` — DESIGN §5.2, where the failure is one Error token per space. Underscore is where the table and the lexer disagree on purpose (§5.1). |
| [src/lexical_analyzer/lexer.cpp](src/lexical_analyzer/lexer.cpp) | One pass, one character of lookahead, no backtracking — which DESIGN §5.5 buys by refusing `<<` and `>>`. Also `intern_word()`, **the lexer's half of the spelling table**: the six single-segment aliases, and the dot filter that leaves the three path rewrites to M13. |
| [src/lexical_analyzer/dump.hpp](src/lexical_analyzer/dump.hpp) · [dump.cpp](src/lexical_analyzer/dump.cpp) | `satl --tokens`. **The lexer's consumer, in the milestone that wrote it** — the same rule M2 made for the registry, one milestone on. The only part of the module that prints. |

| [src/error_reporter/errors.def](src/error_reporter/errors.def) | **Every message satl can say, as data** — 93 rows since M14, in blocks by who raises them, with `tests/reporter_test/codes.cpp` holding the count and the per-milestone history this row stopped carrying at 35. A code's number is a **column** here where a word's is a **position** in `words.def`, and the file argues why that is deliberate. The second `.def` in the tree and the second thing `HDRS` lists that is not a header. |
| [src/error_reporter/codes.hpp](src/error_reporter/codes.hpp) | The enum, the sentences, the severities and the arities — five expansions of one list — plus the invariants. **The arity is counted from the sentence** and never declared, so there is no column to go stale; `holes_are_dense` is the assert that matters. Says out loud which of DESIGN §9's requirements no assert can see. |
| [src/error_reporter/report.hpp](src/error_reporter/report.hpp) | DESIGN §9's shape, built whole: `Span`, `Note`, `FrameRef`, `Diagnostic`. **`FrameRef` has no producer until M9 and is here anyway** — the header argues that building three of §9's four fields is the retrofit this milestone exists to prevent. `make<Code>` is a template so the arity is a compile error at the call site. |
| [src/error_reporter/report.cpp](src/error_reporter/report.cpp) | **The one place a diagnostic becomes characters.** The first satellite had three, and its own header admits they "render nothing alike". `path:line:column:` because that is the format every editor already parses. A tab is one space, a UTF-8 continuation byte is not a column, and the caret is clamped to the line — v1's hardest-won line in this file, inherited rather than rediscovered. |
| [src/error_reporter/suggest.hpp](src/error_reporter/suggest.hpp) · [.cpp](src/error_reporter/suggest.cpp) | DESIGN §4.6, and the search is **one node's children** — 254 paths hold something within two edits of nearly any word. **Optimal string alignment and not plain Levenshtein**: a transposition is one edit, and without that arm `wihle` never suggests `while`. Here rather than in `satellite_words/` so the registry stays constexpr data and pure functions. |
| [src/error_reporter/dump.hpp](src/error_reporter/dump.hpp) · [dump.cpp](src/error_reporter/dump.cpp) | `satl --errors`. **The registry's consumer, in the milestone that wrote it** — and the strongest case of that rule yet, because a code exists to be looked up. The only part of the module that prints. |
| [src/programs/check_command.hpp](src/programs/check_command.hpp) · [.cpp](src/programs/check_command.cpp) | `satl --check`, and the two helpers every arm that names a file shares. The first consumer whose whole answer is the **diagnostics**: nothing on stdout ever, and the exit status is what a script reads. |

| [src/satellite_cache/cache.hpp](src/satellite_cache/cache.hpp) | The `.satc` module's one door: `Source`, the writer's three entry points, `Reading` and its four answers, and `Save`. The header to read first for **why a `.satc` is a cache and not a bytecode VM** — SATC §7's line, which is that nothing in the file names a handler, an instruction or an evaluation order. |
| [src/satellite_cache/paths.hpp](src/satellite_cache/paths.hpp) · [.cpp](src/satellite_cache/paths.cpp) | SATC §5.1's first four steps — collapse aliases, classify, absorb, slot by arity — over the tree rather than over a string, which is why it is not `words::walk()`. **`kPathMark` is here**: `#` is what stops `1.5` the path being read as `1.5` the float. Carries the two defects the writer's consumer caught, both of which produced files that still parsed. |
| [src/satellite_cache/write_internal.hpp](src/satellite_cache/write_internal.hpp) | The `Writer`, split `parser_internal`-style so each part is a translation unit about one thing. **Everything that turns a path into digits is in `write.cpp`** — a second place that formatted a path id would be a second place the numbering's spelling lives. |
| [src/satellite_cache/write.cpp](src/satellite_cache/write.cpp) | The entry points, the header line, the comment column, and every function that spells a number. **This is `unparse.cpp` with the paths substituted**, said out loud because that is the whole safety argument for having written the printer twice — and the drift is answered by a test rather than by structure. |
| [src/satellite_cache/write_declarations.cpp](src/satellite_cache/write_declarations.cpp) | DESIGN §6's `top_level` and its blocks. Every form here is one the parser gave a node of its own, so none of them reaches the chain matcher and each finds its number by naming the path it always is. |
| [src/abstract_syntax_tree/unparse.cpp](src/abstract_syntax_tree/unparse.cpp) · [unparse_internal.hpp](src/abstract_syntax_tree/unparse_internal.hpp) | **M4, rebuilt at M8.5.** The printer's stack and its two entry points. It keeps its own stack on the heap (DESIGN §7.5) and drains PIECES into one string left to right, so a program's nesting has no depth of its own — `--unparse` died at 19,000 nested brackets before this. |
| [src/abstract_syntax_tree/unparse_declarations.cpp](src/abstract_syntax_tree/unparse_declarations.cpp) · [unparse_expressions.cpp](src/abstract_syntax_tree/unparse_expressions.cpp) | DESIGN §6's three levels, which is the seam `satellite_cache/write_*.cpp` is already cut on — **the two printers are twins in shape as well as in walk**, and this file split at M8.5 for the reason PLAN §3 gives: keeping its own stack took one file to 452 lines of code. |
| [src/satellite_cache/write_expressions.cpp](src/satellite_cache/write_expressions.cpp) | §6's `expression`, and **SATC §3.1's one real decision**: a path becomes a number, a selector stays sugar, and the brackets come back from precedence rather than from memory. |
| [src/satellite_cache/read.cpp](src/satellite_cache/read.cpp) | SATC §4 steps 1 and 2, which is the **order** things are checked in: the format line first because it says what the other two mean, then both of them compared as whole lines against the ones this build would write. Also the milestone's plain-words note, which is the first sentence in this language written for a user rather than for a compiler — and the local numbering that means **a miss costs exactly one walk and leaves nothing behind**. |
| [src/satellite_cache/unnumber.cpp](src/satellite_cache/unnumber.cpp) | The mirror of `paths.cpp` and `write.cpp`: `#1.5.1` back into `satellite.console.display`, and the text handed to **the same lexer and parser a source goes through** — a second grammar for `.satc` would be a second place the language is defined. A text pass, so it skips string literals and comments; a `#` inside a string is five characters and not a path. |
| [src/satellite_cache/save.cpp](src/satellite_cache/save.cpp) | SATC §5: tmp, `fsync`, `rename`, and the thread it happens on. **Joined and not detached** — a detached writer is a thread the process exits out from under, so the cache would never actually be there. A failed write is silent, which is the section's own rule. |
| [src/satellite_cache/file.cpp](src/satellite_cache/file.cpp) | Where a `.satc` lives — `$HOME/.satl/cache`, named with a digest of the source's **absolute** path — and a source's identity for the header. The mtime is whole seconds on purpose; the comment says which filesystems that is for. |
| [src/programs/cache_command.hpp](src/programs/cache_command.hpp) · [.cpp](src/programs/cache_command.cpp) | `satl --satc`. **The only flag with a file of its own**, because it is the only one that is a loop rather than a print: read the cache if it hits, walk and write if it misses, print the `.satc` either way and say on stderr which happened. |
| [src/programs/dump_commands.hpp](src/programs/dump_commands.hpp) · [.cpp](src/programs/dump_commands.cpp) | **M7**, and the half of `main.cpp`'s seam MILESTONES/M6.md §6.1 named and declined. `--words` and `--errors`: a registry dump takes **no operand or a key into itself**, answers out of tables compiled into the binary, opens nothing, and cannot fail about the user's program. The header says what the two subjects are, once. |
| [src/programs/file_commands.hpp](src/programs/file_commands.hpp) · [.cpp](src/programs/file_commands.cpp) | **M7**, the other half. `--tokens` and `--unparse`: a file arm takes a **path** and has a status about what was in it. The rule they share is the one that cost a day once — the dump goes to **stdout even when the program is malformed**, because the operand is a file and a bad token is not a bad command line. |
| [src/satellite_console/console.hpp](src/satellite_console/console.hpp) · [.cpp](src/satellite_console/console.cpp) | **M10, and the milestone.** DESIGN §10.1's console: whole strings into a locked queue, one printer thread out. **The printer creates its own thread and takes none from M6** — the author, 2026-09-02, and `pool.hpp` carries the rule that settles it: that pool takes work that FINISHES. The barrier is a **prefix of the shutdown** rather than a second mechanism — drain, flush, stop, join — so `satellite.console.input` takes step 1 at M14 and `satellite.return` from `satellite.main` takes all four. The queue is **unbounded on purpose**: a bound is a threshold nobody chose in the most-called path in the language. It links nothing but the standard library and does not know what a `Value` is. |
| [src/satellite_console/reader.hpp](src/satellite_console/reader.hpp) · [.cpp](src/satellite_console/reader.cpp) | **M14.** DESIGN §10.1's printer with the polarity reversed: the dedicated thread blocks on stdin and pushes whole lines in, the program asks the queue and never touches the terminal. It parks in `poll()` on stdin AND a control pipe — never bare in `read(2)` — which answers Ctrl-C delivery (the handler writes one byte, whatever thread took the signal) and clean shutdown (a byte means stop, the join is the console's fifth step) with one mechanism, and is what M22's raw-mode prompt will use to park it. **One consumer of stdin at any instant, and it is this thread.** |
| [src/satellite_console/handlers.hpp](src/satellite_console/handlers.hpp) · [.cpp](src/satellite_console/handlers.cpp) | **M10, finished at M14.** The join, and the only file in the tree that includes both `evaluator/dispatch.hpp` and a console — nine rows now, the whole namespace: `display` `1 5 1` (any value, rendered through `satellite_value/render.hpp`), the three `input` shapes (three answers, not one empty string — a line, S1001's end, or nothing-then-the-boundary for Ctrl-C), `typed()` (a line or nothing, immediately), the two live facts, and `clear()`/`home()` as queued escapes — `clear()` homes, and `home()` alone is the flicker-free repaint. `1 5 4`'s arity is ONE: the place never reaches its handler, op_place owns the write. |
| [src/satellite_scalars/handlers.hpp](src/satellite_scalars/handlers.hpp) · [.cpp](src/satellite_scalars/handlers.cpp) | **M11, and the milestone.** The scalars' join, on the console's model: 33 rows into `handlers[path_id]` (41 since M19.5's four) — `satellite.bool.true` `1 17 2` and `.false` `1 17 1` (DESIGN §6.1's module constants, each "a handlers entry and nothing else"), and the install of the two method files below. Also the three checks every method makes first: the receiver re-asked at run time (a declaration is not a value — the slot may hold nothing, S0714, or a value of another type, S0713), and a position as a whole number counted **from 0**. |
| [src/satellite_scalars/string_methods.cpp](src/satellite_scalars/string_methods.cpp) | **M11.** The sixteen `satellite.variable.string` methods, `1 6 1 1`–`1 6 1 16` — QUAD-sourced, so the semantics were decided here and MILESTONES/M11.md §2 carries every decision. Where another language hands back a sentinel these refuse (S0715, S0716, S0719): a refusal can loosen at M12, a `-1` is forever. `split` refuses naming M16, whose list it answers with. `append` and `clear` are the two mutating rows. |
| [src/satellite_scalars/bits_methods.cpp](src/satellite_scalars/bits_methods.cpp) | **M19.5.** The six `satellite.variable.binary` rows, `1 6 5 1`–`1 6 5 6`. `to_number()` is what the bits are WORTH and `as_number()` is the digits read as decimal — one word apart and a thousand apart, **reversed once during the milestone** and settled on the ground that `to_` is already this language's conversion verb and that the surprising answer must not sit behind the expected name. `width()` is the one row that reads §8.5's width, and it is what makes both conversions' loss recoverable. `digits()` always equals `width()` here and earns its row on the other type. `to_hex()` **refuses a width that is not a multiple of four**, `write(x)`'s multiple-of-eight refusal one step in and settled by the same argument: padding invents a bit the program never wrote. None mutates. **No operators**: a method needs a `words.def` row and `+`, `!!`, `[` and the shifts do not, which is why this milestone could decide all of them and build none without minting anything. |
| [src/satellite_scalars/hex_methods.cpp](src/satellite_scalars/hex_methods.cpp) | **M19.5's second half.** The five `satellite.variable.hex` rows, `1 6 11 1`–`1 6 11 5`. **Four of the five route through the bits and therefore call the same `satellite_bits` function a binary row calls**, which is the correctness argument rather than a saving: `x00FF` and `b0000000011111111` are one run, and a reader who converted between them and got two different numbers would have found a bug in the type's premise. `width()` counts BITS (16, not 4) so that `write(x)`'s multiple-of-eight rule reads the same sentence on both radices; `digits()` is the question that costs nothing to keep beside it. **FIVE AND NOT SIX: there is no `as_number()`.** Binary has one at `1 6 5 4` and it means "read these characters as a decimal", which works there only because `0` and `1` are decimal digits too; hex has no decimal reading at all, so the version built on 2026-09-09 read the BIT EXPANSION's characters and answered 11111111 — and the author dropped it the same day on reading that answer. A separate file from `bits_methods.cpp` because the line rule would otherwise have been crossed on the day the second radix arrived. |
| [src/satellite_scalars/number_methods.cpp](src/satellite_scalars/number_methods.cpp) | **M11, finished at M15**, over paths M8's ledger line owns — M8 built everything they answer *with*; the row itself could not exist before M9's table. Twelve ran from M11 (`shift_left`/`shift_right` are DESIGN §5.5's exact × 2ⁿ and ÷ 2ⁿ; `modulus` shares S0601 with `%`) and the three that refused naming M15's rounding rule — `power`, `truncate`, `sqrt` — answer since it was chosen, the first two with a FLOAT (§8.6's result-type argument). None mutates — `n = n.round()` is how a program keeps one, the same shape `+` has. |
| [src/satellite_system/handlers.hpp](src/satellite_system/handlers.hpp) · [.cpp](src/satellite_system/handlers.cpp) | **M15, and `satellite.system.persist` since M22.** `satellite.library.system`'s four dials behind the tables — the reads `1 14 2 1`–`1 14 2 4` as module constants answering the running machine's Policy, and their writes as the retune's first rows. A leaf module the way the console is one, because it must see both sides of a seam the evaluator keeps: `min_free_mb` is the watchdog's, read from `machine_limits` (unset answers **nothing** — an unwatched floor is not a `0`) and its write refused with the reason. `tests/eval_test` proves the mechanism with rows of its own and links none of this. **The M22 rows are a different node and the first child of `satellite.system` `1 22` that anything implements** — thirty numbered paths have sat under it since the transcription and no milestone reached one. `persist()` `1 22 7` answers whether a finished program's variables are kept and `persist(x)` `1 22 8` sets it, two rows because §1.3's arity-is-identity and because the same parent already carries `threshold()`/`threshold(n)`. It is **the one process-wide thing in the file**, which the header calls out as a departure: the four dials belong to a Machine because a program obeys them, and this belongs to the prompt, which outlives every Machine it makes. |
| [src/satellite_help/help.def](src/satellite_help/help.def) | **M18. Generated, and it is the third `.def` registry in the tree.** One row per node `words.def` numbers — 274 of them since M19.5, in the same order — carrying the entry's prose, its worked line, and the node it is asked about **under**, so that `satellite.help(satellite.console.input)` answers for `input()`, `input(prompt)` and `input(prompt, target)` together. Written by `help_lines/gen.py` in the same run that writes [HELP.md](HELP.md), which is the whole point: a hand-maintained C++ copy of 269 entries would be the second document DESIGN §4.6 exists to remove, with a compiler in front of it. **Never edited in place.** |
| [src/satellite_help/help_text.hpp](src/satellite_help/help_text.hpp) | **M18.** The entries as a table indexed by `PathId`, and **the join asserted at compile time**: one row per node, in `words.def` order, every group head its own head. A word appended to the registry with no entry written for it is a build error naming the node rather than a blank page somebody finds in six months — which is DESIGN §4.6's "help cannot drift from what exists", mechanised. |
| [src/satellite_help/built.hpp](src/satellite_help/built.hpp) · [.cpp](src/satellite_help/built.cpp) | **M18, and the milestone's real content.** What "built" means: a **handler** row, an **assigner** row, a **front-end word** (`words.def`'s fourth list, data because a set inferred from which spellings a parser compares against drifts silently), or **anything with one of those underneath it**, which is derived in one backward pass and must not be listed. Of the seven paths hello world is written in, exactly one is a handler row — so `handlers[path_id]` alone would have named `display` and stayed silent about the rest. **Computed fresh on every ask**: the table is filled by each module's `install_handlers()` at the top of a run, so a remembered answer would be right or wrong depending on which program asked first. |
| [src/satellite_help/render.hpp](src/satellite_help/render.hpp) · [.cpp](src/satellite_help/render.cpp) | **M18.** The walk — **one walk at three depths and no root case**. `satellite.help`, `satellite.help()` and `satellite.help(satellite)` are the same program, and the topics a bare ask prints are the built *children of `satellite`*, which is the same clause that prints display and input under `satellite.console`. A node is answered for together with the shapes written beside it; what is not built is never named in a listing, and a built word that is nevertheless **unlisted** (`satellite.returns`, which is optional and is not how a capsule is written here) answers when asked about and is not offered. |
| [src/satellite_help/handlers.hpp](src/satellite_help/handlers.hpp) · [.cpp](src/satellite_help/handlers.cpp) | **M18.** The three rows — `1 19`, `1 19 0`, `1 19 1` — and a **join** the way `satellite_console/handlers.cpp` is one: the walk knows nothing about a console and the console knows nothing about what is worth printing. **Help is a handler row like any other, which is not a detail**: `built()` names a node when it has one, so help installing itself into the table it reads is what makes `satellite.help` appear among the topics it prints. Not installed by `--call`, which starts no printer — the same boundary `display` has there. |
| [src/satellite_prompt/raw_mode.hpp](src/satellite_prompt/raw_mode.hpp) · [.cpp](src/satellite_prompt/raw_mode.cpp) | **M22.** Raw mode, and the four ways back out of it — destructor, `atexit`, the emergency exit hook, and a signal-safe `restore_terminal()`. **The only registrar of M6's hook**, which had been built with no caller since 2026-08-30 because until there is a prompt nothing has put the terminal into a state a `_exit(EXIT_LIMIT)` has to undo. ISIG off is DESIGN §10.2: Ctrl-C at the prompt is the byte `0x03` and no signal is raised. **OPOST stays ON**, which every "enter raw mode" recipe turns off — the Console's printer thread writes `\n` and expects CR-LF, and without it a program's output staircases. Entered **per line**, so a program M14's `input` runs in finds the terminal the way every other run of it does. |
| [src/satellite_prompt/keys.hpp](src/satellite_prompt/keys.hpp) · [.cpp](src/satellite_prompt/keys.cpp) | **M22.** Bytes to keys, **one byte at a time** — an arrow key is `ESC [ A` and nothing promises those arrive from one `read()`, so code that sees ESC and then reads two more blocks forever on a slow link holding half a key. UTF-8 is counted rather than decoded: a multi-byte character arrives as ONE `Key::Char` with all its bytes, which is what makes a left arrow move one character. A tab becomes spaces here so nothing downstream ever holds one. |
| [src/satellite_prompt/history.hpp](src/satellite_prompt/history.hpp) · [.cpp](src/satellite_prompt/history.cpp) | **M22.** The lines already typed, **in memory for the life of one session** — a decision and not an unfinished stage: a file needs a path in the user's home, and `~/.satl` is the INSTALL directory. No line stored twice in a row. |
| [src/satellite_prompt/editor.hpp](src/satellite_prompt/editor.hpp) · [.cpp](src/satellite_prompt/editor.cpp) | **M22.** One line being typed: the bytes, the cursor, and browsing backwards. **Draws nothing and reads nothing** — which is what lets `tests/prompt_test` feed it `KeyEvent`s and assert on the buffer with no pty anywhere. A word boundary is a space and nothing cleverer, because a satellite path is one word and a word-left stopping at every dot would take five presses to cross a name. Browsing keeps the half-typed line, which is the part people notice when it is missing. |
| [src/satellite_prompt/render.hpp](src/satellite_prompt/render.hpp) · [.cpp](src/satellite_prompt/render.cpp) | **M22.** Drawing one line, wrapped across rows. **Writes to the descriptor directly and not through the Console**, which is a boundary rather than a shortcut: a queue is right for a program's output and wrong for a cursor, so `prompt.cpp` drains before drawing and the two never hold the descriptor at once. The width comes from `Console::width()` — PLAN M22's own instruction, M14 owns the ioctl. Remembers the tallest the line has been, because a wrapped line cannot be erased by one clear-to-end-of-line; and resolves the **deferred wrap**, which is the piece that looks like superstition and is not. |
| [src/satellite_prompt/line_reader.hpp](src/satellite_prompt/line_reader.hpp) · [.cpp](src/satellite_prompt/line_reader.cpp) | **M22.** Where the four above meet. **Reads cooked when there is no terminal**, which is the other half of the job rather than a fallback: it is how the prompt is tested without a human, and a reader that refused a pipe would make the prompt the one part of this tree with no automated coverage. `EINTR` is not the end of input — treating it as one is how a prompt ends a session because the window was resized. |
| [src/satellite_prompt/block.hpp](src/satellite_prompt/block.hpp) · [.cpp](src/satellite_prompt/block.cpp) | **M22.** What a typed line IS, decided **through the language's own lexer** — so a brace inside a string is not a brace, and a comment is not code, without a second implementation of DESIGN §5. Two questions in one pass: how deep the line leaves us, and whether it goes inside `main` or beside it (S0204's four words). And whether it **owes a body**, which was measured rather than designed: this language writes `{` on its own line, so a `for` header counts zero braces and was being run without its body — MILESTONES/M22.md §3.1. |
| [src/satellite_prompt/session.hpp](src/satellite_prompt/session.hpp) · [.cpp](src/satellite_prompt/session.cpp) | **M22, and the half PLAN's entry does not describe.** A typed line is not a program — S0204 — so the session wraps it in `satellite.capsule satellite.main()`, **no parameter and no return**, both measured against `example/scalars.satl` rather than copied from DESIGN §3. A top-level form is **kept**, so a capsule declared at the prompt is still there next line; a statement is not. Diagnostics are rebased onto the line the person typed, **counting** the accumulated forms rather than assuming three — a constant would drift by exactly as much as the user had declared. **And what a line declares survives it**: a held variable is written into the wrapper's PARAMETER list and its value handed to `Machine::call`, so nothing is re-run to restore it — replaying `s = satellite.console.input(...)` would ask the question again every line. It comes back out of `Machine::last_frame()`, the outermost frame's slots kept past its return. `Session::kept()` is M18's help store, filled: name, type as the node its declaration ends at, and value. |
| [src/satellite_prompt/prompt.hpp](src/satellite_prompt/prompt.hpp) · [.cpp](src/satellite_prompt/prompt.cpp) | **M22.** The loop: the banner (`opening_text()`, which `programs/opening.hpp` predicted a second caller for three milestones early, *"and the first satellite's mistake was to write that banner as a separate literal"*), the exit words (v1's four), the continuation, and `run <file>`. **`help` is refused and points at `satellite.help()`** rather than meaning it — DESIGN §1, and the author's own wording; the language owns one bare identifier (§7.7's `arguments`) and should not own a second by accident. |
| [src/evaluator/closure.hpp](src/evaluator/closure.hpp) | **M9.** The compiled form: an `Op` is a function pointer and four payload words in **24 bytes** — `ast.hpp`'s `Node` with the kind replaced by the address it would have jumped to, which is PLAN §2.3's "one indirect call with no tag test" kept rather than described. Plus the capsule table, the constants and the inline-cache cells. |
| [src/evaluator/machine.hpp](src/evaluator/machine.hpp) · [machine.cpp](src/evaluator/machine.cpp) | **M9, and the milestone.** Four heap vectors — work, values, slots, frames — carrying what a recursive evaluator would have put in C++ frames, so a program's recursion is bounded by memory and nothing else. The ceiling is **handed in and not read**, which is what lets `tests/eval_test` link no `machine_limits` and mean its 1,000,000-frame fixture. A call owns its frame from both ends and there is no sentinel op. **And since M22 it keeps one thing past a run**: `unwind()` copies the OUTERMOST frame's slots before giving the storage back, and `last_frame()` hands them over — six lines, so that a finished program's variables still exist for the prompt to use. Only the outermost, so a million frames copy nothing until the last returns; a copy and not a view, because the Machine outlives the call and `--call` runs a second capsule through the same one. |
| [src/evaluator/operations.cpp](src/evaluator/operations.cpp) · [operations_control.cpp](src/evaluator/operations_control.cpp) | **M9.** The machine's arms, split on the line DESIGN §6 already draws: an **expression** arm leaves exactly one value and a **statement** arm leaves none. No arm ever calls another — that is the rule the explicit stack exists to keep. `op_while` asks to be resumed at its own step 0, which is why a loop of a billion rounds is the same stack height as one of ten. **Since M11 the three control arms carry the statement boundary Ctrl-C lands on** — one relaxed load through Policy's pointer per statement or iteration, and a null listener short-circuits even that. |
| [src/evaluator/operations_dispatch.cpp](src/evaluator/operations_dispatch.cpp) | **M11.** The three dispatching arms over one core: `op_dispatch` (module calls and module-constant reads, no receiver anywhere), `op_method` and `op_method_global` (DESIGN §6.4's sugar with the receiver as argument 0 and the slot in the fourth operand). A row whose `mutates` column is set has its answer **written back to the receiver's slot by the op** — and the answer IS the receiver's new value, one value with two destinations. §6.4's "nowhere to write back" (S0717) and "no program may write the long spelling" (S0718) are both enforced here, by the only code that knows which rows bind a receiver. |
| [src/evaluator/evaluator_internal.hpp](src/evaluator/evaluator_internal.hpp) | **M9.** What the module's files share: the arms, so the compiler can take their addresses, and the `Compiler`. |
| [src/evaluator/compile.cpp](src/evaluator/compile.cpp) · [compile_expressions.cpp](src/evaluator/compile_expressions.cpp) · [compile_statements.cpp](src/evaluator/compile_statements.cpp) | **M9.** Arena AST to closure tree, in three passes whose ORDER is resolve's — every capsule numbered before any body is compiled, so a call to one declared further down the file resolves. **It keeps its own stack too**, the fifth walk in this tree to do so and the first born that way: DESIGN §7.5 has no exception for a pass that runs once. |
| [src/evaluator/dispatch.hpp](src/evaluator/dispatch.hpp) · [.cpp](src/evaluator/dispatch.cpp) | **M9.** `handlers[path_id]` — one vector and a bounds check, which is what DESIGN §4's numbering buys and what PLAN §1.1 counts the cost of not having. Carries DESIGN §6.4 q2's receiver-binding tag. **Empty until M10**, which installed the first row it has ever held: `satellite.console.display` `1 5 1`, from `satellite_console/handlers.cpp`. **And since M15 the write half beside it**: `Assigners`, a second small table making an assignment to a numbered language path a handler-shaped write — the retune, decided once in front of all four `1 14 2` rows; no row is S0724, S0721's write-side twin. |
| [src/evaluator/evaluate.hpp](src/evaluator/evaluate.hpp) · [.cpp](src/evaluator/evaluate.cpp) | **M9.** The one door over the module: compile a resolved program, and find a capsule by its path. |
| [src/evaluator/dump.hpp](src/evaluator/dump.hpp) · [.cpp](src/evaluator/dump.cpp) | **M9.** `satl --compile` — every op, every frame, and **every refusal**, which is the part a person reads: each piece of DESIGN §6's grammar this evaluator does not run yet, with the milestone that will build it. |
| [src/programs/evaluate_commands.hpp](src/programs/evaluate_commands.hpp) · [.cpp](src/programs/evaluate_commands.cpp) | **M9.** `satl --compile <file>` and `satl --call <file> <capsule> [n...]`. Two flags in one file because they share every line up to the point where one prints and the other runs. **Neither reads the `.satc`**, and that is a rule rather than an omission: a cached tree's spans index the cache, where line 11 of `hello_world.satl` is line 10, and an evaluator can raise a diagnostic at any moment during a run — so "parse it again from the source" is not available the way it is to resolve. |
| [src/programs/run_command.hpp](src/programs/run_command.hpp) · [.cpp](src/programs/run_command.cpp) | **M10.** `satl <file>` and `satl --run <file>` — the arm this binary has been pointing at since M1. Finds `satellite.main` `1 3` **by its path and not its spelling**, starts the console, runs the globals, calls `main`, and takes the four shutdown steps **before** printing any diagnostic, so a program's output cannot arrive under its own error message. A `main` that declares DESIGN §3's parameter is met with a caret under it and **M16** rather than with an argument count. **What a program answers is not an exit status** — the file says why in full, and the short version is that `opening.hpp`'s four numbers are satl's. |
| [src/programs/built_program.hpp](src/programs/built_program.hpp) · [.cpp](src/programs/built_program.cpp) | **M10.** Read, parse, resolve, compile — the four passes and their ORDER, in one place because three arms need them: `--compile`, `--call` and `satl file.satl`. Split out of `evaluate_commands.cpp` when the third arrived. Carries M9's rule about the `.satc`: **nothing on this road reads the cache**, because a cached tree's spans index the cache and an evaluator that has already printed cannot be run again from the source. |
| [src/programs/number_command.hpp](src/programs/number_command.hpp) · [.cpp](src/programs/number_command.cpp) | **M8.** `satl --number <a> <op> <b>`. **Three operands and not an expression**, because DESIGN §6 is already the grammar and a second one here would be a second place expression syntax is decided. Turns its own arguments into a one-line source so M5's carets work over a command line. |
| [src/programs/resolve_command.hpp](src/programs/resolve_command.hpp) · [.cpp](src/programs/resolve_command.cpp) | `satl --resolve`. The one arm with a decision no other has: a `.satc` is written from a tree that PARSED, and resolve is the first pass that can find something wrong in one — so a warm run whose program does not resolve is **thrown away and taken again from the source**, because the tree's spans index into the cache's words and a caret from them lands on the wrong line. |
| [src/name_resolver/resolve.hpp](src/name_resolver/resolve.hpp) | **M7**, and the one door over the module. DESIGN §7's frames and slots: three sentinels and not six, `Info`'s side table indexed by node, and `Origin` — walked, cached, bound or parsed. **Its depth bound was withdrawn on 2026-08-31, the day after it landed, and is gone**: `kMaxDepth` and S0501 were deleted that day and the walk stopped recursing at M8.5, so the language has no depth limit here in both halves of the sentence (DESIGN §7.5). `MILESTONES/M8.5.md`. |
| [src/name_resolver/resolve_internal.hpp](src/name_resolver/resolve_internal.hpp) | The `Resolver`, split the way `parser_internal.hpp` is: the passes, the scopes, the walk, the numbers. |
| [src/name_resolver/resolve.cpp](src/name_resolver/resolve.cpp) | §7.3's four passes **in order** — every capsule name, every spacesuit name (M26's, and a named hole), the top level, then each body. The order is why resolve is not in the parser: mutual recursion is unresolvable in single-pass recursive descent. Also the `stable_sort` that puts problems back in source order, which four passes do not produce. |
| [src/name_resolver/scopes.cpp](src/name_resolver/scopes.cpp) | §7.2's slots and §7.4's **fresh** one — a redeclaration rebinds rather than overwriting, because reusing the slot would leave every handle to the first instance pointing at the second. DESIGN §4.6 over the names in scope, which `errors::suggest()` cannot search: those names were met four milestones after the trie was written. |
| [src/name_resolver/walk.cpp](src/name_resolver/walk.cpp) | The two walkers and the depth guard. The initialiser is resolved **before** the name enters scope, which decides `number x = x`. |
| [src/name_resolver/names.cpp](src/name_resolver/names.cpp) | What a name reaches, in the order it is looked for — and that order is the language's shadowing rule, written down once. DESIGN §7.7's object, recognised for `satellite.main`'s parameter and **nowhere else**. |
| [src/name_resolver/numbers.cpp](src/name_resolver/numbers.cpp) | The `.satc` skip, the type check, and WORD_NUMBERS §1.5's fold. **The trie walk is READ and not written again** — `satellite_cache/paths.cpp` already had it, and a second copy would be a second place a path's identity is decided. |
| [src/name_resolver/dump.hpp](src/name_resolver/dump.hpp) · [dump.cpp](src/name_resolver/dump.cpp) | `satl --resolve`. **The pass's consumer, in the milestone that wrote it** — and the strongest case of that rule yet, because a frame produces no text at all until M9 puts values in it. The only part of the module that prints. |
| [src/machine_limits/limits.hpp](src/machine_limits/limits.hpp) | **M6**, and the one door over the module. What satl is holding to, where each value came from, and the split that PLAN §4.5.3 left open: the SHOUTED three (`THREAD_COUNT`, `CORE_COUNT`, `MEMORY_MAX`) are machine settings in no numbering, and the quiet four are `satellite.library.system.*` `1 14 2 1`–`1 14 2 4`. The file seeds the dials at startup and the dials are the authority afterwards. |
| [src/machine_limits/limits.cpp](src/machine_limits/limits.cpp) | Where the file is — beside the binary, **never** the working directory — the machine's answers first so a one-line config changes one thing, and the clamp that answers §4.5.4's "does a machine with less than the file claims win?" It does, and it is **said** rather than done quietly. |
| [src/machine_limits/config_internal.hpp](src/machine_limits/config_internal.hpp) | What a `satellite_config.ini` may *contain*: the seven keys, the nine units, the bounds, and the text helpers. The four dial names come **out of `words.def`** rather than being written again, so the spelling in the file is the spelling in the language by construction. |
| [src/machine_limits/config.cpp](src/machine_limits/config.cpp) | What happens when it contains something else: ten codes, every one with a real span and a caret. A malformed config is **refused whole** — unlike a bad `.satc`, which is ignored, and errors.def's S08xx note carries the difference. |
| [src/machine_limits/pool.hpp](src/machine_limits/pool.hpp) · [.cpp](src/machine_limits/pool.cpp) | PLAN §4.5.1.2's pool, with its two measured requirements as the specification: the main thread spawns **one** thread which builds the rest (~20 µs rather than ~590), and below ~170 units the work stays on the caller. Deliberately leaked and its threads detached — a destructor that joined would put that cost on every `satl --version`. `parallel_for` does not exist, and the header says so. |
| [src/machine_limits/watchdog.hpp](src/machine_limits/watchdog.hpp) · [.cpp](src/machine_limits/watchdog.cpp) | §4.5.2's thread, with **both** checks: `MEMORY_MAX` against this run's resident set, and `min_free_mb` against the machine's. `_exit` and not `exit`. No terminal hook — **M22 owns that**, because nothing has put a terminal into raw mode yet. `satl --watchdog` is the flag that holds the process open, and it exists because the done-when cannot be met without one. |
| [src/machine_limits/dump.hpp](src/machine_limits/dump.hpp) · [.cpp](src/machine_limits/dump.cpp) | `satl --limits`, M6's consumer, and the line `satl --words` ends with. A value and, beside it, **who decided it**. |
| [src/system_facts/facts.hpp](src/system_facts/facts.hpp) | **M6.** The public surface of the three readers ported from the first satellite. Nothing here knows what a satellite program is; `machine_limits/` is the policy over it and `satellite.system`'s twenty-eight paths are M20. |
| [src/system_facts/memory_facts.cpp](src/system_facts/memory_facts.cpp) | `/proc/meminfo` and `/proc/self/statm`. Six of v1's eleven questions; swap and the root-only DMI read stayed behind because neither is a *limit*. Used is total minus **available**, in one place — v1 had that subtraction twice and rounded differently in each. |
| [src/system_facts/host_facts.cpp](src/system_facts/host_facts.cpp) | Threads and cores. Ported **changed**: `sched_getaffinity` rather than `hardware_concurrency`, because the pool starts on every run and `taskset -c 0-3` would otherwise spawn 24 threads onto 4 CPUs. Cores are counted from `/sys` topology and **never** threads ÷ 2. |
| [src/system_facts/user_facts.cpp](src/system_facts/user_facts.cpp) | **M9**, and M6's own note is what scheduled it: `username()`, `home_dir()` and `cwd()` "are forty lines and they come with M9". They are `satellite_string`'s live codes 95, 96 and 100. `cwd()` is the one line that is not a port — v1's fixed `char buf[4096]` is a constant deciding how long a path the user may have, which DESIGN §7.5 forbids, so the buffer doubles until it fits. |
| [src/system_facts/interrupt.hpp](src/system_facts/interrupt.hpp) · [.cpp](src/system_facts/interrupt.cpp) | **M11.** Ctrl-C as a flag: v1's SIGINT machinery ported whole — installed **without `SA_RESTART`** (hard-won; a blocked read must come back `EINTR` so M14 can tell a Ctrl-C from a closed stdin), first press sets a lock-free flag, second press runs the emergency hook and `_exit(130)`. The machine never includes this: `evaluator/machine.hpp`'s Policy carries a function pointer and `programs/built_program.cpp` wires it, which is the ceiling's seam applied to a signal. The hook has **no registrar until M22's raw mode**, ported now so M22 finds the socket where v1 left it. |
| [src/system_facts/stack_facts.cpp](src/system_facts/stack_facts.cpp) | `RLIMIT_STACK` and this thread's own stack, ported whole — plus `widen_stack()`, which is **new and is the one thing in this directory that changes the machine rather than reporting it**. The 8 MiB a login shell hands out is a *soft* default with an unlimited hard limit behind it, so satl raises its own to a share of the machine's memory -- 32 KiB for every MiB, never under 128 MiB, which is 1.9 GiB here and 32 GiB on a terabyte machine -- and 500,000 nested brackets stop segfaulting. `RLIM_INFINITY` still answers **unknown** and not unbounded, from both sides. **M9's depth ceiling was derived from this until 2026-08-31 and there is no ceiling now** — DESIGN §7.5 was rewritten and the reader lost that consumer. |

**`src/error_reporter/` landed at M5 on 2026-08-30**, and it is the one module
built **before** anything needs it rather than when something does — DESIGN §9's
whole argument. Everything after it reports through `errors::render`, and the
four places that used to compose their own sentence (`main.cpp` twice,
`cache_command.cpp`, and `parser.cpp`'s `ParseError`) are gone. It depends on
`satellite_words/` for the suggester and on nothing else; the lexer, the parser
and the cache depend on **it**.

**`src/satellite_cache/` landed at M4.5 on 2026-08-30**, between the parser and
the error reporter, because it serialises a parsed program — there is nothing to
serialise before M4 — and because a malformed `.satc` is the first thing in the
language that has to say something to a user in plain words, which is the job M5
inherits and generalises.

`src/abstract_syntax_tree/` and `src/parser/` have a paragraph above and **no
rows in this table**, which is a gap M4 left and neither M4.5 nor M7 closed
because the files are M4's to describe. MILESTONES/M4.5.md §6 carries it.

**`src/name_resolver/` landed at M7 on 2026-08-31**, after the machine limits and
before the value model, and it is the module with the **most dependencies and the
fewest of its own decisions**: it reads the parser's tree, the reporter's codes,
the registry's numbering and — the one that looks backwards and is not —
`satellite_cache/`'s trie walk. DESIGN §6.3 says that walk is M7's, and M4.5 had
already written it to decide what a `.satc` may substitute, so the milestone that
owns it reads the file that had it rather than writing a second one.
`numbers.cpp` opens with the argument.

**`src/machine_limits/` and the rest of `src/system_facts/` landed at M6 on
2026-08-30**, between the error reporter and resolve, because three later
milestones read something M6 builds and none of them said so until 2026-08-28:
M8's `Number` reads `division_digits`, M9 derives its recursion ceiling from
`RLIMIT_STACK`, and ~~M10's printer thread is the pool's first tenant~~. *(Struck
2026-09-02: the printer creates its own thread and always was going to — PLAN
§4.5.1. The two that are left are the two that were checked against code.)* The
seam between the two directories is that `system_facts/` reports what the machine
says and `machine_limits/` decides what satl does about it.

**`src/satellite_number/` was filled at M8 on 2026-08-31**, and until that
morning this paragraph read *"exists and is empty, holding a name for work that
has not started."* Ten files and about 1,300 lines, ported from the first
satellite, which PLAN §6.1 had surveyed at 10 files and 1509 and called
*"internally closed, so it ports alone."* **Both halves of that sentence needed
one correction each**: the module now includes `satellite_random/random.hpp`,
because this tree already had the `Bits32` seam and the draw ceiling and a
second copy would be two facts in two places each; and the port came out
shorter, because DESIGN §8.1's explicit sign deleted three blocks of code that
existed only so that `-LLONG_MIN` had somewhere to land.

**`src/abstract_syntax_tree/` and `src/parser/` landed at M4 on 2026-08-30.** Two
directories rather than one, because the arena is a data-layout decision that
everything else rides on (PLAN §2.6) and the parser is a reader of DESIGN §6 —
and because the tree outlives the parse: M4.5 serialises it, M7 resolves it and
M9 compiles it, none of which need the parser linked in.

**`src/satellite_string/` was filled at M3 and PLAN M9 owns it**, which is a
pull-forward and is written here rather than left to be discovered. DESIGN §5's
first sentence makes the alphabet the lexer's dependency, and a milestone cannot
lex without one. What came across is the **character half**; codes 95–100 resolve
to live system facts at decode time. Nothing in the lexer can reach one:
`encode_raw` maps every source byte to a letter, a digit, a punctuation code or
the raw area, so no live code can occur in a program's text at all.

**M9 FINISHED IT FROM ONE MODULE UP, WHICH IS NOT WHERE PLAN §6.1 PUT IT.** That
section predicted "replacing six lines with six calls" inside this directory. The
calls are in `src/satellite_value/render.cpp`, because **the lexer calls
`decode()` on every token's text** and `--unparse` prints that text back — so a
live decode here would write this machine's thread count into the source of any
program holding a `\threads` escape. `decode()` has a second entry point taking a
table of the six instead: the alphabet fills none of them and the value module
fills all of them, which is `lexer.hpp`'s own split between a token's `text` and
its `str`.

**`src/satellite_value/` and `src/evaluator/` landed at M9 on 2026-09-01.** Two
directories rather than one, on the same seam `abstract_syntax_tree/` and
`parser/` are cut on: a `Value` is a data-layout decision that everything else
rides on, and the evaluator is a reader of it. The value model outlives any one
run of the machine — M10's console prints one, M16's containers hold them, M19's
files return them — and none of those needs the machine linked in.

**`src/satellite_scalars/` landed at M11 on 2026-09-03, and it is the console's
wall generalised.** The evaluator must not know what `upper` does and
`satellite_string` must not know what a Machine is, so the rows connecting them
live in a module whose whole job is being the join — which is what
`satellite_console/handlers.cpp` already was, promoted from a file to a shape.
When M16's containers and M19's files bring their methods, each module brings
its own handlers file and the table stays a property of the build.

**`src/satellite_console/` landed at M10 on 2026-09-02, and it is two files with
a wall between them.** `console.cpp` includes nothing but the standard library:
it knows about strings, a queue and a thread, and not what a `Value` is or that
there is such a thing as a path number. `handlers.cpp` is the only file in the
tree that includes both `evaluator/dispatch.hpp` and a console, and it exists so
that **the module owning the word owns the row** — when M14 adds `input`,
`typed()`, `width`, `height`, `clear()` and `home()`, six more of the nine
children of `console` `1 5`, they arrive beside `display` rather than inside the
evaluator. The same wall is why `tests/console_test` can redirect its own
descriptor 1 and prove DESIGN §10.1's atomicity with eight threads: the subject
is a queue, and everything else in that binary is there only to reach it the way
a program does.

`src/satellite_random/` landed ahead of any milestone that called it, the way
`satl-term` did, and from M2 to M12 this paragraph said **`satl` still does
not link it**. M13 ended that: the twelve rows of `satellite.random.*` are its
consumer in the language, `tiers.cpp` and `handlers.cpp` arrived beside the
generator, and `satellite_time/` landed the same day as the other half of the
same milestone. What M8 had already changed stays true and stays load-bearing:
`tests/number_test/draw.cpp` drives the `Bits32` seam with a splitmix32 stub —
the proof the seam is generator-agnostic — and M13's distribution fixtures in
`tests/eval_test/clock_and_dice.cpp` drive `tiers.hpp`'s `draw_*` layer through
the same stub, so a hundred-run claim costs microseconds and no spin. Everything under `satellite_words/` except `dump.cpp` is `constexpr` data and
pure functions over it, so a future `.satc` reader or disassembler can read the
numbering without linking anything.

## `MILESTONES/` — what each milestone did

One file per milestone that has been built, written when it landed. **Reviews
and not plans**: PLAN §8 says what a milestone *will* be and a file here says
what it turned out to be. [MILESTONES/README.md](MILESTONES/README.md) is the
index and the convention — a note opens `**Landed <date>** (`<hash>`)`, naming
the commit that landed the **work** rather than the one that added the note.

**Two of the nine are reviews of `prototype/` and say so in their first line.**
`M11.md` and `M16.md` opened "Landed 2026-08-29" flat until 2026-08-30, which
reads as landed in `src/` and is not what happened; PLAN §8 lists both as
milestones that have not started. Recording the commit is what found it.

**The ninth is `M1.5.md`, and it is that finding with its sign flipped.** *(Added
2026-08-30.)* The window landed on 2026-08-27 and had **no note here at all**, so
it was counted as unlanded and sat twenty-third in PLAN §8's build order. A rule
that catches a note claiming too much cannot catch a milestone with no note; what
found it was PLAN §8 being renumbered into build order that day, which is also
where these filenames come from. **`M11.md` and `M16.md` were `M9.md` and
`M10.md`** and the drafts they review keep their old directory names.

## `example/` — the acceptance programs

Not samples. **Each of these is what a milestone means by done**, which is why
PLAN §8 and DESIGN §3 cite them by path rather than describing them in prose.

| file | what it is the done-when for |
| --- | --- |
| [example/console.satl](example/console.satl) | **M10, and the first program in this tree that runs.** The bare `satellite.main()` form — DESIGN §6's grammar reads `"(" [ param_list ] ")"` and §3 keeps both shapes — with one `satellite.console.display` and a `satellite.return(satellite)`. `satl example/console.satl` prints `hello!` and exits 0. Written by the author on 2026-09-02, the day M10 was built. |
| [example/bare_main.satl](example/bare_main.satl) | **M10**, and the same shape with **nothing printed** — which is not a lesser copy of the file above it. A program that prints nothing still takes the four-step shutdown, and a barrier that waited on a printer nobody started would hang every one of them; this is the commonest exit path in the language and the easiest to deadlock on. `satl --compile` says the difference in one line: `0 call sites with an inline cache` against 1. |
| [example/hello_world.satl](example/hello_world.satl) | **M17**, and DESIGN §3 is a byte-for-byte copy of it. Declares `satellite.container.list<satellite.variable.string> arguments` again as of 2026-08-28; the program never reads it, so what it needs is one empty list — **which is M16's, so M17 runs after M16** and PLAN §8's opening carries the reordering. Its `//` comments are specified in DESIGN §5.6. |
| [example/help.satl](example/help.satl) | **M18**, and it is the done-when written down: *"a program whose whole body is `satellite.help`, and the output names exactly the paths that are built when it runs."* All three shapes in one file — the bare word, a topic by its path, and a variable answering for its declared type before it holds anything. **It is the one acceptance program whose output is supposed to change**: run it again at M19 and `satellite.file` joins the topics, with no edit here. |
| [example/advanced.satl](example/advanced.satl) | The console milestone that **does not exist** — `input(prompt)` `1 5 3` and `input(prompt, target)` `1 5 4`. Also uses `+` on strings, specified nowhere. |
| [example/thread_test.satl](example/thread_test.satl) | **M23**, and it cannot be M23's done-when yet. `.start()` `1 6 13 1` and `.join()` `1 6 13 2` **were numbered on 2026-08-28** and this row said otherwise until M2 transcribed them; what is still missing is that `satellite.thread.new(f(x))` needs the deferred call `1 6 16`, which no milestone owns. SCRATCH.md/THREADS.md. |
| [example/super_advanced.satl](example/super_advanced.satl) | **M15**, and it is the float's *exact* half — `+` is DESIGN §8.6's class 1, which never rounds, so it runs before the rounding rule is chosen. |
| [example/frames.satl](example/frames.satl) | **M7**, and the only file here written for the milestone that owns it. Every clause of DESIGN §7 in one program: a recursive `factorial` (§7.1's failure, which returned 1 for every input under one shared slot), a capsule called **before** it is declared (§7.3's four passes), `counter` declared twice for the fresh slot (§7.4), `arguments.machine.cores` six numbers deep (§7.7), and `sort("down")` folding to `1 4 2 5` while the `.satc` keeps the literal (WORD_NUMBERS §1.5). |
| [example/binary.satl](example/binary.satl) · [hex.satl](example/hex.satl) | **M19.5's done-when, one program per radix**: a program declares one of each, displays it at its written width, and writes it to a file with `write(x)`. **Every method is called on a NAME and that is the language rather than a style** — a selector folds only through a declared name (WORD_NUMBERS §1.5's one hop), so `run.width().to_string()` does not compile and the two steps are written out; a conversion verb is the shape people chain, so these two types meet the rule more often than most. `hex.satl` carries the three things binary has no version of: `width()` answering bits while `digits()` answers digits, `x00ff` displaying as `x00FF`, and `x00FF == x00FF.to_binary()` being **false** with identical bits on both sides. |
| [example/satellite_config.ini](example/satellite_config.ini) | **M6**, and the only file here that is not a satellite program — `satl --limits example/satellite_config.ini` is the milestone's done-when. It carries this machine's measured numbers (24 threads, 12 cores) and documents the format in its own comments, including which of the four dials nothing reads yet. |
| [example/broken_config.ini](example/broken_config.ini) | **M6**, and a file that must **not** read — the same standing `class_test.satl` and `gui_example.satl` have, and FORMAT/CXX.md §6.2's rule. Seven of the ten rows of `errors.def`'s S08xx block, one per line, with the reason written above each; the file says which three it cannot reach and why. |

**None of them runs.** M2 landed on 2026-08-28, **M3, the lexer, on 2026-08-29**,
and **M4, the parser, and M4.5, the cache, both on 2026-08-30**;
`satl --tokens example/hello_world.satl` was the first command in this tree that
read one of these files and answered about its contents, `satl --unparse` is the
first that answers *in satellite*, and `satl --satc` is the first that leaves
anything behind it — a numbered copy of the program in `$HOME/.satl/cache`,
which it reads back on the next run. **Five of the seven parse and round-trip; `class_test.satl` and
`gui_example.satl` do not**, and MILESTONES/M4.md §6 names the two constructs —
neither is in DESIGN §6's grammar and both are the files' rather than the
parser's. These are written first on purpose, because a milestone whose
done-when is a program somebody can read is one that cannot be argued about
afterwards.

**M10 landed on 2026-09-02 and this directory gained the first two programs that
run.** "Nothing executes until M10, which is where a program first runs at all"
stood here until that day; `console.satl` and `bare_main.satl` are what it was
waiting for, and the author wrote both on the day the milestone was built.

~~**None of the five older ones runs, and that is the boundary rather than a
shortfall.**~~ **`hello_world.satl` runs, and has since M16 bound the list on
2026-09-05.** Every one of the five declares `satellite.main`'s parameter —
DESIGN §3's `satellite.container.list<satellite.variable.string> arguments`, an
empty list and therefore **M16's** — so until then each answered S0720 with a
caret under the word `arguments` and exited 3. That was the milestone boundary
landing exactly where PLAN §8's M10 entry said it would: *"its done-when cannot
be DESIGN §3."* **M17 is where DESIGN §3 became the done-when it is.** The
sentence is struck rather than deleted, because the boundary it describes is what
the split between M10 and M17 was *for*, and a paragraph that quietly stops
saying so takes the reason with it.

**The other four now stop somewhere further in, and where is worth recording
because the parameter is no longer the answer for any of them.** *(Measured
2026-09-06, each run with empty lines on stdin.)* `frames.satl` reaches line 25
and `arguments.machine.cores`, which is **M20** and is DESIGN §7.7's object
rather than the slot M16 filled — the two halves of the handover M17 writes
down, seen in one program. `thread_test.satl` reaches line 8 and
`satellite.thread.new`, **M23**. `advanced.satl` gets furthest of the four: it
reads a line from the console and stops at line 10 on **S0711**, `"YOU ENTERED:
" + local_user_input`, because `+` is arithmetic and DESIGN §6.6's table gives
no operator a string reading — that is a question §13 has not been asked yet
rather than a milestone anybody is waiting for, and naming it here is what
stops it being rediscovered. `super_advanced.satl` stops at line 14 on
**S0903**, a range refusal from `satellite.random.fast`. `class_test.satl`
still does not parse, which is M26 and is what `parser_test` asserts about it.

**And `hello_world.satl` is now an input to `parser_test` twice over.** *(M17,
2026-09-06.)* It was already the file `section_roundtrip()` prints back; it is
also the file DESIGN §3 is compared against, byte for byte, with DESIGN.md
itself a prerequisite in `make_support/065-tests.mk` — the arrangement
`WORD_NUMBERS.md` has with `words_test`. LAYOUT's own line about `example/`
holds: these are **not samples**, and a file that a document claims to be a copy
of is the strongest form of that claim this tree has.

## `tests/` — the suite

**New at M2**, and until then FORMAT/CXX.md §6's opening sentence was literally
true: *"there is no test infrastructure in this tree -- not a target, not a
directory, not a harness."* The first satellite's was ported rather than
reinvented, cut down to what one test needs.

`TESTNAMES` in `030-directories.mk` is the single place a test is declared to
exist; the source lists, the binaries, the run list and what `clean` removes are
all derived from it. The first satellite hand-copied that list, added two tests
to the run list and to neither build list, and `make test` ran binaries nobody
had built — reporting PASS from stale objects.

| file | what it is |
| --- | --- |
| [tests/words_test/words_test.cpp](tests/words_test/words_test.cpp) | The harness — three functions and a counter, no framework — and `main`. |
| [tests/words_test/words_test.hpp](tests/words_test/words_test.hpp) | The harness declarations and the three section prototypes. A *dependency*, which is why 065-tests.mk wildcards headers separately from sources. |
| [tests/words_test/authority.cpp](tests/words_test/authority.cpp) | **Opens WORD_NUMBERS.md and walks all 222 rows of §2.2**, plus §2.3's nine spellings. The one check no `static_assert` can make: a row left out of `words.def` does not leave a hole, it silently renumbers every sibling after it, and both files stay internally consistent. |
| [tests/words_test/walking.cpp](tests/words_test/walking.cpp) | PLAN M2's two worked examples by name, the interner in both directions, and what a failed walk says. |
| [tests/words_test/runtime.cpp](tests/words_test/runtime.cpp) | The live child counter and user names — PLAN §8.1's half, and the only check it gets until M4 calls it. |
| [tests/lexer_test/lexer_test.cpp](tests/lexer_test/lexer_test.cpp) · [.hpp](tests/lexer_test/lexer_test.hpp) | The harness and `main`, the same three-functions-and-a-counter shape. **Unlike `words_test` this binary links objects** — a registry is constexpr data, a lexer is a function that has to run. |
| [tests/lexer_test/characters.cpp](tests/lexer_test/characters.cpp) | DESIGN §5.1, §5.2, §5.5 and the comment rule. §5.1's own worked example is checked as a **count**, because that is the form the design states it in: 6 tokens with the rule and 16 without. |
| [tests/lexer_test/literals.cpp](tests/lexer_test/literals.cpp) | Numbers, strings, the five escapes in both directions, and §8.5's Bits — including **`x0009` is not `x9`**, which is the width being part of the value. |
| [tests/lexer_test/spellings.cpp](tests/lexer_test/spellings.cpp) | The words half. **`console` is the load-bearing fixture**: the language spells it twice, so its one spelling id and its two path numbers are what prove a `SpellingId` is not a `PathId`. Also the seven spellings of `arguments`. |
| [tests/lexer_test/spans.cpp](tests/lexer_test/spans.cpp) | Every span sliced back out of the source, the line counter, and `example/hello_world.satl` lexed end to end. **Fails loudly if it cannot read that file**, never skips. |
| [tests/parser_test/parser_test.cpp](tests/parser_test/parser_test.cpp) · [.hpp](tests/parser_test/parser_test.hpp) | The harness, and a `Program` that carries **the parse and the numbering together** — because only one of the two holds what a user name is spelled. |
| [tests/parser_test/arena.cpp](tests/parser_test/arena.cpp) | What `static_assert`s cannot reach: that the arena is flat, that node 0 and list handle 0 are real sentinels, and that no node holds text. |
| [tests/parser_test/expressions.cpp](tests/parser_test/expressions.cpp) | §6.2's postfix loop — `foo().bar()` is the defect it exists for — precedence, and **the same-line rule**, checked as `x` then `(f(y))` coming out as two statements. |
| [tests/parser_test/statements.cpp](tests/parser_test/statements.cpp) | **§6.1's collision, and it is the milestone's most important check**: `satellite.control.return my_time` must declare nothing, while `satellite.variable.time my_time` declares a variable. Same four tokens. |
| [tests/parser_test/declarations.cpp](tests/parser_test/declarations.cpp) | The name allocator: a user capsule takes **1 14 3**, which is PLAN §8.1's worked example checked; a name the language owns is refused; and the limit M4 found in M2 is asserted rather than described. |
| [tests/parser_test/depth.cpp](tests/parser_test/depth.cpp) · [tests/satc_test/depth.cpp](tests/satc_test/depth.cpp) | **M8.5's done-when.** 100,000 nested brackets, calls, subscripts, unary minuses, blocks and `if` blocks, 10,000 nested generic arguments and sections, left-associativity at 100,000 terms, and a round trip at 100,000 levels. **Neither binary links `machine_limits`**, so both run against the 8 MiB a login shell hands out and a walker that still recursed would fail rather than pass. |
| [tests/parser_test/roundtrip.cpp](tests/parser_test/roundtrip.cpp) | **The done-when**: the four acceptance programs parsed, printed, parsed and printed again, identical. Also the two files in `example/` that do **not** parse, checked as not parsing, with the reason pinned to a line. |
| [tests/eval_test/eval_test.cpp](tests/eval_test/eval_test.cpp) · [.hpp](tests/eval_test/eval_test.hpp) | **M9.** The harness, and a `Run` that holds the numbering, the tree and the closures together — because an op's operands index the arena beside it and a user's PathId is valid inside one run only. **This binary links no `machine_limits`**, which is the most important line in its build rule: everything below runs against the 8 MiB a login shell hands out. |
| [tests/eval_test/depth.cpp](tests/eval_test/depth.cpp) | **The milestone.** A capsule **1,000,000 frames deep** answers; a runaway one is refused in words about RECURSION with a call stack; a 100,000-term expression evaluates. And the one assertion that cannot be made any other way: a `while` loop of 100,000 rounds costs **exactly the same control stack** as one of 10, which is `NO_LIMITS.md` §2.2 turned from a sentence into a check. |
| [tests/eval_test/calls.cpp](tests/eval_test/calls.cpp) | DESIGN §7.1's defect from the side it was invisible from: `fact(5)` is 120, where the first satellite's one registry answered 1 for every input. Mutual recursion, forward references, and two activations of one capsule alive at once with two of the same local. |
| [tests/eval_test/arithmetic.cpp](tests/eval_test/arithmetic.cpp) · [control.cpp](tests/eval_test/control.cpp) | §8.1 and §6.6 through the evaluator — `0.1 + 0.2` is `0.3`, `10 - 3 - 2` is 5 — and the three compound statements. Plus the rule half of them enforce: **a condition is a bool and nothing else**, because a number standing in for a test is DESIGN §1.1's "behind their back". |
| [tests/eval_test/compiling.cpp](tests/eval_test/compiling.cpp) · [dispatch.cpp](tests/eval_test/dispatch.cpp) | That every node kind becomes an op or a **named refusal**, that a 100,000-deep expression compiles at 8 MiB, and that `handlers[path_id]` dispatches, checks its arity before the handler runs, and caches. The dispatch fixtures install their own handlers and put the table back — it is process-wide, and a test that passes in one order and fails in another is worse than one that fails. |
| [tests/eval_test/clock_and_dice.cpp](tests/eval_test/clock_and_dice.cpp) | **M13.** The distribution claims through the stub (both ends reachable, the short answers happening, the step set closed — a hundred draws in microseconds) and the language claims through the real rows: the alias carrying a real call, thirteen refusals answering their codes, ISO-8601 out of `display`, `holding`'s sixth word, and the tier floors — **floor only**, because a ceiling is a machine's to miss under load. The one deliberately slow section in the suite: its ~3 s IS the ultra floor behaving. |
| [tests/eval_test/bits.cpp](tests/eval_test/bits.cpp) · [hex.cpp](tests/eval_test/hex.cpp) | **M19.5**, one section per radix, nineteen checks each. **Most of both files would pass under a representation that got the type wrong**, so each names the handful that would not: `b0010 != b10` and `x0009 != x9` are DESIGN §8.5's whole claim, and an integer with a length beside it fails exactly there and nowhere else. `xF != b1111` is pinned because it is delivered by the two radices being separate arms of the variant rather than by any comparison written for it — a shared arm with a radix field would make it TRUE the day somebody forgot to check the field. Hex adds `x00ff == x00FF`, case not being part of the value, and `as_number()` answering 11111111 — the row's whole argument, since the digits `00FF` have no decimal reading at all. |
| [tests/console_test/console_test.cpp](tests/console_test/console_test.cpp) · [.hpp](tests/console_test/console_test.hpp) | **M10.** The harness, and the one thing that makes this suite different from every other: `capture()` **dup2s a temporary over descriptor 1**, runs a fixture, shuts the console down and gives the descriptor back. The claims are about bytes reaching an fd, so a fake sink would remove the half that can fail — glibc's buffering IS what DESIGN §10.1's flush is for. |
| [tests/console_test/printing.cpp](tests/console_test/printing.cpp) | Four claims, each invisible in the others: everything queued is written **in order** (which is what proves the printer is joined and not detached); **eight threads interleave lines and never characters**, because the unit queued is a whole string; the un-newlined form writes no terminator; and `drain()` returns only when the bytes have reached the fd, read back through `/proc/self/fd/1` **while the console is still running and with nothing flushed by hand**. |
| [tests/console_test/dispatching.cpp](tests/console_test/dispatching.cpp) | `satellite.console.display` `1 5 1` reached the way a program reaches it — parsed, resolved, compiled, dispatched — and not by calling the handler. That is the point: calling it directly would skip the trie walk, the array index, the arity check and the inline cache, which is everything DESIGN §4's numbering is for. |
| [tests/console_test/terminal.cpp](tests/console_test/terminal.cpp) | **M14's done-when**: the real `satl` under `forkpty(3)`, one fixture per clause, **asserted on the screen and not on the bytes** — the counter that keeps rising while `/q` sits half-typed (and the CPU that proves nothing spins), the empty line against nobody typing, the prompt that appears before the wait, Ctrl-C as the byte `0x03` answering S0730 and 130, the pty resized between two asks, and sixty `home()`s that never land inside a line. The suite's one section that runs the real binary, and why its build rule depends on `satl`. |
| [tests/prompt_test/prompt_test.cpp](tests/prompt_test/prompt_test.cpp) · [.hpp](tests/prompt_test/prompt_test.hpp) | **M22.** The harness and four sections. It links **four** of the prompt's nine files — `keys`, `editor`, `history`, `block` — and that omission is the design: those four are pure, bytes in and a line or a `Scan` out, so three of the four sections run on a machine with no tty at all. The other five need a terminal or a whole interpreter, and the section that exercises them does it honestly. |
| [tests/help_test/help_test.cpp](tests/help_test/help_test.cpp) · [.hpp](tests/help_test/help_test.hpp) | **M18.** The harness and three sections, redirecting its own stdout for `console_test`'s reason — help's answer is bytes reaching descriptor 1 through the printer thread. **Its central fixtures are sweeps and not samples**, because the done-when's own check passed by accident before the milestone started: every one of the 183 groups is asked about, the 118 built ones must answer, the 65 unbuilt ones must refuse, and the refusal must be **S1101 and never S0721** — S0721 being the code that would mean the argument had been evaluated after all. It compiles and runs real programs rather than calling handlers, so the trie walk, the compiler arm that declines to compile the argument, the array index and the arity check are all in the path. |
| [tests/file_test/file_test.cpp](tests/file_test/file_test.cpp) · [.hpp](tests/file_test/file_test.hpp) | **M19.** The harness and four sections. **It runs in a directory of its own, made with `mkdtemp` and removed at the end** — the subject is a filesystem and `satellite.file.new` is O_EXCL, so a suite that wrote into the tree would pass once and fail forever after, which is the exact defect PLAN M19 forbids in the acceptance program arriving in the thing that checks it. Every fixture asserts whether anything **refused** as well as what it answered, because a refusal and a false answer are indistinguishable from the printed output. |
| [tests/file_test/round_trip.cpp](tests/file_test/round_trip.cpp) | **M19.** The lifecycle: a failed open that is a value, a second `new` that will not clobber, the two write verbs side by side, and `open` reopening a closed handle **from the beginning** while an already-open one answers true and moves nothing. Its last fixture is the reference type stated as a test — two opens on one name are `false`, an assignment is `true`, and closing one leaves the other open. |
| [tests/file_test/reading.cpp](tests/file_test/reading.cpp) | **M19.** The cursor, which is all M19 adds to v1. An empty line is a `string` of size 0 and the end of the file is `nothing` (DESIGN §8.7, asked through `holding` because both print as very little); a last line with no newline is still a line; `read_all` rewinds and leaves the cursor at the end; **a write between two reads does not move the read cursor**, which is the fixture a `read`-based implementation fails; and a 70000-character line comes back whole. |
| [tests/file_test/refusals.cpp](tests/file_test/refusals.cpp) | **M19.** Every S12xx code by number, which is the half `example/persistence.satl` cannot contain — a program that reaches S1201 stops, so it would never print its PASS. **The code and not the text**, for M18's reason: a fixture asserting only "something refused" would pass if all seven raised S0713. |
| [tests/file_test/listing.cpp](tests/file_test/listing.cpp) | **M19.** `satellite.directory` and `satellite.system.delete`. `.` and `..` absent and a real dotfile present; a change that cannot happen answering false without dying; delete in **both shapes**, by path and by open handle. Its last fixture asserts what the suite left behind is exactly the six files whose programs refused before reaching their own cleanup — and it is where the sort-as-`SatString`s claim is actually exercised, since `closed2.txt` sorts before `closed.txt`. |
| [tests/prompt_test/keys.cpp](tests/prompt_test/keys.cpp) | **M22.** Bytes to keys, and the first clause is the one that matters: `ESC`, `[`, `A` offered **one at a time**, with only the third allowed to answer. A test that fed the three together would pass against code that cannot work on a slow link. Plus the `;5` word-modifiers, `0x03` as a key rather than a signal, a UTF-8 character arriving whole, and a truncated one dropping its half and keeping the next byte. |
| [tests/prompt_test/editing.cpp](tests/prompt_test/editing.cpp) | **M22.** The buffer and the cursor with no terminal anywhere — inserting in the middle, backspace over a multi-byte character taking **both** its bytes, word moves that stop at spaces and not at dots, and the history round trip that gives the half-typed line back. |
| [tests/prompt_test/blocks.cpp](tests/prompt_test/blocks.cpp) | **M22.** The clause this file exists for: `display("{")` **does not open a block**. That is what counting tokens rather than characters buys, and it is the one an obvious implementation fails. Plus the owed body, the four top-level forms, and a bare `capsule` being the user's name and not a declaration. |
| [tests/prompt_test/session.cpp](tests/prompt_test/session.cpp) · [pty.hpp](tests/prompt_test/pty.hpp) | **M22.** The real `satl --repl` under `forkpty(3)`, typed at. A pipe would test the opposite of what it claims: with no terminal there is no raw mode, ISIG is never turned off, and Ctrl-C would be a signal. **Two of its clauses were wrong and a mutant found them** — one matched digits the prompt had echoed, the next was defeated by the prompt repainting the line on every keystroke; M22.md §3.2 is the account. |
| [tests/satc_test/satc_test.cpp](tests/satc_test/satc_test.cpp) · [.hpp](tests/satc_test/satc_test.hpp) | The harness, the same three-functions-and-a-counter shape, plus the helpers that make a check read as the form it is about — one statement in, the whole line out. |
| [tests/satc_test/shapes.cpp](tests/satc_test/shapes.cpp) | SATC §5.1 steps 1, 3 and 4 — aliases collapsed, the reserved word absorbed, and a call slotted by arity. **`input()` against `input(prompt)` is the fixture that matters**, because both read back perfectly when they are wrong. |
| [tests/satc_test/ownership.cpp](tests/satc_test/ownership.cpp) | §3 and §3.1 — what may **not** become a number. A user's capsule, a selector after a receiver, and a literal, each checked as still being itself. |
| [tests/satc_test/examples.cpp](tests/satc_test/examples.cpp) | Every acceptance program written as a `.satc`, with **every path in the comment column checked back through the trie** — a comment that has drifted from its line is worse than none, because it is what a reader trusts instead of looking the number up. Also §5.2's source order. |
| [tests/satc_test/header.cpp](tests/satc_test/header.cpp) | §2's three lines. The digest is checked against `words::digest_text()` and never against a literal, because a literal would be edited to keep the build quiet. |
| [tests/satc_test/reading.cpp](tests/satc_test/reading.cpp) | **The milestone's done-when**: write a program, read the file back, write *that*, and the two files are identical — over both fixtures and all four acceptance programs. Plus §4's three misses and one refusal, each made by **bending one field of a file this build produced**, so that a header line somebody adds later is still checked here. |
| [tests/satc_test/writing.cpp](tests/satc_test/writing.cpp) | §5's write, on a real disk in `/tmp`. The two failures the atomic rule exists to prevent are both invisible to a test that does not write: a truncated file, and a `.tmp` left behind per run. Also that the thread is **joined** — a detached one would be racing this read and would usually lose. |
| [tests/reporter_test/reporter_test.cpp](tests/reporter_test/reporter_test.cpp) · [.hpp](tests/reporter_test/reporter_test.hpp) | The harness, plus `source_row` and `caret_row`, which **build** an excerpt's two lines rather than spelling them out — a check that writes the gutter is a check about the gutter, and nine of them turned into assertions about spaces the day the margin changed. |
| [tests/reporter_test/codes.cpp](tests/reporter_test/codes.cpp) | What a `constexpr` cannot see: that the blocks `errors.def` declares in a comment are the blocks the rows are in, that **the reserved blocks are still empty**, that a code survives being written out and read back, and that `satl --errors` reaches every row. |
| [tests/reporter_test/rendering.cpp](tests/reporter_test/rendering.cpp) | DESIGN §9's block, **including the arms nothing raises yet** — a call stack, a note with no span, a span past the end of its text, a hole with no argument. Possible only because the reporter takes three integers and a string; necessary because a branch nothing exercises is a branch that does not work. |
| [tests/reporter_test/suggesting.cpp](tests/reporter_test/suggesting.cpp) | DESIGN §4.6 by name — `consle` → `console` — and **`wihle` → `while`, which is the transposition**. Also the check a mutation asked for: a bare row's empty spelling must not displace a real child, and losing that loses the suggestion rather than making it wrong. |
| [tests/reporter_test/lexing.cpp](tests/reporter_test/lexing.cpp) | The one lexical error: its code, its caret **under the opening quote**, and the note where the line ran out — both spans out of one token, because the string arm sets both ends for exactly that. |
| [tests/reporter_test/parsing.cpp](tests/reporter_test/parsing.cpp) | **The section that catches what a registry makes possible**: one assertion per row of `errors.def` the parser owns — nineteen of twenty-two, with the other three named as unreachable and why — all through `parse()`, because a site raising a neighbouring code renders perfectly and describes a different problem. Also the four did-you-mean sites and the three notes. |
| [tests/limits_test/limits_test.cpp](tests/limits_test/limits_test.cpp) · [.hpp](tests/limits_test/limits_test.hpp) | The harness, and the header that says what each of M6's four halves is checkable *by* — including the one that is not: the watchdog's proof is a process that dies, which lives in MILESTONES/M6.md rather than in a suite. |
| [tests/limits_test/reading.cpp](tests/limits_test/reading.cpp) | The config format, and **one assertion per row of the S08xx block** — the check MILESTONES/M5.md §5 says a message registry needs, one registry on. Both units families with their real values, and `1 GiB ≠ 1 GB` as the assertion that justifies requiring a unit at all. |
| [tests/limits_test/examples.cpp](tests/limits_test/examples.cpp) | The two files in `example/`, off the disk: one that must read, with this machine's numbers, and one that must **not**, with one assertion per line of it and a count so a ninth code is a finding. |
| [tests/limits_test/facts.cpp](tests/limits_test/facts.cpp) | The machine readers, and the section that **says what cannot be asserted**: the authority is `/proc` and so is the code under test, so what is checked is the relationships — cores never exceed threads, used plus available is total exactly, a resident set is not a virtual one, and `RLIM_INFINITY` answers *unknown*. |
| [tests/limits_test/pool.cpp](tests/limits_test/pool.cpp) | **The pool's only caller in the tree**, which is `parallel_for` not existing said as a test. The floor at its boundary, a 100,000-unit batch with every unit marked so a doubled one and a missed one cannot cancel out, and a wait on a *condition* rather than a duration. |
| [tests/resolve_test/resolve_test.cpp](tests/resolve_test/resolve_test.cpp) · [.hpp](tests/resolve_test/resolve_test.hpp) | **M7.** The harness, plus `Run` — a parse, its resolve and **its numbering**, kept together because a user's `PathId` is valid inside one run and a test that let the `Words` object die would be asking about numbers that no longer mean anything. |
| [tests/resolve_test/frames.cpp](tests/resolve_test/frames.cpp) | DESIGN §7.2 and §7.4. The slot numbering is **per capsule** and starts again at 0, which is what makes §7.1's 1585-wrong-results-out-of-1600 impossible; a redeclaration holds **two** rows; and §7.3's order, proved by two capsules that call each other. |
| [tests/resolve_test/names.cpp](tests/resolve_test/names.cpp) | The lookup order — a local shadows a capsule — DESIGN §4.6 over the scope stack, and `number x = x` naming the outer x. |
| [tests/resolve_test/paths.cpp](tests/resolve_test/paths.cpp) | Every number **written out** rather than derived, the way `words_test` does it: a test that computed what it expected would compute it the way the code does and agree with a wrong answer. Also WORD_NUMBERS §1.5's fold, and the two S052x refusals from **both** the Member arm and the Call arm — which a mutation found were two sites and not one. |
| [tests/resolve_test/arguments.cpp](tests/resolve_test/arguments.cpp) | All six spellings, the seventh refused, a name that was never trying to be one left alone, and `arguments.machine.threads` at `1 14 1 1 1 3` — six numbers deep. |
| [tests/resolve_test/cache.cpp](tests/resolve_test/cache.cpp) | MILESTONES/M4.5.md §5's clause, **counted**. The answers first — both runs must reach the same numbers, or the skip is a cache that changes what a program means — and then the counts, **per path**: one chain, one type, one call shape. Asserting the totals alone let half the skip be deleted with no failure, and finding that found an inverted ternary in `unnumber()`. |
| [tests/resolve_test/examples.cpp](tests/resolve_test/examples.cpp) | All five acceptance programs that parse, resolved through the real entry point — three of them written for milestones that have not landed, so nothing about them was chosen to suit this pass. `example/frames.satl` clause by clause. |
| [tests/number_test/number_test.cpp](tests/number_test/number_test.cpp) · [.hpp](tests/number_test/number_test.hpp) | **M8.** The harness, and the header that says what a suite is *for* when its subject is a port: re-proving the first satellite's arithmetic would be checking a transcription, so the weight is on the sign — which is the only thing that is new. **Every assertion compares text**, because `operator==` is `compare()` and half these sections are about `compare()`. |
| [tests/number_test/sign.cpp](tests/number_test/sign.cpp) | DESIGN §8.1's own subject, and the largest section. The flag defaults to true; **twenty-two enumerated ways to reach a zero**, each checked for a positive sign, rather than three sampled ones; the ordering of negatives, which is where a missing reversal hides; and the round trip of `LLONG_MIN`, **which v1 refused** — measured against v1's objects before the claim was written. |
| [tests/number_test/arithmetic.cpp](tests/number_test/arithmetic.cpp) | Exact where DESIGN §8.1 promises exact. `0.1 + 0.2`, `1e20 + 1`, a sixty-digit product; both halves of v1's recorded division bug; and the two things the unsigned significand paid for, **one of which a failing fixture found** — `9e18 + 9e18` now stays inline where a signed significand would have boxed it. |
| [tests/number_test/limbs.cpp](tests/number_test/limbs.cpp) | `BigInt` reached through `Number`, because its constructor takes limbs nobody outside the module builds. The limb boundary, `digit_count` being about the value and not the storage, and **`payload_bytes()` as the check on DESIGN §8.2's "the small case never allocates"** — a claim about allocation, so asserted by counting bytes. |
| [tests/number_test/rounding.cpp](tests/number_test/rounding.cpp) | floor, ceil and round — **every claim made twice, once each side of zero**, because the three share one body and the mode reads the sign flag, so a mode consulting the wrong side gets every positive fixture right. |
| [tests/number_test/text.cpp](tests/number_test/text.cpp) | parse and `to_string`. What parse refuses, one per shape, because that list **is** S0610's sentence; trailing zeros as spelling; and the notation boundary both sides, including 30! printing whole — the case the restated rule exists for. |
| [tests/number_test/exact.cpp](tests/number_test/exact.cpp) | **The three operations that never round** — modulus `1 6 4 12` and the two shifts `1 6 4 1` and `1 6 4 11`, all three added on 2026-08-31 after DESIGN §8.6's classification turned out to say they were never M15's. Every fixture is chosen so a ROUNDING implementation gives a different answer: a quotient that does not terminate, a shift whose exact value is 70 digits, and §8.6's own `7.5 % 2.1 = 1.2`, which is the one a `double` gets wrong. |
| [tests/number_test/draw.cpp](tests/number_test/draw.cpp) | The uniform draw, and **`satellite_random`'s first consumer**. Driven by a splitmix32 stub rather than PCG, so the uniformity measured is the sampler's. Re-measures the skew rather than quoting v1's figures — and finds it one level in from where the comment points: the visible 2:1 is the *second* rejection, the one that redraws instead of folding `[0, top+1)` back with `%`. |

## `make_support/` — the build

Numbered because the order is load-bearing in three places: 010 before 020, which
both fragments say at their own top, and 045 and 047 before 050, which each says at
its own top.

**Three files here are not `.mk` fragments and are not included by the `Makefile`**,
which is new as of 2026-08-31: `startup.rows` is data and `startup.sh` is a program,
both read by `067-startup.mk` at the moment its target runs. They live here rather
than in a directory of their own because they are build machinery and this is where
the build lives — the same argument `install_support/` makes for its numbered shell
scripts.

| file | what it is |
| --- | --- |
| [make_support/010-compiler.mk](make_support/010-compiler.mk) | Which compiler, and `OPT` as the one flag knob. Explains why `CXXFLAGS` is not one. |
| [make_support/020-version.mk](make_support/020-version.mk) | The two version numbers, the build stamp, and `version_defs` — a function, because satl is built twice and the two must not describe themselves identically. |
| [make_support/030-directories.mk](make_support/030-directories.mk) | One variable per module directory, so a directory that moves is one edit. |
| [make_support/040-sources.mk](make_support/040-sources.mk) | **What gets compiled and linked**, named one by one rather than wildcarded. Also holds the measured startup numbers. |
| [make_support/045-microarchitecture.mk](make_support/045-microarchitecture.mk) | The two microarchitecture builds: the `-march=x86-64-v3` flags, the `-dumpmachine` test that decides whether there are two, and the object lists. |
| [make_support/047-window.mk](make_support/047-window.mk) | Whether this machine can build `satl-term`: the `pkg-config vte-2.91-gtk4` probe, the flags it yields, and the window's two sources. |
| [make_support/048-static.mk](make_support/048-static.mk) | `STATIC=0/1/full` — **`full` by default since 2026-09-06**, so a bare `make` produces what an install ships — the `-print-file-name` probe that asks the compiler whether it *can* link statically, the `.ldflags-stamp`, and `env -u LD_RUN_PATH` so a static binary carries no RPATH into somebody else's home directory. **It had no row here until 2026-08-31**, which is the gap `make startup` found by needing to name it. |
| [make_support/050-build.mk](make_support/050-build.mk) | The default goal and the four link rules. The first fragment that declares a target. |
| [make_support/060-compile.mk](make_support/060-compile.mk) | How a `.cpp` becomes a `.o`, for both variants, plus the two flag stamps that catch a changed command line. |
| [make_support/065-tests.mk](make_support/065-tests.mk) | **New at M2.** The `test` target and the per-test source wildcards, all derived from `TESTNAMES`. Read before 070 so `clean` can name `$(TESTBINS)`. |
| [make_support/067-startup.mk](make_support/067-startup.mk) | **New at M7.** The `startup` target, and the empty-program floor it measures against — linked through the same variables 050 links `satl` through, so the two sides cannot drift apart. |
| [make_support/startup.rows](make_support/startup.rows) | **New at M7.** The registry: which commands `make startup` times and what each cost last time. Adding a command to the measurement is one line here. Carries the argument for why a *share* is comparable across link modes and a raw row is not. |
| [make_support/startup.sh](make_support/startup.sh) | **New at M7.** How a measurement is taken: best of five runs of 200, the floor subtracted, the baseline diffed. The only consumer of `startup.rows`. |
| [make_support/070-clean.mk](make_support/070-clean.mk) | Removing what a build made, named one by one rather than by deleting a directory. |

## `satellite_enterprise/` — the Enterprise Linux install

| file | what it is |
| --- | --- |
| [satellite_enterprise/install.sh](satellite_enterprise/install.sh) | An index. Locates the tree, then sources the eight fragments below in order. |

### `install_support/`

A sourced fragment runs as it is read, so the order below *is* the script.

| file | what it is |
| --- | --- |
| [010-defaults.sh](satellite_enterprise/install_support/010-defaults.sh) | The root (`$HOME/.satl`) and the flags, with the measurement behind `--link` and `--desktop` defaulting off. |
| [020-saying-things.sh](satellite_enterprise/install_support/020-saying-things.sh) | `die`, `usage`, and the `run`/`show`/`quoted` trio that makes `--dry-run` an honest, pasteable transcript. |
| [030-arguments.sh](satellite_enterprise/install_support/030-arguments.sh) | The command line, and the refusal to accept a root that belongs to something else. |
| [040-machine.sh](satellite_enterprise/install_support/040-machine.sh) | Reads `/etc/os-release` without sourcing it, notes a non-EL system, and checks there is a compiler and a make. |
| [050-building.sh](satellite_enterprise/install_support/050-building.sh) | Builds, then runs `satl-cpu-level` to pick the variant to install, and looks for `satl-term` to decide whether the window installs. |
| [060-install-tree.sh](satellite_enterprise/install_support/060-install-tree.sh) | **The one declaration of what gets installed**, read by both the install and the uninstall: three programs — `satl`, `satl-cpu-level`, `satl-term` — the launcher, the mime packet and the artwork. |
| [070-desktop.sh](satellite_enterprise/install_support/070-desktop.sh) | The optional symlinks under `~/.local` — `satl`, `satl-term`, the launcher, the mime packet, the icons — and the ownership check that refuses to overwrite another install's files. |
| [080-report.sh](satellite_enterprise/install_support/080-report.sh) | What happened, verification by running the installed `satl` **and `satl-term`** — both answer `--version` without a display — and what the word `satl` actually gets you. |

### `icons/` — installed

Layout mirrors the install destination exactly, so installing is a copy and not a
translation.

| file | what it is |
| --- | --- |
| [application-x-satellite.xml](satellite_enterprise/icons/application-x-satellite.xml) | The `.satl` mime packet. **Read its comments before changing anything about icons or the mime type** — each records something found the hard way. |
| [org.satellite.terminal.desktop](satellite_enterprise/icons/org.satellite.terminal.desktop) | The launcher for `satl-term`. **Installed since 2026-08-28**, conditional on the binary it names having been built — M1.5 built that on 2026-08-27 and `060-install-tree.sh` now carries the row. |
| [org.satellite.terminal.svg](satellite_enterprise/icons/org.satellite.terminal.svg) | A complete scalable icon that is **deliberately never installed** — shipping it alongside the PNGs makes which one a shell draws unpredictable. |

And the pixel artwork, two files at each of nine sizes:

| path | what it is |
| --- | --- |
| `icons/hicolor/<size>/apps/org.satellite.terminal.png` | The application icon, at 16, 22, 24, 32, 48, 64, 128, 256 and 512. |
| `icons/hicolor/<size>/mimetypes/application-x-satellite.png` | The icon drawn on a `.satl` file, at the same nine sizes. |

Eighteen files. The two basenames are the two names a desktop looks up, and neither
may drift: `org.satellite.terminal` matches the GApplication id and the `.desktop`
filename, and `application-x-satellite` is the mime type with `/` replaced by `-`.

**The nine `apps/` files come from TWO sources since 2026-08-31 and that is worth
knowing before editing any of them.** 128, 256 and 512 are
`icon_artwork/satellite_icon_{128,256,512}.png`, byte-for-byte — the author
re-exported those three that day with a soft fringe around the dish removed, and
they were copied in unaltered. 16, 22, 24, 32, 48 and 64 are still the older
`icon_artwork/satl-app-icon/satl-icon-*.png` line, also byte-for-byte at 64 and
downscaled below it. **So the set is deliberately not one export**: the large
sizes are the corrected artwork and the small ones are what they always were,
which is the scope the author chose rather than have anything generated on their
behalf. Bringing the small sizes over needs a de-blurred export at those sizes,
and it is theirs to make.

**The `mimetypes/` nine are untouched by that** and all nine still come from
`icon_artwork/file_icon_final/` — a different image with its own history.

### `icon_artwork/` — source, never installed

The user's own exported work, copied byte-for-byte from the first satellite.
Nothing here is installed and nothing here should be re-encoded.

**Byte-for-byte is a rule and not a description**, and it is the one thing about
this directory a reader has to take literally: these are somebody's exports, and
re-encoding one — scaling it, stripping a chunk, "cleaning it up" — replaces
their work with a guess about their work. When the author updated three of them
on 2026-08-31 the update was `cp`, and the sizes they did not update were left
alone rather than derived from the ones they did.

| file | what it is |
| --- | --- |
| `for_redo/satellite-icon.xcf` | The editable GIMP source for the satellite icon. |
| `satellite-icon.avif` | The icon as exported to AVIF. |
| `5829875.png` | The original photograph the satellite icon was cut from. |
| `satellite_icon_{64,128,256,512}.png` | The satellite icon at four sizes. **128, 256 and 512 were re-exported 2026-08-31** with the fringe around the dish removed, and are the installed `apps/` icons at those sizes. The 64 is the older export. |
| `scaled/satellite-icon-{64,128,256,512}.png` | A scaled set of the same. |
| `satl-app-icon/satl-icon-{64,128,256,512}.png` | The app-icon variant at four sizes — the same image as `satellite_icon_*` with the older fringe, exported separately. **It is the installed `apps/` icon at 64 and below only**, since 2026-08-31; see the note above the `icon_artwork` heading. |
| `file_icon/352-3528073_piece-paper-frames-illustrations-piece-of-paper-icon.jpg` | The stock sheet-of-paper image the file icon was built from. |
| `file_icon/file-icon.png`, `file_icon/file-icon-512.png` | Intermediate file-icon work. |
| `file_icon_final/final_file_icon_{64,128,256,512}.png` | The finished file icon at four sizes. |

## `help_lines/` — where `HELP.md` is written and checked

The source of [HELP.md](HELP.md), which is generated and must not be edited in
place. Its own [README.md](help_lines/README.md) says what each file is; the two
things worth knowing from out here are why it exists at all.

**The node table is measured and not typed.** `nodes.tsv` is the output of a
walk over the trie with every module's handlers installed, asking
`eval::Handlers::table().find(id)` for each node from 1 to `kNodeCount`. So
"130 of the 269 run today" is what the dispatch table holds rather than what a
document claims about it, and **the walk is also the shape M18 needs** — PLAN
§8's `built()` predicate is this plus the front-end word set.

**And every worked line in the document is executed.** `verify.py` wraps each
example into a whole program and runs it under `SATL_NO_WINDOW=1`. It caught
five statements that were written down confidently and were wrong — among them
that `console.typed()` answers a bool, which it does not, and that a capsule
must declare `satellite.returns`, which it need not. **The fourth of the five
turned out at M19 to be a BUG rather than a fact**: `list.remove("bolt")` was
refused because the resolver guessed which words take a literal option, and the
entry had been rewritten to work around it and passed. A worked line proves the
sentence beside it runs; it cannot prove the sentence is the one that should
have been written. **A help text nobody ran is
exactly the drift M18 exists to end**, and it drifted five times before it was
finished. PLAN §8's M18 entry has the argument in full, with the first
satellite's `help.cpp` as the evidence.

## `SCRATCH.md/` — the things that do not last forever

A folder, and the `.md` in its name is deliberate: it sorts beside the four
permanent documents it is the opposite of. **Nothing in it decides anything**, and
every file in it is written to be deleted. The test for whether something belongs
here rather than in DESIGN, PLAN, LAYOUT or WORD_NUMBERS is whether it *stops being
true when the work it describes is finished.*

Its own [README.md](SCRATCH.md/README.md) lists what is in it and the condition for
deleting each one, so this table does not repeat them.

**Three files were deleted from it on 2026-08-31, which is the only thing this
folder can do that proves it is working.** `WORD_SURFACE.md` and `M6_STATE.md`
had both carried **CONDITION MET — deletable now** in that README since the day
their conditions were met, and M7's own `M7_START.md` — the reading-in that found
M7 had no done-when, that the draft in `prototype/M6/` over-reaches into M26's
spacesuits, and that DESIGN §7.5 reads as one recursion bound where there are two
— had the condition *"M7 lands and MILESTONES/M7.md carries whatever of it turned
out to be true."* It does. **A row that says "deletable now" and stays is this
folder failing at its one job**, because a temporary record nothing maintains is
how a decision gets made twice. `plans/` used to hold the
author's first note; that note has been converted into the permanent documents and
the file deleted, and its conversion is recorded in
[SCRATCH.md/FIRST_NOTE.md](SCRATCH.md/FIRST_NOTE.md) until nothing needs it.

## `prototype/` — drafts, and not a source

Nine directories, `M3` through `M11`, each a standalone sketch of a milestone
with its own `Makefile`. **These names are PLAN §8's numbering as it stood before
2026-08-30 and they are deliberately not renumbered** — a draft is dated by
construction, and `prototype/M9` is what `MILESTONES/M11.md` reviews. §8's opening
holds the old-to-new table. **They are read as drafts and never as sources**, which
is a rule two milestones have now had to state: [MILESTONES/M3.md](MILESTONES/M3.md)
§3 records that `prototype/M3/` confused a `SpellingId` with a `PathId` — and
compiled, because the two are the same 32 bits — and
[MILESTONES/M4.md](MILESTONES/M4.md) §3 records that `prototype/M4/` decided every
dispatch by comparing strings against `"satellite"`, which is PLAN §1.1's founding
finding reproduced.

They are **tracked** because those two reviews cite them by path, and a citation
to a file that is not in the repository is a citation to nothing. Their objects
and test binaries are not; `.gitignore` says why in the same terms it uses for
`tests/`.

## `satellite_debian/` — the Debian family build

A second, self-contained build of the same `src/` for Debian and its
derivatives: its own `Makefile` over its own `make_support/`, its own
`install.sh` over its own `install_support/`, and a `build/` directory it owns
outright and gitignores itself. Its [README.md](satellite_debian/README.md) is
the authority on it; this table does not repeat what that file says.

**It compiles the same sources and does NOT declare them a second time**, which
is the one thing to know before editing either. `020-inherited.mk` *includes*
six of the root's fragments — the six that declare no targets — and states only
where they are rooted, so `SATL_SRCS` has exactly one home. Its own comment says
what a copy would cost: the two lists would agree until the day the language
gained a source file, and then this build would keep linking from a list that no
longer describes the interpreter.

**Verified on 2026-08-30**, when M4 added eight sources and four headers to the
root's list and this build picked up all twelve with **no edit here at all** —
`build/satl` and the root `satl` produce byte-identical `--unparse` output.

## `old_versions/`

| path | what it is |
| --- | --- |
| `old_versions/first_satellite/` | The complete first satellite — 644 files, its own build, its own `design/`. A **reference**, not a dependency: nothing here includes from it and `satl` compiles with the folder absent. Deliberately outside this repository's history. |

## Build output

None of this is source, all of it is gitignored, and `make clean` removes it by
name.

| path | what it is |
| --- | --- |
| `satl` | The interpreter, baseline build — runs on any x86-64. |
| `satl.haswell` | The interpreter, `-march=x86-64-v3 -mtune=haswell`. Built only on x86-64. |
| `satl-cpu-level` | The detector the installer runs to choose between the two. |
| `satl-term` | The GTK4 + VTE window (M1.5). Built once at the baseline, and only where `pkg-config` finds `vte-2.91-gtk4`; `make` skips it with a note elsewhere. |
| `src/*/*.o` | Objects. `main.o` and `main.haswell.o` are the same source compiled against the two instruction sets. |
| `.cxxflags-stamp` | The exact compiler and flags the baseline objects were built with, so a changed command line forces a rebuild. |
| `.cxxflags-stamp-haswell` | The same for the haswell objects — a second file, because one could only ever describe one of the two. |

## Installed, outside the tree

| path | what it is |
| --- | --- |
| `$HOME/.satl/satl` | Whichever build this machine can run. `satl --version` prints the flags it was compiled with, so it is its own record of which one. |
| `$HOME/.satl/share/mime/packages/application-x-satellite.xml` | The `.satl` file type. |
| `$HOME/.satl/share/icons/hicolor/…` | The eighteen icons. |

Twenty files. `satellite_enterprise/install.sh --uninstall` removes them by name.
