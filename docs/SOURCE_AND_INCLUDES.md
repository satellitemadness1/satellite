# satellite_source and recursive include gathering

Answers: where does all the source text live once includes are followed
recursively?

**Storage is `std::vector<std::string>`, one entry per line** — used exactly
like a vector, because manipulating source is mostly manipulating lines:

```cpp
src.lines.insert(src.lines.begin() + 2, "new line\n");
src.lines.erase(src.lines.begin() + 7);
src.lines[0] = "replaced\n";
for (const std::string& line : src.lines) { ... }
```

## The one rule that makes it correct: keep the newlines

`std::getline` strips newlines. Store lines stripped and the file can no longer
be reconstructed byte for byte — trailing whitespace and `\r` are gone, and a
`satellite.cxx` block rebuilt from those lines comes back a byte short. That
block goes straight to a C++ compiler, so it has to arrive exactly as written.

So `set_text()` splits **keeping** every newline. `all_text()` is then a plain
concatenation and is exact. A file that did not end in a newline is remembered
by its last line simply not having one.

Verified in `tests/test_source.cpp`:

```
all_text() reproduces the file byte for byte     ok
trailing whitespace survives                     ok
cxx block spans lines exactly                    ok
file with no trailing newline                    ok
```

An earlier draft of this document argued against a vector of lines on the basis
of a benchmark showing a rebuilt block coming back 73 bytes where the original
was 74. That was an artifact of the benchmark using `getline`, not a property of
the data structure. Corrected.

`text_of(n)` returns the line *without* its newline, for display and comparison.

## SourceMap uses a deque, and that part is not about ergonomics

`SourceMap` owns every loaded `Source` in a `std::deque`, not a `std::vector`.
A deque never moves its elements as it grows, so a `const Source&` held by other
code stays valid while more files load. A `vector<Source>` would invalidate
those references on reallocation, which surfaces as corrupted data rather than a
crash. Tested: a reference survives 500 subsequent loads.

`SourceId` is a 1-based index; 0 (`kNoSource`) means "not from a file".

---

## Recursive include gathering

```cpp
struct IncludeResult {
    SourceId                 root = kNoSource;
    std::vector<SourceId>    order;          // deterministic load order
    std::vector<Diagnostic>  diagnostics;
    bool                     ok = false;
};

IncludeResult gather_includes(SourceMap& map,
                              const std::string& main_path,
                              const std::vector<std::string>& search_paths = {});
```

### Algorithm

1. Load the main file into the map.
2. Run `strip_comments()` on its text (see `docs/COMMENT_STRIPPER.md`) — this is
   exactly the job that motivates the stripper, so a commented-out
   `// satellite.include("x")` cannot be mistaken for a real one.
3. Scan the stripped text for include statements.
4. Resolve each path: relative to the *including file's* directory first, then
   each search path. Append `.satl` if the name has no extension — the same rule
   `resolve_satl_path()` already implements for the CLI and GUI.
5. Already in `by_path_`? Skip silently. Including the same file twice is normal
   and is not an error.
6. On the current DFS stack? **Cycle** — report it with the full chain.
7. Recurse.

### The two forms of include are different

The lexer distinguishes them for free:

```
satellite.include(satellite)      // Tok::Satellite inside the parens
satellite.include("some_file")    // Tok::Str inside the parens
```

The first is the **runtime marker** every main file must carry — it names no
file and loads nothing. The second is a real file. The gatherer must only follow
the string form. (Version 001 merely checks the marker is present; see
`declares_satellite_include()` in `include/satellite/interpret.hpp`.)

### Cycle detection needs three colours, not a visited set

A plain "seen" set cannot tell a diamond from a cycle:

```
main -> a -> c          diamond: legal, c loads once
main -> b -> c

main -> a -> b -> a     cycle: must be an error
```

Standard DFS colouring: **white** unvisited, **grey** on the current stack,
**black** finished. Reaching grey is a cycle; reaching black is a diamond and is
fine. Report the cycle as the chain of greys, so the message reads
`a.satl -> b.satl -> a.satl` rather than just "cycle detected".

---

## Parallel scanning — the original design idea, and where it actually pays

The project's founding vision had a worker per include, scanning ahead. The
measurements in the repo README support that **for this specific phase**: include
gathering is I/O-bound, and Bench 1 showed blocked threads cost nothing (400
threads matched 24 on the same work). This is the one place massive
oversubscription is straightforwardly right.

Two constraints:

- **`SourceMap` needs a lock**, or per-thread gathering with a serial merge.
  A `std::deque` is not thread-safe to push concurrently.
- **The order must be deterministic.** If files land in whatever order the disk
  returns them, diagnostics and any order-dependent semantics change between
  runs of the same program. Load in parallel, but **sort into a fixed order
  before recording `IncludeResult::order`** — breadth-first by (parent id,
  position of the include statement in the parent). Same program, same order,
  every time.

Simplest correct shape: process one BFS level at a time. Read all of level N's
files in parallel, merge them into the map serially in deterministic order, then
scan level N+1. Loses a little overlap; keeps the result reproducible.

---

## What this unblocks

`gather_includes` plus `SourceMap` is the input to everything downstream — the
parser reads from a `Source`, diagnostics resolve offsets through it, and the
compiler needs the deterministic order to decide what to compile first. It is
independently testable today with no parser and no VM.

## Sizing

`source.{hpp,cpp}` ~200 lines, `includes.{hpp,cpp}` ~200 lines, tests ~250.
Depends on `strip_comments()` being written first.

## Tests it must pass

- load the same path twice → one `Source`, same `SourceId`, file read once
- `locate()` returns correct line/col for offsets at a line start, mid-line, at
  end of line, and at end of file
- a file with no trailing newline still reports its last line correctly
- diamond include (`a`→`c`, `b`→`c`) → no error, `c` loaded once
- direct cycle (`a`→`a`) and indirect (`a`→`b`→`a`) → error naming the chain
- `satellite.include(satellite)` is **not** treated as a file
- a commented-out include is not followed (proves the stripper is wired in)
- `include("hello")` finds `hello.satl`
- a missing include reports the file and the line that asked for it
- addresses stay valid: hold a `const Source&` to the first file, load 500 more,
  confirm it still reads correctly
- parallel gathering produces byte-identical `order` to serial gathering
