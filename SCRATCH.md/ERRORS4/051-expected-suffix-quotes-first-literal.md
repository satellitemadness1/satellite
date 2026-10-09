# 051 -- A percentage, binary or hex name given an expression is told "ERROR: expected <first number><suffix>", built from its first literal alone: p = 1/4 is "expected 1%", p = 2 * 25% is "expected 2%", and a binary given a sum is "expected b1"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, floats fractions percent  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A percentage name given a fraction or an expression is told "ERROR: expected <its first number>%": p = 1/4 is told "expected 1%", p = 2 * 25% "expected 2%".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.percent p = 1/4
    satellite.console.display(p)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at f3.satl (variants f3b.satl: 2 * 25%, f3c.satl: 50, f3d.satl: 0.5)

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(check): in satellite.main, ERROR: expected 1%

directory: f3.satl:5
syntax: satellite.variable.percent p = 1/4
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

A refusal naming what was written. For 1/4: a percentage name holds only a percentage, and 1/4 is a fraction. For 2 * 25%: that expression answers a number. It should not suggest 1% or 2%, which are not the values written.

The percent help limits the "expected 50%" rule to a bare number given to the name. A refusal that says something false (that the value written is 1 missing its %) is an error by the task's definition.

## Variants

These fail (the suggestion drops the rest of the value): 2 * 25% gives "expected 2%", and the hunter also reports 3/4% giving "expected 3%" and 1.5/2 giving "expected 1.5%". These are correct: a bare 50 gives "expected 50%", and 0.5 gives "expected 0.5%".

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:357-372 (called at :2019 and :2221)

percentage_is_written_with_percent skips minus signs and brackets, then refuses whenever the next token is a number_token. It never looks at what follows that token (a /, a *, and so on), and it builds why = "ERROR: expected " + text_at(row, k) + "%" from that single token.

## Notes

- Why the skeptic kept it: I reproduced it. percentage_is_written_with_percent reads only the first number token after the = and builds "ERROR: expected <that token>%". It does this even when that token begins a fraction (1/4) or an expression (2 * 25%). The refusal of the wrong program is right, but its advice is false. 1/4 is not '1 missing its %', and writing the suggested 1% gives a different value from the one written. The help's rule ("A percentage name given 50 is refused ... with ERROR: expected 50%") is for a bare number. It does not cover a fraction or an expression. Nothing in the known lists or among the confirmed findings mentions this.
- Nearest known entry: none
- Merged: The same flaw, with the same fix, in parallel checks (program_check.cpp:329-334 and :357-372, hexadecimal_values.cpp:107-112): the first number_token is judged without looking at what follows it.

### Also seen as: A binary or hex name declared with a sum is refused as "expected b1" / "expected x200": the check quotes only the first literal of the expression

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.binary a = 1 + 1
    satellite.console.display(a)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(check): in satellite.main, ERROR: expected b1

directory: f5a.satl:5
syntax: satellite.variable.binary a = 1 + 1
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```
