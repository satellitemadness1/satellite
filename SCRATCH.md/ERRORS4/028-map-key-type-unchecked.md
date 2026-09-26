# 028 -- A map declared with a key type that can never be a key (list, map, float) is accepted, and satellite.access prints a fill line (m[key] = 1) that is always refused

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** invalid-accepted-silently  
**Severity:** low

## What happens

A map declared with a key type that can never be a key (list, map, float...) is accepted, and satellite.access prints a fill line (m[key] = 1) that is always refused.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.container.list<satellite.variable.number>, satellite.variable.number> m
    satellite.access(m)
    satellite.container.list<satellite.variable.number> key = {1}
    m[key] = 1
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at p4.satl. Variants p4a-p4e are in the same folder.

## What satl does

```
m is a map (list of numbers -> number), 0 keys
  value       {}
  m[key]      a number, key a list of numbers
  m[key] = 1  puts a number under key
  m.size      how many keys it holds
  m.keys      the keys of it, as a list
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): in m[...] = ..., the key is a list, and a key must be a number, a
string, a bool, a binary or a percentage -- something that cannot change after
it is filed under

directory: p4.satl:8
syntax: m[key] = 1
         /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

The declaration is refused before anything runs, because a list (or map, or float) can never be a key. At the least, satellite.access should not offer m[key] = 1 as the way to fill it.

satellite.container.map help: "A key must be a number, a string, a bool, a binary or a percentage, never a list or another map." satellite.access help: "the lines that fill a level show a value of the right shape, written as a program writes it."

## Variants

p4a: the declaration plus access alone runs to exit 0 with no warning. p4b: map<float, string> gets access line m[key] = "text", then m[1.5] = "a" is refused at run time with S301. p4c: "before" prints, then the write is refused, so the refusal comes after earlier lines ran. p4d/p4e: a literal with a list key or a float key is refused at run time.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/access_calls.cpp:127-150 (index branch of reach_into prints the fill line for any key shape); the key-kind rule is enforced only at run time in bytecode/expression.cpp:763/862/971/1964 and container_calls.cpp:448, with no check on the declared key type

The type parser accepts any TypeShape as a map's first parameter. The key-kind rule (number/string/bool/binary/percentage) is applied only to key VALUES at run time. access_calls.cpp's index branch writes `reached + " = " + example` with a placeholder key and never checks whether the declared key kind can be a key at all.

## Notes

- Why the skeptic kept it: I reproduced it. The declaration satellite.container.map<satellite.container.list<satellite.variable.number>, satellite.variable.number> m is accepted without a word; map<satellite.variable.float, string> is accepted too. satellite.access(m) then prints "m[key] = 1  puts a number under key". Every attempt to fill it is refused at run time with S301 "a key must be a number, a string, a bool, a binary or a percentage", and that happens after the earlier lines ran ("before" printed in p4c). So the declared key type can never hold a key. The map help says a key must be one of those kinds, "never a list or another map". The access help says "the lines that fill a level show a value of the right shape, written as a program writes it", and CONTAINERS.md says every line access prints is run as code by the sweep, but this printed line can never run. Nothing in ERROR.md, errors.md, ERRORS2, NEW_ERROR_LIST, CONTAINERS or DESIGN.md mentions key types at declaration.
- Nearest known entry: none
- Merged: Single finding.
