# Session state — 2026-08-24, later

Written so this survives a `/clear`. Everything below is in the working tree.
`make test` is 16/16 with zero failures as of the last run recorded here.

## LANDED AND TESTED

**view_forge** (`/home/madness/code/satl/view_forge`, not this repo)
- 501 -> 681 objects. Four declaration files were complete and never wired in:
  `chapters` (70), `krieg_war` (40), `necrons_deep` (40), `chaos_champions` (30).
  Added the includes and the load blocks. 681 == the `call_set_object_id(` count.
- `view_type` gained six getters; the forge now holds a `type_library`, indexes it
  ONCE into a `satellite.container.map` (43 lookups, not 29,283), and renders the
  type description under every object.
- Added `DOCUMENT` (44) and `SPACE_AGENCY` (45); retyped two `HISTORY` objects to
  `HISTORICAL_EVENT`, which already had 23 users. All 40 object types now resolve.
- Fixed 35 phantom render entries: the forge looped to `.length()` (which counts
  the `{"UNSET","UNSET"}` placeholders) instead of the object's own size counters.
  Added four size getters. `get_information`'s own narration had said the counter
  was what separates real data from placeholders.
- Two display bugs: `+ 1` concatenating ("AT POINT 21" for 3) and `.size()` where
  `.length()` was meant ("LENGTH=50" for "JAPAN").
- STILL OPEN there: three duplicate object names (`COLD WAR`, `NATO`,
  `UNITED NATIONS`) - and the search loop has no `break`, so the LAST duplicate
  wins and the other is unreachable. Also `ethnic_group`/`occupation`/`nicknames`
  are still dead fields. `plans/missing.txt` has stale notes (highest id is 2249
  not 1078; 40 type strings in use not ~23).

**satellite.help topics** - `random` `fast` `normal` `ultra` `wide`, bare and quoted.
- `help_for_topic()` in `src/evaluator/help.cpp` is the one table. Normalisation
  strips `satellite.statement.`, `satellite.random.`, `random.` so every spelling
  lands on one entry (verified: all three `ultra` spellings return 1270 bytes).
- Bare form: `src/environment/names.cpp` `resolve_call` suppresses ONLY the
  "unknown variable" complaint for a bare path argument to `satellite.help`, after
  every scope the user owns said no. §1 holds - verified a local `ultra`, a capsule
  `random()`, a global, and a real typo all still behave correctly.
- Evaluator side matches on the ARGUMENT EXPRESSION before `eval_args`, the same
  mechanism `display(100ms)` uses (`src/evaluator/expr.cpp`).

**-O3** - `OPT ?= -O3`; the 15 test recipes moved off literal `-O2` to `$(OPT)`.
The TSAN binary deliberately stays `-O1 -g`. Clean rebuild 10.7s, zero warnings.

**--version** on both binaries, version 002 revision 01.
- `src/system_facts/version.hpp` (NEW FILE, untracked - `git add` it).
- One source of truth: `--version`, the REPL banner and `satellite.help()` all read
  `version_line()`. The banner had said "satellite 0.1" until now.
- `VERSION_DEFS` is on three recipes (`main.o`, `window.o`, `help.o`) and NOT in
  CXXFLAGS - in CXXFLAGS the per-second build stamp would rewrite
  `.cxxflags-stamp` and rebuild all 43 objects on every make, forever.
- `SOURCE_DATE_EPOCH` is honoured (verified: stamps 2023-11-14 for 1700000000), so
  the `Architecture: any` .debs stay reproducible.
- Man pages updated to `"satellite 002"`, and `satl.1` documents `-V/--version`.
- FIXED A REAL BUG: `version.hpp` was in no rule at all, so editing it rebuilt
  nothing. That is `format.def`'s defect from §17. Now a prerequisite of exactly
  those three recipes - verified: touch it, 3 objects rebuild; second make, 0.

**LLVM_BIN -> `$(HOME)/opt/clang-24-2/bin`** (Makefile:67).
- Same git revision (3c2eaf39) as clang-24, but clang-24 has NO compiler-rt and
  clang-24-2 ships it. Verified with the Makefile's own TSAN probe by hand: yes for
  clang-24-2, NO for clang-24, yes for /usr/bin/clang++ and c++.
- So `TSAN_CXX` now resolves to `$(CXX)` instead of falling back. Corrected two
  comment blocks that asserted the old state as **verified**.
- NOTE: clang-24-2 is root-owned (installed with sudo). Fine to build against; a
  later non-sudo `ninja install` into that prefix will fail on permissions.

