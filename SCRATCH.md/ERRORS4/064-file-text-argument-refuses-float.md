# 064 -- A file word's text argument converts a whole number (f.append(42)) but refuses a float, percentage, hex, binary or fraction as "takes text", while "" + 2.5 joins

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** inconsistency  
**Severity:** low

## What happens

A file word's text argument converts a whole number (f.append(42)) but refuses a float, percentage, hex, binary or fraction as "takes text", while "" + 2.5 joins.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.file f = satellite.file.new("prices.txt")
    f.append(42)
    f.append(2.5)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: No prices.txt beside the program beforehand.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): in f, f.append takes text, and was given a float

directory: .../verify2/batch-1/f8/p.satl:7
syntax: f.append(2.5)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------
(prices.txt afterwards: 42)

exit 27
```

## What it should do

prices.txt holds 42 and 2.5, each conversion recorded as S020 in satellite.log, as for the whole number.

The author's 2026-09-16 ruling (MILESTONES.md:544, file_calls.cpp:60 quotes it) converts a number where text is expected. A1 in NEW_ERROR_LIST converts every kind of number in string +. The refusal is only at run time, after line 6 already wrote.

## Variants

Refused the same way: f.append(50%), f.append(x1F), f.append(b101), f.append(1/2). Converted: f.append(42), f.append(-7). "" + each of these joins as text. The S301 paragraph ("this operator has no scenario...") also speaks of an operator for a method argument, which is the known ERRORS2 E pattern.

## Where it comes from

satellite/bytecode/file_calls.cpp:63-74 (text_of: only value.as_number() is converted)

text_of checks is_string(), then as_number(), which is the whole-number arm only. Floats, percentages, hex, binary and fractions fall through to the types_do_not_meet refusal.

## Notes

- Why the skeptic kept it: Reproduced. f.append(42) and f.append(-7) are written as text (S020 logged), but f.append(2.5), 50%, x1F, b101 and 1/2 are all refused at run time: "f.append takes text, and was given a float/percentage/hex/binary/fraction". Meanwhile "" + 2.5 (and the others) join as text. The author's ruling (MILESTONES.md:544, 2026-09-16) is that "a number where a string is expected" is "just convert the number to the string ... record the warning in satellite.log". NEW_ERROR_LIST A1 built string + number for "every kind of number". text_of converts only Value::as_number(), the whole-number kind. Not in any known list.
- Nearest known entry: none
- Merged: Single finding.
