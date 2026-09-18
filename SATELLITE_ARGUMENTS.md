# satellite-004 — SATELLITE_ARGUMENTS

The `arguments` special variable: one class, one variant, one wrapper, and the
switch in the interpreter that makes looking for it cost nothing when a program
does not ask for it.

Written 2026-09-17 from the author's brief, which is kept whole in Part 1 --
this file is that prompt turned into milestones, and the milestones are Part 4.

**A SECOND BRIEF ARRIVED 2026-09-18** -- the values, the units, `satellite.variable.memory`,
and what all of it is for: the last known name, type and value of everything. It is
Part 4B, its milestones are Part 4C, and `arguments.access` from it is BUILT.

**THE ACCEPTANCE TEST IS THE COMPILER.** The author, in the brief: *"none of
this will be tested or checked or anything -- we just write it, make sure it
compiles, and if it compiles then we just accept it as the gospel!"* No check.sh
row, no pty test, no example program. A milestone is done when the tree builds.
Behaviour is checked later, once the values exist.

---

# Part 1 — the brief, as the author wrote it

> claude, this is the SATELLITE interpreter, and we are working on finishing
> building the special variable arguments, it has various aliases:
>
> arg, argz, args, argv, arguments, argument, argumentz
>
> it has a declaration:
>
> `satellite.container.list<satellite.variable.string> arguments`
>
> that is the only declaration for it, using those names however, and it
> triggers a special variable inside of the system, but we are going to build a
> class for it that returns different objects, we need to use multiple functions
> to create this variable, but a single class,
>
> ```cpp
> class satelliteArguments
> {
>     protected:
>         std::vector<satelliteObject> argument_cases;
>
>         bool add_argument(satelliteObject object_input)
>         {
>             // code here to check that the object IS an actual argument, we
>             // wrap the objects in a special class -- this class just has
>             // specific std::string to it called "argument_name" and it =
>             // arguments.name.name.name UP TO 3 specific, special std::string
>             // argument_name_r1 for "register #1" and r2, and r3 and then we
>             // can just add additional registers as we need them for
>             // arguments, HERE is the code that calls return_name_register1
>             // return_name_regsiter2 and these registers are set to "VOID" for
>             // objects that do not employ that register, so it's kinda like
>             // the 0 in our number system, so the argument system has it's own
>             // satellite.numbering system,
>
>             // after it has passed through checking that the name is a real
>             // list from our list of argument names, we check that a value
>             // exists that is within bounds -- this value is a std::variant and
>             // has templates for each value type -- when the type is... a
>             // satellite_number, then we use a template for that type to check
>             // that the value is in bounds. If after it passes the name and
>             // template value checks, we can finally add it to our list, all of
>             // this is done under a wrapper class that uses templates to define
>             // instances of different variants -- one single std::variant for
>             // the class encompasses all possible argument types,
>
>             // none of this will be tested or checked or anything -- we just
>             // write it, make sure it compiles, and if it compiles then we just
>             // accept it as the gospel!
>             argument_cases.push_back(object_input);
>
>             // the only checking we ever do with the arguments variable flies
>             // at us as just making sure that it does what we set the values to
>             // do, and the variant object in the class can be of these types:
>             // satellite string, satellite number, satellite.variable.bool,
>             // satellite.variable.binary, satellite.variable.hex, unsigned long
>             // long int, signed long long int, std::string, satelliteObject
>             // (you may use a shared ptr here), satelliteUserDefinedObject or a
>             // satelliteUserDefinedSpacesuit, so the argument can be absolutely
>             // anything that we can make in satellite, some of these will not
>             // make sense just write every class_name in to the std::variant,
>             // so that we can just hold anything,
> ```
>
> so this will be how we build our arguments object, and do not test anything,
> just write the code, make sure it compiles ONLY, and then we just later test
> the code so that it works as we want it to work, ignoring all other testing,
>
> here are all of the values that I want:
>
> `arguments.system.memory` (alias `arguments.system.memory.total`) this reports
> the total amount of memory available on the system....
>
> `arguments.system.memory.used`
>
> `arguments.system.memory.free`
>
> `arguments.system.memory.satellite.total`, `used`, `free`, memory free to the
> satellite interpreter,
>
> `arguments.system.thread` (and `threads`) `()` returns how many threads the
> machine can make after it runs a test,
>
> now see all of these special variables, they can be built in at a later step,
> so we won't build any of them just yet, we shall only build special class,
> satelliteArguments, then we will build all of the templates and functions as we
> have described, let's turn this entire prompt into... SATELLITE_ARGUMENTS.md,
> then we'll build milestones out of this whole prompt, then we'll finally follow
> those milestones:
>
> 1) define the class satelliteArguments
>
> 2) define the std::variant object, as literally any class that we could make,
>
> 3) even satelliteCapsule, unless we don't have that yet, we'll build that into
> a milestone as the very first thing that we do if it's not done yet,
>
> 4) we have defined the special class, the varaint object, now we need another
> special class -- a wrapper for a single satelliteObject that is an std::variant
> of two different types -- a satelliteUserDefinedObject or a satelliteObject,
> the special class is to hold the std::string registers that make up the names
> of that argument, so when someone calls arguments, we don't instantly access
> the argument, we search through every single argument until we find the name
> that was typed in, when the arguments variable is turned on, it trips a
> satellite.variable.bool that is special and while turned on, the interpreter
> has to run the special code for arguments, and when it's not turned on, while
> the interpreter doesn't have to look for the arguments variable because of
> this: `satellite.capsule satellite.main()` (no arguments here)
>
> then we skip looking for arguments in the interpreter -- this slows down the
> interpreter by like 3nanoseconds while it checks if arguments is turned on or
> not, so we have just slowed down the interpreter to add a new feature, and
> building this `satellite.variable.bool arguments_active = true/false` is an
> entire milestone, the very first milestone we do, we build the arguments_active
> for the interpreter!
>
> What a great idea, it works anyways, so fuck it? Let's build this prompt into
> milestones, small little milestones, and follow them one by one, but we are
> going to skip the "build the capsule" for this project... but by the end of the
> project, we shall have an extremely valuable arguments variable, and we can
> even create two of them if we wanted to -- we can run two satellite
> interpreters from one master interpreter this way, and I think that is what I
> want to do, split the interpreter someday into a global system wide machine,
> and then just have that single machine control every interpreter running, this
> way we can divide up the parallel code into multiple machines running, you
> know? This makes parallel easier to make, I guess. Like python, but this is the
> opposite of the Global Interpreter Lock, it's like, an anti-lock, or something.