**REPL multi-line entry** (`src/programs/main.cpp`, `src/interpreter/interp.{hpp,cpp}`)
- Typing a capsule/spacesuit/statement head auto-supplies `{` and reads indented
  continuation lines until the matching `}`. `scan_block()` counts braces over
  TOKENS (a `{` in a string literal is not a brace) and lives in `interp.*` so it is
  testable without gtk.
- The printed indentation is stored INTO the source, not just echoed - that is what
  keeps an error caret (a byte offset) under the character the user sees.
- Lines join with real newlines, because `expect_statement_end` compares token line
  numbers.
- Ctrl-D or `:cancel` inside a block abandons it and reports how many lines went.
- **§16's open question is answered**: a capsule typed at the prompt survives to the
  next line. `resolve()` gained an optional `inherited` table
  (`src/environment/{env.hpp,env_internal.hpp,run.cpp}`), merged AFTER this
  program's own names (so a redefinition wins) and BEFORE every pass that consults
  the tables (so a spacesuit TYPE checked during pass 3 is found).
  - Merging TABLES not source text, so `line 1` still means the line under the cursor.
  - `borrowed` guards the two suit passes: they STAMP SLOTS into AST nodes, and
    re-walking an inherited suit overwrites slots live instances are using. Pass 4
    never had this bug because it iterates `program.items`.
  - Opt-in (`session` bool, default false) because five test files drive
    `run_source` with `fresh_ns()` and would otherwise leak declarations.
- Regression tests added to `src/interpreter/interp_test.cpp`.

## STAGED, NOT YET INTEGRATED

`/tmp/claude-1000/.../scratchpad/TOPICS_CXX.txt` holds ready-to-paste C++ for five
more help topics: `if` (68 lines), `while` (57), `for` (46), `else` (30),
`network` (87). Generator: `scratchpad/mk_topics.py`.

They were NOT pasted because a subagent was mid-edit in `src/evaluator/help.cpp`.

TO INTEGRATE:
1. Paste the five blocks into `help_for_topic()` in `src/evaluator/help.cpp`.
2. Add `"satellite."` as the LAST entry of `kPrefixes` (longest-first matters, so
   `satellite.statement.` and `satellite.random.` must stay ahead of it). That makes
   `satellite.help("satellite.network")` and `("network")` one entry.
3. Add the topic names to the `topics` block in `help_overview()`.
4. NOTE: the control-flow topics are reachable QUOTED ONLY.
   `satellite.help(satellite.statement.if)` cannot parse - §5 dispatches on segment
   1 and `statement` has its own parse rule, so the parser sees the start of an if
   STATEMENT. `satellite.help("satellite.statement.if")` is fine, for §19.1's
   reason: a string literal was never a name.

Content came from a workflow: 4 drafts, each followed by an adversarial checker
that re-ran every claim against `./satl`. The checkers found **53 problems**.
Full uncondensed drafts (656 lines) are in `scratchpad/topics/*.txt` if more
detail is ever wanted.

## PLANNED, NOT BUILT

