# 059 -- a[1] += 2 (a compound assignment to an indexed item) gets an unrelated S110 "not a method call" sentence instead of S210 "not built yet -- write a[1] = a[1] + ..."

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

a[1] += 2 (and other compound assignments to an indexed item) is refused with an unrelated S110 "not a method call" sentence instead of S210 "not built yet -- write a[1] = a[1] + ...".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list a = {1, 2, 3}
    a[1] += 2
    satellite.console.display(a)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl. The comparison program is n.satl (same program with `satellite.variable.number n = 1` / `n += 2`). The variants are v_*.satl in the same folder.

## What satl does

```
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, a is followed by something that is not a method
call -- a call's answer cannot be given a value, a bracket must close on its
line, and one statement is one line

directory: .../verify/lists-10/prog.satl:6
syntax: a[1] += 2
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

For comparison, n.satl (`n += 2`) gives:
S210: NOT_BUILT_YET
satl(check): in satellite.main, n += ... is not built yet -- write n = n + ...
syntax: n += 2
machine code 14 not_built_yet -- satl exits with this.

exit 13
```

## What it should do

The same refusal a name gets: S210 NOT_BUILT_YET, "a[1] += ... is not built yet -- write a[1] = a[1] + ...", with exit 14. The written-out form a[1] = a[1] + 2 is accepted and runs (exit 0).

These are equivalent forms that disagree. `n += 2`, `s += "y"`, `a += {4}` and `args.n += 1` are all refused by name with S210 and a rewrite, and check.sh:4625 tests args.n += 1. /home/madness/code/cxx/satellite/satellite.help/satellite.variable.number/help_text.txt:30 says "n += 1 (S210, 'write n = n + ...')". ERROR.md, in the "operator with no meaning" FIXED entry, says "n += 1 ... the checker now refuses it by name before anything runs (not_built_yet, 14)". The sentence given to the item form states three reasons, and none of them is true of `a[1] += 2`: there is no call, the bracket closes on its line, and it is one statement on one line. So the refusal names the wrong thing.

## Variants

Get the same unrelated S110 "not a method call" sentence, exit 13:
- a[1] -= 2
- a[1] *= 2
- a[1]++
- g[1][1] += 1 on a list<list<number>>
- m["k"] += 1 on a map
- a[2] += 5 on a list<number> inside a non-main capsule (says "in bump")
- args["n"] += 1 on arguments. This also disagrees with args.n += 1, which gets S210 "args.n += ... is not built yet -- write args.n = args.n + ...".

Refused correctly with S210 and a rewrite, exit 14: n += 2, n -= 1, n *= 3, n /= 3, s += "y", a += {4} (whole list), args.n += 1.

n++ and n-- get S110 "n is followed by something that is not = , and there is no statement of that shape". That is accurate, not misleading.

a[1] = a[1] + 2 is accepted and runs (exit 0).

In every case the refusal comes from the checker before anything runs ("before" is not printed), so there is no ordering problem.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:808-814 (a_call_to_its_end), with the name path at program_check.cpp:381-389 (after_the_name). Line numbers are in the tree as it stands now, which may be newer than BUILD 0099.

Two functions check what follows a name, and only one knows about compound assignment. after_the_name, used when a declared name is followed directly by an operator, maps plus_assign/minus_assign/times_assign/divide_assign/modulus_assign tokens to not_built_yet with the "write n = n + ..." sentence. A statement that goes on with `[`, like `a[1]`, or with `.method`, is judged by a_call_to_its_end instead. After the run of [..]/.method it accepts only the line's end, or `=` when the run ended on an index (`if (code == token::assign_token && ended_on_an_index) return success;`). Any other token, including plus_assign_token after `]`, falls through to the one generic sentence: "is followed by something that is not a method call -- a call's answer cannot be given a value, a bracket must close on its line, and one statement is one line". So the compound-assign tokens after an indexed item never reach the S210 wording.

## Notes

- Why the skeptic kept it: Reproduced on the BUILD 0099 copy. Every compound assignment written on an indexed item (a[1] += 2, -=, *=, a[1]++, g[1][1] += 1, m["k"] += 1, args["n"] += 1) gets the generic S110 sentence. That sentence gives three reasons: a call's answer cannot be given a value, a bracket must close on its line, and one statement is one line. None of them applies to these lines. The same operator on a name (n += 2, s += "y", a += {4}) or on a dotted name (args.n += 1, which check.sh:4625 tests) gets S210 NOT_BUILT_YET with the right rewrite. The help page satellite.variable.number/help_text.txt:30 says += is refused with S210 and "write n = n + ...". ERROR.md records that += was made a by-name refusal. The item form was missed in that fix. No entry in ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md covers it. ERRORS2 #9 and NEW_ERROR_LIST B5 are about 003 spellings such as machine.cores, which is a different problem. DESIGN.md does not rule on +=. MILESTONES.md:839 lists += among the QUESTION rows, which says +=/-= are not decided yet. It does not say the item form should get a different refusal. The refusal comes before anything runs, and the exit code is 13 instead of 14, so only the message and the machine code are wrong. Low severity.
- Nearest known entry: none
- Merged: Single finding. It shares a_call_to_its_end with lists#11 and spacesuits-and-library#11, but each needs its own fix.
