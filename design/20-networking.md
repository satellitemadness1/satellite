*satellite design docs, §20 of 20. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§19](19-nine-additions.md).*

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

### 20.3 The wire format: a satellite value on a socket

This is the part that is bigger than the sockets, and the part worth getting
written down.

The goal is that a program on one machine can hand a satellite value to a
program on another and have the same value arrive: a number, a string, a
list, a map, a bool, a time, and an instance of a spacesuit the two of them
share.

**§17's container is the starting point and not the answer.** That format was
designed to carry *code* to a VM on the same machine. This one carries *data*
between machines that were built separately. Three of its decisions transfer
unchanged and one of its assumptions does not:

- **transfers**: the length that carries a continuation bit, so nothing in
  the format has a fixed ceiling (§17.3). A satellite `Number` has an
  unbounded significand by definition; any fixed width would smuggle back
  the machine integer §8.1 spent a section removing.
- **transfers**: the endianness marker in the header. §17 put one in because
  the Debian packages are `Architecture: any`. Over a network it stops being
  prudence and becomes the point — the two ends are not the same build, and
  may not be the same architecture.
- **transfers**: a format version word, bumped when the registry changes.
- **does not transfer**: that both ends were produced by one compiler at one
  version. A file format can assume its reader is the writer's sibling. A
  wire format must assume the other end is older, newer, or hostile.

#### 20.3.1 What may cross, and what is refused

| type | crosses | as |
|---|---|---|
| nil | yes | a tag, no payload |
| `bool` | yes | a tag |
| `number` | yes | sign, exponent, significand digits (§20.3.3) |
| `string` | yes | a count of `SatChar`s and the codes (§20.3.2) |
| `time` | yes | int64 nanoseconds since the Unix epoch, UTC (§8.2) |
| `list<T>` | yes | a count, then the elements |
| `map<K,V>` | yes | a count, then key/value pairs **in insertion order** |
| spacesuit instance | yes, conditionally | a type id, then its fields (§20.3.5) |
| `file` | **refused** | |
| `window` | **refused** | |
| `thread` | **refused** | |

**The last three are a refusal and not an omission.** §8.3 draws the line
this rests on: satellite has "value semantics for values and reference
semantics for the external resources that `file` and `window` name". A
descriptor is a number that means something to one kernel. Sending it would
produce a handle on the far side that names a different file, or nothing, and
`.ok()` would answer `true` about it. That is worse than refusing, and this
language refuses rather than pretends — see §18's `MAX_RANDOM_DIGITS`, which
is a refusal "rather than a clamp: a program that asks for a million digits
has made a mistake, and silently handing back a hundred thousand would hide
it."

Sending a `file` is a mistake with an obvious right answer, so the error says
it: send `my_file.read()`, which is a string, and is data.

The map's **insertion order is part of the wire format**, not an accident of
it. §8.6 made the map insertion-ordered because "§5's only loop is the
C-shaped `for`, so a map is walked by indexing `.keys()`; an unordered map
would make every program that walks one non-deterministic". A wire format
that dropped the order would put that non-determinism back, across machines,
where it is harder to see.

#### 20.3.2 Strings, and the one genuinely hard problem

**A satellite string is not text.** §8.5: `SatChar` is `char16_t` over
satellite's own 101-code table plus a 256-code raw area, and — the part that
matters here — six of those codes are **live**. `\home`, `\cwd`, `\user`,
`\threads`, `\memtotal`, `\memused` are single codes that expand at
`decode()` time, so a string "reflects the current directory *now* rather
than when it was built".

That makes one question unavoidable, and it has no answer that is right in
every case:

> A program sends a string containing `\home`. Does the receiver see the
> **sender's** home directory, or its **own**?

**Decision: send the codes, so the receiver sees its own.** Three reasons,
in order of weight.

1. It is what the type already means. §8.6 hit the same fork for map keys and
   went the same way, for the same reason: "hashing decoded text would make a
   key's identity depend on the machine, the user and the current directory".
   A string that decoded differently on two machines is not a bug in the wire
   format; it is the type behaving as specified.
2. The alternative leaks. Expanding before sending puts the sender's username
   and home directory on the wire in every string that happens to contain one
   — silently, in a program that never meant to disclose either.
3. It is reversible and the other way is not. A program that wants the
   sender's expansion can send `decode`d text by asking for it; a program
   that receives pre-expanded text cannot recover the code.

**The consequence must be documented at the call site, not here**, because it
is genuinely surprising: two strings that are `==` on the sender may not be
`==` on the receiver, and a program that round-trips a path through the
network gets a *different, correct* path back.

