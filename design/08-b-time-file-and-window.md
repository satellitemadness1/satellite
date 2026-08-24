*satellite design docs, §8, part 2 of 4. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§8 part 1](08-a-numbers.md), On: [§8 part 3](08-c-bool-characters-and-maps.md).*

---

## 8. Types — time, file and window

*Continues [§8 part 1](08-a-numbers.md).*

### 8.2 Time

**Verified**: time cannot be a `double`. Epoch nanoseconds now is 1785988800000000000,
needing 61 bits; a double's 53-bit mantissa gives 198 ns resolution at the current epoch, so
`satellite.time.now()` called twice in quick succession can return the identical value.

`struct Time { int64_t ns; }` — 8 bytes, does not grow the variant (SatString's 32 bytes
already dominate). **Absolute instant only** in v1: nanoseconds since the Unix epoch, UTC,
no timezone stored, no instant/duration mode flag. `a.minus(b)` returns a `number` of
nanoseconds. Adding `satellite.variable.duration` later only adds overloads.

### 8.3 File and window are reference types

The immutable-Value contract **survives**, but only because it was never a claim about the
resources a Value names. The `shared_ptr` member never changes after construction, so the
Value node's bytes are immutable and the lock-free snapshot protocol is completely
unaffected. What is mutable is the OS file description behind it.

State the split explicitly rather than hedging:

> satellite has **value semantics for values** and **reference semantics for the external
> resources that `file` and `window` name**. Copying a file value copies the handle, not the file.

True value semantics is not achievable: `dup()` shares the file offset, so independent
offsets require re-`open()` by path, which fails for pipes, sockets and unlinked files.

Ship an explicit `my_file.close()` returning a status — a destructor cannot report that
`close()` failed with ENOSPC/EIO, and buffered writes commit at close. After close, other
snapshots see a closed handle and get a clean language-level error; never silently reopen.
Guard fd state with `std::atomic<int>`. RAII in the destructor stays as the backstop.

**Require accessor methods (`my_window.height()`); do not add bare field access.** R4
specified method calls, and consistency matters more than the saved parentheses here.

### 8.3.1 What landed for `file`

**Done.** `satellite.file.open(path, mode)` returns a `satellite.variable.file`, and the
value answers `.ok()`, `.path()`, `.error()`, `.read()`, `.write(s)` and `.close()`. The
handle is the `shared_ptr<FileHandle>` §8.3 specified, with `std::atomic<int>` guarding the
descriptor, an explicit `close()` reporting its own status, and RAII in the destructor as
the backstop. The window is not built, so M7 is what remains of this section.

Two decisions §8.3 did not have to make, both forced by the first line of code that opens
a file:

- **`open` is the only constructor**, and a declaration cannot make one. Opening needs a
  path and somewhere to report failure, and `satellite.variable.file f` offers neither, so
  it is `nil` — **verified** — for the same reason §14 makes a spacesuit-typed field `nil`:
  a reference may name nothing.
- **A failed open is a value, not an error.** The handle comes back holding `errno` and the
  caller asks `.ok()`. Failing at the call site instead would make "does this file exist"
  unanswerable without killing the program that asked, which is the one question a script
  most often has.

Modes are the strings `"read"`, `"write"` and `"append"` rather than flags, because the
language has no enum and wants none: the three words say at the call site what a bitmask
never does.

**§10's two Library fixes are no longer hypothetical.** They were filed as harmless "before
resources land", and a file is a resource: a top-level `satellite.variable.file` declaration
writes the handle into `satellite.library`, and reassigning that variable now destroys the
old handle — `close(fd)`, which can block for seconds on NFS or FUSE — while the write lock
is held. Neither fix has been made.

### 8.3.2 The rest of the file surface, and the two things it refused

**Done.** `satellite.file.new(path[, mode])`, `.clear()` and `.open()` land, and the mode
words gain a fourth: `"read_append"`, a handle that can be written and read back. The
surface is now

```
satellite.file.open(path, "read" | "write" | "append" | "read_append")
satellite.file.new(path)              -- read_append, already open
satellite.file.new(path, mode)
    .ok() .path() .error() .size()
    .read()      the whole file, from the beginning
    .write(s)    exactly the bytes of s; the mode decides where they land
    .clear()     truncate to zero and rewind
    .open()      reopen after a close; a live handle is left alone
    .close()
```

**Read-and-write is a fourth WORD and not a second argument**, and that is the decision the
rest depends on. The second slot has meant the access mode in every call written since
§8.3.1, so giving it a second axis would silently reinterpret calls that already exist; an
optional third slot was the other candidate and dies below. A fourth word costs one row in
the mode table and one id in `format.def`, reads at the call site exactly as the other three
do, and **leaves every existing call unchanged** — verified: the thirteen `satellite.file.open`
call sites in `eval_test.cpp` are the only ones in the tree, `view_forge` and `example/` have
none, and not one of them was edited.

`"read_append"` is `O_RDWR | O_APPEND` and never a bare `O_RDWR`, which is forced rather than
preferred. POSIX gives one file offset per open file description and the language has no
`.seek()`, so on a bare `O_RDWR` handle a write would land wherever the last read left that
shared offset — a position no satellite program can observe, name or control. `O_APPEND` makes
the answer stateless and atomic, and it is exactly what was asked for: a write goes on the end.

