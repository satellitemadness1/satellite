# 015 -- A container holding a see-through colour shows its ", 50" with no delimiter, so the list reads as if it had an extra item

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool  
**Kind:** wrong-answer  
**Severity:** low

## What happens

A container holding a see-through colour displays its ", 50" undelimited, so the list reads as having an extra item.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.color b = x000000, 50
    satellite.container.list xs = {b, "a, b", 3}
    satellite.console.display(xs)
    satellite.console.display(xs.size)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
{x000000, 50}
{x000000, 50, "a, b", 3}
3

exit 0
```

## What it should do

A display whose items can be counted, e.g. {(x000000, 50), "a, b", 3}, the way a string item is quoted so that its comma is not read as a separator.

satellite.container.list help shows containers displayed as their literal ({"zoe", "al", "bo"}, {{1, 20}, {3}}), with strings quoted so that item boundaries are exact. A 3-item list printed as 4 comma-separated items is a false picture of its contents.

## Variants

Hunter's program, list<color> {a, b}: {x112233, x000000, 50} with size 2. A list<color> with just b: {x000000, 50}, which looks like two items. The literal form {x112233, (x000000, 50)} is refused ('opened with { and never closed with }'), so there is no spelling to read it back. Side note, not part of this finding: list<color> cs = {x112233, x000000, 50} and m["j"] = x112233 on a map<string, color> are refused 'it holds a hex' (exit 27), so a colour literal cannot be put straight into a colour slot.

## Where it comes from

satellite/satellite_variable_color/satellite_color.hpp:77 (to-text: "x" + digits() + ", " + transparency), used unchanged by the container display

A container's display joins its items' plain to-text forms and quotes only strings. A colour's to-text puts its transparency after a bare comma, which is the same character as the item separator.

## Notes

- Why the skeptic kept it: I reproduced it: a list<color> holding x112233 and a see-through x000000, 50 displays as {x112233, x000000, 50} while .size is 2. In an untyped list, {b, "a, b", 3} with b = x000000, 50 displays as {x000000, 50, "a, b", 3} with .size 3. So container display quotes a string so that its commas cannot be read as item separators, but it does not set off a colour's own comma. The printed list reads as four items and cannot be told apart from a list that really holds x000000 and 50. There is also no way to type the item back: {x112233, (x000000, 50)} is refused. The colour help only rules on a lone colour ('shows as x000000, 50, the way you write it'). The list and map pages show containers displayed in their literal form ({"zoe", "al", "bo"}, {{1, 20}, {3}}), and that form breaks here. Not in any known list or among the confirmed entries.
- Nearest known entry: none
- Merged: Single finding.
