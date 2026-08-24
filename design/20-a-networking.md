*satellite design docs, §20, part 1 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§19 part 2](19-b-input-arguments-and-booleans.md), On: [§20 part 2](20-b-the-wire-format.md).*

---

## 20. Networking, and sending a satellite value to another machine

Status: **plan only. None of this is built.** It is written down first
because a wire format is the one kind of decision that cannot be revised
quietly: the moment two machines have exchanged a byte under it, every
choice in it is load-bearing forever. §17 learned that about a *file*
format and spent four defects doing it; a network format has the same
property and a wider blast radius, because the two ends are not upgraded
together.

Everything below is either **decided** with a reason, or **open** and marked
so. Nothing here is marked verified, because nothing here has been run.

### 20.1 One type, three constructors

The request was for three types — `satellite.variable.network`,
`satellite.variable.http`, `satellite.variable.https`. **It should be one type
and three constructors**, and the argument that decides it is not taste:

> `matches()` (src/evaluator/types.cpp:31-45) is exact-name equality for the
> `variable` space, and the only subtype relation in the language is `is_a()`,
> which is spacesuit-only. So a capsule declared to take a
> `satellite.variable.http` could **never** accept an https value — forever,
> with no recourse, because §12 defers user-defined generics.

Two precedents already in the language cut the same way. §8.3.1 makes a file's
mode a **string** and not a type — "the three words say at the call site what a
bitmask never does" — and protocol is to a socket what mode is to a file. And
§18's three random tiers are three module *paths* onto **one** type: `fast`,
`normal` and `ultra` all return a `satellite.variable.number`. The distinguishing
word lives in segment position, which is exactly the requested surface.

§8.7's rule settles the rest: "a word means one thing, and dispatch on the
receiver is for types that answer the *same* question differently, not for types
that answer different questions." `http` and `https` answer the same questions —
`.ok()`, `.read()`, `.write(s)`, `.close()`. The whole difference is a layer
under the bytes.

The **constructors survive verbatim**; only the declaration's type word collapses:

```satellite
satellite.variable.network my_server = satellite.network.open(8080)
satellite.variable.network my_http   = satellite.network.http(8080)
satellite.variable.network my_https  = satellite.network.https(8443, "cert.pem", "key.pem")
```

with `.protocol()` answering `"tcp"`, `"http"` or `"https"`.

Counted: one alternative costs ~9 dispatch edits of which **only 2 are caught by
the compiler**; three alternatives cost ~21, of which 15 are silent. §20.3 lists
which.

**`open`, not `new`.** §14 says "A declaration **is** the construction site;
there is no `new`" — that is about spacesuits, and §8.3.1 already drew the
resource exception, because `satellite.variable.file f` has nowhere to put a
path or an errno. A socket is the same case, so a module-function constructor is
required and §14 is not contradicted. But the *word* should be `open`: every
constructor the language has names the act rather than the allocation
(`satellite.file.open`, `satellite.time.now`), `new` is C++'s and Java's word —
§16 rejected `module` on exactly that ground — and it reuses word 31 for zero new
ids. This is a preference about *reading*, not a collision: §17's registry is flat,
"one id per distinct word, flat across all positions", so `new` is minted once
and every path may use it. `satellite.file.new` and `satellite.thread.new` are
`{1,25,100}` and `{1,<thread>,100}` — different paths, different arities,
different acts, one word. `open` (31) already does exactly this as both a
selector and a path segment. So `satellite.network.open` is a choice about what
reads truest for a socket, and nothing anywhere has to be coordinated.

### 20.1.1 A client and a server are different objects

```satellite
satellite.variable.https thing =
    satellite.network.https("example.com/data.json", 443)

satellite.variable.https server =
    satellite.network.https(8443, "cert.pem", "key.pem")
```

A client connects out, asks for one thing, reads the answer and is done. A
server binds a port and waits for other people. They have different
lifecycles, different failure modes and different arguments, and folding
them into one constructor produces a signature whose arguments are
meaningless half the time.

The client is the form that answers "how do I receive an object", and it is
also the tractable half — see §20.2.

```
thing.ok()                  did it work at all
thing.error()               and if not, why, in words
thing.status()              200
thing.header("content-type")
thing.length()              bytes received
thing.body()                the bytes, as a satellite.variable.string
thing.save("local.json")    or straight to a file, never resident
thing.close()
```

`.body()` can hold a PNG or a tarball, because §8.5 gives a satellite string
a raw area of 256 codes — one per possible byte — so arbitrary bytes
round-trip through one uncorrupted. `.save()` exists for what should not be
resident, which is §9's argument about the Console one level over.

`satellite.network.http(...)` is the same shape without TLS.
`satellite.network.new(port)` is a raw TCP socket under the same rules.

**The port is always written out.** There is no implied 443 or 80. A hidden
default is a thing the reader has to know, and §8.3.1 already made this
trade for file modes: the word at the call site says what a bitmask never
does.

**A failure is a value, not an error.** A refused connection, a bad
certificate, a port already bound — the handle comes back holding the reason
and the caller asks `.ok()`. §8.3.1 settled this for `file` and the argument
carries unchanged: "does this host answer" is a question a program most
often wants to *ask*, and failing at the call site makes it unaskable
without killing the program that asked.

### 20.2 What is blocked, and on what

**Raw TCP and plain `http` need no new library.** POSIX sockets are in libc.
That half can be built today.

**`https` cannot be.** TLS needs OpenSSL or GnuTLS. **Measured** here rather
than assumed, 200 runs of `satl --where` with the libraries force-loaded:

| build | `ldd satl` | startup | delta |
|---|---|---|---|
| today | **6** | 2.520 ms | — |
| + OpenSSL (`libssl`, `libcrypto`, `libz`) | **9** | 3.520 ms | **+1.0 ms, +40%** |
| + GnuTLS (and its five) | **12** | 4.307 ms | **+1.79 ms, +71%** |

TLS is 23× cheaper than §9's gtk figure of 23.4 ms — and it is still **40% of
everything the split bought**, paid by every headless run that never opens a
socket. M7's own done-when asserts the number literally: "`ldd satl` still lists
six objects". §18 already treats this as a design constraint, choosing
`getrandom(2)` over any crypto library because "one syscall, no dependency ...
and `ldd satl` stays at six".

§16 already designed the way out: a native module the runtime `dlopen`s only
when a program writes `satellite.include(satellite.network)`. That mechanism
is **M7 and is not built**. So `https` waits on infrastructure rather than on
anybody's opinion, and `http` is where work can start.

**There is no third way out.** `grep` for `fork`/`execvp`/`popen`/`system(`
across `src/` returns nothing outside tests: the language cannot spawn a
process, so shelling out to `curl` is not an escape hatch either.

**`accept()` blocks.** A server that serves one caller at a time and does
nothing while it waits is still a real server — a health endpoint, a webhook
receiver, a control socket — and it works today. What needs threads is
*concurrent* connections. Networking needs exactly one thing from
`satellite.variable.thread`: a capsule run on another thread with a connection
value as its argument. Accept, then hand off.

**`satellite.network.https(port)` is not a constructible server**, and this is a
defect in the requested surface rather than in the implementation. A TLS server
needs a certificate chain and a private key; a port is neither. So either that
form means a *client* — in which case the argument is a host and not a port —
or it is a server and needs both filenames. The three constructors are not
symmetric and cannot be made so. Settle it at the surface, before any shim.
