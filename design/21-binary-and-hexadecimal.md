*satellite design docs, §21 of 21. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§20](20-networking.md).*

---

## 21. Binary and hexadecimal

Status: **built and tested**, unlike §20, which was written ahead of its code.
Everything below was checked by running it against the interpreter in this
repo. The one thing here that is *not* built is the wire encoding in §21.7 —
§20's format does not exist yet, and this section says what §21 will hand it
when it does.

### 21.1 The request, and the two things in it

The request was for `satellite.variable.binary` and `satellite.variable.hex`
(with `hexadecimal` as a second spelling), written as `x0009999CCCDDBBDFBDBDBD`
and `b10101011110101011`, so that binary data could be sent over §20's
protocol.

That request contains two things, and they have different answers:

1. **A notation** — a way to write a value in base 16 or base 2.
2. **A value whose WIDTH is part of it** — `x0009` is four digits, and the
   three leading zeros are information.

The first alone needs no type. The second is the whole reason there is one, and
it is worth stating exactly, because it is the argument the rest of this section
rests on:

> `satellite.variable.number` is an exact decimal (§8.1), and **exact is not the
> same as wide**. `Number::parse("0009")` is 9, and 9 is what `to_string` hands
> back — **verified**. To a number the leading zeros were never there. So a hex
> constant that went onto a socket as `x0009` would come off it as `9`, and a
> program that round-tripped a four-digit field would get a one-digit one back.

A value that cannot survive its own round trip is not the type this was for.
Hence a type.

### 21.2 Two type names, one alternative

`binary` and `hex` are **exact-name distinct** at the surface, exactly as every
other variable type is: a capsule taking one does not accept the other, and
`satellite.variable.hex h = b1010` is refused. **Verified.**

Underneath they are **one** `Value` alternative carrying a radix. That is not a
hedge, and §8.7's rule is what licenses it: "a word means one thing, and
dispatch on the receiver is for types that answer the *same* question
differently, not for types that answer different questions." `binary` and `hex`
answer the same questions — `.digits()`, `.bytes()`, `.to_number()` — and differ
only in the base the digits are written in. That is the relation §18's three
random tiers have, and the relation §20.1 collapsed `http`/`https`/`tcp` onto.

What is different here, and why the collapse stops at the *representation*
rather than reaching the *type name*: §18's tiers differ in how a value is
**produced** and §20.1's protocols differ in what happens **under** the bytes,
so in both cases one type name was enough. A hex value and a binary value differ
in what the value **is** — `x0F` has two digits and `b00001111` has eight — and
a program that means to hold one and is handed the other has a bug that
exact-name matching catches for free.

**The cost is real and is named here rather than discovered later.** `matches()`
is exact-name equality and §12 defers user-defined generics, so **a capsule that
accepts "either" cannot be written**. The conversions are what a program uses
instead: `.to_hex()` and `.to_binary()` are total and value-preserving, so the
caller converts at the boundary rather than the callee accepting both.

### 21.3 The literal, and the one thing it costs

```satellite
satellite.variable.hex    my_hex = x0009999CCCDDBBDFBDBDBD
satellite.variable.binary my_bin = b10101011110101011
```

**The rule**: a lower-case `x` or `b`, followed by at least one character, **all**
of which are digits in that base. Everything else is the identifier it always
was.

The rule is all-or-nothing over the whole word, and that is what keeps it from
eating names. `x2_y` — the lexer's own example of a Word — stays a Word, because
`_` is not a hex digit. So do `xyz`, `box`, `bob`, and `be`, because `e` is not a
binary digit. Upper case is **not** a prefix, so `X0F` and `B1010` are names:
hex digits are already spelled in either case, so an upper-case prefix would
take `XAD` and give nothing back.

**What it does cost**: `x1`, `xff`, `b0` and `b1010` are literals now and can no
longer be variable names. That was **measured before it was chosen** — the only
`x`/`b`-prefixed identifiers in every `.satl` file in this repo are `x`, `b` and
`be`, and the rule leaves all three alone. It is a measurement and not an
assumption that nobody would name a variable `x1`.

**A name position REFUSES rather than accepting.** The tempting fix is to allow
a Bits token wherever a name is expected — `my_object.x1` is unambiguous, after
all — and it builds a trap: it would let `satellite.variable.number b1 = 5`
declare a variable no expression could ever read back, because `b1` in a value
position is the literal 1. The program would print 1 where it declared 5,
silently. So the parser refuses and the message names the collision:

```
'x1' is a hexadecimal literal (§21), so it cannot name a variable; a name may
not be an 'x' or 'b' followed only by digits of that base
```

§18's posture, one level over: refuse rather than half-accept.

### 21.4 The alias

`satellite.variable.hexadecimal` is `satellite.variable.hex`. **Verified.**

This is **the language's one alias**, and §1 wants one spelling for one thing.
It is bent knowingly rather than by accident, because both spellings were asked
for. Two consequences are recorded so neither is a surprise:

- `unparse` always emits `hex`, so a round trip through the tree normalises the
  spelling.
- The registry mints `hexadecimal` its **own id** (104). §17's rule is "one id
  per distinct **word**" — the key is the word, not the thing it means, and two
  spellings are two words.

§20.3.2 declined a second spelling for one type on the ground that "a second
spelling for one type invites the question of which is the default." That
question is answered here rather than avoided: `hex` is the default, and it is
what everything the language prints uses.

### 21.5 Width, case, and what equality means

| written | is | because |
|---|---|---|
| `x0009 == x9` | **false** | the width is part of the value |
| `x0009.to_number() == x9.to_number()` | **true** | that is the other question, asked by name |
| `x00FF == x00ff` | **true** | case is normalised; width is not |

All three **verified**.

