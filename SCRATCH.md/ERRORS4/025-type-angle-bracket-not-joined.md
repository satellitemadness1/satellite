# 025 -- A container type split after its < (list< newline number>) is not joined across lines: "a type was expected here"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** syntax and layout  
**Kind:** valid-program-refused  
**Severity:** low

## What happens

A container type split after its < (list< newline number>) is not joined across lines: 'a type was expected here'.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list<
        satellite.variable.number> xs = {1, 2}
    satellite.console.display(xs)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, a type was expected here, and this is not one

directory: g1.satl:5
syntax: satellite.container.list<
        /\

satellite.has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

{1, 2}, exit 0

NEW_ERROR_LIST A4, the author's ruling: 'accept anything valid across any number of lines'. The same type split after its comma (g2) is joined and runs, and so is a declaration split after its '=' (g5).

## Variants

g2: 'satellite.container.map<satellite.variable.string,' newline 'satellite.variable.number> xs = {"a": 1}' prints {"a": 1}, exit 0. g3: 'satellite.container.map<' newline 'string, number> xs = ...' gets the same 'a type was expected here'. g5: split after '=' prints {1, 2}. g4: split after the closing > (the name on the next line) gets 'declares a name, and there is no name after the >'; that is outside A4's built list (no trailing operator) but inside its ruling.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/bytecode_registry.cpp:670-672 (goes_on in join_statements_across_lines)

join_statements_across_lines continues a statement onto the next line only when it is inside ( [ or a list's {, or the line ends in , = & |, or in a spaced + - * / % ^. A type's < is neither tracked as an opener nor in the trailing-operator set, so the line 'satellite.container.list<' stays a statement on its own and the type parser finds no type after the <.

## Notes

- Why the skeptic kept it: Reproduced. 'satellite.container.list<' followed by 'satellite.variable.number> xs = {1, 2}' on the next line is refused 'a type was expected here', exit 13. The map form split after its comma runs, and so does a declaration split after '='. The author's A4 ruling is 'accept anything valid across any number of lines', and the recorded build covers 'a trailing operator or comma'. A < ending a line is not treated as continuing the statement. Not in any known list.
- Nearest known entry: none. NEW_ERROR_LIST A4 records the join as built for 'open ( [ { or string, a trailing operator or comma, a leading .'.
- Merged: Single finding.
