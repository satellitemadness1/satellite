# 066 -- display of a file is S210 NOT_BUILT_YET with no reason, while a list holding the file gets S301 with "a file has two strings ... write the one you mean"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** inconsistency  
**Severity:** low

## What happens

display of a file is S210 NOT_BUILT_YET with no reason, while a list holding the file gets S301 with "a file has two strings ... write the one you mean".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.file f = satellite.file.open("p1/notes.txt")
    satellite.console.display("before")
    satellite.console.display(f)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: p1/notes.txt exists beside the program, holding "one\ntwo\n".

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(run): satellite.console.display has no scenario for a file

directory: df.satl:7
syntax: satellite.console.display(f)
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

S301 TYPES_DO_NOT_MEET, exit 27, with "a file has two strings, its text (read_all) and its name (path) -- write the one you mean", the same as display of a list that holds the file. Ideally it is refused before anything runs, since the checker knows f is a satellite.variable.file (it already refuses f.upper() on one).

satellite_object.cpp:762-765 says a file refuses to become text as types_do_not_meet, with that sentence. The same mistake should get the same code and sentence whether or not the file is in a list. S210 says the library does not exist yet, which is false for display.

## Variants

A list<satellite.variable.file> holding f, then display(l): S301 with the file sentence, exit 27. "x" + f: S301 "+ was given a string and a file, and there is no scenario for that pair", exit 27. All three stop only after "before" has printed.

## Where it comes from

satellite/bytecode/console_calls.cpp:117-119 (display_text) and :460 (display_with_options)

display_text ignores the code that value.to_string returns, and for a bare file value why comes back empty. display_with_options then refuses every value it cannot show with not_built_yet (console_calls.cpp:460) instead of passing on to_string's code and reason. The list path goes through the list's to_string, which keeps satellite_object.cpp's types_do_not_meet and its sentence.

## Notes

- Why the skeptic kept it: Reproduced. display(f) for an open satellite.variable.file stops at run time, after "before" printed, with S210 NOT_BUILT_YET "has no scenario for a file" and no reason given. The same file inside a list gets S301 TYPES_DO_NOT_MEET, exit 27, with the helpful sentence "a file has two strings, its text (read_all) and its name (path) -- write the one you mean". That sentence already exists in satellite_object.cpp:764, which marks it types_do_not_meet. The direct display drops both the code and the sentence. S210's paragraph is false, because display is built, and the same mistake gets two codes. The spacesuit object part of the report is left out: satellite_object.cpp:756-758 deliberately answers not_built_yet for an object ("satellite does not invent one").
- Nearest known entry: none
- Merged: Single finding. display_text drops the code and reason that to_string returns.
