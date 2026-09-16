# satellite-004 — PROGRESS.md

**satellite 004 revision 04** (the build number is in `satellite/config/satellite_config.hpp`,
and `satl --version` shows it). Where the work stands, 2026-09-15. Read this first
after a break; then PLAN.md (the order of work), DESIGN.md (the standards and
every measurement) and ERROR.md (every known error).

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
| **The prototype runner** — loads a .satl, checks include/main/return, runs `satellite.console.display` of a string, number or bool | `satellite/structured-library.cpp`, `satellite/satl/`, `satellite/arguments/`, `satellite/machine/`, `satellite/version/` | `./check.sh` — 32 passed |
| **The author's config** — every `return_arguments_vector()` row loaded into `arguments`; the title lines (VERSION 004 REVISION 04 BUILD nnnn) on `--version`, `--help` and every start; the build number raised by every build | `satellite/config/satellite_config.hpp`, `satellite/config/build_number.py` | check.sh |
| **256 warm threads** — started from `arguments.threads_startup` and parked before the program runs (a requirement for later, the author) | `satellite/threads/startup_threads.*` | check.sh; about 12 ms and 1.7 MB a run |
| **satellite_number** — sign bool + `unsigned long long` limbs, one-limb fast path (no allocation), + - * / %, text, digits, bytes | `satellite/satellite_variable_number/` | `python3 .../check_numbers.py build/number_cases` — 477,253 cases against Python |
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
./check.sh                             # the runner: 32 checks
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

- **D0.1** which value of `arguments.satc` / `satb` means "never build".
- **D1.1** with `satc = 0`, run line 1 before the whole file is converted?
- **D3.1** does satellite_string become 32 bits a character everywhere, or only in
  the .sati — and store the .sati bits as binary (4 bytes a character) instead of
  `0`/`1` text (32 bytes a character, ×20 the UTF-8, measured)?
- **D9.1–D9.3** polymorph: what "re-included into the individual capsules" means,
  what `args` are passed to, a class declared twice.
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
- **the tokens move to 16 bits** (the author, 2026-09-16), so `REGISTRY.satellite`
  becomes that list: the stored program's codes are 16-bit, which leaves room for
  every token the language will need instead of the 70 free 8-bit codes. The
  characters keep 0-127 in the registry's order at every width.
- **an 8-bit string holds ASCII only** (decided 2026-09-16); 16-bit holds up to
  U+FFFF, 32-bit everything. The 8-bit path is not built yet: the committed
  string chooses between 16 and 32.
- **the races miss ×1.05 in places** (ERROR.md §5), and the four string words
  waiting on satellite_number (`to_number`, `number`, `binary`, `hex`) are not
  built.

Then the prompt (PLAN M0.5 → M0.6 → M0.7). The prototype's defects (ERROR.md §1)
are PLAN M1.

## 6. Other notes

- `POLYMORPH/M1.md`–`M7.md` (top folder, uncommitted) hold the earlier
  polymorph discussion; `POLYMORPH/M7.md` still says 342 words — it is 371 in 003,
  364 in 004.
- `polymorph/test.cpp` (top folder, uncommitted) is the author's timing test.
- The author wants notes at the top of every file, and wants anything
  questionable questioned.
