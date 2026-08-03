# satellite_source and recursive include gathering

Answers the question: where does all the source text live once includes are
followed recursively?

**Not a `std::vector<std::string>`.** That loses which file a token came from,
which line it was on, and who included it — and it invalidates pointers when it
grows. Details below.

---

## The storage decision

### Why not a vector of strings

| you will need | a `vector<string>` gives you |
|---|---|
| "error in which file?" | nothing — index only, no path |
| "error on which line?" | nothing — you would re-scan for newlines every time |
| "included from where?" | nothing — no parent link, so cycle errors cannot print the chain |
| stable text addresses | **no** — `push_back` reallocates and invalidates every pointer and `string_view` into it |

That last row is the one that bites hardest and latest. The moment a token holds
a `string_view` into source text — which is the whole reason to avoid copying
identifier text — a reallocation turns every previously-scanned token into a
dangling reference. It will not crash immediately; it will produce garbage
identifiers under memory pressure, on large programs, intermittently.

### Measured: contiguous text + line index vs vector<string> of lines

The obvious alternative is one `std::string` per LINE. It was measured on a
200-file / 116,866-line corpus (bench/source_bench.cpp equivalent):

```
                                 load ms    heap MB
A) one string + line index           7.0       4.90
B) vector<string> per line          12.6      11.56
   B vs A                          1.79x      2.36x

walk every line 20x                   ms
A) one string + line index           5.7
B) vector<string> per line           6.4        <- no difference

allocations for the corpus
  A:     400     (one string + one index per file)
  B: 117,066     (one string per LINE)
```

The appeal of `vector<string>` is easy line access -- but `line_starts` already
provides that. `line(n)` is a direct index returning a `string_view`, no search
and no copy, and walking every line measures the same either way. So the line
storage adds 2.4x memory and 293x the allocations to buy something the index
already gives.

**The deciding issue is correctness, not speed.** `satellite.cxx { }` blocks
span lines. Contiguous storage hands the block over as a substring, byte for
byte. Line storage has to stitch it back together -- and `getline` ate the
newlines, so the rebuilt block came back 73 bytes where the original was 74,
with trailing whitespace and any `\r` unrecoverable. That block goes straight to
a C++ compiler; losing its exact bytes is a bug, not a slowdown. The same
applies to multi-line block comments and to any multi-line string literal added
later.

A text *editor* genuinely does want line storage -- inserting a character should
not rewrite a 4 MB buffer -- but that is the GUI's concern, and `GtkTextBuffer`
keeps its own line-based representation regardless. `Source` is the compiler's
view, and the compiler wants bytes.

### The shape that works

One `Source` per file, owned by one `SourceMap`, with **stable addresses**:

```cpp
// include/satellite/source.hpp
namespace satellite {

using SourceId = uint32_t;
constexpr SourceId kNoSource = 0;      // 0 reserved for "unknown / not from a file"

class Source {
public:
    SourceId    id   = kNoSource;
    std::string path;                  // canonical absolute path ("" if virtual)
    std::string name;                  // what to print in diagnostics
    std::string text;                  // the file contents, owned here

    SourceId    included_from = kNoSource;   // who pulled this in
    int         included_at_line = 0;        // and from which line

    // byte offset of the start of each line; built once at load
    std::vector<uint32_t> line_starts;

    void locate(uint32_t offset, int* line, int* col) const;   // binary search
    std::string_view line_text(int line) const;                // for error carets
    int line_count() const { return (int)line_starts.size(); }
};

class SourceMap {
public:
    // Canonicalises the path. If already loaded, returns the existing id and
    // does NOT re-read the file -- that is the deduplication.
    SourceId load(const std::string& path, std::string* err);

    // For tests, the REPL, and the GUI's text buffer.
    SourceId add_virtual(std::string name, std::string text);

    const Source& get(SourceId id) const;
    size_t        size() const;

    // "a.satl:12:5" plus the include chain that reached it
    std::string describe(SourceId id, uint32_t offset) const;
    std::string include_chain(SourceId id) const;

private:
    std::deque<Source> sources_;                        // STABLE addresses
    std::unordered_map<std::string, SourceId> by_path_; // dedup + cycle detection
};

}  // namespace satellite
```

**`std::deque`, not `std::vector`.** A deque never moves existing elements when
it grows, so `const Source&` and any `string_view` into `text` stay valid for the
life of the map. `std::vector<std::unique_ptr<Source>>` also works and is more
explicit at the cost of an extra indirection; either is fine, but a plain
`vector<Source>` is not.

**`SourceId` is an index, not a pointer.** Tokens carry a `SourceId` plus a byte
offset — 8 bytes total, trivially copyable, survives anything the map does.

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
