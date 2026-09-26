# satellite-004 — PROGRESS.md

**satellite 004 revision 08** (the build number is in `satellite/config/satellite_config.hpp`,
and `satl --version` shows it). Where the work stands: section 1's table runs to
2026-09-23, the rest of this file as it was written on 2026-09-16. Read this first
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

**2026-09-26 -- FAST PRINTING STEPS 1 TO 4** (SCRATCH.md/FAST_PRINTING.md is the plan, the
measurements and what is still his; check.sh 1035):

| piece | commit | files |
|---|---|---|
| A display asks no other word family: `is_display_word` is one compare, and file, info, infinity, window, container and console are never asked of it. `word::code_of` is `constexpr` and `word::fixed_code<1, 5, 1>` is a code the compiler works out; the window words are a table built at compile time. 1M lines 2.34 s -> 1.07 s | ab6e7d7 | `bytecode/make_word_codes.py`, `word_codes.hpp`, `window_calls.cpp`, `expression.cpp` and the six family checks |
| A string literal is built straight from its codes, with no trip through UTF-8 (`string_literal_at`) | fbf0788 | `bytecode/bytecode_registry.cpp` |
| THE PRINTING SATELLITE: a plain display moves its finished value to a thread of its own, which makes the line and hands 64 KiB pieces to a display thread; `std::cout`'s 8 KB goes through the same door, and a flush waits for the screen. His buffer: `arguments.display.buffer` (131072; `display.buffer =` in config.ini), past which the program stops with S840 and exit 65. 0113 against 0111: +4% lines, +8% numbers, 0-41% on 200 x 1 MiB (0112 read 5/5/36%), kept at his word ("keep step 3"); one fresh reader's eleven findings fixed first | c8fca60 | `satellite/display/` (new), `expression.cpp`, `console_calls.cpp`, `program_walk.cpp`, `arguments.cpp`, `structured-library.cpp` |

**2026-09-26 -- CONTAINERS.md STEPS 1 AND 2: any container in any container, and satellite.access**
(SCRATCH.md/CONTAINERS.md opens with where it stands and what is his to overrule; check.sh 958):

| piece | commit | files |
|---|---|---|
| `satellite.container.map` is the same container as `.index` (`is_an_index_word`); `satellite.container.map()` is an empty one | fdc586b | `bytecode/type_shape.hpp`, `container_calls.cpp` |
| A map written whole: `{"zoe": 30, "al": 4}` (a key twice keeps its first place, its last value) | fdc586b | `bytecode/expression.cpp` (`map_literal`) |
| A write or append through a `multiple` is held to the type the value is held as (`arm_holding`); two types of one kind are not chosen between | fdc586b | `bytecode/type_shape.cpp`, `expression.cpp`, `container_calls.cpp` |
| A `multiple` with one spacesuit among its types calls that spacesuit's capsules | fdc586b | `bytecode/program_check.cpp` (`where.multiples`) |
| `satellite.access(name)`, 1 31 1: the type, the value, a line to reach every level and to fill it, .size and .keys; prints on a line of its own, answers text elsewhere, works at the prompt | fdc586b | `bytecode/access_calls.cpp`, `access_words.cpp` |
| Every container shape to a depth, and every line access prints, run as code (depth 4: 5,412 programs, 0 wrong; check.sh runs depth 2) | fdc586b | `utility/check_container_shapes.py` |

**2026-09-25, EVENING -- THE ERROR SWEEP AND THE AUTHOR'S ANSWERS, every one built and pushed**
(SCRATCH.md/NEW_ERROR_LIST.md holds the questions and his words; check.sh 934 passed at the end):