---

# Part 2 — what the tree already has

Read before starting: four of the brief's pieces are already built, and two of
its names do not exist under the spelling it uses.

- **`satelliteCapsule` IS BUILT** -- `satellite/satellite_object/satellite_capsule.hpp`,
  a name, its parameters and its `satellite_bytecode` body. The brief's step 3
  costs nothing, and the author's own instruction is to skip it.
- **`satelliteObject` IS BUILT** -- `satellite/satellite_object/satellite_object.hpp`,
  a nine-arm variant (nothing, bool, number, string, bytecode, capsule,
  user-defined handle, binary, percentage). **ARMS ARE APPENDED, NEVER
  INSERTED**: `Kind` is the variant's index and a static_assert enforces it.
- **`satelliteUserDefinedObject` IS BUILT** -- `satellite_spacesuit.hpp`, held
  everywhere as `UserDefinedHandle`, a `std::shared_ptr`, because DESIGN 7.4
  makes a spacesuit a reference type.
- **`satelliteSpacesuit` IS BUILT** -- the two-arm variant over "one of ours" and
  "one the user defined".
- **`satelliteUserDefinedSpacesuit` DOES NOT EXIST** under that name. The brief's
  variant asks for it; the built pair is `satelliteSpacesuit` and
  `satelliteUserDefinedObject`, and A10 takes both rather than inventing a third.
- **`satellite_hex` DOES NOT EXIST** as a class. Hexadecimal is a *conversion*
  (`number_to_hexadecimal.hpp`), and `satelliteObject`'s own arm list reserves
  slot 10 for `satellite_hexadecimal_number`, unbuilt. A10 leaves the arm out and
  Part 5 records it as the author's call.
- **`satellite.container.list` IS A WORD, `1 4 2`, and is not built** -- codes
  4121–4139 in `word_codes.hpp`, no implementation. The declaration the brief
  names is therefore a *shape the reader recognises*, not a container that runs.
- **The name tree already exists in `words.tsv`** as
  `satellite.library.main.arguments.*` (`1 14 1 1`), with `.machine.threads`,
  `.memory.total`, `.system.hostname` and forty more. The brief writes
  `arguments.system.memory`. Part 5 records the collision.
- **There is a `satellite/arguments/` folder already, and it is LOAD-BEARING.**
  `class Arguments` holds what satl is told at startup, and four places read it:
  `structured-library.cpp` (the entry point -- `gather_config`, `gather`,
  `threads_max`), `version/version.hpp` (`--version`, `--help`, the title and
  startup blocks), `satl/session.cpp` (`threads_startup`, the startup pool) and
  `satl/satl_file.hpp`. It cannot simply be deleted; A26–A32 move each reader onto
  the new class first. **`command_line.hpp` is not part of this** -- parsing
  `argv` is a separate job and it stays untouched.
- **Variables live in `VariableTable`** (`value.hpp`), one per running body,
  reached through `ExpressionContext` (`expression.hpp`). There are no globals.
  That context is where `arguments_active` belongs.

---

# Part 3 — the rulings this file takes

Reversible, one line each to undo, taken so the milestones can be followed
without stopping.

- **R1. The switch is a C++ `bool`, not a satellite variable.** The brief says
  `satellite.variable.bool arguments_active`. A satellite bool lives in a
  `VariableTable` and costs a hash lookup; a `bool` on `ExpressionContext` costs
  the 3 nanoseconds the brief budgets. The *name* stays `arguments_active`.
- **R2. `satelliteArguments` holds `satelliteArgumentCase`, not `satelliteObject`.**
  The brief's literal field is `std::vector<satelliteObject>`, but the same brief
  then says the objects are *wrapped* in the register-carrying class. The wrapper
  is what the vector holds; otherwise the registers have nowhere to live.
- **R3. Four registers, not three.** The author's own value list overflows three:
  `arguments.system.memory.satellite.free` is four parts. `r4` is built now, with
  the pattern written down for `r5`.
- **R4. `"VOID"` is the zero and it is a literal `std::string`.** Compared with
  `==`, never with a pointer, and never left empty -- an empty register and a
  VOID register must not be two ways to say one thing.
- **R5. Nothing is added to `satelliteObject::Held`.** The arguments variant is
  its own type in its own file. Appending an arm to `Held` renumbers `Kind` for
  everything and buys nothing here.
