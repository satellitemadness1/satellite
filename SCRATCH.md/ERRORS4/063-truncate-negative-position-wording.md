# 063 -- a.truncate(-1) is refused with "items count from 1", though truncate takes a count and truncate(0) is valid

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

a.truncate(-1) is refused with "items count from 1", though truncate takes a count and truncate(0) is valid.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list a = {1, 2, 3}
    a.truncate(-1)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S410: NOT_A_POSITION
satl(run): in a, a.truncate was given -1, and items count from 1

directory: l16a.satl:6
syntax: a.truncate(-1)
        /\

a position or a line number below zero. Lines count from 1, and nothing counts
below zero.
machine code 19 not_a_position -- satl exits with this.

--------------------------------------------------------------------------------

exit 19
```

## What it should do

"a.truncate was given -1, and a count is 0 or more", as .reserve says.

truncate(n) keeps n items, and a.truncate(0) runs and gives {}. So "items count from 1" implies that 0 is refused, which is false. .reserve, the other count-taking method, words the same mistake correctly.

## Variants

a.truncate(0) then display: {} (exit 0). a.reserve(-1): "satl(run): in a, a.reserve was given -1, and a count is 0 or more" (exit 19).

## Where it comes from

satellite/bytecode/container_calls.cpp:505-519 (only reserve_token gets the count wording; truncate_token takes the position wording, and "takes an item number" on line 511)

truncate is grouped with insert/remove_at in the position branch, and the ternary that picks the count wording tests only for reserve_token.

## Notes

- Why the skeptic kept it: Reproduced. a.truncate(-1) says "items count from 1", but truncate takes a count and a.truncate(0) runs and empties the list (displayed {}). The same guard gives .reserve(-1) "a count is 0 or more". The branch puts truncate with the position-taking methods (insert/remove_at), so the sentence is false for it. A non-number argument to truncate likewise says it "takes an item number". Not in any known list.
- Nearest known entry: none
- Merged: Single finding.
