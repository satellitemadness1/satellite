*satellite design docs, §20, part 3 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§20 part 2](20-b-the-wire-format.md), On: [§21](21-binary-and-hexadecimal.md).*

---

## 20. Networking, and sending a satellite value to another machine — the frame, and what a receiver refuses

*Continues [§20 part 2](20-b-the-wire-format.md).*

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

### 20.4.1 A version the receiver does not know

Unit 1 carries the format version. The rule for reading it has to be written
down *before* anything ships, because it is the one rule that cannot be added
afterwards: a receiver that shipped without it has already done something else
with an unknown version, and that behaviour is now the format's.

**A receiver refuses a version it does not speak, and names both numbers.** Not
"bad packet" — §20.5 — but "this program speaks wire format 1; the sender sent
3". A version it has never heard of is a stream whose **tags may not mean what
they say**, so there is nothing safe to attempt. A partial parse of an unknown
format is a decoder being driven by a stranger, which is the whole subject of
§20.6.

**Backward is supported; forward is refused.** A newer receiver still reads an
older stream, forever. That asymmetry is the entire return on the registry's
mint-never-reuse rule (§20.6.1) — if old streams stopped being readable, id 56
could be recycled and the rule would be costing without buying. Old versions
staying readable is what that permanent scar is *for*.

**There is no negotiation and no downgrade.** Two ends do not agree a version
between themselves. A sender writes the version it was built with; a receiver
reads it or refuses it. A handshake is a second protocol layered under the
first, and §20.8 defers schema languages and asynchrony on the same ground.

**The version is bumped when the registry changes, and never otherwise.** It is
not a build number, not a release number, and not a date. Two builds of `satl`
that encode the same values identically carry the same wire version however far
apart they were built. A version that moved for unrelated reasons would be a
field receivers learn to ignore, and a version field that is ignored is worse
than none — it looks like a check.

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

**Next free `format.def` id is 111.** §21 took 102 through 110, on top of the
`file` work's 100 `new` and 101 `clear`. This sentence has now been wrong twice
— it said 80, then 102 — which is an argument *for* the rule and not against
it: `format.def` is the truth, prose about it drifts, and the assert that
catches the drift is `kWordIds[kWordCount - 1] == 110` in `format_test.cpp`.
Read the file, never this line.

Adding rows fires three deliberate tripwires in `format_test.cpp` (the last-id
assert, the selector count, and the selector-range check, which needs a
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

### 20.6.3 How a program receives — DECIDED 2026-08-24

The question §20 had left unanswered is "how do we receive bits", and the answer
is that **a port receives nothing; a connection does.** Three steps, and the
middle one is the one that was missing a name:

1. **open** — claim the port. What comes back is a **listener**. A listener has
   no `.receive()`; its only job is to answer "has anyone knocked?"
2. **accept** — wait. When someone knocks this hands back a **second object**, a
   connection. One listener produces many connections over its life.
3. **receive / send** on that connection. Only now do bytes move.

```satellite
satellite.variable.network door = satellite.network.open(8080)

satellite.statement.while(door.ok()) {
    satellite.variable.network caller = door.accept()   // waits here
    thing = caller.receive()                            // the object itself

    satellite.console.display(thing)
    caller.send(thing)
    caller.close()
}
```

and the one-liner, for a program that receives and never answers:

```satellite
thing = satellite.network.receive(8080)
```

**A listener and a connection are ONE type with different methods answering**,
the way §20.1 collapsed the three protocols. Calling `.receive()` on a listener
is refused in words that name the fix — it does not silently accept.

**THE RECEIVER DOES NOT DECLARE WHAT IS COMING, and does not need to.** The wire
format is self-describing: §20.3's every value opens with a tag saying what it
is, which is the whole reason it has tags. The decoder reads the tag and builds
what it says. The language already has the landing spot and it needed no
addition — **verified**: a bare `thing = 5` binds with no type named, and a bare
`satellite.container.list` holds `[5, hi, x0F]`, mixed. So a pile of received
values of unlike type already has a home.

Naming a type is therefore an **assertion, not a requirement**. A program that
knows what it expects writes `satellite.variable.number n = caller.receive()`
and gets a clean type error if something else arrives; a program that does not
know writes `thing = caller.receive()`. Both are ordinary.

**The one thing that must be known ahead is a spacesuit**, and §20.3.5 already
settled why: the descriptor is verified, never constructed, because a receiver
that builds a type from the wire is a receiver that can be made to run something
it never declared. §16 is the agreement mechanism.

### 20.6.4 What the wire owes the user

The language's purpose governs this section as much as any: **do absolutely
everything for the user**, and never make them think about the transport. So the
codec is **entirely invisible** — there is no `.serialize()`, no buffer size, no
framing to get right, no byte order to consider, no partial read to handle.
`caller.send(thing)` and `thing = caller.receive()` are the whole surface, and
`.receive()` blocks until a WHOLE value has arrived rather than ever handing
back half of one.

There are no flags and no bitmasks anywhere on this path. If an option is ever
needed it is a **word** at the call site, which is the trade §8.3.1 made for
file modes and §20.1 made for protocols.

**"Do everything for the user" is not "do things behind the user's back",** and
the language already drew that line: §8.3's rule for `file` is **never silently
reopen**. So this path does all the plumbing — framing, byte order, buffering,
sharing, cycles — and none of the policy. It never silently truncates a message,
never silently converts a type, and never silently reconnects a dropped
connection. §20.6's ceiling refuses in words rather than quietly delivering less
than was sent, which is §18's refusal-not-clamp.

### 20.7 Build order

1. `satellite.network.new(port)` and raw TCP. No new dependency; proves the
   handle, the failure-as-a-value rule and `.ok()`/`.error()`.
2. The encoder and decoder for the **value** types: nil, bool, number,
   string, time, binary/hex, list, map. Written as an exhaustive `std::visit`
   with no `auto` fallback (§20.3.1), so the compiler refuses the build when a
   twelfth `Value` alternative lands. Testable with no socket at all — encode,
   decode, compare — which is how it should be tested first.
3. Spacesuit instances and the descriptor check (§20.3.5).
4. Sharing and cycles (§20.3.4), with a test that a cyclic object graph
   round-trips and terminates.
5. `satellite.network.http` on top of 1 and 2.
6. `https`, **after** §16's native module mechanism (M7) exists.
7. The server half, **after** `satellite.variable.thread` exists. §20.6.3 is
   the surface it builds; the blocking single-caller form works before threads
   and is where `accept()` should first be proven.

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