**`design/20-networking.md`** (NEW, 366 lines) - the whole networking plan, added
to DESIGN.md's index. Covers: client vs server as different objects; the wire
format for satellite values; what is REFUSED (`file`/`window`/`thread` handles,
because a descriptor means something to one kernel only); the live-escape problem
for strings (`\home` etc. are expanded at decode time, so send the CODES and let
the receiver expand its own - same call §8.6 made for map keys, and the other way
leaks the sender's home directory); numbers as decimal digits with §17.3's
continuation bit; sharing and cycles via the seen-set §8.7 already uses; spacesuit
type descriptors verified rather than trusted; and the build order.

The network recon agent finished and CORRECTED the first draft in four places,
all now folded in:
- ONE type, not three. `matches()` is exact-name equality and the only subtype
  relation is spacesuit-only `is_a()`, so a capsule declared
  `satellite.variable.http` could NEVER accept an https value, permanently.
  Constructors survive verbatim; only the type word collapses. §18's three
  random tiers onto one number type are the precedent.
- TLS cost MEASURED, not assumed: 6 -> 9 objects and +1.0 ms (+40%) with
  OpenSSL; 6 -> 12 and +1.79 ms (+71%) with GnuTLS, against a 2.520 ms
  baseline. That is 40% of everything §9's binary split bought.
- `open`, not `new` - reuses word 31, zero new ids, and names the act the way
  every other constructor does. COORDINATE WITH `satellite.thread.new()`
  before either feature mints that word.
- `satellite.network.https(port)` IS NOT A CONSTRUCTIBLE SERVER. A port is not
  a certificate. Either it is a client (and takes a host) or it needs two more
  filenames. The three constructors cannot be made symmetric.
Also: next free format.def id was 100 when the recon ran and is **102** now -
the file API took 100 (`new`) and 101 (`clear`). Not the 80 §17's prose claims;
three `format_test` tripwires must be bumped in the same commit, and the
selector-range one needs a THIRD range, not a widened bound. And
`satellite.variable.network n` already declares a nil today, because
check_type validates a type's space and not its name.

KEY BLOCKERS RECORDED THERE: `https` needs TLS, which breaks §9's six-shared-object
property; §16's native-module mechanism (M7) is the designed fix and IS NOT BUILT.
Plain `http` and raw TCP need nothing new. The server half also wants threads.

**Threading** - full implementation plan at `scratchpad/threading_plan.md` (67KB,
from a 6-agent workflow). Three findings that matter most:
1. `satellite.thread.new(worker(1,2))` RUNS `worker` today at the call site -
   arguments are reduced before the module path is examined. Must intercept on the
   argument EXPRESSION (the `display(100ms)` mechanism).
2. §10's "the compile errors are the feature" is only HALF TRUE. Only two
   exhaustive `std::visit` sites stop the build; everything else is
   `switch(index())` with a `default:` or a `get_if` chain and will silently accept
   a tenth type. The plan lists both halves.
3. `Evaluator` has an implicit copy constructor, so `Evaluator worker = *this;`
   compiles and copies `current_` - a raw pointer into the parent's live C++ stack.
   Delete copy and move explicitly.
Also: a worker's Evaluator must be constructed ON the worker thread, because
`read_max_depth()` derives the ceiling from the calling thread's stack.
Also: `std::atomic<shared_ptr<T>>` measured `is_lock_free() == 0` on this
toolchain - libstdc++ uses an internal spinlock, so the docs' "lock-free reads"
means "no torn read, no user-visible mutex", not lock-freedom.

`.count()` (thread elapsed seconds) is NOT yet built. Two notes for whoever does:
- §18 already settled the clock with measurements: `steady_clock`, never
  `high_resolution_clock` (which is `system_clock`, `is_steady` false). For an
  ELAPSED time a non-steady clock can be stepped backwards by NTP and return a
  negative duration. The user asked for `high_precision_clock`; the concern was
  raised and NOT answered. Default to `steady_clock` and say so.
- It returns a `satellite.variable.number`, not a float - there is no float. ns/10^9
  is division by a power of ten, which §8.1 makes EXACT.

## SUBAGENTS THAT WERE STILL RUNNING AT /clear

- **file API** (`satellite.file.new`, `.clear()`, read+write, auto-open) - was
  actively editing `value.hpp`, `methods.cpp`, `modules.cpp` and will likely touch
  `help.cpp` and `eval_test.cpp`. CHECK ITS WORK AND RUN `make test` BEFORE
  BUILDING ON IT.
- **network design recon** - read-only, confirming the M7/six-object blockers.

## DECIDED BY THE USER, 2026-08-24

1. **`.count()` uses `high_resolution_clock`.** The concern was raised twice -
   §18 settled on `steady_clock` with measurements, and for an ELAPSED time a
   non-steady clock can be stepped backwards by NTP and hand back a negative
   duration - and the user reaffirmed `high_resolution_clock`. That is the
   decision; build it that way. Worth a comment at the site recording that the
   trade was made knowingly, so §18's note and the code do not read as though
   one of them is a mistake.

2. **`satellite.thread.new()` keeps the word `new`,** and it is NOT a conflict
   with `satellite.file.new()`. §17's registry is flat - "one id per distinct
   word, flat across all positions" - so `new` is id 100 once and both paths
   use it: `{1,25,100}` and `{1,<thread>,100}`. `open` (31) already does the
   same as both a selector and a path segment. The earlier warning in this file
   and in §20 about "two features minting the same word" was WRONG and has been
   corrected in both. Nothing to fix.

## OPEN QUESTIONS THE USER HAS NOT ANSWERED

2. `dist/satl.1:287` uses `\(sc` for the section sign, which `groff -Tascii` does
   not define, so `man satl` renders those marks wrong. Pre-existing, one-character
   fix, not made.
3. Whether to update `view_forge/plans/missing.txt` - six of its `[OPEN]` items are
   now closed and two of its notes are stale.
