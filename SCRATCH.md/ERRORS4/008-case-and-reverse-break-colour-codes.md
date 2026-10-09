# 008 -- .upper() and .reverse() on a coloured string rewrite its embedded terminal codes, so raw escape bytes reach a pipe (ESC[38;2;0;255;0M), and at a terminal ESC[...M deletes lines

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** strings  
**Kind:** wrong-answer  
**Severity:** medium

## What happens

.upper() and .reverse() on a coloured string rewrite its terminal codes, so raw escape bytes reach a pipe (ESC[38;2;0;255;0MOK), and at a terminal ESC[...M is 'delete lines'.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("ok".foreground(x00FF00).upper())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: stdout is a pipe (the runner's default); shown through cat -A

## What satl does

```
^[[38;2;0;255;0MOK^[[39M$

exit 0
```

## What it should do

OK (plain in a pipe, green at a terminal). "ok".foreground(x00FF00).reverse() should give ko, coloured. The text changes and the codes stay intact.

string help: '.foreground(c) ... The colour is part of the string wherever the program runs, as the terminal's own codes ... satellite.console.display is what leaves them out, writing the string plain into a pipe or a file'. '.upper() ... answers the text in capital letters'. '.reverse() turns a string round by character'. The coloured g on its own, and g.lower(), both display plain 'ok', so display strips codes that are intact.

## Variants

g = "ok".foreground(x00FF00): display(g) gives ok. g.upper() gives ^[[38;2;0;255;0MOK^[[39M. g.lower() gives ok. g.reverse() gives m93[^[kom0;552;0;2;83[^[. "Ok".background(xFF0000).lower() gives ok. "ok".upper().foreground(x00FF00) gives OK, so the correct order works. g.upper().lower() gives ok, because the codes are restored. g.upper().find("O") gives 15, which matches the documented rule that .find sees the codes.

## Where it comes from

satellite/bytecode/expression.cpp:498 (string_case on the whole stored string, codes included). satellite/bytecode/container_calls.cpp:225 reverse_of, reached from expression.cpp:452-456.

upper/lower and reverse work on every character of the stored string, including the embedded ESC[...m colour codes. upper turns the SGR terminator 'm' into 'M', and reverse scrambles the sequence. display's colour stripper then no longer recognises them and passes them through raw.

## Notes

- Why the skeptic kept it: Reproduced byte for byte. "ok".foreground(x00FF00).upper() displays ESC[38;2;0;255;0MOK ESC[39M into a pipe: the SGR final 'm' became 'M', display no longer recognises the code as a colour, and it writes the escape raw. .reverse() writes the codes backwards (m93[ESC kom0;552;0;2;83[ESC). .lower() and .upper().lower() come out plain 'ok', because the codes are already lower case. The help says the colour is part of the string and that display writes it plain into a pipe, and that .upper() gives the text in capitals and .reverse() turns the string round by character. Neither says the terminal codes are text to be changed. At a terminal, CSI ... M is Delete Line. None of the known lists or DESIGN.md mentions colour with case or reverse.
- Nearest known entry: none (not in ERROR.md, errors.md, ERRORS2/3, NEW_ERROR_LIST, CONTAINERS or DESIGN.md)
- Merged: Single finding.