**`.read()` is the whole file, from the beginning, on every handle.** It always was, and
nothing said so — every open starts at offset 0 and nothing in the language moved it, so "from
the current offset" and "the whole file" were the same sentence with one reachable exception,
a *second* `.read()` on the same handle. **Measured before the change**: two
`.read().length()` calls on a six-byte file printed `6` then `0`. `""` is how an empty file
reads, so the old answer was not merely unhelpful, it was a different fact wearing the same
string. `read_append` is what forced the decision rather than merely permitting it, since
`O_APPEND` leaves the offset at the new end and the read-back would have been that same
misleading `""`. A pipe is the honest exception: `ESPIPE` has no beginning to return to, and
there "read on from here" is the only meaning the call can have.

**`.write(s)` did not change, and the request that it "append the line" is met by the mode.**
**Verified by running it first**: `f.write("one")` then `f.write("two")` produced the six bytes
`onetwo`, so the call has always appended to what the handle already wrote and has never added
a separator. Adding a newline would have moved every byte count in every existing program by
one per call, would have made a file with no trailing newline unwritable, and would have taken
a second meaning for a word — §8.7's rule — when §3.4 has already given the language a real
`\n` to write at the call site.

**`satellite.file.new` refuses to clobber.** `O_CREAT | O_EXCL`, so a path that already exists
comes back `.ok()` false holding `EEXIST` — a value, by §8.3.1's rule, since "does this file
already exist" is a question a script asks constantly and has to survive being answered yes.
The argument is not squeamishness: `satellite.file.open(path, "write")` is already
`O_CREAT | O_TRUNC`, so a truncating `new` would be a *second* spelling of an operation the
language has while leaving "make this only if it is not there" with none. `O_EXCL` is also the
kernel answering atomically, where a check-then-create written in satellite would be a TOCTOU
race. The mode is optional and defaults to `"read_append"`, which is the whole of "not even
require `.open()` for new files": what comes back is already open, in the one mode that can
both write the file you just made and read it back.

**`.open()` does not violate "never silently reopen".** Read §8.3's sentence in place — it is
the answer to what the *runtime* does when a snapshot uses a closed handle, and that answer is
unchanged: `.read()`, `.write()` and `.clear()` each refuse a closed descriptor and say so, and
not one of them reopens anything on the program's behalf. What the rule governs is the reopen
nobody asked for. `f.open()` is a line of source with a status the caller reads, which is the
opposite of silent. Two narrowings keep the snapshot contract whole: a handle that is **already
open** answers true and does nothing — the same answer `.close()` gives a second close, and the
only one that neither leaks the live descriptor nor swaps the file description out from under
every other snapshot with no close having happened — and a reopen races through
`compare_exchange_strong`, so the loser closes the descriptor it just opened rather than
dropping one nothing will close. `.open()` is never *required*, on a new file or an old one,
because both constructors already hand back an open handle; it exists for the two cases a
constructor cannot cover, a handle that was closed and a failed open whose reason has gone away.

**`"text"` and `"binary"` were refused, not invented, and the refusal says why.** The request
wrote the word unquoted — `satellite.file.open("name", text)` — which §1 already settles:
**run and confirmed**, a bare identifier names something the user owns, so it fails with
`no such variable: text`. Quoted, it is refused on its merits, and there are three pieces of
evidence rather than a preference:

1. **POSIX has no newline translation to switch off.** `fopen`'s `"b"` is documented as having
   no effect and `open(2)` has no equivalent bit. There is nothing for the flag to control.
2. **A satellite string is already a byte container.** `encode_byte` maps every one of the 256
   possible bytes to exactly one `SatChar` — the code table for the ones it names, §8.5's raw
   area for the rest — and `decode` maps each back, so `.read()` round-trips arbitrary bytes
   today. A `"binary"` mode would be spelled the same way `"text"` is, byte for byte. There is
   a test that writes all 256 and reads them back through no mode at all.
3. **The one transformation that *would* differ is `encode()`**, which expands backslash
   escapes — and §3.3 has caught that exact substitution three times, most recently inside
   `.read()` itself. Making it a mode would be shipping a known defect as a feature.

So the refusal is loud rather than quiet, the way §18 refuses a digit count past
`MAX_RANDOM_DIGITS` instead of clamping it and §8.6 makes a missing key an error instead of a
nil that would be indistinguishable from a stored one. A flag that did nothing would be worse
than no flag, because a program that passed `"binary"` would believe something had been
arranged for it.

**`FileHandle` grew three fields and one of them was already there.** `writable` had been in
the struct since §8.3.1 and *nothing read it* — set at open, consulted nowhere, recording an
intention instead of enforcing one. It is live now and `readable` joins it, because
`"read_append"` is what made the mistake they guard ordinary: while every mode went one way,
asking for the other produced `EBADF`, and "Bad file descriptor" is the wrong sentence about a
descriptor in perfect health. `reopen_flags` is the third, and it is not simply the flags the
handle was opened with: `satellite.file.new` adds `O_EXCL`, and replaying that on a reopen
would fail with `EEXIST` against the program's own work. `sizeof(ValueBase)` is untouched at
40 — a `FileHandle` lives behind the `shared_ptr`, which is the whole point of §8.3's reference
type.

**`open` is the first word in the registry that is both a selector and a segment of a path.**
It is word 31 in `my_file.open()` and word 31 in `satellite.file.open(path, mode)`, one word
with one id, because the space is flat and that is what flat means. Nothing is ambiguous, for
§7's own reason: the method desugars to `satellite.variable.file.open(my_file)`, whose
quadruple is `{1,17,25,31}`, against the module function's `{1,25,31,0}`, and `find_path` keys
on all four segments. `format_test` carried a check that no selector appeared anywhere in the
arity table; it was a true observation about the table as it stood, frozen into an invariant it
never was, and it is now the narrower statement that is actually true — no selector is a path
*by itself*. That is the third stale generalisation in that file to be narrowed to what it
meant, after §17.5's literal selector range and the two hardcoded holes in its report.
