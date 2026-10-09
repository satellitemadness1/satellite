# 072 -- counter.bump() on a spacesuit's own name says counter has no satellite.variable line declaring it

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

counter.bump() on a spacesuit's own name says counter has no satellite.variable line declaring it.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit counter()
{
    satellite.public
    {
        satellite.capsule bump()
        {
            satellite.return(1)
        }
    }
}

satellite.capsule satellite.main()
{
    counter.bump()
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, counter has no satellite.variable line declaring
it

directory: v14a.satl:16
syntax: counter.bump()
        /\

this name was used and no satellite.variable line ever declared it. A name has
to be given a type before it can hold anything.
machine code 25 name_not_declared -- satl exits with this.

--------------------------------------------------------------------------------

exit 25
```

## What it should do

A refusal saying counter is a spacesuit and bump runs on an object of it: declare one (counter c) and call c.bump().

counter is declared on line 3 as a spacesuit, so "no satellite.variable line declaring it" is false and misdirects. satl gives spacesuit-aware sentences in the neighbouring cases (program_check.cpp:1186 and :1273).

## Variants

Inside display (the hunter's form) the refusal is the same; there "before" does not print, because the checker refuses first.

## Where it comes from

satellite/bytecode/program_check.cpp:1238-1252 (capsules.reach(scope, {counter, bump}) finds no file or space) falling to :1285-1297 (the undeclared-name S201)

For a dotted name with no variable, the checker tries capsules.reach over files and namespaces only. A spacesuit's name is neither, so reach fails without through_a_scope, and the name falls to the generic undeclared-variable refusal. suit_named() is consulted only when brackets follow the bare name (counter(...)).

## Notes

- Why the skeptic kept it: Reproduced: counter.bump(), where counter is a declared spacesuit, is refused before the run as S201 "counter has no satellite.variable line declaring it". That is false, because counter is declared (as a spacesuit), and it does not point to the fix (declare an object). The checker already has a spacesuit-aware sentence for counter(...) ("counter is a spacesuit, and an object of it is made by declaring one...") and for a press naming a spacesuit's capsule, but not for Suit.capsule(). Not in any known list: B5/ERRORS2 #9 cover 003 spellings, which is a different case.
- Nearest known entry: none
- Merged: Single finding.