- **R6. `satelliteArguments` replaces `Arguments`' STORAGE, and keeps its
  GATHERERS.** The author, 2026-09-17: *"remove any other arguments code that we
  have, except the code we need for the wiring."* What dies is the duplicate
  store -- `struct Argument` with its five parallel fields, `enum class
  ArgumentKind`, and the five `add_text`/`add_count`/`add_number`/`add_flag`/
  `add_bytes` setters -- all of which the one variant and the one `add_argument`
  now do. What lives is the code that READS THE MACHINE: `physical_cores()`, the
  `sysconf`/`uname`/`meminfo` readers, and `gather_config()` reading
  `satellite_config.hpp`. Those become the feeders that call `add_argument()`.
  Two arguments systems is the thing being removed; reading the machine twice is
  not the thing being removed.

---

# Part 4 — the milestones

Each one is small enough to finish and check by building. **A1–A32 are this
project.** C1–C8 are `satl --config`, which stands on its own. D1–D9 are the
values, which the author has deferred.

## Phase 0 — the switch (the author's first milestone)

- **A1** — ~~Add `bool arguments_active = false` to `MachineState`.~~ **Done 2026-09-18** (`8f909ad`) — next to the feature register, and for the reason that choice already gave: there are no globals, and a flag a thread cannot see the right copy of turns a feature on for some threads and not others.
- **A2** — Carry `arguments_active` through `ExpressionContext` to every expression.
- **A3** — Guard the walker's argument lookup behind `if (arguments_active)`.

## Phase 1 — the names

- **A4** — Write the seven aliases as one table: arg…argumentz.
- **A5** — Recognise `satellite.container.list<satellite.variable.string> <alias>` as the declaration.
- **A6** — That declaration, and only it, sets `arguments_active` true.

## Phase 2 — the variant, anything satellite can make — **DONE 2026-09-18 (`8f909ad`)**

- **A7** — ~~Create `satellite/arguments/argument_value.hpp` holding one `std::variant`.~~ **Done** — thirteen arms.
- **A8** — ~~Give it satellite's arms: string, number, bool, binary, percentage.~~ **Done.**
- **A9** — ~~Give it the plain arms: `unsigned long long`, `signed long long`, `std::string`.~~ **Done.**
- **A10** — ~~Give it the object arms: object, user-defined handle, spacesuit, capsule.~~ **Done** — all four. Hex is left out (red note 2 stands) and no third spacesuit name was invented.

**THE ARM ASSERTS ASK THE TYPE, NOT A VALUE.** `ArgumentValue(satellite_number()).index()` does not compile: most arms are not literal types, so the index cannot be taken in a constant expression. `std::variant_alternative_t` asks the same question of the type alone and asks it more strictly — one assert per arm, and the count asserted too.

## Phase 3 — the wrapper and its registers — **DONE 2026-09-18 (`8f909ad`)**

- **A11** — ~~Write `satelliteArgumentCase`: one variant value, wrapped.~~ **Done.**
- **A12** — ~~Give it `argument_name_r1` through `argument_name_r4`, all `std::string`.~~ **Done.**
- **A13** — ~~Default every unused register to `"VOID"`.~~ **Done** — and a `static_assert` under `set_register_at()` fails until a raised count has its register, so a half-raised `kArgumentRegisters` does not compile.
- **A14** — ~~Write `return_name_register1` through `return_name_register4`.~~ **Done**, plus `register_at(i)` so a loop does not repeat itself four times.
- **A15** — ~~Write down how `r5` is added.~~ **Done**, in the header, as three numbered steps.

## Phase 4 — the checks — **DONE 2026-09-18 (`8f909ad`)**

- **A16** — ~~Write the table of every real argument name.~~ **Done, BY NOT WRITING ONE.** `words.tsv` is the table and `word_codes.hpp` is generated from it, byte for byte, by a step `check.sh` runs. A hand-written list here would be a second table free to fall behind the first.
- **A17** — ~~Write `is_a_real_argument()`.~~ **Done** — the registers are spelled back out and `word::code_of_spelling()` answers. `arguments.memory.satellite.free` is correctly false until that row exists.
- **A18** — ~~`template <typename T> bool value_in_bounds(T)` accepts by default.~~ **Done** — so an arm added later is accepted rather than silently refused by a check nobody wrote for it.
- **A19** — ~~Specialise for `satellite_number`.~~ **Done, AND IT IS A FLOOR AND NOT A CEILING.** The language promises no limits and a bound is never the fix, so a number of any size passes; what is refused is a NEGATIVE count, which is a reader that failed and returned -1 rather than a small number.

## Phase 5 — the class — **DONE 2026-09-18 (`8f909ad`)**

- **A20** — ~~`class satelliteArguments`, protected vector.~~ **Done.**
- **A21** — ~~`add_argument()`: name check, bounds check, then push_back.~~ **Done**, in the brief's order. **A name already held is OVERWRITTEN, never added twice** — the brief's search stops at the first match, so a second row would be unreachable and the value silently stale.
- **A22** — ~~`find_argument()`: walk every case, compare the registers.~~ **Done**, linear, which is the brief's own shape and the right one: dozens of arguments and not thousands, a miss costs one compare and not four, and there is no map to keep in step with the vector.
- **A23** — ~~The public reader.~~ **Done** — `nullptr` for a name never gathered, which is a different thing from a name that is not real.

## Phase 6 — it compiles, which is the whole test — **DONE 2026-09-18**

- **A24** — ~~Add the new files to the Makefile.~~ **Done** — headers, so `HEADERS` and not `INTERPRETER_SOURCES`.
- **A25** — ~~Build the tree clean; no test is written.~~ **Done** — 28 libraries, 0 errors, 0 warnings, and no `check.sh` row, exactly as instructed.

