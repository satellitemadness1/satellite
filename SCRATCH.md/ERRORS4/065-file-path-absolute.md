# 065 -- f.path answers the absolute resolved path, not the path as written, contrary to SATELLITE_FILE_OPERATIONS.md

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** contradicts-help  
**Severity:** low

## What happens

f.path answers the absolute resolved path, not the path as written, contrary to SATELLITE_FILE_OPERATIONS.md.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.file f = satellite.file.new("notes.txt")
    satellite.console.display(f.path)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: No notes.txt beside the program beforehand.

## What satl does

```
notes.txt

exit 0
```

## What it should do

notes.txt

SATELLITE_FILE_OPERATIONS.md Part 7: "`path` still answers the path as written"; the 3.3 word table: "the path it was opened on"; satellite_file.hpp:102: "the path it was opened on, as given".

## Variants

satellite.file.open("sub/a.txt") then f.path gives .../f10/sub/a.txt, again absolute.

## Where it comes from

satellite/bytecode/file_calls.cpp:329-341 (resolved(...) is passed to make_new/open_existing); satellite/satellite_variable_file/satellite_file.hpp:102,200

The interpreter resolves the written path under the include rule (3.6) and gives only the resolved string to the handle. path_, documented as "as given", therefore holds the absolute path, and there is no field for the written one.

## Notes

- Why the skeptic kept it: Reproduced for new() and open(), with a relative subfolder path as well. f.path answers the absolute resolved path. SATELLITE_FILE_OPERATIONS.md Part 7 (the choices of 2026-09-18) says "a handle keeps the ABSOLUTE path it opened ...; `path` still answers the path as written". The word table says "f.path: the path it was opened on", and satellite_file.hpp:102/200 comment path_ as "as given". file_calls.cpp resolves the path first and passes only the resolved one to make_new/open_existing, so what was written is lost. There is no file help topic (FO-7 not built), so the design doc is the statement. Not in any known list.
- Nearest known entry: none
- Merged: Single finding.
