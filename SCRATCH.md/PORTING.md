# Porting `satellite_number` and `satellite_string` from the first satellite

**This file is scratch.** When the port lands, its conclusions belong in PLAN.md
(what was kept and what changed, §6 and §7) and LAYOUT.md (the files), and this
file gets deleted.

The user's instruction: *"you can build our number in ./src/satellite_number/ and
you can also build our string in ./satellite_string … I think we are keeping almost
all of the satellite_string and satellite_number code that you have written, so
that can just be copied into this version."*

Both target directories already exist in this tree and are empty. LAYOUT.md says
so, and says they are *"holding names for work that has not started."*

---

## 1. What is actually there

`old_versions/first_satellite/src/satellite_number/` — **10 files, 1509 lines**

| file | lines | what it is |
|---|---:|---|
| `bignum.hpp` | 40 | the umbrella header; the only door a consumer opens |
| `bignum_bigint.hpp` | 96 | the limb vector underneath `Number` |
| `bignum_internal.hpp` | 50 | shared internals, not for consumers |
| `bignum_number.hpp` | 228 | `class Number` — the declarations |
| `limbs.cpp` | 284 | limb arithmetic, the base-10⁹ core |
| `number_arith.cpp` | 229 | add, subtract, multiply, divide |
| `number_core.cpp` | 195 | construction, conversion, comparison |
| `number_query.cpp` | 186 | abs, floor, ceil, round, digit queries |
| `random.cpp` | 121 | the three random tiers over `Number` |
| `render.cpp` | 76 | `Number` to text |

`old_versions/first_satellite/src/satellite_string/` — **2 files, 249 lines**

| file | lines | what it is |
|---|---:|---|
| `satellite_string.hpp` | 94 | `SatChar`, `SatString`, the code table |
| `satellite_string.cpp` | 155 | encode, decode, the live codes |

**Every file is already under PLAN §3's 300-line ceiling.** The largest is
`limbs.cpp` at 284. So the line rule costs this port nothing — which is worth
noticing, because §3 says a ceiling applied after the fact preserves a file's
shape rather than changing it, and here the shape already fits.

## 2. What they drag in with them

Neither module is free-standing.

- `satellite_number` is internally closed — every include is a sibling under
  `satellite_number/` plus the C++ standard library. **It ports alone.**
- `satellite_string` includes `system_facts/system.hpp`, because of §3 below.
  So it needs `system_facts/` first, or the live codes stubbed.

`system_facts/` in v1 is **1054 lines across 10 files**, of which the port needs
`system.hpp`, `system_internal.hpp`, `host_facts.cpp` (44) and `memory_facts.cpp`
(236). This tree already has `src/system_facts/version.hpp`, so the directory and
the include convention exist.

## 3. The finding: `satellite_string` already knows the machine

`satellite_string.hpp`'s code table is not just characters. Codes 95–100 are
**live values, resolved when the string is decoded**:

    95   linux_home      the user's home directory
    96   linux_username  the user's name
    97   threads         hardware thread count
    98   mem_total_mb    total machine memory in MB
    99   mem_used_mb     used machine memory in MB
    100  cwd             current working directory

**97, 98 and 99 are three of the numbers `satellite_config.ini` is supposed to
carry.** The string type already reaches them, through `system_facts`. That is
either a gift or a duplication depending on a decision nobody has made yet: if the
config file is the authority on THREAD_COUNT, then code 97 and the config can
disagree, and something has to say which one a program sees.

## 4. The bigger finding: the memory ceiling already exists, with a different policy

`system_facts/memory_facts.cpp` already contains everything the MEMORY_MAX
requirement needs, built and working:

- **`process_memory_bytes()`** reads `/proc/self/statm` field 2 (resident pages)
  × page size, fresh on every call. Its own comment says why it is never cached:
  *"the question is what this program is using NOW, while it runs, so a cached
  answer would be the wrong one by definition."* It chose `statm` over `status`
  because statm is six integers on one line — no key matching, no unit parsing —
  *"which matters when a program calls this inside a loop to watch itself grow."*
- **`start_memory_watchdog()`** already runs a detached thread that wakes once a
  second, reads a threshold, compares, and shuts down with a plain-words message
  on stderr through an emergency exit hook that restores the terminal first.

**But its policy is the opposite way round from what the user asked for.**

| | v1's watchdog | what the user asked for |
|---|---|---|
| watches | the **machine's** available memory | **satl's own** usage |
| threshold | `min_free_mb`, default 4096 | `MEMORY_MAX` |
| fires when | the machine runs low | satl grows too big |
| configured by | `satellite.library.system.min_free_mb` | `satellite_config.ini` |

Both are defensible and they are not the same guarantee. v1 protects the *machine*
from satellite; the user is asking to protect the machine from satellite *by
bounding satellite*. The second is the stronger promise and the easier one to
explain, and `process_memory_bytes()` is already the call that implements it.

**So the port is small: keep the thread, keep the exit path, change what it
compares.** Ideally keep both checks — a program can be under MEMORY_MAX and the
machine can still be dying because of something else.

## 5. The other finding: v1's config mechanism is not a file

The watchdog reads its threshold from `satellite.library.system.min_free_mb` —
**the language's own library namespace, readable and writable by a running
program**, not from a file. `Number` does the same thing for
`satellite.library.system.division_digits`.

That is a real precedent and it sits directly under the user's request for a
`satellite_config.ini`. They are not in conflict; they answer different questions:

- a **file** is where a machine's settings live before a program starts, and is
  what an installer can write and a user can edit;
- **`satellite.library.system.*`** is how a running program reads and retunes them.

The obvious shape is that the file seeds the library namespace at startup and the
namespace is what everything reads afterwards — one authority at runtime, one place
to edit at rest. **Not yet decided, and worth putting to the user**, because it
also decides whether `satellite.library.system.*` needs numbering in
WORD_NUMBERS.md (it does, if a program can name it).

## 6. What still has to be decided before copying

1. **Does `Number` keep its dependency on `satellite.library`?** Two constants —
   `division_digits` and `min_free_mb` — reach into the library namespace, so
   porting `Number` as-is drags the library registry in with it, at M2, years
   before PLAN §8 schedules it. The alternative is a compile-time default now and
   the library lookup restored at the milestone that builds the library.
2. **Does the code table stay 16-bit?** The header argues it well — 101 codes plus
   a 256-entry raw area does not fit in 8 bits, and 16 bits halves what a corpus
   costs against 32. Nothing in the second satellite's design contradicts it. Port
   as-is unless something does.
3. **`satellite.random` lives in `satellite_number/random.cpp` in v1.** In this
   tree it may want its own directory, since DESIGN §11 gives it a whole section
   and three tiers with a documented statistical character. A naming decision, not
   a code one.
4. **The arena.** PLAN §2.2 replaces the AST with an arena of PODs. `Number` and
   `SatString` are *values*, not AST nodes, so the arena does not touch them — but
   DESIGN §8.2 says a `Value` is 40 bytes with the static_assert to come, and a
   ported `Number` has to fit that budget. **Check `sizeof(Number)` on arrival**;
   it is the one number that could make this port not fit.