**Open**: whether `satellite.network` should offer an explicit
`.send_expanded(s)` for the cases where the sender's view is the intended
one. It costs one word and closes the hole in the reasoning above. It is not
taken here because a second spelling for one type invites the question of
which is the default, and the default should be the safe one.

#### 20.3.3 Numbers

§8.1 makes a `Number` an exact arbitrary-precision decimal: a `long long`
significand, an `int` exponent, and a `shared_ptr<const BigInt>` of base-10⁹
limbs behind it when the value is large.

On the wire it is **sign, exponent, digit count, digits** — decimal digits,
not limbs. Sending base-10⁹ limbs would be smaller and would put an
implementation detail into a format two independently built programs have to
agree on; §17.3 makes the same call for the same reason, and the cost is one
conversion at each end of a type whose whole point is exact decimal I/O.

The digit count carries the continuation bit of §17.3. There is no ceiling,
because §8.1 removed the ceiling from the language and "a 64-bit cap in its
bytecode would be smuggling one back in through the basement" — a cap on the
wire would be the same smuggling with a longer wire.

**A receiver must bound what it will allocate**, which is a different
question from what the format can express. See §20.6.

#### 20.3.4 Containers, sharing, and cycles

A list is a count and then its elements. A map is a count and then its pairs.
Both recurse.

**Shared substructure is preserved, and it has to be.** §8.7 already
established that a list holding the same sublist twice holds *one* sublist,
and that `.size()` "counts each distinct piece once" by carrying a set of the
storage it has already seen. A wire format that ignored sharing would
duplicate it — turning a compact structure into an exponentially larger one —
and on a **cycle** would never terminate at all.

So the encoder carries the same seen-set `.size()` does, and emits a
**back-reference** for storage it has already written. §12 records that "an
object *can* close a `shared_ptr` cycle, which no previous value could", and
§8.7 already had to solve exactly this to make `.size()` terminate. This is
that mechanism, one level out.

The decoder must therefore accept a back-reference to something it has
already built, and must **refuse a forward reference** — a back-reference to
an index it has not yet seen is a malformed stream, not a puzzle to solve.

#### 20.3.5 A spacesuit instance, and the type-agreement problem

This is the entry with the most ways to be subtly wrong.

An instance is a type plus field values. §14 flattens the layout at resolve
time — "a superclass's fields and methods are copied into the subclass",
superclass fields first — so the *values* are an ordered vector and are easy.
The type is the problem: the receiver must have the same spacesuit.

**Decision: send a type descriptor and verify it; do not send the type.**

The descriptor is the spacesuit's name, its superclass's name, and the
ordered list of its field names. The receiver matches that against the
spacesuit it has of that name and **refuses on any mismatch**, naming the
first field that differs.

Three alternatives were considered and rejected:

| alternative | rejected because |
|---|---|
| send the name only | two programs with different `point` spacesuits would exchange garbage that type-checks. Field order is exactly what §14's flattening makes load-bearing. |
| send the field *types* as well | it is stricter than the language is. Satellite checks types at insertion and declaration (§8, §12 defers a static checker); demanding structural type equality on the wire would refuse programs the interpreter would run. |
| send the class definition itself | that is code, and a receiver that builds a type from the wire is a receiver that can be made to run something it never declared. §20.6. |

**The right way to agree is §16.** Two programs that exchange objects should
`satellite.include` the same spaceship holding the declaration, exactly as
two files in one program do. The descriptor is then a *check* that they did,
rather than a mechanism for pretending they did.

**Identity does not survive the wire, and cannot.** §14: "Equality is
identity … Two instances with equal fields are two instances." Sending an
object necessarily makes a copy, so the received object is never `==` to the
sent one. This is not a limitation to be engineered around; it is what
sending a reference type to another address space means. It has to be said
out loud, because a program that sends an object and compares it back will
otherwise conclude the network corrupted it.

**A protected field still crosses.** Access control is a rule about what
*code* may name (§14), not a claim about what the bytes are; `.size()`
already walks fields the caller cannot read. A program that does not want a
field on the wire should not put it in an object it sends.

### 20.4 The frame

```
unit 0   magic, and a known byte pattern that reveals byte order
unit 1   format version | flags
unit 2   payload length, with §17.3's continuation bit
unit 3.. the value
```

The endianness marker earns its place here more than it does in §17. Two
machines on a network are not two builds of one package; they may genuinely
differ, and a byte-reversed 64-bit word is the kind of bug that survives
testing on one architecture forever.