**IT WAS RUN ONCE ANYWAY, OFF THE TREE, AND THIRTEEN BEHAVIOURS ANSWERED RIGHT.** The gospel rule is about not *writing* tests, and it is kept — nothing was added to the suite. But "it compiles" and "it works" are two claims, and only one of them had been checked: aliases fold (`args.` and `argv.` reach the same case), an unknown name is refused, a negative count is out of bounds, `arguments.memory` and `arguments.memory.total` are different names, and the same name overwrites rather than doubling.

## Phase 7 — the wiring, and the old code goes

**A26–A32 WERE BLOCKED AND ARE NOW UNBLOCKED (`62e7e7d`, 2026-09-18).** The
milestones assumed the old store and the new class name the same things. They do
not, and the number is decisive: **a real gather produces 29 names and only 11
are words in `words.tsv`.** The other eighteen — `arguments.file`,
`arguments.debug_mode`, `arguments.threads_startup`, `arguments.argument_1`,
`arguments.version`, `arguments.disk.free` and twelve more — have no row, so
`is_a_real_argument()` refused every one. A straight swap would have dropped
`arguments.file`, which is how satl knows what to run, and **satl would not have
started**.

`ArgumentOrigin` is the fix: `word` keeps the word-table check, `satl` does not,
because the language has no word for a config row and is not supposed to.
Nothing became unchecked — a config row is checked by `gather_config()` against
the author's own `return_arguments_vector()`, and a fact satl fills in by
`filled_in_by_satl()`, which `arguments_cases.cpp` already pins to what
`gather()` really adds.

**Measured through the real gather into the real `add_argument()`: 29 gathered →
29 held, 11 words, 18 satl's, 0 refused.** `arguments_a_program_can_read()`
answers the eleven, which is what the `arguments` variable should expose — a
program has no business reading `arguments.threads_startup`.


The author, 2026-09-17: *"then just wiring it into the interpreter as a single
step, remove any other arguments code that we have, except the code we need for
the wiring."* Readers move first, then the duplicate store is deleted -- in that
order, because the reverse does not compile at any point in between.

- **A26** — Point `version.hpp`'s version, title and startup blocks at `satelliteArguments`.
- **A27** — Point `session.cpp`'s `threads_startup` read at `satelliteArguments`.
- **A28** — Point `structured-library.cpp`'s startup and `threads_max` at `satelliteArguments`.
- **A29** — Delete `struct Argument`, `enum class ArgumentKind` and the five setters.
- **A30** — Delete `Arguments`' own store; its machine readers now feed `add_argument()`.
- **A31** — Leave `command_line.hpp` alone: parsing `argv` is a different job.
- **A32** — Build the tree clean; the removal is done when it compiles.

## Phase 8 — `satl --config`, the step that measures once

The author, 2026-09-17: *"let's just create a function that creates once on a
config step... we can run a satl --config and it will configure the system, one
of the things that it does is it checks how many threads it can possibly create
on the machine."* **This is the right shape and it is the reason the probe is
affordable at all:** 9.6 seconds is unthinkable at every startup and nothing at
all once per machine. Independent of A1–A32 -- it can be built before, after, or
alongside them.

**BUILT 2026-09-18 (`6bff781`)** — `satellite/config/machine_probe.hpp` measures, `run_config.hpp` says. Split that way because `arguments.machine.threads` wants the first and prints nothing.

- **C1** — ~~Add `Command::config`; `--config` is the whole line.~~ **Done, PLUS ONE OPTIONAL WORD.** See below.
- **C2** — ~~The probe: threads park on a condition variable, never spin.~~ **Done** — raw pthreads, 256 kB stacks, which is what the 2026-09-17 table was measured at.
- **C3** — ~~Ramp in doubling steps, recording nanoseconds and resident per thread.~~ **Done** — threads stay alive across rungs, so the resident figure is the real cost of holding that many at once.
- **C4** — ~~Stop at the lowest `/proc` ceiling less headroom, never at failure.~~ **Done** — three quarters, which is the author's own 400,000-of-506,566 rounded to a number a person can hold.
- **C5** — ~~Write the measured count and date to `~/.satl/machine.conf`.~~ **Done** — its own file and not config.ini, because config.ini is the PERSON'S and this is the MACHINE'S: copying config.ini to a new machine must not copy a thread count measured somewhere else.
- **C6** — ~~`arguments.machine.threads` answers from that file when it exists.~~ **Done** — `threads_this_machine_allows()`.
- **C7** — ~~With no file, answer the ceilings' minimum and never probe.~~ **Done** — a person who never runs `--config` still gets a true answer.
- **C8** — ~~`satl --config` prints what it measured and where.~~ **Done** — every ceiling named, and the binding one marked, so a person can go raise THAT one rather than guess.

**IT REPRODUCED THIS FILE'S OWN TABLE, WHICH IS WHY IT WAS WORTH RUNNING.**
8,192 threads: **71 MB and 24,792 ns each**, against the recorded 72 MB and
24,780. The ceilings match too. Two independent measurements, a day apart.

**`satl --config <most>` — ONE WORD MORE THAN C1 ASKED FOR.** C1 said "--config
is the whole line" and could not have known that the uncapped run here is nine
seconds and three gigabytes. A command nobody can afford to try once is a
command nobody tries; capped, it costs 203 ms. It is also the right answer in a
container. **A cap above the ceiling is REFUSED and not clamped** — asking for
more threads than the kernel allows is a person who believes something untrue
about their machine, and the useful answer is the number. **A capped run says so
in `machine.conf`**, because otherwise a number asked for once while trying the
command becomes what `arguments.machine.threads` answers forever.

