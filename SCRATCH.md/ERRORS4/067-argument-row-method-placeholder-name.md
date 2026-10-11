# 067 -- A .number refusal on an argument row names "that argument.number" instead of what was written (args.argument_1.number)

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** files and io  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A .number refusal on an argument row names "that argument.number" instead of what was written (args.argument_1.number).

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.variable.arguments args)
{
    satellite.variable.number count = args.argument_1.number
    satellite.console.display(count)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: One word after the program: ten

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S120: NUMBER_NOT_READ
satl(run): in count = ..., that argument.number: that text is not a whole number
this can read

directory: an.satl:5
syntax: satellite.variable.number count = args.argument_1.number
                                                                /\

this is not a number satellite can read. A number is an optional '-' then
digits, with no spaces or separators, and a float has one point in it: 12.34.
machine code 3 int_error -- satl exits with this.

--------------------------------------------------------------------------------

exit 3
```

## What it should do

"args.argument_1.number: that text is not a whole number this can read", naming what was written, the way s.number is named for a string variable.

The same refusal on a string variable says "s.number: ...". "that argument" is a placeholder, not what the program says, and when several rows are on one line it does not say which one failed.

## Variants

Two rows on one line, args.argument_1.number + args.argument_2.number with 5 and ten: still "that argument.number", exit 3. args["argument_1"].number with ten: "args.number: ...", which names args.number, a thing never written. A string variable s = "ten" with s.number: "s.number: ...", which is correct.

## Where it comes from

satellite/bytecode/expression.cpp:951

The dotted argument-row reader ends with maybe_a_method(row, at, std::move(current), "that argument", context), passing a fixed placeholder as the receiver's name. It should pass the row's written name (name + "." + key, which the lines above already build for slot_through_index).

## Notes

- Why the skeptic kept it: Reproduced. args.argument_1.number on the word "ten" is refused as "that argument.number: that text is not a whole number". For a string variable s the same refusal says "s.number". "that argument" is a fixed placeholder passed at expression.cpp:951. With two argument rows on one line (args.argument_1.number + args.argument_2.number, given 5 and ten), the refusal still says "that argument.number", and its caret sits at the end of the line, so the reader cannot tell which row failed. Nothing about this is in any known list. The already-confirmed lists#4 is about the chain's head name standing in for a list item, which is a different code path.
- Nearest known entry: none
- Merged: Single finding.
