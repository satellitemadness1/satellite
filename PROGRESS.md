# satellite-004 — PROGRESS.md

**satellite 004 revision 02.** Where the work stands, 2026-09-15. Read this first
after a break; then PLAN.md (the order of work), DESIGN.md (the standards and
every measurement) and ERROR.md (every known error).

**2026-09-15: satellite 004 is the top of the repository.** satellite 003 revision 07
moved, unchanged, to `old_versions/second_satellite/` (tag `satellite-003-revision-07`,
branch `archive/satellite-003-revision-07`). The next milestone is PLAN M0.5: port
`make_support/` and the installer.

---

## 1. What is built and checked

| piece | files | checked by |
|---|---|---|
| **The prototype runner** — loads a .satl, checks include/main/return, runs `satellite.console.display` of a string, number or bool | `structured-library.cpp`, `satl_file.*`, `arguments.*`, `machine_state.*`, `machine_codes.hpp`, `version.hpp` | `./check.sh` — 16 passed |
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
cd satellite-004
make                                   # interpreter + every numbered library
./check.sh                             # the runner: 16 checks
make build/string_cases && python3 strings/check_strings.py   # char32_t conversion against Python
make build/string_methods && python3 strings/check_string_methods.py   # against 003's satl
build/satellite-004 --version          # satellite 004 revision 02
python3 words/make_words.py            # regenerate the word table (needs old_versions/second_satellite/satl)
```

## 2. Decided (by the author unless marked)

- **Version 004 revision 02** (2026-09-15). 003 07 is archived in `old_versions/second_satellite/`.
- **Numbering:** 003 06's words minus the 7 GUI words, renumbered once
  first-available, frozen since. Next free: `satellite` 1 25, `satellite.variable`
  1 6 16, string methods 1 6 1 23. (Author delegated the choice.)
- **Every word is a library** in a folder named by the exact word, arguments
  included (`satellite.variable.string.find(x)`), built as `<numbers>.so`.
- **Strings are 32 bits a character.** Translating satellite into other languages:
  dropped.
- **Machine codes 0–19** in `machine_codes.hpp`; a code is added to the list
  before it is used.
- **Config values are quoted text, used as ceilings** (DESIGN §1).
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

The author's order (2026-09-14): **satellite_string (done) → satellite_number →
spacesuits**, because nothing can be tested without them. So next is PLAN M4,
`satellite_number` (DESIGN §4): limbs of `unsigned long long`, a sign bool, digit
count and byte size as satellite_numbers, small numbers inline. Then the four
string words waiting on it. The prototype's defects (ERROR.md §1) are PLAN M1.

## 6. Other notes

- `POLYMORPH/M1.md`–`M7.md` (top folder, uncommitted) hold the earlier
  polymorph discussion; `POLYMORPH/M7.md` still says 342 words — it is 371 in 003,
  364 in 004.
- `polymorph/test.cpp` (top folder, uncommitted) is the author's timing test.
- The author wants notes at the top of every file, and wants anything
  questionable questioned.