**C4 IS THE ONE THAT MATTERS.** `threads-max` is system-wide, so a probe that
runs to failure takes the last thread on the machine and the desktop cannot make
one either. Headroom is not politeness, it is the difference between a stutter
and a login shell that cannot fork.

## Deferred — the values (the author: *"not just yet"*)

- **D1** — `arguments.system.memory`, alias `.total`: the machine's whole memory.
- **D2** — `arguments.system.memory.used`: what the machine is using.
- **D3** — `arguments.system.memory.free`: what the machine has left.
- **D4** — `arguments.system.memory.satellite.total`: the interpreter's own whole.
- **D5** — `arguments.system.memory.satellite.used`: what this interpreter is using.
- **D6** — `arguments.system.memory.satellite.free`: what this interpreter has left.
- **D7** — Read every thread ceiling the system reports into one list.
- **D8** — Answer the lowest of them; nothing is ever spawned.
- **D9** — `arguments.system.thread` and `.threads` both answer that number.

**THE MINIMUM OF THE CEILINGS** (the author, 2026-09-17): *"grab the thread_count
from all sources, and then settle with the lowest number."* Measured here
2026-09-17, so nobody has to measure it again:

| source | this machine |
|---|---|
| `/proc/sys/kernel/pid_max` | 4,194,304 |
| `MemAvailable` ÷ 8.33 kB resident per thread (measured) | ~6,799,000 |
| `satellite_config.hpp` `arguments.threads_max` | 1,000,000 |
| `/proc/sys/vm/max_map_count` ÷ 2 (two mappings per stack) | 524,288 |
| **`/proc/sys/kernel/threads-max`** | **506,566** ← binds |
| `ulimit -u` (RLIMIT_NPROC) | unlimited |
| cgroup `pids.max` | max |
| `nproc` / `_SC_NPROCESSORS_ONLN` | 24 (not a ceiling, see below) |

**The lowest ceiling is 506,566, and that is D8's answer here.** Two readings,
each one line to reverse:

- **`nproc` IS NOT IN THE MINIMUM.** 24 is lower than every ceiling on every
  machine, so putting it in the set makes the minimum always `nproc` and the
  other seven sources dead code. It is already reported on its own, as
  `arguments.machine.threads` (`arguments.cpp:237`).
- **`arguments.threads_startup` (1,024) IS NOT A CEILING.** It is how many to
  start, not how many may exist, so it is not in the set either.

**A CORRECTION, RECORDED BECAUSE IT WAS WRITTEN DOWN WRONG FIRST.** This table
first said the binding limit was `MemAvailable ÷ ulimit -s` = **6,913**, and that
stack size therefore decided the count. **Both are false, and measuring said so.**
A thread's stack is *lazily committed*: 50,000 threads asking for the default
8 MiB each did not take 400 GB, they took **419 MB resident** -- about 8.33 kB of
real memory apiece, and that figure barely moves whether the stack requested is
256 kB or 8 MiB. Stack size costs *virtual address space*, which this machine has
128 TB of. So memory is nowhere near binding, and the kernel's own `threads-max`
is what stops you.

**AND PARKED THREADS DO NOT FREEZE THE MACHINE -- MEASURED, NOT ARGUED.** The
author, 2026-09-17: *"this doesn't freeze my machine, it just stutters once doing
it."* He is right, and the 2026-09-16 freeze was the BUSY-WAIT, not the count:

| parked threads, 256 kB stacks | time | resident | ns per thread |
|---|---|---|---|
| 8,192 | 203 ms | 72 MB | 24,780 |
| 100,000 | 2.4 s | 836 MB | 24,450 |
| 400,000 | 9.6 s | 3.3 GB | 23,735 |

Perfectly linear across a 50x range, no degradation, no stutter, and it stopped
at the cap rather than at any limit. 200,000 **runnable** threads against 24 CPUs
is a scheduler death spiral and it cost a hard reboot; 200,000 **sleeping**
threads is 1.7 GB and four seconds. The probe must park every thread it makes --
a condition variable, never a spin -- and that one property is the whole
difference.

**THE RUN STOPPED AT 400,000 ON PURPOSE, 106k SHORT OF THE LIMIT.** `threads-max`
is *system-wide*: taking it to zero means nothing else on the machine can make a
thread either, including the desktop. The probe must always leave headroom rather
than find the exact ceiling, because the exact ceiling is a number `/proc` states
for free.

---

# Part 4B — the second brief, 2026-09-18

The author came back with the values, the units, a new type, and the thing all
of it is for. Kept whole, then turned into milestones below.

## What it is all for — the last known name, type and value

> for satellite.history and for satellite.access we keep the LAST KNOWN
> variable's name type and value, under any circumstances, because when the
> program stops, you have to be able to run satellite.access(object_name) for any
> object in satellite, so we keep a list of everything inside of satellite, but we
> keep it in satellite.history, satellite.library is cleaned up, history is the
> lasting copy -- as long as the interpreter runs, we keep the last name, type and
> value of everything.... but only the last, UNLESS satellite.history is turned
> on, then we keep everything saved onto disk

So there are **two stores and two valves**, and they are not the same thing:

| | what it keeps | where | valve |
|---|---|---|---|
| the last-known store | one row a name: name, type, value | memory, for the life of the interpreter | `arguments.access` |
| the history | every value a name ever held | disk | `arguments.history` |

`satellite.library` is **cleaned up** — freed as it always was. The last-known
store is what survives, and it is bounded by the number of NAMES rather than by
the number of assignments, which is what makes "only the last" the affordable
one. `satellite.access(object_name)` reads it after the program has stopped.

