# 041 -- satellite.console.display() with no value gets past the check and stops at run time as S210 NOT_BUILT_YET, while two values are refused before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** inconsistency  
**Severity:** medium

## What happens

satellite.console.display() with no value gets past the check and stops at run time as S210 NOT_BUILT_YET, while two values are refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.console.display()
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
satl(run): satellite.console.display has no scenario for nothing

directory: d0.satl:6
syntax: satellite.console.display()
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

Refused before anything runs, so "before" never prints: S110 "satellite.console.display takes one argument, and was given 0", exit 13.

The console help says "satellite.console.display(value) writes one line". The checker already counts arguments for 2 values, and its own comment at program_check.cpp:1489 says `display()` "is refused here by name rather than at run time by a value that is not there". check.sh:3345 requires "c.display takes 1 argument, and was given 0" before anything runs for a console's display.

## Variants

display( ) with a space inside gives the same S210 at run time. display(end="!") gives the same S210 at run time, "has no scenario for nothing -- it holds nothing, which has no text". display("a", "b") is refused at check time, S110, exit 13.

## Where it comes from

satellite/bytecode/program_check.cpp:1494-1496 (takes_one); the run-time refusal is satellite/bytecode/console_calls.cpp:460

takes_one is worked out as spelled.find('(') != npos. satellite.console.display's registered spelling is "satellite.console.display", with no brackets, so takes_one is false and the given == 0 case is not refused. At run time display_with_options gets a nothing value, display_text fails, and console_calls.cpp:460 refuses every value it cannot show with not_built_yet.

## Notes

- Why the skeptic kept it: Reproduced. display() with no value gets past the check: "before" prints, and then the run stops with S210 NOT_BUILT_YET, exit 14. display("a", "b") is refused before anything runs with S110 "takes one argument, and was given 2", exit 13. The checker's own code (program_check.cpp:1487-1498, the "GTK-12" comment) says display() should be refused there by name as "takes one argument, and was given 0", and check.sh:3345 requires exactly that for c.display. S210's paragraph ("the library behind it does not exist yet") is false, because display is built. Nothing about this is in any known list.
- Nearest known entry: none. ERRORS2 B, a bare string that holds nothing and cannot be displayed, is a different case and is fixed.
- Merged: Single finding. takes_one tests for "(" in a spelling that has none (program_check.cpp:1494-1496).
