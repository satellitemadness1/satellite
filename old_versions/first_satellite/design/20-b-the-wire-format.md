*satellite design docs, §20, part 2 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§20 part 1](20-a-networking.md), On: [§20 part 3](20-c-the-frame-and-what-is-refused.md).*

---

## 20. Networking, and sending a satellite value to another machine — the wire format

*Continues [§20 part 1](20-a-networking.md).*

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
| `binary` / `hex` | yes | radix, a digit count, then the digits (§20.3.6) |
| `time` | yes | int64 nanoseconds since the Unix epoch, UTC (§8.2) |
| `list<T>` | yes | a count, then the elements |
| `map<K,V>` | yes | a count, then key/value pairs **in insertion order** |
| spacesuit instance | yes, conditionally | a type id, then its fields (§20.3.5) |
| `file` | **refused** | |
| `window` | **refused** | |
| `thread` | **refused** | |
| `arguments` | **refused** | but see below — it is a `list<string>` to `matches()` |

**Every `Value` alternative has a row here**, and two of the rows — `window`
and `thread` — are types that do not exist yet, refused in advance so that
building them cannot quietly make them sendable. The other eleven rows are the
eleven alternatives `ValueBase` holds, one each. The rows are ordered for a
reader — simple before compound, crossings before refusals — and **not** in
variant order, so the correspondence to maintain is one-to-one and not
positional.

`ValueBase` is `APPEND ONLY` by the comment above it (value.hpp), so an
alternative is only ever added at the end — and when one is, **this table is
stale until a row joins it.** That has already happened twice: §21 appended
`BitsRef` at index 9 and the arguments work appended `ArgsRef` at index 10, both
after §20 was written, and neither had a row here until it was found by hand.
Nothing in the build catches it. `ValuePrinter` and `SizeVisitor` are the only
two sites that fail to compile when an alternative lands, and they do so for one
reason worth copying:
`SizeVisitor` is "one `operator()` per alternative and no generic `auto`
fallback" (value.cpp), precisely so a new alternative cannot be silently billed
as zero bytes.

**So the encoder must be written the same way** — an exhaustive `std::visit`
with no `auto` fallback — because that is what makes the compiler, rather than a
reader's diligence, the thing that keeps this table true. An encoder with a
fallback would ship a new type as whatever the fallback does, which on a wire
format is the failure this whole section exists to prevent.

**The four refusals are refusals and not omissions.** §8.3 draws the line
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

**`arguments` is refused for both reasons at once**, which is why it is worth
its own paragraph. An `Arguments` holds the command line that started *this*
process — argv[0] included — and the environment entries §8's table lists, the
compiler that built the binary and the kernel it is running on among them. It is
local to one process in exactly the way a descriptor is local to one kernel, so
the paragraph above applies unchanged.

But it is also the leak §20.3.2 closed one level up. That section refused to
expand `\home` before sending because doing so "puts the sender's username and
home directory on the wire in every string that happens to contain one —
silently, in a program that never meant to disclose either." An `arguments`
object puts the sender's whole invocation and environment there in a single
call, and it is the one value in the language where that is its entire content.
Refusing it is the same decision, made where it is most obvious rather than
least.

**The refusal must be on the variant alternative and never on the declared
type**, and this is the trap in the row rather than a detail of it. An
`Arguments` **satisfies `satellite.container.list<string>`** — `matches()` says
so deliberately, because that is "what keeps §2's signature — and therefore
hello world, the man page, and every program anybody has written — exactly as it
was" (types.cpp). So a capsule whose parameter is declared `list<string>` will
accept the arguments object without complaint, and a program can reach `.send()`
holding one through a parameter that says `list`. An encoder that decided what
to do from the *declared* type would encode it as a list and put the sender's
environment on the wire, having passed every check. Dispatching on the
alternative is what makes the refusal real.

**And the error must call it by its own name.** `type_name_of` answers
`satellite.container.arguments` rather than `.list` for exactly this reason —
"an arity or type message that named it a list would send the reader to the
list's method table, which does not have `.names()` or `.count()` in it"
(types.cpp). A refusal that said "cannot send a `list<string>`" would be both
confusing and false: lists send fine, and the reader would go looking for what
is wrong with lists.

The right answer is the same shape as the file's: a program that means to send
one entry sends `args.get("name")`, which is a string, and is data — and one
that means to send several builds the list it actually wants. That is more
typing than handing over the whole object, which is the point. §20.3.2 took the
same trade for the same reason: the leak is the default that is easy to reach
for, so the safe thing is the one that is reachable and the leak is the one that
is not.

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

#### 20.3.6 Binary and hexadecimal

§21's `satellite.variable.binary` and `satellite.variable.hex` are one type
underneath — a `Bits` is a radix of 2 or 16 and a string of digits — and they
cross as **radix, digit count, then the digits**. That is §20.3.3's rule for a
number applied one type over: send the digits, not a packed representation.

Packing four bits to a nibble would halve the payload, and it would do something
worse than putting an implementation detail on the wire. **The digit count is
part of the value and must survive.** §21 fixed the width deliberately — 22
digits in, 22 digits out — so `x00FF` and `xFF` are two values and not one
value written twice. A format that packed to bytes would drop the leading zeroes
that distinguish them, and a program that sent `x00FF` and got `xFF` back would
have been handed a value that compares unequal to what it sent. That is the
silent conversion §20.6.4 forbids by name, arriving through the encoder instead
of the policy.

Nothing on this path normalises, and nothing needs to: §21 normalises hex digits
to upper case **at construction**, so `x00ff` was already `x00FF` before it was
ever a value. The wire carries what the value holds. Normalising again here
would be a second place where spelling is decided, which is how two ends start
disagreeing about what `==` means.

**A radix that is not 2 or 16 is a malformed stream**, not a third base to
support. The lexer produces exactly two prefixes, so a third cannot arrive from
any program; it can only arrive from a sender that is broken or one that is
hostile, and §20.6 records which of the two a decoder is obliged to assume.

This subsection is numbered after the spacesuit rather than beside the number it
resembles, for the reason `BitsRef` is variant index 9 rather than something
tidier: **appending is safe and inserting is not.** §20.3.4 and §20.3.5 are
cross-referenced from §20.6, §20.6.3 and §20.7, and renumbering them to put this
in its natural place would break every one of those for a cosmetic gain.