**`arguments.access` IS BUILT AND WORKING as of 2026-09-18** — `249d748`. It
reads, it writes, it lasts in `$HOME/.satl/config.ini`, and it defaults to true
because the author said *"arguments.access will always be on, on this machine, as
we are testing it"*. **What it does not yet do is govern anything**, because the
store it is the valve for is not built. That is the order A1 already argues for:
the switch before the thing it switches.

**`arguments.history` is numbered and deliberately unbuilt** — the author:
*"let's leave history alone for right now, and add the access stuff"*. It answers
`not_built_yet` with its own name.

## The values

> arguments.memory() (alias of arguments.memory.total())
> arguments.memory.total / .free / .used
> arguments.memory.system (the memory available to the satl interpreter)
> arguments.memory.system.reserved (memory reserved only for the satl
>   interpreter, if you run the satl interpreter, this grabs that much memory and
>   just... holds onto it for only the interpreter, this will be an entire 20
>   minute block just to build this into the interpreter, it will be alot of work)
> arguments.memory.system.used (how much memory this interpreter has used, not a
>   copy of it... just this particular interpreter, this value is always
>   available...)
> arguments.memory.swap.total / .free / .used
> arguments.username = the linux username that is being used,
> arguments.directory = the current directory,
> arguments.directory.history = the directory history, in case the user enters
>   ".." just a list of the directories satl has been in ... when the satl
>   interpreter enters a new directory to grab an include, then it also ends up in
>   this list
> arguments.threads = how many threads the interpreter is allowed to create, this
>   is set to I think 1024 by default
> arguments.cores = how many cores exist on the machine, so for this it's 12
> arguments.system.stack = a satellite.variable.number of kilobytes the stack size
>   is PER megabyte ... blocked as 8 megabytes ONLY on linux
> arguments.system.x where x is any size configurable for the machine

**THE SPELLING IS SETTLED BY THIS BRIEF, AND IT SETTLES PART 5's RED NOTE 1.**
The first brief wrote `arguments.system.memory`; this one writes
`arguments.memory.total`. That is *exactly* what `words/words.tsv` already
carries at `1 14 1 1 2 1`. **The word table wins, and the red note is closed.**

**WHAT THE TABLE ALREADY HAS**, so these cost a library and no new row:
`arguments.memory` `1 14 1 1 2`, `arguments.memory()` `…2 0`,
`arguments.memory.total` `…2 1`, `arguments.username` `1 14 1 1 3`,
`arguments.machine.cores` `…1 1`, `arguments.machine.threads` `…1 3`.

**TWO NAMES ARE ALREADY SPELLED ANOTHER WAY**, and the author should pick:
`arguments.threads` and `arguments.cores` exist as `arguments.machine.threads`
and `arguments.machine.cores`. Red note 5 below.

## The units

> for some of these, we have to take in "b" to return "bytes" ... and "mb" ... and
> we need "kb" and the alias "kilobytes" and we need "gb" and the alias gigabytes,
> and we need "tb" ... and "pb" and the alias "petabytes" and we need all aliases
> to be "kilobyte" without the S so if we accidentally type in "kilobyte" or
> "megabyte" or "byte" it accepts it

Six units, three spellings each, eighteen words accepted:

| short | plural | singular |
|---|---|---|
| `b` | `bytes` | `byte` |
| `kb` | `kilobytes` | `kilobyte` |
| `mb` | `megabytes` | `megabyte` |
| `gb` | `gigabytes` | `gigabyte` |
| `tb` | `terabytes` | `terabyte` |
| `pb` | `petabytes` | `petabyte` |

`arguments.system.stack("kb")` answers the value in kilobytes. **`Arguments`
already carries a `size` kind** with a `long double` and a unit string, and
`arguments.hpp` already argues for the `long double`: 18 exact digits, every
64-bit byte count held exactly, and dividing by 1024 exact too. So the arithmetic
is built; the eighteen spellings are not.

## The new type

> I am thinking we need a special class inside of satellite that looks like this:
> satellite.variable.memory mem_name = 1gb or 1mb, or 1kb, or 1b, or 1tb, that is
> how you write it into code, and this turns it into an object, THEN we use the
> object to store our arguments... this is such a great idea, I know it's feature
> creep, but it's just too good of an idea to not use you know?

`satellite.variable.memory` is **a new satellite type**, written as a literal
with its unit stuck to it — `1gb`, `512mb` — the way `b1010` is a binary and
`50%` is a percentage. Those two are the precedent and the map:

- a **lexer token** (`binary_token`, `percentage_token` → `memory_token`),
- a **`satelliteObject` arm**, appended never inserted, because `Kind` is the
  variant's index and a `static_assert` enforces it,
- a **`Value` kind** and its `is_*`/`as_*` pair,
- a **`word::code_of(1, 6, n)`** row for the declaration,
- **`program_check.cpp`** refusing `satellite.variable.memory m = 5` the way it
  already refuses a percentage written without its `%`.

The author knows it is feature creep and wants it anyway. Recorded as wanted, and
milestoned last, because every value above can be built without it and then
re-answered through it.

---

# Part 4C — the milestones from the second brief

**B1–B9 are the order the author gave.** The author: *"work on arguments until we
can successfully add arguments.access = true/false"* first, then the values, then
the type.

## Phase A — the valve (DONE, 2026-09-18, `249d748`)

- **B1** — ~~`arguments.access` reads, writes, and lasts in `$HOME/.satl/config.ini`.~~ **Done.**
- **B2** — ~~A word a program can assign to: one arm each in the checker, the walker and the expression reader.~~ **Done.**
- **B3** — ~~The installer writes a config.ini and never overwrites one.~~ **Done.**

