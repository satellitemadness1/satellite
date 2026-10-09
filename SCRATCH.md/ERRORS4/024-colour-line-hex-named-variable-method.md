# 024 -- On a colour's declaring line, a colour variable whose name is spelled in hex letters and followed by a method (bead.transparency(20)) is refused as "read here as hex digits"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool  
**Kind:** valid-program-refused  
**Severity:** low

## What happens

On a colour's declaring line, a colour variable whose name is spelled in hex letters followed by a method (bead.transparency(20), c.transparency(20)) is refused as "read here as hex digits".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.color bead = x87CEEB
    satellite.variable.color faded = bead.transparency(20)
    satellite.console.display(faded)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(check): in satellite.main, ERROR: bead is read here as hex digits, and it
is also a variable -- on the line that declares a color, a run of hex digits is
the color's digits. For the variable, declare the color first and give it bead
on the next line; as digits it is 4 hex digits, and a color is exactly six hex
digits, like x00FF00

directory: f11a.satl:6
syntax: satellite.variable.color faded = bead.transparency(20)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

x87CEEB, 20
exit 0

The same program with the colour named sky prints 'x87CEEB, 20' and exits 0, and so does `faded = bead.transparency(20)` on a later line. color_check.cpp says a value with 'a method on it -- is left to the walker'. The help's digits-or-name rule (and tests/color_digits_or_variable.satl) is about a bare run of hex digits as the whole value.

## Variants

FAIL: color c = x87CEEB; color d = c.transparency(20) -> 'c is read here as hex digits ... as digits it is 1 hex digit'. PASS: color sky ...; color faded = sky.transparency(20) -> x87CEEB, 20. PASS: color faded = x000000 then faded = bead.transparency(20) on the next line -> x87CEEB, 20.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/color_check.cpp:80-94 (the_digits_are_right)

The branch `if (code == token::hexadecimal_token && a_variable)` refuses whenever the first token is hex digits that also name a variable. Unlike the later branches, it never tests `the_value_ends(next)`, so it cannot tell a bare `bead` (ambiguous) from `bead.transparency(20)` (a method call on the variable).

## Notes

- Why the skeptic kept it: I reproduced this. `satellite.variable.color faded = bead.transparency(20)`, where bead is a declared colour, is refused before anything runs as 'bead is read here as hex digits ... as digits it is 4 hex digits'. The same line with the colour named sky prints 'x87CEEB, 20', and so does the assignment on a later line. So a valid method call on a variable is refused only because the variable's name happens to be spelled in hex letters (c, d, e, bead, face...), and two equivalent programs disagree. The recorded rule (tests/color_digits_or_variable.satl) is about a bare run of hex digits as the whole value (`d = facade`). color_check.cpp's own comment says 'A value that is more than one literal -- a sum, a method on it -- is left to the walker', but the hex-token-and-variable branch refuses without checking what follows. Not by-design: neither the help nor that ruling covers a name followed by a method. Not in any known list.
- Nearest known entry: none (the related ruling is tests/color_digits_or_variable.satl, for a bare `d = facade` only)
- Merged: Single finding. color_check.cpp:80-94 does not test the_value_ends.