Case is normalised to upper at evaluation, so how a value was typed does not
change what it is — the same call §8.6 makes for map keys. It is a normalisation
of *spelling*; the digit count is never touched.

Equality being width-sensitive follows from the width being part of the value
rather than being a separate decision. If `x0009` and `x9` compared equal, one
would have to be a valid substitute for the other, and it is not: they pack to a
different number of bytes and arrive off a socket as different values.

### 21.6 The methods

```
.digits()      how many digits, leading zeros included — the WIDTH
.bytes()       the packed size, rounded up to a whole byte
.to_number()   the exact integer. THE LOSSY DIRECTION: x0009 -> 9
.to_hex()      .to_binary()      value-preserving conversion
.to_string()   the printed form, prefix and all
.concat(v)     same radix only
.size()        §8.7's bytes, as every value answers
```

`.to_number()` is exact at any length — `x0009999CCCDDBBDFBDBDBD.to_number()` is
`45334948838468516429245`, **verified** against an independent computation. It
goes through decimal string arithmetic rather than `Number::divide`, because
`divide` takes a digit count, which is a *precision*, and a base conversion has
no business choosing one.

**Conversion is exact in value both ways and exact in WIDTH only from hex to
binary**, because one hex digit is exactly four binary digits:

- `x0F.to_binary()` is `b00001111`, and `.to_hex()` back is `x0F` — a fixpoint.
- `b101.to_hex().to_binary()` is `b0101`. Three binary digits are one hex digit,
  and that hex digit converts back to four.

That is stated rather than hidden, and it is why conversion is a method a
program asks for rather than something that happens on its own.

`.concat()` **refuses a mixed radix** rather than converting one side. Joining a
hex value to a binary one has two defensible answers — convert the argument, or
convert the receiver — and a call with two defensible answers is a call the
program should write out.

**Not a map key.** §8.6 restricts keys to string and number, and that rule is
untouched here; `map_key_of` refuses and the existing error says so.

### 21.7 Crossing the wire

§20.3.1's table gains one row:

| type | crosses | as |
|---|---|---|
| `binary` / `hex` | yes | a radix, a **digit count**, then the packed bytes |

**The digit count is not redundant with the byte count, and leaving it out is
the one way to get this wrong.** Packing loses the width: `x0F` and `x000F` pack
to the same single byte. A wire format that sent bytes alone would hand back a
value of a different width than the one that was sent — which is precisely the
failure §21.1 says this type exists to prevent. So the count travels beside the
bytes, and `bits_unpack` takes it back.

The bytes are **big-endian and left-padded to a whole byte**. §20.4's endianness
marker does not govern this and must not be applied to it: a hex constant is
written most-significant digit first, and the bytes it names are in that order.
Byte-reversing them on one architecture would make `x0102` arrive as `x0201`,
which is not an endianness question at all.

Everything else §20.6 requires applies unchanged — the digit count is a count
and is therefore checked against what is actually there before it is believed,
which is §18's `MAX_RANDOM_DIGITS` precedent and §20.6's rule.

### 21.8 Registry note

Seven ids, 102 through 108. **Next free is 109.**

```
102 binary        103 hex           104 hexadecimal      (type names)
105 bytes         106 to_number     107 to_hex     108 to_binary
```

`digits` (86), `concat` (44), `to_string` (43) and `size` (85) are **reused
unchanged**, which is the flat registry paying for itself: a binary value
answers `.digits()` with the same id a number does, because it is the same word
asking the same question.

102..104 are type names and are **not** selectors, exactly as `map` (73) and
`list` are not — a type name answers to no receiver.

All three of `format_test`'s deliberate tripwires fired and were bumped in the
same commit, which is what they are for: the last-id assert, the selector count
(44 -> 48), and the first-unassigned-id check (102 -> 109). The selector range
check took a **third run** rather than a widened bound, as its own comment
requires by name.

### 21.9 One laxness closed on the way past

§20.6.2 recorded that `satellite.variable.network n` **already declared a nil**,
because `Resolver::check_type` validated a type's *space* and not its *name*.
That is closed: every variable type name is listed, and anything else is named
as the mistake it is at the declaration. **Verified** — `satellite.variable.network n`
is now `no such type: satellite.variable.network`.

When §20 is built, `network` joins that list. It is the one thing in this
section that a future section has to remember to do.

### 21.10 What the register file taught

`reg_test` announced that it "round trips for all **nine** Value alternatives",
and it enumerated them by hand. `Reg` boxes anything it does not recognise, so
the tenth alternative **compiled and passed without ever being converted in
either direction**.

That is the silent half of §10's "the compile errors are the feature". Two sites
stopped the build and had to be answered — `ValuePrinter` and `SizeVisitor`, both
exhaustive `std::visit` with no `auto` fallback, both deliberately so. Everything
else was a `switch` on the variant index with a `default:` or a `get_if` chain,
and every one of those would have accepted a tenth type in silence.

The count in that test's PASS line is now the tripwire for the eleventh.

### 21.11 Deliberately deferred

- **Octal, and any other base.** Two bases were asked for and two are built.
  A third is a lexer prefix and a row in `bits_convert`, and nothing in the
  design forbids it; it is absent because nothing has asked.
- **Bit operations** — `and`, `or`, `xor`, shifts. The lexer's own comment
  already records that satellite has no `<<` or `>>` and that shifts, if ever
  wanted, are named methods. They belong on this type when something needs them.
- **A signed binary value.** These are unsigned digit strings. Negative is a
  question about a *number*, and `.to_number()` is where a program goes for one.
- **Arithmetic.** `.plus()` on a hex value would have to answer what the width
  of the result is, and there is no answer that is right in every case.
  `.to_number()` says which question is being asked.
