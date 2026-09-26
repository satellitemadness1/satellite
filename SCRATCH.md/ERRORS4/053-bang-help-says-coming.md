# 053 -- The ! operator works as bool negation, but the only help page that mentions it lists it as COMING and says "None of this reads today"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, control flow and capsules  
**Kind:** contradicts-help  
**Severity:** low

## What happens

! works as bool negation, but the only help that mentions it says it is COMING and that 'none of this reads today'.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.bool done = satellite.bool.false
    satellite.console.display(!done)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
true

exit 0
```

## What it should do

Either ! is refused as not built yet, as the help says of 'conditions: && || !', or the help documents ! as built (bool negation) and drops it from COMING / 'None of this reads today'.

satellite.help/binary_and_hexadecimal: 'COMING, ruled by the author on 2026-09-22 ... - conditions: && || !' and 'None of this reads today'. token_codes.hpp:87: not_token 'QUESTION: §13 leaves open whether ! is the negation'.

## Variants

!(1 == 2) -> true, exit 0. statement.if(!done) runs its block. !n on a number -> run-time S301 'a ! was put in front of a number' (27), after earlier lines run. t && t -> run-time S110 'satellite.console.display was given something it could not read to the end of -- check that every math sign has a space on both sides' (13), a different refusal than the help promises for undecided symbols (the false spacing hint is the already-confirmed lists#11 family).

## Where it comes from

satellite/bytecode/expression.cpp:1057-1067 (not_token handled as bool negation); satellite/bytecode/bytecode_registry.cpp:198 ('!' -> not_token); satellite.help/binary_and_hexadecimal/help_text.txt:28-41.

one_operand() in expression.cpp implements ! as negation of a bool, while the help page (written for the 2026-09-22 ruling) still lists ! as coming and not reading, and no help page covers ! as built.

## Notes

- Why the skeptic kept it: Reproduced: !done on a bool prints true and statement.if(!done) runs, exit 0. The only help page that mentions ! (satellite.help/binary_and_hexadecimal) lists 'conditions: && || !' under COMING and says 'None of this reads today'. No help page (there is no bool or statement.if topic) or DESIGN.md entry documents ! as built, token_codes.hpp:87 still marks it a QUESTION, yet expression.cpp:1057 implements it as bool negation. The help's 'None of this reads today' is false for !. Not in any known list (grep for ! / negation found nothing) nor among confirmed entries.
- Nearest known entry: none
- Merged: The same finding reported twice (satellite.help/binary_and_hexadecimal/help_text.txt against expression.cpp:1057).

### Also seen as: The ! operator works, but the only help page that mentions it lists it as COMING and says 'None of this reads today'

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.bool done = satellite.bool.false
    satellite.statement.if(!done)
    {
        satellite.console.display("not done")
    }
    satellite.console.display(!done)
    satellite.return(satellite)
}
```

```
not done
true

exit 0
```
