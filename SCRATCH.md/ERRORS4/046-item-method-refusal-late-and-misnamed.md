# 046 -- A not-built method on a list item or on a method's answer (names[1].size, a.join(", ").size) is refused only at run time, and the refusal names the list (names.size), which is a built word

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

A not-built method on a list item or on a method's answer (names[1].size, a.join(", ").size, a[1].append) is refused only at run time, and the refusal names the list (names.size, a.append), a word that is built.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list<satellite.variable.string> names = {"zoe", "al"}
    satellite.console.display("before")
    satellite.console.display(names[1].size)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at p1.satl

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(run): names.size is not built for a string yet -- so far a file and a
container have it

directory: .../verify/lists-4/p1.satl:7
syntax: satellite.console.display(names[1].size)
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

satl(check) should refuse it before "before" prints, and the refusal should name what was written, `names[1].size` (item 1 of names, a string), for example: "names[1].size is not built for a string yet". It should not name `names.size`, which is built.

/home/madness/code/cxx/satellite/satellite.help/satellite.variable.string/help_text.txt lists .size .empty .contains(x) and the others as "not built yet in satellite 004" and says: "Each is refused before anything runs (S210)". /home/madness/code/cxx/satellite/satellite.help/satellite.container.list/help_text.txt lists `.size` as a list's own method, and `names.size` printed 2 on the same list (p4.satl). So a message that says "names.size is not built" is false. The equivalent forms are refused by the checker, before anything runs, under the name written. `satellite.variable.string s = names[1]` then `display(s.size)` gives "satl(check): in satellite.main, s.size is not built for satellite.variable.string yet" (p3.satl). `display("zoe".size)` gives "satl(check): ... that string.size is not built for a string yet" (v2.satl). The list's declaration, list<satellite.variable.string>, already gives the checker the item's type.

## Variants

These fail the same way: run time, after "before", and naming the list's head:
- untyped `a = {"zoe","al"}`, `a[1].size`: "satl(run): a.size is not built for a string yet"
- `a.join(", ").size`: "satl(run): a.size is not built for a string yet"
- `names.first.size`: "satl(run): names.size ..."
- `names[1].contains("z")`: "names.contains is not built for a string yet"
- `names[1].empty`: "names.empty ..."
- `a = {1, {2}}`, `a[1].append(3)`: "satl(run): in a, a.append is not built for a number yet"
- list<number> `nums[1].append(3)`: "in nums, nums.append is not built for a number yet"
- `satellite.variable.number k = names[1].size`: "in k = ..., names.size is not built"
- inside a capsule taking list<string> names, `names[1].size`: the same "names.size" at run time.

These behave correctly:
- `s.size` on a string variable: refused by check.
- `"zoe".size` literal: refused by check, "that string.size".
- `s = names[1]` then `s.size`: refused by check.
- `n.append(3)` / `n.size` on a number variable: refused by check, "not built for satellite.variable.number". This shows the "not built for a number" wording is satl's usual phrasing, so the hunter's sub-complaint about it does not hold.
- `names[1].upper()` gives ZOE and `names[2].reverse()` gives la; the built string methods on an item work.
- On a list of lists, `g[1].size` gives 2 and `g[2].append(4)` gives {{1, 2}, {3, 4}}.
- `names[1].frobnicate` is refused by check, S201.

Probes are saved in  (p1-p4, v1-v10, w1-w4, c1).

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/expression.cpp:626 (wrong name); /home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:1289-1309 and 560-563 (not checked before the run)

Wrong name: expression.cpp:626 builds the run-time refusal as `name + "." + spelling`. `name` is the chain's head variable, `names`, not the item or answer the chain has reached (`*live`), so `names[1].size` and `names.first.size` are both reported as `names.size`. The container_calls.cpp:883 fallback builds its `what` the same way, which gives "in a, a.append".

Run time instead of check: the checker's name walk (program_check.cpp ~1289-1309) judges an item's member after `[...]` only when the list holds spacesuit objects (`where.lists`). Otherwise it calls `method_on_a_name`, which judges only the method straight after the name (program_check.cpp:567 returns success unless the next code is a method token). Its comment at 560-563 says "Only the first method is judged: what a method answers is a run-time fact, so a chain's later segments are left to the walker". So `names[1].size`, `a.join(...).size` and `a.first.size` all go to the walker, even when list<string> gives the item's type.

## Notes

- Why the skeptic kept it: I reproduced it on the BUILD 0099 runner copy. `names[1].size` on a list<string> prints "before" and then gives a satl(run) S210 that names `names.size`. That word is built: the same program's `names.size` printed 2. Two equivalent forms disagree with it. `satellite.variable.string s = names[1]` followed by `display(s.size)`, and `"zoe".size`, are both refused by satl(check) before anything runs, and each names what was written. The string help says of .size/.empty/.contains and the others: "Each is refused before anything runs (S210)". None of the known lists has this. ERRORS2 #10 (`n.power` named `power_of`) is a different mechanism: a registry spelling, not the head of a chain standing in for the item. DESIGN.md has no ruling on it. One sub-claim of the hunter's does not hold. Calling `.append` on a number "not built for a number yet" is satl's usual wording: a plain `satellite.variable.number n` with `n.append(3)` gets "n.append is not built for satellite.variable.number yet" from the checker. So that phrase is not an error. The wrong name `a.append` is.
- Nearest known entry: none (closest is ERRORS2.md #10, "n.power(2) is refused under the token's name" (power_of). That is a different cause, the registry's spelling rather than the chain's head name standing in for the item, and it is not this case)
- Merged: Single finding.