## Phase B — the units, before any value needs them

Built first on purpose: every value below answers through them, and a unit added
after six libraries already parse their own would be six places to fix.

- **B4** — One header: the eighteen spellings to one enum, and the enum to a divisor.
- **B5** — A word that takes a unit answers `types_do_not_meet` for a spelling not on the list, naming the eighteen.
- **B6** — `Arguments`' existing `size` kind answers in any of the six.

## Phase C — the values that are free

Every one of these is a `/proc` or `sysconf` read that `arguments.cpp` **already
does** — they need a library and a row, not a new reader.

- **B7** — ~~`arguments.memory.total`, `.free`, `.used` — `/proc/meminfo`.~~ **Done 2026-09-18.** Verified against `/proc/meminfo` on this machine: total `66509373440`, exact.
- **B8** — `arguments.memory.swap.total`, `.free`, `.used` — the same file. **The three rows are not in `words.tsv` yet**; B7's two were appended, and swap's three go the same way.
- **B9** — ~~`arguments.username` — `getpwuid`.~~ **Done** — answers `madness` here. `getpwuid` and **not** `$USER`: the environment's copy is whatever was exported, the passwd entry is who the process really is, and under `sudo` they disagree.
- **B10** — ~~`arguments.cores`, `arguments.threads`.~~ **Done**, under the word table's spelling (`arguments.machine.cores`, `arguments.machine.threads`) — **red note 5 is still the author's**, and an alias is one row when he picks. Cores answers `24`; threads answers `506566`, which is this machine's `threads-max` exactly, **and is C6**: the measured count from `machine.conf` when `satl --config` has run, the lowest `/proc` ceiling when it has not, and **never a probe**.
- **B11** — `arguments.directory` — `getcwd`. The reader is written (`machine_facts::working_directory`) and **the word has no row in `words.tsv`** yet.

## What Phase C needed first, and it was not a reader

**A NEW SCENARIO SHAPE.** Every scenario a library could fill in CONSUMED what a
program handed it and reported how it went; `flag_setting` was the first that
answered, and it answers a bool. A machine fact answers a **count or some text**
and can never be written, so it is `FactScenario` — appended last, as that file
requires.

**READ-ONLY BY HAVING NO WRITE PATH**, rather than by refusing one. Widening
`flag_setting` to carry a count would have given every fact a write path for
something that cannot be written — `arguments.memory.total` is what the machine
has, not a preference — and a word that can be assigned to and must not be needs
its refusal written somewhere. This version has nothing to get wrong.

**NOTHING IS CACHED, AND THAT IS THE POINT.** `arguments.memory.free` that
answers what was free a minute ago is a wrong answer wearing a right answer's
face. Two runs a second apart gave `55091806208` and `54885224448`, which is the
feature working.

**`used` IS TOTAL LESS `MemAvailable`, NOT TOTAL LESS `MemFree`.** `MemFree`
leaves out the page cache, which the kernel hands back the moment anything wants
it — so `free` off `MemFree` reads as almost nothing on a machine that is
perfectly healthy, and `used` off it reads as almost everything.

**A FAILURE IS SAID, NEVER ANSWERED AS 0.** A machine that does not state a fact
refuses with `machine_fact_not_read` (36), naming what could not be read. 0 is a
number a program would divide by.

**A NEW WORD ROW NEEDS THREE REGENERATIONS, AND `make` DOES ONLY SOME OF THEM.**
`words_004.tsv` → `words.tsv` (`words/make_words.py`) → `word_codes.hpp`
(`satellite/bytecode/make_word_codes.py`). The libraries built and the word still
answered `name_not_declared` until `make_word_codes.py` was run by hand. Learned
the hard way; written here so the next person does not.

## Phase D — the values that need something built

- **B12** — `arguments.memory.system.used`: this process's own resident size, `/proc/self/statm`. The author: *"this value is always available"*.
- **B13** — `arguments.directory.history`: a list every directory satl has entered, appended to when `satellite.include()` reaches into a folder. **Needs a store with the same bound the last-known store needs** — a directory entered in a loop must not grow it without limit.
- **B14** — `arguments.system.stack`: the stack size, in the unit asked for. The author remembers 003 having to work around an 8 MiB block on Linux; SATELLITE_ARGUMENTS.md's own thread table is the correction — a thread's stack is *lazily committed*, 8.33 kB resident apiece whatever is requested, so the 8 MiB is address space and not memory.
- **B15** — `arguments.memory.system.reserved`: grab that much memory at start-up and hold it for this interpreter alone. The author budgets *"an entire 20 minute block"* and is right that it is the hard one — see red note 6.

## Phase E — the type

- **B16** — `memory_token` in the lexer: a number with a unit stuck to it.
- **B17** — A `satelliteObject` arm, **appended**, and the `static_assert` kept true.
- **B18** — A `Value` kind, its `is_memory()`/`as_memory()`, and `kind_name()`.
- **B19** — `satellite.variable.memory` as a `1 6 n` declaration row.
- **B20** — `program_check.cpp` refuses one written without a unit, the way it refuses a percentage without its `%`.
- **B21** — Every Phase C and D value re-answers as one.

---

# Part 5 — left to the author

Red notes. None of them blocks A1–A25.

