# 039 -- An expression that stops early for any reason gets the fixed "& | << >> !! are undecided" (or "math sign spacing") sentence at run time: n = n -1, n = 5 6, if (n = 5), 5., 0x1F, 0b101, and 1.2.3 with its caret past the end of the line

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, floats fractions percent, integers  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

An assignment, declaration or while/if/for condition that stops early is blamed on "& | << >> !! are undecided" when none is there (n = n -1, n = 5 6, if (n = 5)); display(...) gives a different sentence for the same mistake.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 5
    n = n -1
    satellite.console.display(n)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
[satellite] satl(run): n = ... could not be read to the end -- it stops at something with no meaning there yet (& | << >> !! are undecided), so the part before it is not the whole value (machine_code: 13 satl_line_not_understood)

exit 13
```

## What it should do

A sentence about what is actually on the line. For `n -1`, that the minus needs a space on both sides, as display(n -1) already says. For `5 6`, that there are two values with no operator between them. For `if (n = 5)`, to compare with ==.

The sentence names & | << >> !!, and none of them is on these lines, so it points the reader at something that is not there. The same mistake inside display(...) gets a different, more useful sentence, so the two forms disagree. The parenthetical is also out of date: the author ruled on these operators on 2026-09-22 (ERRORS3 #10).

## Variants

`satellite.variable.number m = n -1` -> same sentence, "m = ...". `satellite.statement.while (n -1 > 10)` -> "satellite.statement.while's condition could not be read to the end -- ... (& | << >> !! are undecided)". `satellite.statement.if (n = 5)` after a display -> prints 5, then the same sentence for the if's condition, exit 13. `n = 5 6` and `n = 1, 2` -> same sentence. `display(n -1)` and `display(5 6)` -> full S110 report, "check that every math sign has a space on both sides".

## Where it comes from

satellite/bytecode/program_walk.cpp:79-81 (kNotReadToTheEnd), used at :790 (while), :854 (if), :963/:1034/:1078 (for), :1180 (assignment), :1322, :1375, :1426, :1554, :1735, :1751, :1845, :1933

kNotReadToTheEnd was written when the only way an expression stopped early was one of the undecided QUESTION operators (the ERROR.md fix of 2026-09-17). Every early stop in the walker now reuses it: a half-spaced minus read as a negative literal, a second value, a comma, or = in a condition. The sentence never says what the evaluator actually stopped at.

## Notes

- Why the skeptic kept it: Reproduced. `n = n -1`, `n = 5 6`, `n = 1, 2`, `satellite.variable.number m = n -1`, `satellite.statement.while (n -1 > 10)` and `satellite.statement.if (n = 5)` all get the same one-line sentence: "...could not be read to the end -- it stops at something with no meaning there yet (& | << >> !! are undecided)". None of those operators is on any of these lines. The if case runs only after earlier lines printed. The same `n -1` and `5 6` inside display(...) get a different sentence: "check that every math sign has a space on both sides". So equivalent forms disagree. The one-line format with no S-code is known (ERRORS2 #7) and is not counted here; the wrong content is separate. Related, but not the same: ERRORS3.md #10 records that call_word's catch-all "spaces" sentence blames spaces that are there, and notes that the "undecided" text is stale for && and ||. ERRORS3 is not one of the enumerated known lists. It also does not record that this sentence is the refusal for every early stop in an assignment, declaration or condition (a half-spaced minus, two values with no operator, = in a condition), where it points at operators that are not present.
- Nearest known entry: Related but distinct: SCRATCH.md/ERRORS3.md #10 (the catch-all "spaces" sentence in call_word; the "undecided" text called stale for && ||). The one-line run-time format is ERRORS2 #7 (known, not counted).
- Merged: One sentence (kNotReadToTheEnd, program_walk.cpp:79-81) is reused for every early stop. One fix: name what the evaluator stopped at, and have the checker refuse two values standing side by side.

### Also seen as: A float literal with a trailing point (5.) is refused only at run time, with a sentence blaming "& | << >> !!" (or math-sign spacing) that are not on the line; 1.2.3 is refused at run time with its caret past the end of the line

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.float x = 5.
    satellite.console.display(x)
    satellite.return(satellite)
}
```

```
before
[satellite] satl(run): x = ... could not be read to the end -- it stops at something with no meaning there yet (& | << >> !! are undecided), so the part before it is not the whole value (machine_code: 13 satl_line_not_understood)

exit 13
```

### Also seen as: 0x1F / 0b101 (the C spelling) as a number or display value is refused at run time with hints naming & | << >> !! or math-sign spacing, none of which applies

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.number n = 0x1F
    satellite.console.display(n)
    satellite.return(satellite)
}
```

```
before
[satellite] satl(run): n = ... could not be read to the end -- it stops at something with no meaning there yet (& | << >> !! are undecided), so the part before it is not the whole value (machine_code: 13 satl_line_not_understood)

exit 13
```
