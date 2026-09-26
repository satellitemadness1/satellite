# 038 -- A capsule whose name is not a name (x1, xa, b1, 5) gets no refusal on its header: the capsule is dropped silently, and the first line of its body is refused as "outside every capsule" or "satellite.return(satellite) goes inside satellite.main"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** control flow and capsules, syntax and layout  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

A capsule named x1, b1 or xff is dropped without a word, and the return inside it gets a false 'satellite.return(satellite) goes inside satellite.main' refusal on the wrong line.

## Program

```satellite
satellite.include(satellite)

satellite.capsule x1()
{
    satellite.return(5)
}

satellite.capsule satellite.main()
{
    satellite.console.display(x1())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none; c2_x1.satl

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): satellite.return(satellite) goes inside satellite.main, as its last
line -- outside a capsule nothing runs

directory: c2_x1.satl:5
syntax: satellite.return(5)
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

A refusal at line 3 that names the problem, for example 'x1 reads as a hex number and cannot be a capsule's name'. Or the capsule is accepted and prints 5.

The refusal names line 5, which is inside a capsule and is not satellite.return(satellite), so every clause of the sentence is false for this program. satellite.help/satellite.capsule/help_text.txt places no limit on capsule names beyond a name's usual characters, and x1 is made of letters and digits and does not start with a digit.

## Variants

Names b1, xff and x1 fail the same way. Names x and b1c work (print 5). 'satellite.variable.number x1 = 4' is refused at the right line, but the sentence 'is followed by something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does not start with a digit' describes x1 as valid.

## Where it comes from

satellite/bytecode/capsule_scopes.cpp:537-541 and 686-692

capsule_header() finds no name token after satellite.capsule, because x1 lexes as a hex literal, and returns 0. The scan then does '++i; continue;' with no refusal (line 541). The capsule's { is taken as a top-level block, so the satellite.return inside it reaches the top-of-file branch (lines 686-692), which always says 'satellite.return(satellite) goes inside satellite.main'.

## Notes

- Why the skeptic kept it: Reproduced. Capsules named x1, b1 and xff all get S110 'satellite.return(satellite) goes inside satellite.main, as its last line -- outside a capsule nothing runs', pointing at line 5 (satellite.return(5)). Capsules named x and b1c run and print 5. The sentence is false: line 5 is inside what the person wrote as a capsule, and it is not satellite.return(satellite). The real cause is on line 3, where x1 lexes as a hex literal and b1 as a binary literal, and it is never named. A variable named x1 gets a different but also misleading refusal: 'is followed by something that is not a name -- a name is made of a-z, A-Z, 0-9 and _, and does not start with a digit', although x1 fits that description. The capsule help sets no such limit on names. This is in no known list and not among the already-confirmed.
- Nearest known entry: none
- Merged: Same cause: capsule_header returns 0 with no trouble sentence when the name slot holds a literal (capsule_scopes.cpp:159-174 and :537-541).

### Also seen as: A capsule whose name is not a name (x2, xa, b1, 5) is refused on its body's first line as 'outside every capsule' (or as a misplaced satellite.return(satellite)), not on its header

```satellite
satellite.include(satellite)

satellite.capsule x2()
{
    satellite.console.display("in")
}

satellite.capsule satellite.main()
{
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): this line is outside every capsule, and nothing outside a capsule
ever runs -- there are no globals in satellite, so it goes inside satellite.main
or another capsule

directory: v8a.satl:5
syntax: satellite.console.display("in")
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```
