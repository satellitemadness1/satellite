# 010 -- List item matching (.contains, .index_of, .remove, list == list, index == index) never matches values of different kinds that == calls equal: a[3] == 3.0 is true, but a.contains(3.0) is false

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** wrong-answer  
**Severity:** medium

## What happens

List item matching (.contains, .index_of, .remove, list == list, index == index) ignores the language's ==: a number never matches an equal float, binary or hex (a[3] == 3.0 is true, but a.contains(3.0) is false).

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list a = {1, 2, 3}
    satellite.console.display(a[3] == 3.0)
    satellite.console.display(a.contains(3.0))
    satellite.console.display(a.index_of(3.0))
    satellite.console.display(a == {1, 2, 3.0})
    a.remove(3.0)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at min.satl. Variants: v1.satl, v2.satl, v3.satl in the same folder. The hunter's original program is prog.satl there.

## What satl does

```
true
false
0
false
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S420: TEXT_NOT_FOUND
satl(run): in a, a.remove: there is no such item in it -- a.contains(x) asks
before removing

directory: min.satl:10
syntax: a.remove(3.0)
        /\

the text looked for is not in this string.
machine code 15 text_not_found -- satl exits with this.

--------------------------------------------------------------------------------

exit 15
```

## What it should do

The list methods should agree with the language's own ==. That means: true, true, 3, true, and a.remove(3.0) removes the 3, so the program exits 0. The other consistent choice is for == to say 3 == 3.0 is false. What is wrong is that the two disagree.

The same program prints a[3] == 3.0 as true. satellite.help/satellite.container/help_text.txt says "== and != compare what two containers hold". satellite.help/satellite.variable.binary/help_text.txt says "b1010 == 10 is true". satellite.help/satellite.variable.hex/help_text.txt says "x1F == 31 is true". A person who writes the search by hand with == gets a different answer from the method: in v3.satl, a loop testing a[i] == 2.0 finds position 2, but a.index_of(2.0) answers 0. The .max/.min tie rule pinned by check.sh (tests/list_from_003.satl, "{1.0, 2, 1, 2.0}") already treats 1 and 1.0 as equal values.

## Variants

These fail (the method disagrees with ==):
- satellite.variable.float f = 3 (holds 3.0): a[3] == f is true, a.contains(f) is false.
- list<float> prices = {1.5, 2.0}: prices[2] == 2 is true, prices.contains(2) is false, prices.index_of(2) is 0.
- {1, 2, 3} != {1, 2, 3.0} is true.
- {b11}.contains(3) and {3}.contains(b11) are false, while b11 == 3 is true.
- {x1F}.contains(31) is false, while x1F == 31 is true.
- Index equality: m = {"a": 3} and n = {"a": 3.0} give m["a"] == n["a"] true but m == n false.
- A for-loop testing a[i] == 2.0 finds 2, while a.index_of(2.0) is 0.

These agree, so they are correct:
- {2.50}.contains(2.5) is true.
- b0011 == b11 is false and {b0011}.contains(b11) is false (width counts, as the binary help says).
- x00FF == xFF is false and {x00FF}.contains(xFF) is false.
- .search(3.0) on {3} is 0, which is correct because search matches text and "3" does not contain "3.0".

Side notes, not part of this finding:
- list<satellite.variable.float> w = {1, 2} is refused at run time with "w was declared satellite.container.list, and item 1 of it does not fit: it holds a number" (exit 27). Yet float f = 3 is accepted as 3.0.
- The .remove refusal's explanation line reads "the text looked for is not in this string" when the thing searched is a list.

## Where it comes from

satellite/satellite_object/satellite_object.cpp:277

bool operator==(const satelliteObject&, const satelliteObject&) at satellite_object.cpp:275 begins with `if (l.kind() != r.kind()) return false;` (line 277). Its comment says "Two values of different kinds are never the same value -- nothing is converted to find out". The list methods in satellite/bytecode/container_calls.cpp all use this operator: .remove at line 594 (`body.items[at] == arguments.front()`), .index_of at line 662, and .contains at line 707. The list and index arms of the same operator== compare their items and values with it too (satellite_object.cpp, list case and index case), so container ==/!= inherit the rule through expression.cpp:175. The language's scalar == takes a different path. It goes through satelliteObject::compare (expression.cpp:215), which does compare a number with a float, binary or hex by worth. So the scalar == and every item-matching method answer by two different rules.

## Notes

- Why the skeptic kept it: Reproduced on BUILD 0099 and not refuted. The language's == says a[3] == 3.0 is true, b11 == 3 is true and x1F == 31 is true. On the same values, .contains and .index_of say "not there", list == list says false, and .remove refuses with exit 15. A for-loop that searches with == finds 2.0 at position 2, while a.index_of(2.0) on the same list answers 0. None of the known lists mentions this: ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md and CONTAINERS.md "Still open" / "Found and not fixed" all have nothing on it. There is no by-design ruling either. DESIGN.md has no rule about equality between kinds. The source comment above operator== cites "DESIGN 1.1", but that section is "the fastest C++ path" and says nothing about equality; the comment's only example is 4 vs "4". check.sh has no test that uses these methods with mixed kinds. Its own tie rule for .max/.min (test list_from_003: {1.0, 2, 1, 2.0}) treats 1 and 1.0 as equal. The help pages support the hunter's expectation. satellite.container says "== and != compare what two containers hold". satellite.variable.binary says "b1010 == 10 is true". satellite.variable.hex says "x1F == 31 is true". So by the language's own == these lists hold the item.
- Nearest known entry: none
- Merged: Single finding. operator== at satellite_object.cpp:277 returns false whenever the kinds differ.
