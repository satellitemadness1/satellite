# 011 -- satellite.console.input() takes its line as raw bytes: it keeps the \r of a CRLF line ("12\r" refuses .number and != "12") and a leading UTF-8 BOM, both of which satellite.file drops

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io, number string conversion  
**Kind:** wrong-answer  
**Severity:** low

## What happens

satellite.console.input() keeps the \r of a CRLF line, so input piped from a Windows file gives "12\r": .number() refuses it (S120), == "12" is false, and 5 + input joins instead of adding.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string a = satellite.console.input()
    satellite.console.display(a.number() * 2)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: Saved at min.satl. The stdin file crlf.txt holds 4 bytes, made with printf '12\r\n' (od -c shows 1 2 \r \n). The control file lf.txt was made with printf '12\n'.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S120: NUMBER_NOT_READ
satl(run): a.number: that text is not a whole number this can read

directory: min.satl:6
syntax: satellite.console.display(a.number() * 2)
        /\

this is not a number satellite can read. A number is an optional '-' then
digits, with no spaces or separators, and a float has one point in it: 12.34.
machine code 3 int_error -- satl exits with this.

--------------------------------------------------------------------------------

exit 3
```

## What it should do

It should print 24 and exit 0, the same as with "12\n" as input (verified: the LF run prints 24, [exit 0]). python3: 12*2 = 24.

satellite.help/satellite.console/help_text.txt says input() and .input(prompt) "wait for a person to type a line, and answer it as a string without its newline". In a file saved on Windows, \r\n is that newline. satl's own file reader agrees. satellite.file.open() on the same crlf.txt returns line 1 as "12", and from_file.number()*2 prints 24. SATELLITE_FILE_OPERATIONS.md:269 says "a `\r` just before it is part of the ending", and split_text in satellite_file.cpp strips it. A program file with \r\n endings also runs (NEW_ERROR_LIST.md "A file saved on Windows runs"). So console.input() is the one reader that leaves the \r in. The program cannot fix it cleanly: .trim is not built (S210) and only a - "\r" works.

## Variants

These also fail with "12\r\nbob\r\n" as input (var.satl). a == "12" is false. 5 + a displays 512 followed by \r, joining instead of adding (LF gives 17). "[" + a + "]" displays "[12\r]". input("name? ") given "bob" then "hello " + b + "!" writes "hello bob\r!", which at a terminal shows as "!ello bob". The workaround (a - "\r").number()*2 prints 24. In cmp.satl, satellite.file.open(crlf.txt)[1].number()*2 prints 24, but comparing that line with console.input() read from the same file gives false (true with the LF file). --repl with a program piped as LF lines and the answer line "12\r" gives the same S120, exit 3. Separately, and maybe worth its own look: --repl with the whole piped session in CRLF refuses every program line with "U+000D (\x0d) has no meaning in a program", exit 13, although the same CRLF text runs as a .satl file. MILESTONES.md:167 lists that behaviour as an M0.6 port choice ("a piped line ... keeps a \r, for the statement reader to judge"), so it may be intended. These pass: the LF input (24, a.find("\r") refused with S420), and satellite.file reading of CRLF lines.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/console_calls.cpp:428

The input() path in console_calls.cpp reads with `if (!std::getline(std::cin, line)) got = InputAnswer::ended;` (line 428) and passes `line` to Value::of_utf8 (about line 441) without dropping a trailing '\r'. std::getline splits only at '\n', so the \r of a CRLF line stays in the string. The prompt reader used under --repl or --console (input_source(), line 419) keeps a piped \r too, per MILESTONES.md:167. By contrast, split_text in /home/madness/code/cxx/satellite/satellite/satellite_variable_file/satellite_file.cpp:140 treats a '\r' just before '\n' as part of the line ending.

## Notes

- Why the skeptic kept it: I reproduced it. With the input bytes "12\r\n", satellite.console.input() returns "12\r". So a == "12" is false, .number() refuses it with S120, and 5 + a joins the text into "512\r". With "12\n" the same program prints 24. Two readers of the same file disagree: satellite.file.open() on those same bytes returns line 1 as "12" (.number()*2 prints 24), while console.input() returns "12\r", and comparing the two gives false. None of ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md has an entry for this. Their CRLF entries (NEW_ERROR_LIST "A file saved on Windows runs", ERROR.md #13) cover only program files. DESIGN.md has no ruling on it. The closest note is MILESTONES.md:167, one of the M0.6 prompt port's choices ("a piped line is split at `\n` only and keeps a `\r`, for the statement reader to judge"). That note is about the --repl prompt reader and statement lines, is listed as "each one line to reverse", and says nothing about the value console.input() returns. The finding's path is a program run from a file, which reads with std::getline, not the prompt reader. The console help promises the line "as a string without its newline". The user has no clean way to strip it either: .trim is S210 not built yet, and only the workaround a - "\r" works. Severity is low. A person typing at a terminal never hits it, because the tty turns Enter into \n. It happens only when input is piped from a file saved with Windows line endings.
- Nearest known entry: none. The closest entries are NEW_ERROR_LIST.md "A file saved on Windows runs" and ERROR.md #13, which cover program files only. MILESTONES.md:167 is an M0.6 prompt-port choice about piped REPL statement lines, listed as reversible, and is not a ruling on console.input()'s value.
- Merged: Same line (console_calls.cpp:428, std::getline with no normalisation) and one fix: give console input the line-ending and BOM handling that the file reader already has.

### Also seen as: satellite.console.input() keeps a leading UTF-8 byte-order mark that satellite.file.open drops, so the same BOM file gives "12" one way and "﻿12" the other

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.file f = satellite.file.open("numbers.txt")
    satellite.console.display(f[1] == "12")
    satellite.variable.string a = satellite.console.input()
    satellite.console.display(a == "12")
    satellite.console.display("[" + a + "]")
    satellite.return(satellite)
}
```

```
true
false
[\xEF\xBB\xBF12]

exit 0
```