| piece | commit | files |
|---|---|---|
| A file saved on Windows runs (`\r\n`; a lone `\r` in a file with no `\n`); a byte-order mark at the start is dropped | b603f03, 3fdd50a | `satl/satl_file.cpp` |
| Unquoted `./x` and `../x` includes; an include naming no file is refused | b603f03 | `bytecode/include_shape.cpp`, `capsule_scopes.cpp` |
| A line outside every capsule, a `}` that closes nothing, `satellite.main()` without `capsule` -- refused before anything runs; a shebang first line is allowed | b603f03 | `bytecode/capsule_scopes.cpp` |
| A misspelled word asks "did you mean"; `print(...)`, `'quotes'`, a closing `;` are told what satellite writes | b603f03 | `bytecode/program_check.cpp`, `capsule_reach.cpp` |
| `arguments.cores` = physical cores (12); `arguments.threads`/`.thread` = what satl may create; `arguments.machine.threads`/`.thread` = hardware threads (24) | b603f03, 3fdd50a | `satellite-numbers/machine_facts.hpp`, `words/aliases.tsv` |
| The installer puts `satellite.help/` beside satl; `build_libraries.py` rebuilds on every header a library reads | b603f03 | `satellite_enterprise/install_support/060-install-tree.sh`, `satellite-numbers/build_libraries.py` |
| A capsule parameter takes a value by a declaration's rules (a whole number to a float) | b603f03 | `bytecode/program_walk.cpp` |
| string + any number joins it as its `.string`; a number + text that spells a number adds (`4 + "2"` is 6), other text joins (`4 + "abc"` is `4abc`) | 3fdd50a, 1a913c0 | `satellite_object/satellite_object.cpp`, `string_and_number_add.hpp` |
| Only a checkout holding `.satellite_counts_builds` counts builds; a clone never writes `satellite_config.hpp`, so `git pull` works | 3fdd50a | `config/build_number.py`, `.gitignore` |
| A character with no meaning outside a string or comment is refused by name; an unknown escape is refused | 3fdd50a | `bytecode/capsule_scopes.cpp` (`first_thing_with_no_meaning`) |
| S016 CONFIG_ROW_NOT_UNDERSTOOD: an unknown `config.ini` row is named, the run carries on | c4edcd2 | `structured-library.cpp`, `config/config_file.hpp` |
| `satellite.main` ends with `satellite.return(satellite)` (S103); written anywhere else it quits the whole program, exit 0 (machine code 64 `program_returned`) | c7ec8a1 | `bytecode/program_walk.cpp`, `program_check.cpp`, `machine/thread_stop.hpp` |
| `.upper()`/`.uppercase()`/`.up()` and `.lower()`/`.lowercase()`, every language's letters | c7ec8a1 | `satellite_object/string_case.hpp`, `REGISTRY.satellite` |
| `display(...).center()` / `.centre()` | 6563258 | `bytecode/console_calls.cpp` |
| A statement over any number of lines, and a string over as many as it spans; what the file ends inside is refused where it opened | 353c585 | `bytecode/bytecode_registry.cpp` (`join_statements_across_lines`) |
| A capsule calling itself mid-body a million deep: fresh stack segments under 1 MiB left | 8c8dc35 | `machine/stack_segments.hpp` |
| A number first + text that spells a number adds (`4 + "2"` is 6); a bool joins a string as `true`/`false` with one space | 1a913c0, 0117872 | `satellite_object/satellite_object.cpp` |
| BLOCKS AT THE PROMPT: a block is gathered until its braces close; `{` and a spacesuit's constructor/protected/public are written for the person; a `}` on an indent-only line steps back; capsules and spacesuits are kept for the session, an if/while/for runs (an if waits one line for its else) | d2d6f24 | `satl/session.cpp`, `prompt/line_reader.hpp`, `prompt/editor.cpp`, `bytecode/program_check.cpp` (`check_prompt_statements`), `program_walk.cpp` (`run_prompt_statements`) |
| A block's `{` on its header's line -- `if(x) {`, `} satellite.statement.else {`, `while(...) {` -- moves to the next line's front before lexing, so every statement takes it; line numbers unchanged | d2d6f24 | `bytecode/bytecode_registry.cpp` |

