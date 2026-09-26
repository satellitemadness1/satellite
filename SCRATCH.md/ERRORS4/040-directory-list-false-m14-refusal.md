# 040 -- satellite.directory.list() in a program is refused at run time with "satellite has no list type built yet to hold them (MILESTONES M14)", though M14's list is built

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

satellite.directory.list() in a program is refused at run time with a false reason, "satellite has no list type built yet to hold them (MILESTONES M14)", though M14's list is built.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.container.list<satellite.variable.string> names = satellite.directory.list()
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: Run from the program's folder (it held p.satl and one other entry).

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(run): in names = ..., satellite.directory.list() read 2 names, and
satellite has no list type built yet to hold them (MILESTONES M14) -- at the
prompt the same line draws the table

directory: .../verify2/batch-1/f6/p.satl:6
syntax: satellite.container.list<satellite.variable.string> names = satellite.directory.list()
                                                                                              /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

The names as a list of strings (the list type exists), or a refusal before anything runs that says truthfully that directory.list does not answer a value in a program yet.

MILESTONES.md:480 marks the M14 list BUILT. The S210 paragraph ("the library behind it does not exist yet") is also false, since the library ran and read the names.

## Variants

satellite.console.display(satellite.directory.list()) gives the same sentence without "in names = ...". The hunter saw the same for satellite.directory.system() ("read 12 names").

## Where it comes from

satellite/bytecode/expression.cpp:1588-1590

The directory word still ends in a hard-coded not_built_yet refusal written before M14 built the list. It was never updated to return reply.names as a list, and the checker does not refuse it before the run.

## Notes

- Why the skeptic kept it: Reproduced. satellite.directory.list() inside a program is refused at run time, after earlier lines ran, with "satellite has no list type built yet to hold them (MILESTONES M14)". MILESTONES.md:480 says "M14 -- satelliteContainer ... the list BUILT 2026-09-18 and 2026-09-23", and the failing line itself declares a satellite.container.list<satellite.variable.string>. The sentence is false today. PROGRESS.md recorded this as owed ("list() inside a program answers not_built_yet until satellite has a list type (M14)"), but that condition has since been met, and the item is in none of the known-error lists.
- Nearest known entry: none (PROGRESS.md:78 'Owed: list() inside a program answers not_built_yet until satellite has a list type (M14)', which is an intent note whose condition M14 has since met, not a known-error entry)
- Merged: Single finding.
