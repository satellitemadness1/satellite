# 069 -- A refusal on ** names ^, a sign the line does not contain (0 ** -1 is "the ^ of 0 and -1")

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** integers  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A refusal on ** names ^, a sign the line does not contain (0 ** -1 is "the ^ of 0 and -1"; "a" ** 2 is "^ was given a string and a number").

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(0 ** -1)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S401: DIVISION_BY_ZERO
satl(run): the ^ of 0 and -1 is a division by zero

directory: e1.satl:5
syntax: satellite.console.display(0 ** -1)
                                    /\

a divisor worked out to 0. There is no number this could answer, so it answers
nothing rather than something.
machine code 22 division_by_zero -- satl exits with this.

--------------------------------------------------------------------------------

exit 22
```

## What it should do

"the ** of 0 and -1 is a division by zero" (and "** was given a string and a number"), naming the spelling that was written.

** is a documented spelling (satellite.help/satellite.variable.number line 13). The author's recorded rule for spellings, in ERRORS2.md #10, is "The refusal should use what was written."

## Variants

`0 ^ -1` -> the same message, which is correct there. `"a" ** 2` -> S301 "^ was given a string and a number, and there is no scenario for that pair". A touching `2**3` is refused separately by the whitespace rule.

## Where it comes from

satellite/bytecode/bytecode_registry.cpp:551-556 (spaced ** lexes to token::power_token, so the spelling is lost)

The lexer turns a spaced ** into the same power_token as ^ and keeps no record of the spelling. Every later report prints the token's canonical sign, ^.

## Notes

- Why the skeptic kept it: Reproduced. `display(0 ** -1)` gives S401 "the ^ of 0 and -1 is a division by zero", and `"a" ** 2` gives S301 "^ was given a string and a number". Neither line contains a ^. satellite.help/satellite.variable.number documents ** as a spelling in its own right: "^ for power (** is its second spelling)". The recorded principle in ERRORS2 #10 is "The refusal should use what was written". That entry covers only n.power being called power_of, a method in a different code path, so this operator case is a new instance of that principle, not the same entry. Neither DESIGN.md nor any help topic says a refusal about ** should name ^. The lexer comment, "nothing after the lexer ever learns there were two spellings", describes how it is built. It is not a ruling on what reports should say.
- Nearest known entry: none; sibling of ERRORS2.md #10 (n.power named power_of), which covers only the method spelling
- Merged: Single finding.
