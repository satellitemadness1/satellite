# 079 -- satellite.access position letters collide with the name shown: a list named m is shown as m[n][m], and a list named n as n[n]

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

satellite.access position letters collide with the name shown: a list named m is shown as m[n][m], a list named n as n[n].

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list<satellite.container.list<satellite.variable.number>> m = {{1, 2}, {3}}
    satellite.access(m)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
m is a list of lists of numbers, 2 items
  value           {{1, 2}, {3}}
  m[n]            a list of numbers, n counting from 1 (1 to 2)
  m[n][m]         a number, m counting from 1
  m.append({1})   adds a list of numbers
  m[n].append(1)  adds a number
  m.size          how many lists of numbers it holds

exit 0
```

## What it should do

Placeholder letters that are not the name itself (e.g. m[n][p]), so the reach line can be read and filled in as code.

satellite.access help: it shows 'the way to reach one item', and lines are 'written as a program writes it'. Read as code, m[n][m] indexes m by m (a list), which is refused; substituting 'm counting from 1' replaces the name too.

## Variants

n = {{{1}}} (list<list<list<number>>>) shows 'n[n]  ... n counting from 1', 'n[n][m][p]', 'n[n].append({1})', 'n[n][m].append(1)'. A list named p shows p[n][m] correctly (no clash). Per the hunter, a spacesuit capsule parameter named n clashes the same way: rooms["key"][n].call_put(key, n).

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/access_words.cpp:157-161 (position_letter), used at /home/madness/code/cxx/satellite/satellite/bytecode/access_calls.cpp:112

position_letter(level) returns a fixed n, m, p, q, ... by depth and reach_into uses it without skipping letters equal to the name being shown (or to capsule parameter names).

## Notes

- Why the skeptic kept it: Reproduced. satellite.access on a list of lists named m shows 'm[n][m]  a number, m counting from 1', and on a 3-deep list named n shows 'n[n]', 'n[n][m][p]', 'n[n].append({1})'. The position letters are a fixed n, m, p, q... and are never checked against the name being shown, so a person substituting 'm counting from 1' gets 2[n][2]-style nonsense and read literally m[n][m] indexes m by itself. The help says fill lines are 'written as a program writes it'; CONTAINERS.md records the n/m/p letters as the implementer's choice ('My choices, his to overrule'), not an author ruling, and says nothing about clashing with the name. Not in any known list. Low severity: the help does say n, m, p stand for positions.
- Nearest known entry: none
- Merged: Single finding.