1. ~~**`arguments.system.memory` or `satellite.library.main.arguments.memory.total`?**~~
   **CLOSED 2026-09-18 by the second brief.** The author wrote
   `arguments.memory.total`, which is exactly what `words.tsv` carries at
   `1 14 1 1 2 1`. The word table is the spelling; `arguments.system.memory` from
   the first brief is dead. A row is still only ever APPENDED -- a code is 4097
   plus the ROW in `words/words.tsv`, so inserting one renumbers every word after
   it -- and the two rows added that day went on the end for that reason.
   **AND THE TABLE IS GENERATED**: `words.tsv` comes out of `words_004.tsv`
   through `make_words.py`, and `check.sh` regenerates it and compares byte for
   byte. Edit the source, never the output. (Learned by editing the output.)
2. **Is `satellite_hexadecimal_number` built first, or is hex left out?**
   The brief's variant names `satellite.variable.hex`; the class does not exist,
   and `satelliteObject` reserves arm 10 for it. A10 omits it for now.
3. **Does `satellite.container.list` get built before the declaration means
   anything?** A5 recognises the shape; the container behind it is unbuilt, so
   `arguments` is a name the reader knows rather than a list a program can walk.
4. **Two interpreters, one master -- the anti-GIL.** The brief's closing idea:
   a system-wide machine owning every running interpreter, parallel work split
   across them. Recorded here, not milestoned. Nothing in A1–A25 forbids a second
   `satelliteArguments`; that is what makes the idea cheap later.

## From the second brief, 2026-09-18

5. ~~**`arguments.threads` or `arguments.machine.threads`?**~~ **CLOSED
   2026-09-18 BY THE AUTHOR: the long one, and the short one as an alias.** His
   words: *"let's set it to arguments.machine then, and let's use the longer
   choice for each one, can we have an alias for them though?"* So
   `arguments.machine.cores` and `arguments.machine.threads` are the words, and
   `arguments.cores` / `arguments.threads` answer the same thing. Both are built
   and both were checked against each other. The old text is kept below for the
   record.

   **HOW AN ALIAS IS BUILT HERE, so the next one is a diff and not a decision.**
   An alias is a second ROW in `words_004.tsv` and a second `.so`, because a code
   is one number and one library. What it is **never** is a second copy of the
   answer: every answer lives once, in `machine_facts.hpp`, and both libraries
   point at it. That is the only way an alias can really go wrong — answering a
   different number from the word it aliases — and it is designed out rather than
   tested for.

   **THE ALIASES THAT EXIST**, all verified equal to their canonical word:

   | alias | is | answers here |
   |---|---|---|
   | `arguments.cores` | `arguments.machine.cores` | 24 |
   | `arguments.threads` | `arguments.machine.threads` | 506,566 |
   | `arguments.user` | `arguments.username` | madness |
   | `arguments.memory` | `arguments.memory.total` | 66509373440 |
   | `arguments.memory()` | `arguments.memory.total` | 66509373440 |
   | `arguments.ram` | `arguments.memory.total` | 66509373440 |
   | `arguments.dir` | `arguments.directory` | the working directory |

   `arguments.memory()` is the author's own spelling from the second brief and it
   reaches a **different code path** — brackets make it a call, so it is answered
   in `call_word` rather than the bare-word arm. Both give the same number, and a
   fact given an argument is refused, because a fact is what the machine has and
   there is nothing to hand it.

   **STILL THE AUTHOR'S:** *"can we alias them as arguments.direct_variable for
   the arguments?"* — read here as **"give the deep names a short, direct
   spelling"**, which is the table above. If `arguments.direct_variable` was
   meant as a literal word of its own, say so and it is one more row.

5b. **The original note, kept:** **`arguments.threads` or `arguments.machine.threads`? `arguments.cores` or
   `arguments.machine.cores`?** The second brief writes the short pair; the word
   table carries the long pair, at `1 14 1 1 1 3` and `1 14 1 1 1 1`. Unlike red
   note 1 these do **not** agree, so one of the two is an alias and the author
   picks which. An alias is cheap -- a second row pointing at one library -- and
   picking neither is what is expensive, because a library built under one
   spelling has to move if the other wins.
6. **`arguments.memory.system.reserved` -- what does "reserved" mean to the
   kernel?** The author budgets *"an entire 20 minute block"* and is right that it
   is the hard one, but the hardness is not the code. Linux does not hand out
   memory on request: `malloc` of 8 GiB takes address space and no pages until
   they are touched, which is the same lazy commit SATELLITE_ARGUMENTS.md's own
   thread table already measured (50,000 threads asking 8 MiB each took 419 MB
   resident). So "grabs that much memory and just... holds onto it" is one of
   three different things, and they cost differently:
   - **touch every page** -- really resident, really unavailable to anything
     else, and start-up pays for all of it up front;
   - **`mlock`** -- resident and never swapped, and needs a privilege or a raised
     `RLIMIT_MEMLOCK`, so it fails on an ordinary account;
   - **reserve the address space only** -- free and instant, and reserves nothing
     a person would recognise as memory.

   Which one the author means decides whether B15 is twenty minutes or a day.
7. **Does the last-known store have a bound?** "Only the last" bounds it by the
   number of NAMES, which is what makes it affordable -- but
   `arguments.directory.history` (B13) is bounded by the number of directories
   ENTERED, and a program that includes in a loop grows it forever. The console
   queue on 2026-09-17 was exactly this shape and it took the machine's memory.
   Anything appended per-iteration needs its bound decided before it ships.
8. **`satellite.variable.memory` -- feature creep the author wants anyway.** His
   own words: *"I know it's feature creep, but it's just too good of an idea to
   not use you know?"* Recorded as wanted and milestoned last (B16–B21), because
   every value in Phase C and D can be built without it and then re-answered
   through it -- which is the order that makes the type cheap to be wrong about.
