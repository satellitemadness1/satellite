# 016 -- A // comment in a place where only line ends are stepped over breaks an if/else/while/for: a comment line between a header and its {, a comment after else, or a comment after an if's } or before the else gives "has no body" or "else with no satellite.statement.if before it"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** control flow and capsules, realistic programs, syntax and layout  
**Kind:** valid-program-refused  
**Severity:** medium

## What happens

A comment between if/else/while and its { (or after else on its line, or between } and else) is refused as 'has no body' / 'else with no if before it'.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 7
    satellite.statement.if(n == 1)
    {
        satellite.console.display("one")
    }
    satellite.statement.else // anything but one
    {
        satellite.console.display("not one")
    }
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none; c4_else.satl (variants c4_ifline, c4_while, c4_ifinline, c4_elseback, c4_cap, c4_for in the same folder)

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.statement.else has no body

directory: c4_else.satl:10
syntax: satellite.statement.else // anything but one
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

not one

A comment has no meaning to the program. satellite.help/satellite.return/help_text.txt:20-22 says of a capsule header: 'only line ends and comments come between its ) and its {', and satl accepts exactly that for capsules. The same comment after if(...) on its own line is accepted, so if and else disagree, and capsule and if headers disagree.

## Variants

A comment line between if(...) and { gives 'satellite.statement.if has no body' (line 6). A comment line between while(...) and { gives 'satellite.statement.while has no body'. A bare else, then a comment line, then { gives 'else has no body'. A comment line between } and else gives 'satellite.statement.else with no satellite.statement.if before it'. Accepted: a comment line between a capsule header and { (prints 8), 'if(n == 7) // c', and 'for(...) // two'.

## Where it comes from

satellite/bytecode/program_walk.cpp:129-133 (brace_after); satellite/bytecode/program_check.cpp:1749-1753, 1765-1766, 1771-1780; program_walk.cpp:761, 827, 888 (the run-time twins)

brace_after() skips only token::line_end_token, so a comment_token between a header and its { stops it, and the checker reports 'has no body'. For if/while the header end comes from past_the_statement, which swallows an inline comment, so only a comment on its own line hurts. For else, the checker calls brace_after(row, at + 1) directly, so even an inline comment on the else line stops it. The else look-back (program_check.cpp:1765-1766) also skips only line_end tokens, so a comment line between } and else hides the if.

## Notes

- Why the skeptic kept it: Reproduced. 'satellite.statement.else // anything but one' followed by { is refused with S110 'satellite.statement.else has no body' (exit 13). A comment on its own line between 'if(...)' and { gives 'satellite.statement.if has no body', and between 'while(...)' and { gives 'satellite.statement.while has no body'. A bare else followed by a comment line gives 'else has no body' too. A comment line between } and else gives 'satellite.statement.else with no satellite.statement.if before it'. By contrast, a comment line between a capsule header and its { works (prints 8), and so do 'if(...) // c' and 'for(...) // two'. The satellite.return help states the rule 'only line ends and comments come between its ) and its {' for a capsule header. A comment changes nothing about a program, so these are valid programs refused. This is not in any known list. ERROR.md only notes that a comment line passes the check in general.
- Nearest known entry: none
- Merged: One cause and one fix: brace_after (program_walk.cpp:129-133) and the else look-back (program_check.cpp:~1766, and its twin in the walker) skip line_end_token but not comment_token. The tail-call case (control-flow-and-capsules#0) follows the same pattern in a different function with a different symptom, so it is kept separate.

### Also seen as: A // comment between a statement's parts is not stepped over: `} // end if` or a comment line before an else, a comment after `else`, or a comment line between an if/while/for header and its { makes a valid program refuse ("else with no if before it" / "has no body")

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 5
    satellite.statement.if(n > 9)
    {
        satellite.console.display("big")
    } // end if
    satellite.statement.else
    {
        satellite.console.display("small")
    }
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.statement.else with no
satellite.statement.if before it

directory: e1.satl:10
syntax: satellite.statement.else
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

### Also seen as: satellite.statement.else followed by a // comment on its line is refused 'has no body' (`else { // c` and `else // c` with { below)

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 5
    satellite.statement.if(n == 6)
    {
        satellite.console.display("six")
    }
    satellite.statement.else { // not six
        satellite.console.display("not six")
    }
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.statement.else has no body

directory: p4.satl:10
syntax: satellite.statement.else { // not six
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

### Also seen as: A // comment after an if's closing } (or on its own line before the else) makes the else refused as 'with no satellite.statement.if before it'

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.statement.if(1 == 1)
    {
        satellite.console.display("six")
    } // c
    satellite.statement.else
    {
        satellite.console.display("no")
    }
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.statement.else with no
satellite.statement.if before it

directory: v5a.satl:9
syntax: satellite.statement.else
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

### Also seen as: A comment line between an if/while/for/else header and its { is refused 'has no body' (and one before an else: 'no satellite.statement.if before it'), while a capsule header accepts it

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.statement.if(1 == 1)
    // the body:
    {
        satellite.console.display("six")
    }
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.statement.if has no body

directory: c1.satl:5
syntax: satellite.statement.if(1 == 1)
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

### Also seen as: A // comment between an if's } and satellite.statement.else (own line or trailing the }) makes the else "with no satellite.statement.if before it"

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 7
    satellite.statement.if(n == 1)
    {
        satellite.console.display("one")
    } // end
    satellite.statement.else
    {
        satellite.console.display("not one")
    }
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.statement.else with no
satellite.statement.if before it

directory: else_trail.satl:10
syntax: satellite.statement.else
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```
