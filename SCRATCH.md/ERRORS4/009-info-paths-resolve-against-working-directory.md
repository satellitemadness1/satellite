# 009 -- satellite.info.file, satellite.info.directory and satellite.directory.list resolve a relative path against the working directory, while satellite.file.* resolve it against the program's folder, so info can describe a different file from the one that was opened

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** wrong-answer  
**Severity:** medium

## What happens

satellite.info.file / satellite.info.directory / satellite.directory.list resolve a relative path against the working directory, while satellite.file.* resolve it against the program's folder.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.file f = satellite.file.open("notes.txt")
    satellite.console.display(f.size)
    satellite.variable.info i = satellite.info.file("notes.txt")
    satellite.console.display(i[1]["line_count"])
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is batch-2/p1/info_rel.satl, with p1/notes.txt holding "one\ntwo\n" beside it. Run it by absolute path with the working directory set to batch-2/other, which is a different folder.

## What satl does

```
2
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S510: FILE_NOT_THERE
satl(run): in i = ..., satellite.info.file: nothing is at notes.txt

directory: info_rel.satl:7
syntax: satellite.variable.info i = satellite.info.file("notes.txt")
                                                                    /\

no file is at that path.
machine code 39 file_not_found -- satl exits with this.

--------------------------------------------------------------------------------

exit 39
```

## What it should do

It should print 2 and 2 with exit 0 from any working directory, which is what it does when run from p1/. info.file("notes.txt") should name the same file that file.open("notes.txt") opened on the line above.

SATELLITE_FILE_OPERATIONS.md 3.6 says: "A path without a leading / is relative to the folder of the .satl that contains the call, the same way an include is." ERROR.md's 2026-09-25 check says "includes AND relative file names resolve against the program's own folder". Two words given the same name should find the same file, and a program should not start failing because it was run from another folder.

## Variants

info.directory("room") run from another folder gives S520 "satellite.info.directory room: No such file or directory", exit 28. directory.list("room") gives S520 "satellite.directory.list(d): No such file or directory", exit 28; that sentence also names the placeholder d instead of "room". Run from p1/, all three work (info_rel prints 2, 2; dir_rel prints "got info"; exit 0).

## Where it comes from

satellite/bytecode/info_calls.cpp:155 and :185 (and the directory.list library behind word 1 18 5) compared with satellite/bytecode/file_calls.cpp:141 resolved()

call_info_word takes arguments[0].text_utf8() as it is and passes it straight to ::lstat / ::statvfs, and on to the directory.list library, without the resolved() step (file_calls.cpp:141) that the file words use to anchor a relative path to the program's folder. The OS then resolves it against the process's working directory.

## Notes

- Why the skeptic kept it: Reproduced. Run by absolute path from another folder, file.open("notes.txt") finds the file next to the program, and one line later info.file("notes.txt") says nothing is there. info.directory("room") and directory.list("room") fail the same way. Run from the program's own folder, everything works. SATELLITE_FILE_OPERATIONS.md 3.6 says a relative path is relative to the folder of the .satl that makes the call, and ERROR.md (2026-09-25 check) says relative file names resolve against the program's folder. No help topic or DESIGN.md entry says the info or directory words use the working directory. None of the known lists mention info.file, info.directory or directory.list.
- Nearest known entry: none. ERROR.md's 2026-09-25 check covered includes and satellite.file.open only.
- Merged: Single finding.
