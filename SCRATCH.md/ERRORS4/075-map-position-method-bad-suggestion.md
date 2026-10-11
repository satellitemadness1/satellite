# 075 -- A map's .insert/.remove_at/.truncate refusal suggests age.keys.insert(...), a line that is itself refused (S110 "has no name to change")

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A map's .insert/.remove_at/.truncate refusal suggests age.keys.insert(...), a line that is itself refused (S110 'has no name to change').

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.string, satellite.variable.number> age = {"zoe": 30}
    age.keys.insert(1, "al")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: To see the suggestion, run p6a.satl (the same program with age.insert(1, "al")). Both are in  (p6a-p6f).

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(run): in age, age.insert changes a container, and this one has no name to
change

directory: p6b.satl:6
syntax: age.keys.insert(1, "al")
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

A suggestion that runs, e.g. keep the keys in a list first (satellite.container.list ks = age.keys, then ks.insert(...)), or no suggestion. The refusal should also name age.keys.insert, which is what was written.

The suggested line is the one satl refuses. The satellite.container.list help says .insert/.remove_at/.truncate change the list itself and need a name.

## Variants

p6a age.insert(1,"al") -> "Take its keys first: age.keys.insert(...)" (27). p6c age.remove_at(1) -> "Take its keys first: age.keys.remove_at(...)" (27). p6d age.index_of("zoe") -> "age.keys.index_of(...)", a suggestion that is valid because index_of changes nothing. p6e age.append(3) -> "write age[key] = value" (fine). p6f: ks = age.keys; ks.insert(1, "al") works and prints {"al", "zoe"}.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/container_calls.cpp:370-377 (the suggestion for every a_position_method) vs :417-424 (a changing method with home == nullptr refused)

The index branch builds "Take its keys first: <name>.keys.<method>(...)" for every position method, including the ones that change a container (insert, remove_at, truncate). For those, .keys is an unnamed answer, and the later check at :421 refuses it because home == nullptr. The `what` in that second refusal is built from the chain's head name, so it says age.insert.

## Notes

- Why the skeptic kept it: I reproduced it. age.insert(1, "al") on a map is refused with "Take its keys first: age.keys.insert(...)". Running that suggested line is refused with S110 "age.insert changes a container, and this one has no name to change", because .keys answers an unnamed list and .insert/.remove_at/.truncate need a name. That refusal also names age.insert, not what was written. The suggestion is right for .index_of and .search, which do not change anything, but it is wrong for insert, remove_at and truncate. What does work is keeping the keys in a name first (p6f prints {"al", "zoe"}). None of the known lists mention it.
- Nearest known entry: none
- Merged: Single finding.
