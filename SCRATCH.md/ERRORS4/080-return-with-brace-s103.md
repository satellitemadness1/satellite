# 080 -- satellite.return(satellite)} as main's last line is refused S103 at a line past the end of the file, which asks for the return that is already there

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** syntax and layout  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

`satellite.return(satellite)}` as main's last line is refused S103 at a line past the end of the file, asking to add the return that is already there.

## Program

```satellite
satellite.include(satellite)
satellite.capsule satellite.main()
{
    satellite.console.display("hi")
    satellite.return(satellite)}
```

Run it with `satl program.satl`. Setup: none (5-line file)

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S103: FILE_HAS_NO_RETURN
satl(check): satellite.main's last line is satellite.return(satellite) -- it is
where the program ends, so it goes just before main's closing }

directory: r1.satl:6
syntax: (the file could not be re-read to show this line)

execution ends inside main, and main has to say so. Add this as main's last
line, just before its closing }:

    satellite.return(satellite)

machine code 12 satl_file_missing_satellite_return_satellite -- satl exits with
this.

--------------------------------------------------------------------------------

exit 12
```

## What it should do

Either it runs (hi, exit 0), or, under the known one-line-block rule, a refusal at line 5 that names the } after the return, as a non-main capsule gets ('satellite.return(...) is the whole statement, and something follows its )').

Line 6 does not exist, the file is readable, and the return the report asks for is on line 5. The same shape in any other capsule gets an accurate line-4 refusal (r4.satl).

## Variants

r2 (the same plus two blank lines): S103 at line 8 of a 7-line file. r3 (`satellite.console.display("hi") }` as the last line): a different S103 with no line, 'this file has no satellite.return'. r4 (`satellite.return(2)}` in capsule two): S110 at line 4, 'in two, satellite.return(...) is the whole statement, and something follows its )', exit 13. r5 (`satellite.return(satellite) // end`): runs, hi, exit 0.

## Where it comes from

satellite/bytecode/program_check.cpp:2485-2496 (S103 raised at last_statement or closing)

Likely cause (not traced to the exact line): for main, the check of satellite.return(satellite) takes the whole line, trailing } included. Main's closing } is never counted, so the body scan runs to the end of the row. The S103 check (program_check.cpp:2485) then finds no return as the last statement and raises at a position past the file's last line, which the report cannot re-read.

## Notes

- Why the skeptic kept it: Reproduced. With `satellite.return(satellite)}` as main's last line in a 5-line file, S103 names line 6, which does not exist, says '(the file could not be re-read to show this line)', and tells the user to add the return that is already on line 5. With two trailing blank lines (r2) it names line 8 of a 7-line file. The same shape in another capsule, `satellite.return(2)}`, gets a correct refusal: line 4, 'satellite.return(...) is the whole statement, and something follows its )'. The known item 'A block written whole on ONE line, { statement }, is refused' is about refusing that shape. This report is about main's return getting a false S103 that points past the end of the file, which is a separate wrong-line and false-advice defect.
- Nearest known entry: Related to SCRATCH.md/CONTAINERS.md 'A block written whole on ONE line, { statement }, is refused' (the shape is refused), but that entry does not cover main's return getting S103 at a line past the end of the file.
- Merged: Single finding. Its cause differs from blank-line-before-main-brace-s103.