The length prefix is what lets a receiver reject an oversized message
**before** allocating for it (§20.6), which is the whole reason it precedes
the payload rather than being implied by the framing.

### 20.5 Errors, in one place

Everything failable answers `.ok()` and `.error()`, and nothing on this path
raises. That is §8.3.1's rule and it matters more here: a network is the one
place in a program where failure is *ordinary* rather than exceptional.

The decoder's errors must name what was wrong with the *stream*, never what
was wrong with the *sender* — "field 3 of spacesuit point: expected a number,
found a string" and not "bad packet".

### 20.6 What a receiver must refuse

Deserialising input from another machine is the classic way a program is
taken over, so the posture is stated rather than assumed.

**The format carries data and never code.** There are no capsules on the
wire, no expressions, and no way to name a capsule to call. A decoder that
could construct a type from a descriptor (§20.3.5's rejected third
alternative) would be a decoder that can be made to run something the program
never declared, which is why that alternative is rejected rather than
deferred.

**Every count is checked against what is actually there before it is
believed.** A length field claiming 2^200 elements must be refused, not
attempted — §18's `MAX_RANDOM_DIGITS` is the precedent: "The check is on the
*count* and happens before anything is built."

**A configurable ceiling on one message**, read the way
`satellite.library.system.min_free_mb` is, so a program that means to receive
something enormous can say so and one that does not is protected by default.

**Recursion depth is bounded.** The decoder recurses over nested containers
exactly as `ValuePrinter` and `.size()` do, and §6's depth guard exists
because "a runaway recursion raises a satellite stack-overflow error instead
of segfaulting the C++ stack". A hostile stream is a runaway recursion someone
else chose.

**No tier of this is described as secure until it is.** §18 refuses the word
for `satellite.random` and explains why — "someone will key something on it"
— and the same applies to the `s` in `https`. Certificate verification is on,
always, with no flag to disable it; a client that skips it is `http` with a
misleading name. That needs a CA bundle path, which is a third filename this
design carries and which is easy not to think of.

### 20.6.1 Registry note

**Next free `format.def` id is 102** as of the `file` work landing (which took
100 `new` and 101 `clear`), not the 80 §17's prose still says — that
sentence is stale and `format.def` is the truth, which is the whole reason it
exists. Adding rows fires three deliberate tripwires in `format_test.cpp` (the
last-id assert, the selector count, and the selector-range check, which needs a
**third range** rather than a widened bound — its comment forbids that
one-character fix by name and records the bug it caused). Bump them in the same
commit; that is what they are for.

**Do not mint `https` until it verifies a certificate.** `format.def` records id
56 (`empty`) as a permanent scar — a selector no receiver implements, whose id
stays spent forever because "reusing a number that shipped in a table is how a
format learns to lie". Minting `https` before TLS works repeats that exactly.

### 20.6.2 One laxness worth closing on the way past

`satellite.variable.network n` **already declares a nil today**, before any of
this is built, because `Resolver::check_type` validates a type's *space* and not
its *name*. That is the same class of laxness §8.6 closed for
`satellite.variable.number<A, B>`, and it means a phantom type is spellable now.

### 20.7 Build order

1. `satellite.network.new(port)` and raw TCP. No new dependency; proves the
   handle, the failure-as-a-value rule and `.ok()`/`.error()`.
2. The encoder and decoder for the **value** types: nil, bool, number,
   string, time, list, map. Testable with no socket at all — encode, decode,
   compare — which is how it should be tested first.
3. Spacesuit instances and the descriptor check (§20.3.5).
4. Sharing and cycles (§20.3.4), with a test that a cyclic object graph
   round-trips and terminates.
5. `satellite.network.http` on top of 1 and 2.
6. `https`, **after** §16's native module mechanism (M7) exists.
7. The server half, **after** `satellite.variable.thread` exists.

Steps 2 through 4 are the durable part and depend on no socket, no TLS and no
threads. They are the right place to start for that reason.

### 20.8 Deliberately deferred

- **Anything resembling remote capsule invocation.** §20.6 is why.
- **A schema or interface language.** §16's spaceship is the agreement
  mechanism; a second one would be a second naming rule for no benefit.
- **Compression.** It is a property of a transport and not of a value.
- **Asynchronous I/O.** The blocking form plus `satellite.variable.thread` is
  the whole of the concurrency story until measurements say otherwise, which
  is the trade §10 made for the evaluator and §9 made for the console.
