*satellite design docs, §19, part 2 of 2. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§19 part 1](19-a-includes-literals-and-rebinding.md), On: [§20 part 1](20-a-networking.md).*

---

## 19. Nine additions, and the program that asked for them — input, named arguments and booleans

*Continues [§19 part 1](19-a-includes-literals-and-rebinding.md).*

### 19.5 `satellite.console.input`, and the only out parameter in the language

1 site, and it is the program's entire reason for existing: it prompts for the
name of a target, searches its library for an exact match, and forges what it
finds. `satellite.console` had `display` and nothing else, so with no input the
program could only ever forge something hardcoded.

Three shapes, and the first two are the ones to prefer:

```
satellite.console.input()                       the line
satellite.console.input(prompt)                 the line, after prompting
satellite.console.input(prompt, target)         writes into target
```

**The two-argument form is the only out parameter in the language and is
deliberately not the beginning of a general facility**: no user capsule can
declare one, because nothing in a capsule's parameter list can say "this one is
written back". It exists because it is the shape a program reaches for, and
because the value form alone would make `satellite.console.input("", answer)` a
mystery rather than a mistake. It is written in terms of the value form, so one
place prompts, one place drains and one place reads. The **place** is checked
before the prompt is printed — a target that cannot be written to is a mistake in
the program, and discovering it after the user has typed an answer would throw
that answer away.

**The drain is the part that is not obvious.** Output goes through §9's printer
thread, so a prompt is *queued* rather than printed, and a read that did not wait
for it would block on an apparently empty terminal with the prompt sitting behind
it. `Console::drain()` — which §9 already provides so that `interp.cpp` can print
an error report after a program's output without the two interleaving — is
exactly the barrier needed, and this is its second caller.

End of input is **loud**. An empty line and no line at all are different answers
— the first is somebody pressing return, the second is nobody being there — and
a program that cannot tell them apart loops forever on a closed stdin.

### 19.6 Named arguments as grammar, and `end=` as the only one understood

```
satellite.console.display("[SATELLITE_VIEW_FORGE]>>", end="")
```

Two absences were tangled here, and only one of them blocked the program: there
is no keyword-argument syntax anywhere in the language, and there was no
unnewlined print. A prompt whose cursor lands on the line below it is ugly and
survivable; a prompt that cannot be written at all is not.

`NamedArg` is **grammar for one named argument and not the start of keyword
arguments.** It parses anywhere and is accepted in exactly one place — the same
shape `DurationLit` already has — and the reason to give it a node at all is the
error: an unrecognised `name=` now reaches one message that states the whole rule,
instead of a parse error complaining about a missing `)`. A bare word followed by
a single `=` cannot be anything else in an argument list, because assignment is a
statement in this language and never an expression, so this steals no form; `==`
is one token, so a comparison is not caught by it.

**The ending is a value, not a flag.** `end=""` is a prompt, `end=" "` puts two
displays on one line, and `end="\n"` is the default spelled out. A boolean
`newline=false` could not express the middle one. It emits **one** piece, for the
reason §9 gives about `display`: the unit handed to the Console is the unit
another thread cannot tear in half, and a prompt should not arrive split around
some other thread's output.

### 19.7 Bare `TRUE` and `FALSE`, without a reserved word

35 sites. `satellite.variable.bool target_list_has_unset = TRUE`.

§8.4 closed with an objection to exactly this: "the obvious alternative — bare
`true` and `false` — would be the language's second and third reserved words, and
§1 has exactly one." **That objection was right, and it is about reserved words
rather than about the spelling.** `satellite.bool.true` and
`satellite.bool.false` remain canonical and remain the only spelling this
document recommends.

The resolution is **where** the two words are recognised. They are answered in
`Resolver::resolve_name` only after a local, a field, a method, a capsule and a
spacesuit have all been asked and said no, and then at run time only after
`satellite.library` has been asked too. So §1 holds exactly as written: a bare
identifier still names something the user owns, and a variable, field, capsule or
spacesuit called `TRUE` still wins the word. **Verified** — a capsule declaring
`satellite.variable.number TRUE = 5` returns 5. Nothing is reserved, and the
words are not taken away from anybody.

Doing this in the **lexer** is the obvious implementation and is the one that
would have broken §1, by taking both words from the user everywhere and for good.

### 19.8 Two spellings accepted, and one silent bug closed

**`satellite.statement.else()`** — 24 sites. An `else` takes no condition, so an
empty pair of parentheses carries nothing and the parser was right to be
surprised. But the program writes parens on every other statement form it uses
and reached for them here by symmetry, which is a mistake a reader makes once per
program rather than once per career. An empty pair is now accepted and dropped;
`else(x)` is still an error, because a condition on an `else` is a
misunderstanding rather than a spelling.

**`\'` inside a double-quoted string** — this one was not a blocker at all, and
that is what makes it the most interesting entry here. §3.4 gave the string lexer
`\n \t \r \\ \"` and left every other backslash to pass through untouched, so
`"DOESN\'T"` printed with the backslash still in it. No error, no refusal, and
therefore invisible: once the nine blockers above were fixed and the program
actually ran, **501 lines of its output were wrong**. An apostrophe needs no
escaping inside double quotes and there are no single-quoted strings for it to be
escaping from, so the program was being over-careful rather than wrong, and was
punished for it.

`\'` is now a redundant spelling of `'`. Rejecting unknown escapes outright was
the alternative and was rejected: `encode()` cannot tell a mistake from a `\d` or
`\s` that somebody put in a string on purpose, so making unknown escapes an error
would break working programs to catch this one. **Verified**: 501 stray
backslashes before, 0 after.

### 19.9 What did not change, and what is still open

Recorded because a nine-item list invites the conclusion that the language was
half-built, and the opposite is what the evidence says. Every one of these was
doubted, probed and found already working: spacesuits with `satellite.protected`
and `satellite.public` blocks, fields, methods and constructors; a bare instance
declaration running the no-argument constructor; a protected capsule calling a
sibling by bare name; a spacesuit name as a parameter type;
`satellite.container.list<user_spacesuit>`; a free capsule handing back a list of
spacesuits **with no `satellite.returns` clause** (all 25 omit it and satl infers
it); `object_list[i].call_get_name_str()`; a list element passed straight into a
call; `string + number` with the number coerced; `satellite.main` with no
explicit `satellite.return`. The object model is the part of this program that
satellite already ran.

Still open:

1. **`string + bool` is refused.** `"flag=" + b` gives
   `+ does not apply to flag= and true`, while `string + number` coerces and
   works. 0 sites today — the program writes the words TRUE and FALSE into its
   literals by hand — but its whole style is to narrate the value of every
   variable it touches, and the moment somebody narrates a bool the way they
   narrate a number, this fails. Whether the asymmetry is intended is worth
   settling deliberately rather than at the first site.
2. **Keyword arguments in general** are not decided, and §19.6 deliberately does
   not decide them. `end` is one name understood in one place.
3. **A bare nested block `{ }`** that opens a scope was one of §19.3's three
   options and is still not in the language. It is the conventional answer and
   would make the rebind unnecessary for programs that wanted the C++ shape
   explicitly. Nothing now depends on it.
