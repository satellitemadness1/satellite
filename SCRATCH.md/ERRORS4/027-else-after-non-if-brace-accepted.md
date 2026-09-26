# 027 -- An else after a } that does not close an if (a while's or for's body, or a second else) passes the checker and is refused only when reached, as a one-line message with no S-code or line

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** control flow and capsules, syntax and layout  
**Kind:** invalid-accepted-silently  
**Severity:** low

## What happens

An else after any } that is not an if's (a while's body, or a second else after an if/else) passes the checker: silent in unrun code, a one-line error after earlier output when reached.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.statement.while(1 == 2)
    {
    }
    satellite.statement.else
    {
        satellite.console.display("else")
    }
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
before
[satellite] satl(run): satellite.statement.else with no satellite.statement.if before it (machine_code: 13 satl_line_not_understood)

exit 13
```

## What it should do

Refused before anything runs, with the full S110 report 'satellite.statement.else with no satellite.statement.if before it' on line 9, as a lone else is (the same program with no while: checker refusal, nothing printed).

The checker already refuses an else that has no } before it, before anything runs. An else after a while's } is the same mistake. It gets past the checker only because the look-back checks for any }, not for an if's }. It breaks 'nothing runs before a refusal', and it lets an invalid capsule that is never called pass silently.

## Variants

The original: a while/else in a capsule that is never called gives 'ran', exit 0. if {} else {} else {} in main: 'before', then the same one-line run error, exit 13. A lone else in main with no } before it: refused at check time with the full report, nothing printed.

## Where it comes from

satellite/bytecode/program_check.cpp:1759-1770 (the else look-back accepts any right_brace_token); program_walk.cpp:1771-1774 (the run-time refusal, one line)

The checker's look-back from an else asks only whether the previous non-line-end code is a `}`, not whether that } closes an if (or else-if) body. So the } of a while, or of an else, satisfies it. The run later reaches the else as a statement of its own, since no if claimed it, and refuses it with a one-line report_error.

## Notes

- Why the skeptic kept it: Reproduced. satellite.statement.else after a while's } passes the checker. In a capsule that is never called it is accepted with exit 0. In main it prints 'before', then a one-line run-time 'satl(run): satellite.statement.else with no satellite.statement.if before it', exit 13. A lone else is refused before anything runs with the full report. A second else after an if/else chain slips through the same way. A source comment in program_check.cpp:1762-1763 admits the gap ('A `}` that closed a while's body slips through this and is refused when it runs'). That comment is an acknowledgement, not a ruling, and the gap is in no known list. Same look-back code as #5 but the opposite symptom (invalid accepted, where #5 is valid refused), so I kept it separate.
- Nearest known entry: none in the known lists. Acknowledged only in the source comment at program_check.cpp:1762-1763.
- Merged: The same finding reported twice: the else look-back (program_check.cpp:1759-1770) accepts any right_brace_token.

### Also seen as: An else after a while's or for's } passes the checker and is refused only after the loop has run, as a one-line message with no S-code or line

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.number i = 0
    satellite.statement.while(i < 2)
    {
        i = i + 1
    }
    satellite.statement.else
    {
        satellite.console.display("else")
    }
    satellite.return(satellite)
}
```

```
before
[satellite] satl(run): satellite.statement.else with no satellite.statement.if before it (machine_code: 13 satl_line_not_understood)

exit 13
```
