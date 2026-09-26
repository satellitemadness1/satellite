# 070 -- A fraction name given a spaced slash is always told it is "whole-number division", even for 1.0 / 3, and after 2/4 it names a pair "4 / 2" that was never written

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** floats fractions percent  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A fraction name given a spaced slash is always told it is "whole-number division", even for 1.0 / 3 (float division), and after 2/4 it names a pair "4 / 2" that was never written.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.fraction f = 1.0 / 3
    satellite.console.display(f)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(check): in satellite.main, 1.0 / 3 is whole-number division -- a fraction
is written with a touching slash: 1.0/3

directory: p4.satl:5
syntax: satellite.variable.fraction f = 1.0 / 3
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

A refusal that is true of what was written, for example 'a float divided is not a fraction' for 1.0 / 3. For 2/4 / 2 it should not suggest '4/2'.

Float help: '4 / 3 is still whole-number division and answers 1; write 4.0 / 3 for the float'. So 1.0 / 3 is float division, not whole-number division. And '4 / 2' does not appear in '2/4 / 2'.

## Variants

fraction f = 2/4 / 2 -> '4 / 2 is whole-number division -- ... touching slash: 4/2'. fraction f = x / 2 (x a float 1.5) -> 'a / with a space on both sides is whole-number division -- ... 1/3'. fraction f = 1 / 3 -> '1 / 3 is whole-number division ... 1/3' (correct). The caret is at column 0 each time.

## Where it comes from

satellite/bytecode/fraction_values.cpp:93-137 (fraction_is_written_right; sentence built at lines 127-132)

fraction_is_written_right refuses any spaced / outside brackets and builds its sentence from 'before', the text of the last number_token. It never checks whether that token is a float (1.0) or the denominator of a touching fraction (2/4 -> 4), or whether a name operand is a float. So it always says 'whole-number division' and pairs whatever number came last.

## Notes

- Why the skeptic kept it: Reproduced. 'fraction f = 1.0 / 3' is refused with '1.0 / 3 is whole-number division'. That is false: the float help says 4.0 / 3 is the float division. 'fraction f = 2/4 / 2' is told '4 / 2 is whole-number division ... 4/2', a pair that was never written, because 4 is the bottom of the fraction 2/4. 'fraction f = x / 2' with x a float is told 'a / with a space on both sides is whole-number division ... 1/3'. The refusal itself is by design (a spaced slash in a fraction's value is refused, per the 2026-09-16 rule in the source comment), but the sentence is false whenever an operand is a float or a fraction, and it names the wrong pair after a fraction literal. 'fraction f = 1 / 3' gives a correct sentence. This does not appear in ERROR.md, errors.md, ERRORS2, NEW_ERROR_LIST, CONTAINERS or the confirmed list.
- Nearest known entry: none
- Merged: Single finding.
