# 052 -- Digits touching letters (1_000, 1e5, 1e-10, 5abc, 00ff00) are refused as an undeclared name made of the trailing fragment (_000, e, ff00). When such a name is declared, they pass the checker and fail at run time

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, control flow and capsules, floats fractions percent, integers  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A digit separator or exponent (1_000, 1e5, 2.5e3) is refused as an undeclared name '_000' / 'e5'; 0x10 gets 'check your math sign spacing' at run time.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(1_000 + 1)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, _000 has no satellite.variable line declaring it

directory: d1.satl:5
syntax: satellite.console.display(1_000 + 1)
        /\

this name was used and no satellite.variable line ever declared it. A name has
to be given a type before it can hold anything.
machine code 25 name_not_declared -- satl exits with this.

--------------------------------------------------------------------------------

exit 25
```

## What it should do

A sentence about the number as written, for example 'a number is digits only -- write 1000' or '0x10 is written x10', given before anything runs.

The refusal names `_000`, which the person never wrote as a name, and the fix it implies (declare _000) is wrong. NEW_ERROR_LIST fix 11 set the pattern of telling other languages' habits what to write in satellite.

## Variants

`display(1e5)` and `display(2.5e3)` give S201 'e5 / e3 has no satellite.variable line'. `satellite.variable.number n = 1_000` gives S201 _000. With `_000` or `e5` declared, the result is satl(run) S110 'could not read to the end of -- check that every math sign has a space on both sides'. `0x10` gives the same run-time S110. `1'000` gives "text goes between double quotes -- write \"hello\", not 'hello'".

## Where it comes from

satellite/bytecode/program_check.cpp (names_in_statement's undeclared-name refusal). The lexer splits `1_000` into a number 1 and a name _000.

The lexer ends a number literal at `_` or `e` and starts a name token there, so the checker sees the literal 1 followed by the name _000 (or e5). It reports the name as undeclared, and never notices that the name was written touching a digit, which would identify a separator or exponent habit.

## Notes

- Why the skeptic kept it: Reproduced. `1_000` and `1e5` are refused before anything runs as S201 '_000 / e5 has no satellite.variable line declaring it'. Nobody wrote _000 or e5 as a name: the person wrote one number with a separator or an exponent. With _000 or e5 declared, the refusal becomes the run-time 'could not read to the end ... check that every math sign has a space'. `0x10` gets that same run-time sentence, and `1'000` is told to use double quotes for text. NEW_ERROR_LIST fix 11 already gives other languages' habits a sentence saying what to write; this one is not covered. Low severity: the program is wrong and it is refused, but the refusal names a thing that was not written.
- Nearest known entry: none. ERROR.md #20 is about digit separators in the C++ config, not in satl programs.
- Merged: One cause: the lexer ends a number at a letter and starts a name there, and only the colour-assignment check joins the two back together (color_check.cpp:119-129). One fix: notice a name that touches a number literal.

### Also seen as: Scientific notation (1e-10), or any number touching letters (5abc), is refused as an undeclared name "e"/"abc"; with such a name declared it gets past the checker and is refused at run time with an unrelated "& | << >> !!" hint

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.float tiny = 1e-10
    satellite.console.display(tiny)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, e has no satellite.variable line declaring it

directory: f9.satl:5
syntax: satellite.variable.float tiny = 1e-10
        /\

this name was used and no satellite.variable line ever declared it. A name has
to be given a type before it can hold anything.
machine code 25 name_not_declared -- satl exits with this.

--------------------------------------------------------------------------------

exit 25
```

### Also seen as: Digits touching letters (00ff00, 87ceeb, 12ab) outside a colour assignment are refused as an undeclared name made of the trailing fragment (ff00, ceeb, ab)

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(00ff00)
    satellite.return(satellite)
}
```

```
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, ff00 has no satellite.variable line declaring it
syntax: satellite.console.display(00ff00)

exit 25
```

### Also seen as: A colour argument without its x, paint(00ff00), is refused as "ff00 has no satellite.variable line declaring it" instead of the colour's "write x00ff00"

```satellite
satellite.include(satellite)

satellite.capsule paint(satellite.variable.color c)
{
    satellite.console.display(c)
}

satellite.capsule satellite.main()
{
    paint(00ff00)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, ff00 has no satellite.variable line declaring it

directory: col.satl:10
syntax: paint(00ff00)
        /\

this name was used and no satellite.variable line ever declared it. A name has
to be given a type before it can hold anything.
machine code 25 name_not_declared -- satl exits with this.

--------------------------------------------------------------------------------

exit 25
```
