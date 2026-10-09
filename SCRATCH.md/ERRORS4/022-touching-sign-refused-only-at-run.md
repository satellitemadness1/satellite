# 022 -- A touching operator (n+1, 4+3, n**3, a*2) is refused only when its line runs: it is accepted silently in a branch not taken or a capsule never called, and refused after earlier lines have printed

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** integers, syntax and layout  
**Kind:** invalid-accepted-silently  
**Severity:** medium

## What happens

A touching operator (n+1, 4+3, n**3) is refused only when its line runs, so in an untaken branch or an uncalled capsule it is accepted silently.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 5
    satellite.statement.if(n > 10)
    {
        n = n+1
    }
    satellite.console.display(n)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
5

exit 0
```

## What it should do

S110 'every math operation is written with a space on both sides of the sign, and this + has none' from satl(check), before anything runs, wherever the line sits. This is how `n += 1` in the same untaken if and a touching for step are already refused.

satellite.variable.number help: 'What it refuses today: a touching sign, 4+3 (S110)'. The checker already refuses the neighbouring shape mistakes (`n += 1`, `i+1` as a for step) before anything runs. A mistake that can be seen without running is accepted silently when its line happens not to run.

## Variants

`display("before")` then `display(4+3)` prints `before`, then satl(run) S110. `satellite.variable.number n = 4+3` and `n = 4 +3` behave the same (run time, after `before`). `n = n**3` gives the run-time 'power is written with a space on both sides'. `n += 1` in the untaken if gives satl(check) S210 before anything runs.

## Where it comes from

satellite/bytecode/expression.cpp:253-294 (refuse_if_reserved is reached only from the evaluator, at lines 1070 and 1525). program_check.cpp never looks for tight_plus/tight_times/tight_modulus/tight_power tokens outside the for step.

The lexer produces distinct tight_*_token codes for a touching sign. Only the run-time evaluator (refuse_if_reserved) refuses them, and the checker's names_in_statement walk lets them through, so the refusal waits until the expression is evaluated.

## Notes

- Why the skeptic kept it: Reproduced. `n = n+1` in an untaken if runs to exit 0 and prints 5. When the line is reached, the refusal comes from satl(run) after earlier lines have printed. `4+3`, `n = 4 +3` and `n**3` all behave the same way. The checker already refuses `n += 1` in the same untaken branch (satl(check), S210) and a touching for step before anything runs. The help's 'refused today' example shows satl(run) output, but no ruling says a touching sign should be refused only at run time, and it is a spelling mistake visible without running anything. This is not ERRORS2 #8, which is about wrong-TYPE literals.
- Nearest known entry: none. ERRORS2 #8 / NEW_ERROR_LIST B2 cover wrong-type literals being refused only when run, not touching operators.
- Merged: The same finding reported twice: only refuse_if_reserved in the evaluator refuses tight_* tokens, and the checker never looks for them.

### Also seen as: A touching sign (a+1, a*2, 4+3) is refused only when its line runs: accepted silently in code not reached, and refused after earlier lines have printed

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number a = 7
    satellite.statement.if(a == 0)
    {
        satellite.console.display(a*2)
    }
    satellite.console.display("done")
    satellite.return(satellite)
}
```

```
done

exit 0
```
