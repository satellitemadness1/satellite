# 019 -- A blank or comment line between satellite.return(satellite) and main's } is refused as S103 "the file could not be re-read", which asks for the return that is already there

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** syntax and layout  
**Kind:** valid-program-refused  
**Severity:** medium

## What happens

A blank line (or a comment line) between satellite.return(satellite) and main's } is refused as S103 'the file could not be re-read', telling the person to add the return that is already there.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("hi")
    satellite.return(satellite)

}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S103: FILE_HAS_NO_RETURN
satl(check): satellite.main's last line is satellite.return(satellite) -- it is
where the program ends, so it goes just before main's closing }

directory: b1.satl:7
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

hi, exit 0

satellite.return(satellite) is main's last statement. A blank line or a comment is not a statement. The help (satellite.return, satellite.main) says the return is main's last line 'just before its closing }', and the author's ruling (A7) is about the last line of code. The same comment on the return's own line runs (b3), a blank line between two statements runs (b5), and a blank line before another capsule's } runs (b6).

## Variants

b2: '    // the end' on its own line before } is refused the same way, with 'syntax: // the end' and a caret. b3: 'satellite.return(satellite) // the end' runs: hi, exit 0. b4: a blank line and a comment AFTER main's } run. b5: a blank line inside main before the return runs. b6: a blank line before another capsule's } runs.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:2464 ('if (depth == 0) last_statement = at;'), judged at 2486-2496

The main-body loop in the checker records last_statement for every position at depth 0 before check_statement runs, including a lone line_end_token (a blank line) or a comment token. The last 'statement' is then the blank or comment line, not satellite.return(satellite), so ends_right is false. For a blank line the caret position has no text, which gives the '(the file could not be re-read ...)' line.

## Notes

- Why the skeptic kept it: Reproduced exactly. A blank line, or a comment line, between satellite.return(satellite) and main's } is refused with S103, exit 12. The same comment at the end of the return's own line is accepted, a blank line anywhere else in main is fine, and a trailing blank line in another capsule is fine. The report is false three times over: it says the file could not be re-read, it names 'FILE_HAS_NO_RETURN', and it tells the person to add a line that is already there. The author's A7 ruling is about the last LINE of code; a blank line is not a statement.
- Nearest known entry: none. SATELLITE_ERROR.md and NEW_ERROR_LIST A7 describe S103 for a main that really lacks the return; no list mentions blank or comment lines.
- Merged: Single finding. last_statement is recorded for line_end and comment positions (program_check.cpp:2464).
