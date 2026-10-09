# 029 -- List .max, .min and .sort().by_value() put bools in order and succeed, while < and > refuse two bools because "only == and != order those"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** invalid-accepted-silently  
**Severity:** low

## What happens

List .max/.min/.sort().by_value() (and a map's .sort().by_value() on bool keys) put bools in order and succeed, but < and > refuse two bools because "only == and != order those".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list t = {satellite.bool.false, satellite.bool.true}
    satellite.console.display(t.max)
    satellite.console.display(satellite.bool.true > satellite.bool.false)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at min.satl. The hunter's original program was re-created at prog.satl and gives exactly the reported output.

## What satl does

```
true
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): > was given two bools, and only == and != order those

directory: min.satl:7
syntax: satellite.console.display(satellite.bool.true > satellite.bool.false)
                                                      /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

.max, .min and .sort().by_value() on a list holding two or more bools should be refused with S301, the same way the operators refuse them and the same way the list methods already refuse two lists ("max finds the largest by what each item is worth, and item 1 and item 2 have no order between them ..."). Right now t.max answers true with no error.

The interpreter's stated rule is that two bools have no order a program may ask for. satellite/satellite_object/bool_and_bool_compare.hpp says: "ONLY == AND != MEAN ANYTHING on two bools, and the caller is what refuses an ordering. This answers the order; it does not decide which spellings may ask." satellite/bytecode/expression.cpp:150-153 says: "AN ORDERING ON TWO BOOLS IS STILL REFUSED ... `a < b` on two bools means nothing in this language; `a == b` does." The list help (satellite.help/satellite.container.list/help_text.txt:30-33) defines .max/.min as "the largest and smallest by what each is worth" and .by_value() as "puts the smallest first". container_calls.cpp:808-809 says .max IS .sort().by_value().last. So all of these ask for the same ordering that > refuses. The list methods are also meant to refuse a pair with no order: all_comparable's comment says a comparison that cannot be made is settled first and refused. Two spellings of the same question disagree: one answers and the other refuses.

## Variants

ALSO FAIL (bools get ordered, exit 0):
- bools held in variables a=true, b=false, t={a,b,a,b}: t.max -> true, t.min -> false, t.sort().by_value() -> {false, false, true, true}, t.sort().by_value().reverse() -> {true, true, false, false}.
- a list of comparison results, {1 == 2, 3 == 3}.max -> true.
- a map with bool keys, {true: "yes", false: "no"}.sort().by_value() -> {false, true}.
- the hunter's full program (prog.satl): prints true / false / {false, true}, then refuses the >.
BEHAVE CORRECTLY, for contrast (refused with S301, exit 27):
- a < b on two bool variables: "< was given two bools, and only == and != order those".
- {{1},{2}}.max: "max finds the largest by what each item is worth, and item 1 and item 2 have no order between them: a comparison was given a list and a list".
- {true, 1}.max: refused, "a bool and a number".
FINE AND NOT A DEFECT: t.sort().by_name() on bools gives {false, false, true, true}. That orders by text, and bools have text.
Side note on the refusal wording: "only == and != order those" is loose, because == and != compare and do not order. That is cosmetic and is not the finding.

## Where it comes from

satellite/bytecode/container_calls.cpp:91-120 (all_comparable, called at :744 for by_value and :825 for max/min)

all_comparable() checks only whether satelliteObject::compare() succeeds for each pair of kinds. For bool against bool, compare() succeeds (satellite/satellite_object/satellite_object.cpp:701-702 calls bool_and_bool_compare, which gives false < true). The design puts the bool-ordering refusal in the caller, and only the operator path applies it (satellite/bytecode/expression.cpp:155-160: `if (an_ordering(op) && left.is_bool() && right.is_bool()) refuse(...)`). all_comparable has no matching check for two bool items, so .max, .min and .sort().by_value() on lists, and .sort().by_value() on a map's keys, order bools without complaint.

## Notes

- Why the skeptic kept it: Reproduced on BUILD 0099. List .max, .min and .sort().by_value() put bools in order (false before true) and answer with exit 0, while the operators <, > (on literals or on variables) refuse two bools with S301 and say only == and != apply. The interpreter's own source says ordering on two bools is meant to be refused: bool_and_bool_compare.hpp says "ONLY == AND != MEAN ANYTHING on two bools, and the caller is what refuses an ordering", and expression.cpp:150-153 says "`a < b` on two bools means nothing in this language". The list-method caller (all_comparable) never applies that refusal. So this is not by-design. It is a caller that skips the rule, and the fix is to refuse in the list methods, not to give bools an order. None of ERROR.md, errors.md, SCRATCH.md/ERRORS2.md, NEW_ERROR_LIST.md or CONTAINERS.md mentions bool ordering, .max/.min/by_value, or all_comparable. DESIGN.md and the help topics give no order for bools.
- Nearest known entry: none
- Merged: Single finding.
