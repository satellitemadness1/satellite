# 062 -- The list help defines .sum as "every item added with +", but .sum refuses any string item, although 1 + "2" is 3 and "a" + "b" is "ab"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** contradicts-help  
**Severity:** low

## What happens

The list help defines .sum as "every item added with +", but .sum refuses any string item, although 1 + "2" is 3 and "a" + "b" is "ab".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display(1 + "2")
    satellite.container.list a = {1, "2"}
    satellite.console.display(a.sum)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
3
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): a.sum adds numbers, and item 2 is a string -- .join(separator) makes
one string of them

directory: l15a.satl:7
syntax: satellite.console.display(a.sum)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

The help and the behaviour agree. Most likely the help should say ".sum adds the number items, and refuses a string (use .join)", since check.sh requires the refusal.

satellite.help/satellite.container.list/help_text.txt: ".sum (every item added with +, and 0 for a list of nothing)". DESIGN.md §5 and the 2026-09-25 ruling: 4 + "2" is 6.

## Variants

{"a", "b"}.sum is refused as "item 1 is a string", while "a" + "b" displays ab. {1, 2.5, x1F, b101, 50%}.sum is refused with S210 at item 3 (float + hex not built), which follows +.

## Where it comes from

satellite/bytecode/container_calls.cpp:777-788 (.sum refuses !a_number_kind before adding); help text satellite.help/satellite.container.list/help_text.txt

.sum checks that every item is a number kind before it adds anything, so the + scenarios for strings are never reached. The help (and the code comment above it) describe it as plain + over the items.

## Notes

- Why the skeptic kept it: Reproduced. The list help defines .sum as "every item added with +". In the same program, 1 + "2" is 3 and "a" + "b" is "ab", yet {1, "2"}.sum and {"a", "b"}.sum are refused (S301) with "adds numbers". check.sh (row ".sum adds numbers, and points a list of strings at .join") shows the refusal is deliberate, and the source comment repeats the "every item added with +" wording. So the fault is in the help text, which contradicts the behaviour. No known list mentions it.
- Nearest known entry: none
- Merged: Single finding.
