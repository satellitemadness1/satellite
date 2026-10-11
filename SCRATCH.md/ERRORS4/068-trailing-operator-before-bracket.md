# 068 -- A trailing operator before ) (display(n +)) is told "this + has none" [space], and a touching-sign refusal inside display puts its caret at column 0

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** integers  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A trailing operator before ) (display(n +)) is told 'this + has none' [space], and a touching-sign refusal inside display puts its caret at column 0.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 5
    satellite.console.display(n +)
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
satl(run): every math operation is written with a space on both sides of the
sign, and this + has none

directory: o1.satl:6
syntax: satellite.console.display(n +)
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

'this + has nothing after it' (as `n + )` says 'there is no value here to work with'), with the caret on the sign, as the assignment form and the ** form already place it.

The + in `n +)` has a space on its left, so 'has none' is false, and the real problem is the missing right operand. One mistake gets three different sentences (`n +)`, `n + )`, `n -)`) and two caret placements depending on whether it sits in display or in an assignment.

## Variants

`display(n + )` gives 'there is no value here to work with'. `display(n -)` gives 'could not read to the end of -- check that every math sign has a space on both sides'. `n = n +)` gives the same 'has none' sentence with the caret near the sign. `display(n+1)` has its caret at column 0. `display(n**3)` has its caret at the sign.

## Where it comes from

satellite/bytecode/expression.cpp:289-292 (generic refusal: context.refuse(...) called without the `at` position, unlike the ** branch at 282-285, which passes `at`)

The lexer tags a + touching the next character (`)`) as tight_plus_token, whatever precedes it. refuse_if_reserved then gives the 'space on both sides ... has none' sentence without checking whether a value follows. It calls context.refuse without a position in the generic case, so inside a call the report falls back to column 0, while an assignment context supplies its own position.

## Notes

- Why the skeptic kept it: Reproduced. In `display(n +)` the + has a space before it and nothing after it. The refusal says 'a space on both sides of the sign, and this + has none', which is false. The + touches `)`, so the lexer made it a tight_plus. `display(n + )` instead says 'there is no value here to work with', and `display(n -)` gives a third sentence ('could not read to the end'). The caret sits at column 0 for display(n +) and display(n+1), but under the sign for `n = n +)` and for `display(n**3)`. These lines are single-line, so the known 'multi-line statement caret at column 0' entry does not cover them.
- Nearest known entry: none. The known column-0 caret entry (ERRORS2 #11) is about multi-line statements, and these are single lines.
- Merged: Single finding (the wording of refuse_if_reserved and its missing position). Kept separate from the run-time-only group.
