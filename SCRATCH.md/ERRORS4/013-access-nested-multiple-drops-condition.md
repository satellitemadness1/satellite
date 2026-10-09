# 013 -- satellite.access drops the inner 'when' condition for a multiple nested under another multiple, so x[n][m] is shown as working when x[n] is a number

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** wrong-answer  
**Severity:** low

## What happens

satellite.access drops the inner 'when' condition for a multiple nested under another multiple, so x[n][m] is shown as working when x[n] is a number.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.multiple<satellite.variable.string, satellite.container.list<satellite.container.multiple<satellite.variable.number, satellite.container.list<satellite.variable.number>>>> x = {1, {2, 3}}
    satellite.access(x)
    satellite.console.display(x[1][1])
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at p10a.satl (p10b swapped order, p10c single level)

## What satl does

```
x is a multiple (string or list of multiples (number or list of numbers)), holding a list of multiples (number or list of numbers), 2 items
  value           {1, {2, 3}}
  x[n]            a multiple (number or list of numbers), n counting from 1 (1 to 2) -- when x holds a list of multiples
  x[n][m]         a number, m counting from 1 -- when x holds a list of multiples
  x.append({1})   adds a list of numbers -- when x holds a list of multiples
  x[n].append(1)  adds a number -- when x holds a list of multiples
  x.size          how many multiples it holds -- when x holds a list of multiples
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): x[...] is a number, and [ ] reads a line of a file, an item of a
list, or a key of an index

directory: p10a.satl:7
syntax: satellite.console.display(x[1][1])
                                      /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

x[n][m] and x[n].append(1) marked "when x holds a list of multiples and x[n] holds a list of numbers", as the single-level case marks lm[n][m] "when lm[n] holds a list of strings".

satellite.access help: "A multiple shows the lines for each type it may hold that can be reached into, each marked 'when it holds ...'".

## Variants

p10b with {{2, 3}, 1} prints the same access text, and x[1][1] prints 2. p10c single-level list<multiple<number, list<string>>> is marked correctly. The hunter's map<string, multiple<number, map<string, multiple<number, list<string>>>>> shows the same loss at the second multiple.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/access_calls.cpp:105 (reach_into, multiple branch)

For each arm of a multiple, reach_into passes `when.empty() ? "when " + path + " holds ..." : when`. Once an outer multiple has set `when`, an inner multiple's arm condition is thrown away instead of being appended (e.g. "... and x[n] holds a list of numbers").

## Notes

- Why the skeptic kept it: I reproduced it. In x = multiple<string, list<multiple<number, list<number>>>> = {1, {2, 3}}, satellite.access(x) marks x[n][m] only "when x holds a list of multiples". That condition holds here, and n = 1 is inside the shown range 1 to 2, but x[1][1] is refused with S301 because x[1] is the number 1. With the items swapped ({{2, 3}, 1}) the same line prints 2. A single-level list<multiple<number, list<string>>> is marked correctly ("lm[n][m] ... -- when lm[n] holds a list of strings"). So the inner multiple's condition is dropped whenever an outer one is already set. The access help promises that a multiple's lines are "each marked 'when it holds ...'". The rest of the claim is also right: x[n].append(1) is shown under only the outer condition. It is in none of the known lists, and CONTAINERS.md's "still open" items are about unchecked writes, not access text.
- Nearest known entry: none
- Merged: Single finding.