| piece | files | checked by |
|---|---|---|
| **The prototype runner** — loads a .satl, checks include/main/return, runs `satellite.console.display` of a string, number or bool | `satellite/structured-library.cpp`, `satellite/satl/`, `satellite/arguments/`, `satellite/machine/`, `satellite/version/` | `./check.sh` — **41 passed, 0 failed** (§6.6 says how the four were retired) |
| **The author's config** — every `return_arguments_vector()` row loaded into `arguments`; the title lines (VERSION 004 REVISION 04 BUILD nnnn) on `--version`, `--help` and every start; the build number raised by every build | `satellite/config/satellite_config.hpp`, `satellite/config/build_number.py` | check.sh |
| **256 warm threads** — started from `arguments.threads_startup` and parked before the program runs (a requirement for later, the author) | `satellite/threads/startup_threads.*` | check.sh; about 12 ms and 1.7 MB a run |
| **Arithmetic** — `satellite.variable.number n = 34587`, `+ - * / % ^`, comparisons, assignment, `while`, per-capsule frames. Every math sign needs a space on both sides; a touching `/` between digits is a fraction | `satellite/bytecode/expression.*`, `program_check.cpp`, `program_walk.cpp`, `satellite/satellite_variable_number/number_arithmetic.hpp` | `./check.sh` 41/41; 482,465 cases against Python (§6.7) |
| **The 16-bit tokens** — `REGISTRY.satellite` is the 16-bit list; `bytecode_registry` is the program as `std::vector<std::vector<std::bitset<16>>>`, **one row a FILE** — the main `.satl` and every spaceship it includes — built on the warm threads in batches of lines. **The first thing the interpreter builds out of a program** | `REGISTRY.satellite`, `satellite/bytecode/` (`make_token_codes.py` → `token_codes.hpp`, `bytecode_registry.*`), called at `structured-library.cpp:155` | check.sh (32); `experiments/bytecode_registry_checks.cpp` — 19 checks; `--debug` shows `bytecode_registry(built)` on every run |
| **satellite_number** — sign bool + `unsigned long long` limbs, one-limb fast path (no allocation), + - * / %, text, digits, bytes | `satellite/satellite_variable_number/` | `python3 .../check_numbers.py build/number_cases` — 482,465 cases against Python (power and both radixes added 2026-09-16) |
| **satellite.variable.binary** — `b10101010` declares, and displays exactly as written, b and leading zeros (the width is part of the value, 003 DESIGN 8.5). Written without its b it is refused before anything runs: `ERROR: expected b10101010` (the author, 2026-09-16). It keeps a sign (the author, 2026-09-17: *"keep a sign with all of these things"*): `-b0101` displays `-b0101` and is worth -5; `= -1010` is `ERROR: expected -b1010`. Arithmetic and orderings read it by worth and answer a number, which departs on purpose from 8.5's decided-and-unbuilt `+` rulings | `satellite/satellite_variable_binary/satellite_binary_number.hpp`, arm 7 of `satelliteObject`, `program_check.cpp` `binary_is_written_with_b` | `./check.sh` (tests/binary*.satl) |
| **satellite.variable.percentage** — `50%`, `12.5%`, `1000000000000%` (the author, 2026-09-17), 004's first word of its own (`1 6 16`, code 4461, appended through `words/words_004.tsv`). Held as a whole number of 10^-32 percents, rounded half away from zero at the 33rd digit. `200 * 50%` 100, `200 - 50%` 100, `200 + 50%` 300, `200 / 50%` 400, `50% * 50%` 25%, `50% / 4` 12.5%; a non-whole number answer is refused (24) until satellite_float. Written without its % it is refused before anything runs: `ERROR: expected 50%`, and `-50` is `ERROR: expected -50%`. Its sign is a bool held with it, `negative()` (the author, 2026-09-17) | `satellite/satellite_variable_percentage/satellite_percentage.hpp`, arm 8 of `satelliteObject`, `satellite_object/object_percentage.cpp` and nine `*_and_percentage_*` / `percentage_and_*` headers, `percentage_token` in REGISTRY.satellite | `./check.sh` — every line of tests/percentage.satl worked out by Python's decimal module |
| **A string minus a string** — takes away the FIRST occurrence of the right string (the author, 2026-09-17: *"minus takes away the smallest string"*, *"first occurrence"*): `"abcab" - "ab"` is `cab`; nothing to take away leaves the left as it was. A match begins only where a character begins, so a wide character's halves are never matched | `satellite/satellite_object/string_and_string_subtract.hpp`, `str_minus_str.cpp`, the string arm of `satelliteObject::subtract` | `./check.sh` (tests/string_minus.satl, every line from Python's `str.replace(right, '', 1)`) |
| **satellite_string** — the author's character table; 16 bits a character, and a wide character (above U+FFFF, or U+9C40 itself) is 40000 and then its number in two 16-bit units (D3.1, the author 2026-09-17: *"its the 16-bit value for wide"*) | `satellite/satellite_variable_string/` | `.../check_strings16.py` — every case agrees with Python; `build/string_table_check` proves the table |
| **The number index** — every compiled library loaded once at start-up | `satellite-numbers/call_number.*`, `number_row.hpp` | loads all 24 libraries |
| **004's word numbers** — 364 words, first-available numbering, frozen (DESIGN §3.2) | `words/make_words.py` → `words/words.tsv`, `words/satellite_words.hpp` | matched 003's words.def row for row; no duplicates; every parent's children exactly 1..n |
| **satellite_string as char32_t** — strict UTF-8 ↔ char32_t ↔ .sati bit text, `bits_to_cxx_str` | `strings/satellite_string.*` | `python3 strings/check_strings.py` — 30,055 cases agree with Python's UTF-8 codec |
| **satellite_string's 23 words, one library each** (`1 6 1 0` .. `1 6 1 22`), behaviour ported from 003 06 | `strings/string_method.hpp`, `satellite-numbers/<exact word>/<exact word>.satellite.cpp` | `python3 strings/check_string_methods.py` — 44 cases match 003's real satl, plus 4 Unicode |
| **The library builder** — names each .so by its numbers from words.tsv, with make's compiler and flags; a different compiler or flags rebuilds every library; a .so no word names is removed | `satellite-numbers/build_libraries.py` | 24 libraries, 0 warnings |
| **PLAN M0.5 — the build, `satl`, satl-term and the installer** (2026-09-17). The Makefile is an index over `make_support/` fragments, 003's shape; objects under `build/objects/` with real header dependencies; `build/satl` (was `build/satellite-004`, now a link), `build/satellite-numbers/`, `build/satl-term`; no RPATH on anything linked. The command line: `satl`, `--run <file> [words]`, `<file> [words]`, `--repl` (the session, M0.6), `--debug` first, `--version`/`--help` alone, every word after the file the program's (`arguments.program`, `argument_N`, `length`, `session.directory`); anything else 23. A code below 0 or above 254 exits 255 (D0.5.2). A word that is not text is shown escaped in every message. satl-term ported from 003 with 004's title lines and codes. `satellite_enterprise/install.sh --root <folder>` only | `Makefile`, `make_support/`, `satellite/arguments/command_line.*`, `satellite/machine/{exit_status,shown}.hpp`, `satellite/version/title_lines.hpp`, `satl-term/`, `satellite_enterprise/` | `make test`: check.sh (131), `satellite_enterprise/check_install.sh` (14), the three string checks; `SATL=<root>/satl ./check.sh` after an install. Reviewed 2026-09-17 by five lenses with a skeptic each; every confirmed finding fixed, satl-term's tabs checked on a headless compositor (a file tab closes on 0, a prompt tab holds on 14) |
| **`satellite.statement.for`** (2026-09-17, MILESTONES M20.A). C's three parts, divided by semicolons: `satellite.statement.for(satellite.variable.number my_int = 0; my_int < 9; my_int + 1)`. A `for` is `run_while` with two more parts -- the same `evaluate_expression`, the same demand for a bool, the same `run_statements` on the body sharing this body's variables. **The third part carries no `=`** because the loop's own name goes in front of it (the author: *"in every circumstance, we are adding `my_int = ` to the code"*), so it is the ordinary evaluator's answer given to the number -- which is why `* 2`, `- 1`, `^ 2` and `% 7` all work without a branch each. **The step is exactly one of three shapes** -- empty, `<name>++`/`<name>--`, or `<name>` and one of `+ - * / % ^` spaced and then an expression -- and the CHECKER refuses everything else, because the step is the only part that runs AFTER the body: a step the walker cannot use is a loop that half-runs, and `--i` (double unary minus, so `i = i`) ran forever printing 0 and saying nothing. **`++` and `--` are not tokens**: the lexer already writes `i++` as the name and two TOUCHING pluses, and touching is not an operation anywhere in satellite, so the pair is read inside this one bracket and has no meaning outside it -- the author's *"the for loop is the only place where this exists"*, with no registry row minted. `**` was refused by name in the step, because `^` is power (2026-09-16); since INF-1 (SATELLITE_INFINITY.md, 2026-09-18) a spaced `**` is `^`'s second spelling everywhere, the step included, and only a touching `i**2` is refused by name. **The number lives only while the loop runs** and is erased after it (M20.A; satellite.history is M20.B and unbuilt), and the CHECKER forgets it at the same point, so `i` after the loop is refused before the loop has printed. The step's shape is read once per loop, not once per turn | `satellite/bytecode/program_walk.cpp` (`for_header`, `for_step_moves_by`, `run_for`, `run_for_step`), `program_walk.hpp`, `program_check.cpp` | `./check.sh` -- `tests/for_loop.satl` (25 lines of output: `+ 1`, `++`, `--`, `* 2`, an empty step, a for in a for, a for in a while, an if in a for, a method call in the condition) and fifteen refusal fixtures, every shape one asserted to have printed nothing first, with a positive control proving the fixtures really do print before their loop. **Reviewed by a fresh reader** (~144k tokens): the step rule is its find. What it could NOT break, each tried and reported clean: the `for_header` scan against a `;`/`)`/`}`/`*` inside a string, a header across two lines, one with no `)`, one with a third `;`, two fors on one line; `parts.condition - 1`/`parts.step - 1` underflow (unreachable, both are >= 3); the erase of the number on every exit path; **iterator invalidation across the body's own declarations** (`run_for_step` re-finds the name after `run_statements` returns, so none is ever held); `forget_the_finished` against seven nestings and reuse patterns; every `for` shape at `--repl`; and 37 malformed headers, none of which crashed or spun. **Open, and the author's** (M20.A): `satellite.return` inside a loop body does not end the capsule (`while`'s bug too, not new), and 8,000 nested `for`s exhaust the C stack -- a depth bound is not the fix |
| **`satellite.statement.if` and `.else`** (2026-09-17, the author: *"satellite.statement.if is just (condition) { call_to_whatever_runs_code } which we have kinda just built the thing that runs code, right?"*). `run_if` is `run_while` without the loop: the condition through the same `evaluate_expression` and the same demand that it answer a bool, the body through `run_statements` at the code past the `{`, sharing this body's variables. `else` follows the `}`, and `else` written straight onto another `if` chains; a branch the chain has already decided against does not EVALUATE its condition, so a call inside one never happens. An `else` no `if` claimed is refused by the check, looking back one code for the `}`. A condition that is not a bool is refused where it RUNS, exactly as a while's is — the checker does not evaluate | `satellite/bytecode/program_walk.cpp` (`run_if`), `program_check.cpp` | `./check.sh` — `tests/if_else.satl` (if, else, else-if, an if in a while and a while in an if), an if with no body, an else with no if, and `if(5)`; each refusal checked to have printed nothing first where the check catches it |
| **PLAN M0.6, second part — the session, the directory words and the table** (2026-09-17). `satl --repl` reads a line with M0.6's line editor, tokenises it with the lexer that tokenises a `.satl`, judges it with the checker that judges a capsule's body, and walks it with `run_statements` — THE SAME SIX SHAPES AND FAST PATHS A FILE RUNS FROM, no wrapper and no second runner. `{`, `}`, `satellite.include`, `satellite.capsule`, `satellite.return` and `help` are refused BY CODE, so a brace inside a string is text. `satellite.directory.change(d)` `1 18 1`, `list()` `1 18 4` and `list(d)` `1 18 5` are three libraries over one header; `change` answers true or false and is never an error, `list` reads through ONE descriptor (`open` + `fdopendir`, so a path deeper than PATH_MAX still lists) and keeps the three failures apart: `directory_not_found` 28, `not_a_directory` 29, `directory_unreadable` 30 (with the system's reason), `path_holds_a_nul` 31, `interrupted` 130. A line that is exactly `1 18 4` or `1 18 5` draws the table (name, type, permissions, owner, created, modified), decided on the COMPILED row and never on the text. Ctrl-C while typing abandons the line; while a line runs it raises the flag a listing reads between entries; a second press exits 130. From a pipe: no banner, no prompt text, and the status is the first failing line's code; at a terminal `exit` is 0. **A word whose row spells its arguments now lexes**: `satellite.directory.list()` and `list(d)` are two rows over one path, so the lexer matches the path plus the shape of the brackets that follow — which is what every other shaped row (332 of them) has been waiting for | `satellite/satl/session.*`, `satellite/satl/listing.*`, `satellite-numbers/directory_words.hpp` + its three word folders, `satellite/machine/stop_flag.hpp`, `check_typed_line`/`run_typed_line`, `shaped_word_code` in `bytecode_registry.cpp` | `build/directory_cases` (18 cases: the libraries' header with no interpreter, the Ctrl-C flag raised, 20,000 entries) and `satellite/satl/check_session.py` (15 checks: a real `satl --repl` under `pty.fork()`, asserted on the screen), both in check.sh, with the piped refusals beside them. **Owed:** `list()` inside a program answers `not_built_yet` until satellite has a list type (M14); the order is the bytes' and 003's was its character table's (M21); the 100,000-entry race is not run yet |
| **PLAN M0.6, first part — the terminal layer, `satellite/prompt/`** (2026-09-17). 003's `raw_mode`, `keys`, `editor`, `history`, `render` and `line_reader`, ported with PLAN's changes: no 16-byte escape buffer (CSI parameters read as numbers, any length) and no 1,000-line history cap; the width asked on SIGWINCH and each character's cells from `wcwidth` (C.UTF-8, per thread); every write finished, with a length; bracketed paste on while a line is read, a paste read whole before any line of it is answered; from a pipe, `read(2)` into the reader's own buffer, lines split at `\n` only, no prompt text (D0.6.2). Also: a control byte ends a half-read escape sequence (003's bare ESC swallowed the Ctrl-C after it); the prompt and line are drawn through `shown()` (D0.6.3); a draw that only adds at the end writes only the addition. Keys typed while a line runs are thrown away (D0.6.5, 003's) by READING and dropping them, never TCSAFLUSH, which cut pastes in two; a paste begun among them is dropped whole. Raw mode's restore flag is set before the terminal changes and cleared after it is restored, so a signal handler never exits leaving it raw. **Not yet in satl:** the session that runs a line, the directory libraries and the table wait on the author's pseudocode (D0.6.4) | `satellite/prompt/` | `build/prompt_cases` (41 cases, no terminal, and a pty that hangs up) and `satellite/prompt/check_prompt.py` (37 checks: `build/prompt_reader` under `pty.fork()`, asserted on an emulated screen), both in check.sh. Reviewed by a fresh reader: 8 defects, all fixed; 26 mutations of the port and the fixes, every one turns a check red; a renderer fuzz (300 seeds) agrees with the emulated screen; the restore race 0 of 150 twice (48 of 150 before) |
| `satellite.console.display` library `1 5 1` | `satellite-numbers/satellite.console.display/` | `make race` — ×1.065 of std::cout today; ×1.002 with `write`/`put` (confirmed, not yet applied: ERROR.md / PLAN M1) |
| **`satellite.variable.file`, and a program that edits its own source** (2026-09-18, SATELLITE_FILE_OPERATIONS FO-1 to FO-4 and 3.8, for the storyline generator in `/home/madness/code/satl/secure_environment`). A text file is a list of lines on the disk, **counting from 1** (the author: *"all line counts all start at 1"*): `satellite.file.new/open/exists/clear`, 23 methods spelled as a list's (`append insert replace index_of search contains remove_at remove truncate clear size empty first last save read_all close open ok error path exists`), `replace(n, x)` and `replace(a, b)` chosen by the first argument's kind, `f[n]`. A method call stands alone as a statement; `satellite.variable.bool` and `.file` declare; word and method calls take several arguments, and a word call lexes to the row with that many parameters. Changes are saved whole or not at all, appends stream. **Error reports quote the LOADED copy of a file**, so a program can rewrite its own `.satl` while it runs (the author: *"grab a copy of the file, leave the file alone, and run the copy"*). The file words have no library (R10, the author's) | `satellite/satellite_variable_file/`, `satellite/bytecode/file_calls.*`, `expression.cpp`, `program_check.cpp`, `program_walk.cpp`, `bytecode_registry.cpp`, `satellite_object.*`, `source_position.hpp`, `REGISTRY.satellite` (`0x0B07`-`0x0B1D`), `words_004.tsv` (`1 8 6`), machine codes 39-47 | `build/file_cases` (222 cases, ASan/UBSan clean, nine mutants each caught) and 14 program checks in check.sh (`tests/file_*.satl`, `tests/method_statement.satl`); `make test` 242 passed, 0 failed, after a fresh reader's findings were fixed and pinned. **Committed `1547deb`** and re-verified through the real paths first; FO-5 to FO-10 are what is left, and SATELLITE_FILE_OPERATIONS Part 8 orders them |
| **A capsule takes ARGUMENTS** (2026-09-21, for SATELLITE_WINDOW.md WIN-11). Until now the walker ignored a capsule's declared parameters entirely -- every capsule was entered with a fresh empty `VariableTable`, and 004's own first program has been written `satellite.main(satellite.container.list<satellite.variable.string> arguments)` since the beginning with nothing ever bound to it. `capsules_in` now reads the header through the SAME `read_type_shape` a declaration uses, so a parameter keeps its whole `TypeShape` and `satellite.container.list<satellite.variable.string>` is one parameter and not a word followed by rubbish. `run_capsule` binds each argument in the capsule's new frame, measured by the SAME `value_fits` a `satellite.variable` line is measured by. **The walker's own inline capsule arm now calls `run_capsule`**, so there is one reader of "call a capsule" and not two. The CHECKER refuses a wrong count before anything runs, seeds each capsule's declared names with its parameters, and refuses a header it cannot read **in a pass of its own** -- the `CapsuleTable` is an unordered_map, so inside the body loop whether a person saw "a parameter is a TYPE and then a name" or the far more confusing "show takes 0 arguments, and was given 1" depended on which capsule the hash put first. **`satellite.main`'s parameters are NOT seeded**, because `run_main` is handed nothing and binds nothing: seeding them moved the refusal from the checker to the walker, and the program printed its first line before stopping. **Owed:** binding `satellite.main`'s declared list to the program's own words, which exist today only as the settings `arguments.argument_1`, `arguments.length` and the rest | `satellite/bytecode/program_walk.*` (`CapsuleParameter`, `capsules_in`, `run_capsule`), `program_check.cpp` | `./check.sh` — nine rows: arguments arriving, an expression as an argument, a wrong count refused before anything runs, a type that does not fit, a header with no name after its type, and `satellite.main`'s own parameter still refused by the checker |
| **A label, and the recipe every widget after it follows** (2026-09-21, GTK_AND_NO_DEPENDENCIES.md GTK-1). `satellite.window.label("a line of text")` is `1 27 3`, and it is the SECOND piece satellite ever had -- a window and a button were the whole vocabulary for a day. A label was chosen first because it is the cheapest widget that is not a button: it draws and it answers nobody, so it paid GTK-0's whole bill without also arguing about signals. **What it paid for, once, for the eighteen milestones behind it:** the two-way `?:` in `piece_name()` became `kPieceNames`, a table the compiler will not let a `Piece` be added to without a row (`static_assert`) or without a widget (a `switch` over every enumerator with no `default`); the three hand-written `window_*_word()` functions became ONE `kWords` table that `is_window_word`, the arity, the wrong-count sentence and `.append`'s own refusal are all written from; `program_check.cpp`'s hand-typed list of what a window does became `window_methods_are()`, asked of the file that knows; and `satellite_window.cpp` -- 274 lines against the author's 300 -- split, with every piece that goes INSIDE a window moving to `window_pieces.cpp`. **`.text` is the new method** (`0x0B2B`), read bare and written with brackets, the pair `.title` already was; a window asked for `.text` is told to use `.title` rather than given a second name for one thing. | `words/words_004.tsv`, `REGISTRY.satellite`, `satellite/satellite_variable_window/satellite_window.hpp`, `window_pieces.cpp` (new), `bytecode/window_calls.*`, `program_check.cpp`, `satellite_object.cpp`, `make_support/047-window.mk` | `./check.sh` — 391 passed, 0 failed (was 382); and proved on a compositor of its own: an 800x600 window, the label appended, `.text` read, `.text("...")` written and read back, a button pressed and the window closed, exit 0 |
| **A person types, and the first value read back OUT of GTK** (2026-09-21, GTK_AND_NO_DEPENDENCIES.md GTK-2). `satellite.window.text_box("")` is one line and `satellite.window.text_area("")` is many -- TWO pieces and not one with a flag, because GTK makes them two widgets and a text area's words live in a `GtkTextBuffer` while a text box's live in the widget. **Everything satellite had read until now it already held**; what a person typed is GTK's, so `.text` crosses to the desk, copies what is there and answers that -- `on_the_desk()` already waited for the job it posted, so bringing a value back was the mechanism that was there and had never been used in this direction. **THE FIRST SHAPE WAS WRONG AND RUNNING IT SAID SO**: it rescued what was typed when the window went away, and printed five `Gtk-CRITICAL` assertion failures. GTK's own source says why it can never work -- `gtk_window_dispose` unparents the child BEFORE the dispose that emits the window's `destroy`, and `gtk_entry_dispose` clears its text and `gtk_text_view_dispose` drops its buffer before the dispose that emits the PIECE's. **There is no moment in a teardown at which a widget's words can still be asked for.** So a closed button or label answers (its words were always ours) and a closed text box is REFUSED with `S505 WINDOW_IS_CLOSED` and a sentence naming when to read it. That choice is also what keeps the module race-free: `text` has one writer and it is the interpreter's thread. | `satellite/satellite_variable_window/window_pieces.cpp`, `window_desk.cpp`, `satellite_window.hpp`, `bytecode/window_calls.cpp`, `words/words_004.tsv` | `./check.sh` — 397 passed, 0 failed (was 391); and on a compositor of its own: both pieces appended, `.text` read through `GtkEditable` and through a `GtkTextBuffer`, written and read again, then a closed label answering while a closed text box refused |
| **The window's widgets and satl's own console** (2026-09-21 to 22, GTK_AND_NO_DEPENDENCIES.md GTK-1 to GTK-17). Every widget -- label, text box and area, checkbox, switch, slider, number box, progress, choice, row/column/grid, picture, scroll, frame, split, tabs, menu, canvas, message and ask, the keyboard, a clock -- and `satellite.console.new(title, w, h)`, a window with a VTE terminal in it. satl opens its own console when nothing gave it one (WIN-9, `7656c88`), and every bare `make` installs 004 into `~/.satl` (`e9445ad`) | `satellite/satellite_variable_window/`, `satellite/bytecode/window_*.cpp` | check.sh, and each GTK-n in GTK_AND_NO_DEPENDENCIES.md names its own proof |
| **`satellite.namespace`: a file is a namespace** (2026-09-22, `4335c75`, revision 07). Every included file is reached by its own name (`other.greet()`), `satellite.namespace name { }` (and `satellite.space`) makes one inside a file, and a program's own capsules are its own | `satellite/bytecode/capsule_scopes.*`, `capsule_reach.cpp` | check.sh |
| **The float, the hex, the colour and the fraction** (2026-09-22: `c72b018`, `91e50c4`, `6becbf1`, `0d55946`, merged `0708501` and `6f01076`). `12.34` is a float held to `arguments.float.decimal` places; `x1F` a hex shown as written; `x00FF00` a colour of six hex digits that can be see-through (`c = x000000, 50`); `1/3` a fraction compared by value | `satellite/satellite_variable_{float,hex,color,fraction}/`, `satellite/bytecode/*_values.cpp` | check.sh, tests/color*.satl, tests/fraction*.satl |
| **`satellite.variable.arguments` and `satellite.help`** (2026-09-22, `8b8b66c`, `044534e`). `main` is handed every row satl holds -- the command line, the machine's facts, the config rows -- as one variable; `satellite.help(topic)` reads `satellite.help/<topic>/help_text.txt`, every example run | `satellite/bytecode/main_arguments.*`, `satellite.help/` | check.sh, SATELLITE_ARGUMENTS.md |
| **Spacesuits** (2026-09-22, `b6b2a98`, MILESTONES M8). Objects made by declaring them, `satellite.protected` / `satellite.public`, `satellite.constructor(args)`, a spacesuit inside a spacesuit, one extending another, capsules that answer (`satellite.returns(T)`) | `satellite/bytecode/suit_*.cpp`, `satellite/satellite_object/satellite_spacesuit.hpp` | tests/spacesuits.satl and 32 rows in check.sh |
| **`satellite.library` values** (2026-09-23, `c459c9c`). `satellite.library.x = <one literal>` at a file's top, read by every capsule of it and as `satellite.library.<file>.x` by a file that includes it; changed by nothing -- a write is S250, machine code 54, before anything runs. Values, not globals (the author: *"globals don't work, but satellite.library does work"*) | `satellite/bytecode/library_values.*` | tests/library.satl, tests/library_settings.satl |
| **`satellite.container.list` takes what 003 had** (2026-09-23, `7263d2c`). `satellite.container.list()` -- 473 lines of the author's programs -- in a capsule or as a spacesuit's field; `.sum .max .min .join(separator) .reserve(n)`. Lists of lists and `grid[x][y]` were already built. A number too big for a machine word is past the end of every list (`a.truncate(10^29)` used to empty it) | `satellite/bytecode/container_calls.*`, REGISTRY `0x0B52`-`0x0B56` | tests/list_from_003.satl and 18 rows in check.sh |
| **Colour on the console** (2026-09-23, `dfdc64b`). 003's display options -- `end= foreground= background= bold= italic=`, the same bytes 003 writes, plain into a pipe, NO_COLOR drops the colours -- and 003's screen words `satellite.console.input() / input(prompt)` (end of input S830, machine code 55), `.width`, `.height`, `.clear()`, `.home()`. And every spelling 003 refused: `"OK".foreground(c)`, `s.background(c)`, `satellite.console.foreground(c)` for every later line, `satellite.terminal.foreground(c)` for the terminal's own colours (OSC 10/11, put back at exit, Ctrl-C and crashes). `name=value` options are a grammar now: the lexer counts plain arguments only | `satellite/bytecode/console_style.*`, `console_calls.*`, `satellite/machine/input_source.hpp` | tests/console_from_003.satl run into a file and under a pty, and 28 rows in check.sh |
| **The arguments are written** (2026-09-23, `9c14577`). `any_name.some_var = some_value` in main (the author's spelling): a name of the program's own is added, overwritten, found by `[]` and `.contains`, can hold a container it changes in place (`args.l.append(5)`, `args.l[1] = 7`), and is shown after satl's rows. A row satl holds (username, memory.total, infinity, argument_7) or a name inside a row (`args.l.size`) is refused 35 -- by the checker, before anything runs, when it is satl's. `access` writes through to config.ini. The rows are filed as every index key is, which fixed a use-after-free the review found (a removed row's name pointed past the end) | `satellite/bytecode/main_arguments.*`, `run_argument_assignment` in program_walk.cpp, `after_an_argument` in expression.cpp | tests/arguments_written.satl, tests/arguments_not_written.satl and 9 rows in check.sh |
| **`arguments.cpu.architecture` and `arguments.cpu.features`** (2026-09-23, `55532ff`). 003's word -- `haswell` when the processor runs the whole x86-64-v3 set, `baseline` when not -- and one LIST of every instruction set the processor and the kernel allow (23 here, no AVX-512). The first list row: `ArgumentKind::list` | `satellite/arguments/cpu_facts.hpp` | a check.sh row against /proc/cpuinfo's own flags |
| **satl for every processor, and `satl-cpu-level`** (2026-09-23, `0d875d1`, MILESTONES M37). `make cpus` builds satl and its libraries for 53 processors -- every distinct instruction set among clang 24's 64-bit targets -- into `build/cpu/<processor>/`; `make CPU=<name>` builds one. `build/satl-cpu-level` names the best build a machine can run, by what each build was compiled to need and never by name (haswell here). A processor's build never raises the build number or installs | `make_support/055-cpus.mk`, `satellite/cpu_level/cpu_level.cpp` | a check.sh row for the chooser; the whole suite, 786 of 786, against `build/cpu/haswell/satl` |
| **`satellite.log` and names** (2026-09-25, `10bfb9c`, MILESTONES M5). Every report satl makes -- refusal, notice, `report_error` -- is kept once as DESIGN §7's `[entry]` (the author's `write_entry`) in `arguments.log_path`, `~/.satl/satellite.log`, one `write(2)` under one mutex; S020 NUMBER_TAKEN_AS_TEXT is the author's 2026-09-16 warning, in the log alone, with its line. A variable, object or parameter named for a capsule or spacesuit in reach is S202 before anything runs (none of the author's 400 programs does it); a name that is not one says what a name is; a report escapes every field, so a name holding ESC [ 2 J no longer clears the screen | `satellite/machine/satellite_log.hpp`, `critical_report.hpp`, `s_codes.hpp`, `bytecode/capsule_reach.cpp` | check.sh: 19 rows -- a clean run writes nothing, one refusal one entry, the path from config.ini, an unwritable log, hostile bytes, S020 once a line, S724's line, and the six a fresh reader broke (a forged marker, a count on a stopped run, a field's line, the prompt, a tab, the speed of a repeat) |

**Waiting on other types** (answer machine code 14 `not_built_yet`):
`to_number`, `number`, `binary`, `hex`, and `string(x)` for anything but a string.
(`.bin`, `.number`, `.string` and `.hex` on a declared name run through the object
model, a binary included; these are the numbered LIBRARIES of the same names.)

**Build and check everything:**

```
make                                   # build/satl + every numbered library + build/satl-term; raises the build number
make test                              # check.sh and the three string checks, every one run even when one fails
./check.sh                             # the checks alone; SATL=<folder>/satl ./check.sh checks an installed satl
make build/string_cases && python3 strings/check_strings.py   # char32_t conversion against Python
make build/string_methods && python3 strings/check_string_methods.py   # against 003's satl
build/satl --version                   # THE SATELLITE PROGRAMMING LANGUAGE / VERSION 004 REVISION 04 BUILD nnnn
sh satellite_enterprise/install.sh --root <folder>   # satl, satellite-numbers/ and satl-term, proven by a run
python3 words/make_words.py            # regenerate the word table (needs old_versions/second_satellite/satl)
```

## 2. Decided (by the author unless marked)

**2026-09-25, his answers to SCRATCH.md/NEW_ERROR_LIST.md, in his words** (all built; section 1):
- A1: *"we need to auto convert here for the user into string, so when we have a string and we add
  a number to it, it has to auto convert"* -- and then *"we need 4 + "2" to return the number 6"*.
- A2: *"I want arguments.threads or arguments.thread = how many the interpreter can create, and
  arguments.machine.thread = how many phyiscal threads exist on the machine"*.
- A3: *"keep the build number in an untracked file, and make on this machine will update it, but
  not on other machines"*.
- A4: *"it should accept anything that is valid satellite regardless of how many spaces or lines are
  in it, we should accept strings that span 90 lines"*.
- A5: *"Let's not accept characters that have no meaning"*.
- A6: *"refuse an escape that is unknown"*, and `\\` writes a backslash.
- A7: *"let's refuse a main that doesnt have satellite.return(satellite) as the last line, but still
  allow the user to satellite.return(satellite) to quit the interpreter from anywhere"*.
- A8: a capsule calling itself mid-body must not crash -- *"we could build code that ONLY applies to
  this special circumstance so the interpreter doesnt' crash"* (it reverses 2026-09-22's "leave it
  broken").
- A9: an unknown config row gets its own code -- he offered SC01 or any free one; it is S016.
- *"we need str.upper() and str.uppercase() and str.up() a str.lower() and str.lowercase()"* and
  *"a .center() that you can attach to satellite.console.display("something").center()"*, centred
  *"just for that console at that time"*.

- **Version 004 revision 04, build numbers from 0050** (2026-09-15), raised by every
  build. 003 07 is archived in `old_versions/second_satellite/`.
- **The sources live under `satellite/`,** one folder a subject (2026-09-15).
- **Numbering:** 003 06's words minus the 7 GUI words, renumbered once
  first-available, frozen since. Next free: `satellite` 1 25, `satellite.variable`
  1 6 16, string methods 1 6 1 23. (Author delegated the choice.)
- **Every word is a library** in a folder named by the exact word, arguments
  included (`satellite.variable.string.find(x)`), built as `<numbers>.so`.
- **A string is 16 bits a character** (the author, 2026-09-15). A character above
  U+FFFF is 40000 and then its number in two 16-bit units, and so is U+9C40, whose own
  number is 40000 (D3.1, 2026-09-17). Codes 0–127 are every ASCII character once, in
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
- **`satellite.returns` is taken out** (the author, 2026-09-24: *"I never asked at any
  time to build the code satellite.returns"*, and *"for 003, I also never requested it
  then either"*). A capsule answers whatever its `satellite.return(...)` hands back, and
  nothing after its brackets says what: only line ends and comments stand between its `)`
  and its `{`, and the word itself is refused there by name with the change to make.
  Measured first: every program in the tree and the checker's verdict on all 32 of his,
  with and without it, came out identical; the only differences were its own five
  refusals. Its row stays in words.tsv (append-only numbering). 003 keeps it.

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
  one 32-bit integer. The lexer writes a character above U+FFFF as `wide_token` and two
  codes, and `wide_run_32_token` is retired; a string holds it the same way (2026-09-17).
- **D9.1–D9.2** polymorph: what "re-included into the individual capsules" means,
  what `args` are passed to. ~~**D9.3**~~ **ANSWERED 2026-09-16:** *"a class declared
  twice is an ERROR: name collision"*.
- ~~**D11.1**~~ **ANSWERED 2026-09-16:** *"we answer what we can, and give an error on what we
  can't"*. An infinity carries a `satellite_float` multiplier -- `infinity + infinity` is
  `infinityx2`, `infinity - 50%` is `infinityx0.5` -- one float for going up or down, kept to
  `arguments.infinity` digits (default 4096) and displayed rounded to
  `arguments.infinity_display` digits (default 32); both configurable (MILESTONES M11).
- **D12.1** the parallel-group syntax in the numbered file.
- ~~**Adopt TBB for the runners?**~~ **ANSWERED 2026-09-16: NO.** The 1024 threads convert
  one line each and stay warm, doing nothing yet; *"we are doing something different later"*.
- ~~**The leading-slash rule**~~ **ANSWERED 2026-09-16:** program root first, then the
  filesystem root, then file not found; every included file's cwd is its own directory.
- ~~**A number where a string is expected**~~ **ANSWERED 2026-09-16:** converted to the
  string and run, *"obviously the programmer meant convert to string"*, with a warning
  in satellite.log (M5).
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
- **an 8-bit string holds ASCII only** (decided 2026-09-16); 16-bit holds everything,
  a character above U+FFFF as 40000 and two units (D3.1). The 8-bit path is not built
  yet.
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
1. **ANSWERED 2026-09-16 (§4): program root, then filesystem root, then file not found.**
   What this item said before the ruling: **the leading-slash rule is a FALLBACK, not a rule.** `find_file()` tries the
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

1. ~~**NOTHING CAN DECLARE A SPACESUIT YET.**~~ **BUILT 2026-09-22** (MILESTONES
   M8): a spacesuit is a third scope kind in the scan (`suit_scan.cpp`), its
   objects share one layout (`suit_layout.hpp`), are made by `suit_run.cpp` and
   called through `capsule_calls.cpp`; 003's rules are ported, and
   `tests/spacesuits.satl` plus 32 rows in check.sh hold them (`b6b2a98`).
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
