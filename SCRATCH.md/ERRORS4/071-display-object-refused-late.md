# 071 -- display of a spacesuit object is refused as S210 only when its line runs, after earlier lines have printed

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** inconsistency  
**Severity:** low

## What happens

display of a spacesuit object is refused as S210 only when its line runs, after earlier lines have printed.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit box()
{
}

satellite.capsule satellite.main()
{
    box a
    satellite.console.display("before")
    satellite.console.display(a)
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
S210: NOT_BUILT_YET
satl(run): satellite.console.display has no scenario for an object

directory: v8a.satl:11
syntax: satellite.console.display(a)
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

The S210 given by satl(check) before anything runs, since the checker knows a holds an object of box. Otherwise, display should show the object.

Other not-built refusals come before anything runs (check.sh method_of_its_type; ERRORS2 says an unbuilt string method "is told so before anything runs"), and the brief counts an S210 that comes after earlier lines ran as an error.

## Variants

The hunter's form, where box declares to_string(), gives the same result: "before" then S210. An empty spacesuit gives the identical refusal.

## Where it comes from

satellite/bytecode/expression.cpp:1879-1882 (the run-time refusal); program_check.cpp does not judge display's argument kind against where.objects

display's scenarios have no arm for a user-defined value, so the walker refuses it with not_built_yet. The checker records which names hold objects but never tests a display argument for that, so the refusal waits until the line runs.

## Notes

- Why the skeptic kept it: Reproduced, even with an empty spacesuit: "before" prints, then display(a) on an object is refused with S210 NOT_BUILT_YET at run time, exit 14. The checker knows a is an object (where.objects) and does judge display's arguments (display("a", "b") is refused before the run). By the brief's rule, an S210 that comes only after earlier lines ran, where the checker refuses others first, is an error. Different from the known run-time S210 cases, which are on specific methods (number-string-conversion#3, lists#4, lists#5), and from ERRORS3 #11 (a name with no value).
- Nearest known entry: none
- Merged: Single finding.
