# 012 -- map.sort().by_value(), .by_name() and .reverse() silently work on the keys and answer a list (ranking a word count by value gives an alphabetical list), while .max/.min/.sum/.join on a map ask which half is meant

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting, realistic programs  
**Kind:** wrong-answer  
**Severity:** low

## What happens

map.sort().by_value() and map.reverse() silently work on the KEYS and answer a list, while .max/.min/.sum/.join on a map ask which half.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.string, satellite.variable.number> scores = {"zoe": 1, "al": 30, "bo": 2}
    satellite.console.display(scores.sort().by_value())
    satellite.console.display(scores.sort().by_name())
    satellite.console.display(scores.reverse())
    satellite.console.display(scores.values.sort().by_value())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
{"al", "bo", "zoe"}
{"al", "bo", "zoe"}
{"bo", "al", "zoe"}
{1, 2, 30}

exit 0
```

## What it should do

Either the same before-run 'say which: scores.values.sort().by_value() or scores.keys.sort().by_value()' refusal that .max/.sum get, or a documented ordering of the map's entries -- not a silent list of keys.

container_calls.cpp refuses .sum/.max/.min/.join on an index because each could mean the keys or the values; list help: 'On an index, .sum .max .min and .join ask you to say which half'. by_value ('puts the smallest first') on a score map reads as 'order by score' but orders the names. The map help lists no .sort or .reverse for a map.

## Variants

scores.sort().by_name() gives the same key list; scores.reverse() gives {"bo", "al", "zoe"} (keys reversed, a list, not a map); scores.values.sort().by_value() gives {1, 2, 30} correctly. In the same program, display(scores.max) is refused by satl(check): 'scores.max -- an index holds keys and values, so say which: scores.values.max or scores.keys.max'.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/container_calls.cpp:42-54 (items_of), :134-138 (asks_what_it_holds), :716-763 (sort/by_name/by_value/reverse)

items_of() on an index quietly returns only the keys (entry.first), and by_name/by_value/reverse build their answer from items_of. asks_what_it_holds, which drives index_refuses, lists only sum/max/min/join, so .sort().by_value(), .by_name() and .reverse() on a map are never asked which half.

## Notes

- Why the skeptic kept it: Reproduced. scores.sort().by_value() on map {"zoe": 1, "al": 30, "bo": 2} answers {"al", "bo", "zoe"} -- the keys, in name order, not the entries by score -- and scores.reverse() answers the key list reversed, while scores.max/.sum/.min/.join are refused before the run with 'an index holds keys and values, so say which'. by_value on a map has the same ambiguity those refusals exist for, yet it silently picks the keys and turns the map into a list. The map help documents no .sort or .reverse on a map. Not in any known list; lists#6 only mentions a map's .sort().by_value() on bool keys as an example of bools being ordered, a different defect.
- Nearest known entry: none
- Merged: Same cause: items_of() gives an index's keys, and asks_what_it_holds lists only sum/max/min/join (container_calls.cpp:42-54 and :134-138).

### Also seen as: On a map, .sort().by_value() / .by_name() quietly order the KEYS, so ranking a word count by value gives an alphabetical list, while .max/.min/.sum/.join refuse and ask 'say which: .values or .keys' (and .sort().by_value().last answers what .max refuses)

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.string, satellite.variable.number> freq = {"to": 4, "be": 2, "or": 1, "not": 3}
    satellite.console.display(freq.sort().by_value())
    satellite.console.display(freq.sort().by_name())
    satellite.console.display(freq.values.sort().by_value())
    satellite.return(satellite)
}
```

```
{"be", "not", "or", "to"}
{"be", "not", "or", "to"}
{1, 2, 3, 4}

exit 0
```
