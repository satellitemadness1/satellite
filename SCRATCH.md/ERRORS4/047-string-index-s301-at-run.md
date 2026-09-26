# 047 -- Reading one character, s[1], passes the checker and is refused only at run time as S301 TYPES_DO_NOT_MEET, though the string help says it is refused with S210 before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** realistic programs, strings  
**Kind:** contradicts-help  
**Severity:** medium

## What happens

Reading one character, s[1], is refused at run time as S301 TYPES_DO_NOT_MEET after earlier lines ran, though the help says it is S210 refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string s = "abc"
    satellite.console.display("before")
    satellite.console.display(s[1])
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): s is a string, and [ ] reads a line of a file, an item of a list, or
a key of an index

directory: .../verify2/strings-2-0/ix1.satl:7
syntax: satellite.console.display(s[1])
                                   /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

S210 NOT_BUILT_YET (exit 14) from satl(check) before anything runs, e.g. 'reading one character with s[n] is not built yet'.

satellite.help/satellite.variable.string/help_text.txt: 'Each is refused before anything runs (S210), and so is reading one character with s[n].'

## Variants

s[1] = "z": late S301 'in s[...] = ..., s is a string, and [ ] = ... changes an item of a list or a key of an index' (27). "abc"[1]: late S110 with the false math-sign spacing hint, which is the lists#2 family and not counted here.

## Where it comes from

satellite/bytecode/expression.cpp:798 (the run-time refusal). The checker has no rule for [ ] on a declared string.

No checker rule refuses [n] on a string name as not built. The walker's indexing path treats a string as a kind with no [ ] scenario and reports S301.

## Notes

- Why the skeptic kept it: Reproduced: s[1] on a declared string prints 'before' and is then refused at run time with S301 TYPES_DO_NOT_MEET ('s is a string, and [ ] reads a line of a file, an item of a list, or a key of an index', exit 27). The help says, in the same paragraph as the not-built methods, that reading one character with s[n] is 'refused before anything runs (S210)'. Both the timing and the code contradict it. s[1] = "z" is the same (late S301). It is in no known list. lists#2 covers [n] after calls and literals, not a string name.
- Nearest known entry: none (lists#2 covers [n] after a call, method, literal or bracket, not a string name)
- Merged: The same finding reported twice: no checker rule covers [ ] on a string (expression.cpp:798).

### Also seen as: Reading one character with s[n] passes the checker and is refused only when its line runs, as S301 TYPES_DO_NOT_MEET, after earlier lines printed, though the string help says it is refused before anything runs with S210

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.string s = "hello"
    satellite.console.display(s[1])
    satellite.return(satellite)
}
```

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): s is a string, and [ ] reads a line of a file, an item of a list, or
a key of an index

directory: s5_index_string.satl:7
syntax: satellite.console.display(s[1])
                                   /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```
