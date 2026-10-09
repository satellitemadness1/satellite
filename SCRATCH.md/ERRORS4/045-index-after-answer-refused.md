# 045 -- [n] after a call's answer, a method's answer, a list literal or a bracketed expression is refused at run time with a false math-sign / "& | << >> !!" hint, though the same value in a variable can be indexed

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

[n] after a call's answer, a method's answer, a list literal or a bracketed expression is refused at run time with a false hint about math-sign spacing (or "& | << >> !!"), though the same value in a variable indexes fine.

## Program

```satellite
satellite.include(satellite)

satellite.capsule squares()
{
    satellite.return({1, 4, 9})
}

satellite.capsule satellite.main()
{
    satellite.container.list t = squares()
    satellite.console.display(t[2])
    satellite.console.display(squares()[2])
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at min_index_call_answer.satl

## What satl does

```
4
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(run): satellite.console.display was given something it could not read to
the end of -- check that every math sign has a space on both sides

directory: min_index_call_answer.satl:12
syntax: satellite.console.display(squares()[2])
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

squares()[2] should print 4, as t[2] does on the line above with the same value. If [n] after a call, method or literal is meant to be unsupported, the refusal should say that [n] reads only a named list, and ideally it should come before anything runs. It should not tell the person to check math-sign spacing (the line has no math sign) or mention & | << >> !! (none are on the line).

satellite.help/satellite.container.list/help_text.txt says "a[n] reads item n" and that a list is written "with braces wherever a value goes". Equivalent forms disagree: `satellite.container.list t = squares()` then `t[2]` prints 4, while `squares()[2]` is refused. The same answer takes .size, .first and .sum directly: squares().first printed 1 and squares().sum printed 14. The refusal says something false: the line has correct spacing and no math sign at all.

## Variants

All run with the probe home. FAIL, display form, same math-sign sentence (S110, exit 13): `display({10, 20, 30}[2])`, `display(a.sort().by_value()[1])`, `display(a.reverse()[1])`, `display(g.first[2])` with g = {{1, 2}, {3}}, `display((squares())[2])`, and the display inside a capsule other than main (v12). FAIL, one-line "& | << >> !! are undecided" refusal (exit 13): `satellite.variable.number x = squares()[2]`, `satellite.variable.number smallest = a.sort().by_value()[1]`, and `satellite.statement.if(a.sort().by_value()[1] == 10)` ("satellite.statement.if's condition could not be read to the end"). In every failing case the lines before it had already run. PASS: `t = squares()` then `t[2]` gives 4, squares().size gives 3, squares().first gives 1, squares().sum gives 14. For comparison, `display(5 -4)` is also refused only at run time with the same sentence, so the late timing matches how satl treats other expression problems. The error here is the unsupported postfix [n] and its false hint.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/expression.cpp:1425 (the only [ chain for a value, and it runs only after a named variable; the other is after_an_argument at :921, for arguments rows). maybe_a_method at expression.cpp:816-822 and call_method at :334 never look for [. The false hint comes from expression.cpp:1657-1667 for a call's argument, and from program_walk.cpp:78-80 (kNotReadToTheEnd) for an assignment or condition.

In one_operand, [ is walked only right after a variable's name (expression.cpp:1425) or an arguments row (after_an_argument, :921). A capsule call's answer, maybe_a_method and call_method (the chain after .method), a list literal and a bracketed sub-expression all return without checking for a following [. The evaluator then stops at the [. For a display's argument, the "must reach its own )" check at expression.cpp:1657 fires, and because the line has not ended it picks the "check that every math sign has a space on both sides" sentence. For a variable or a condition, read_to_the_end in program_walk.cpp fails and gives kNotReadToTheEnd, which names & | << >> !! as the likely cause.

## Notes

- Why the skeptic kept it: Reproduced on BUILD 0099. `[n]` is read only directly after a variable's name. After a call's answer, a method's answer, a list literal or a bracketed expression, the evaluator stops at the `[`, and satl refuses the line while it runs with a false hint about math-sign spacing. Inside an assignment or an if condition the hint names "& | << >> !!" instead, which is also false. The same value held in a variable indexes fine: `t = squares()` then `t[2]` prints 4. Nothing in ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md covers indexing a call or method answer, and DESIGN.md does not say this is intended. CONTAINERS.md's step 1 table only covers indexing named containers. One part of the hunter's case is weaker than stated: refusing only at run time is not special to this form. `display(5 -4)` is also refused only when its line runs, after "before" has printed, so the checker misses this whole class of line and not just this one.
- Nearest known entry: none
- Merged: Single finding.
