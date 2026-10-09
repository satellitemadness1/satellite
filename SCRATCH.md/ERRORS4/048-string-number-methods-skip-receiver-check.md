# 048 -- find/add/to_string/to_number/to_binary/to_hexadecimal on a declared name pass the checker whatever the receiver type and argument count (p.find on a percentage, a.add on a list, s.add() on a string), and are refused only after earlier lines ran

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** floats fractions percent, lists, strings  
**Kind:** inconsistency  
**Severity:** low

## What happens

p.find("x") on a declared percentage (and .find on a number, bool or binary name, and p.number) passes the checker and is refused only after earlier lines ran, while p.size and n.upper() are refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.percent p = 50%
    satellite.console.display(p.find("x"))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at f5.satl. Variants in the same folder: f5b (p.size, checked), f5c (p.add(1)), f5d (number n.find), g1 (bool b.find), g2 (binary b.find), g3 (bool b.add(1)), g4 (p.number), g5 (n.upper(), checked).

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): find was written on a percentage and given a string, and there is no
scenario for that

directory: f5.satl:7
syntax: satellite.console.display(p.find("x"))
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

## What it should do

The checker should refuse p.find before anything runs, as it refuses p.size, n.upper() and f.find on a float. "before" should not print.

.find is a string's method (string help: ".find(x) answers where x first begins"). The checker already judges methods on a declared name by its type, and program_check.cpp:557-561 notes the 2026-09-18 review ruling that n.append("x") on a number must not pass the check and fail after earlier lines. check.sh and tests/float_method_check.satl pin that a method a float does not have is refused before anything runs. The percent help says ".number ... [is] refused", yet p.number also reaches the run.

## Variants

These fail late ("before" prints, then a run-time S301 with exit 27): n.find("x") on a number, b.find("x") on a bool, b.find("1") on a binary, b.add(1) on a bool, p.number on a percentage, and p.add(1). These are checked before the run: p.size (S210, exit 14), n.upper() (S301, exit 27), and f.find on a float (per the hunter).

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:582-584 and :751-752

method_on_a_name sets of_a_string_or_number for find, add, to_string, to_number, to_binary and to_hexadecimal. For any declared type that is not a container, not one of the four (float, hex, color, fraction) and not a file, `if (of_a_string_or_number) return success;` at :752 lets the call through whatever the receiver's type. The walker then refuses it at run time.

## Notes

- Why the skeptic kept it: I reproduced it, and it is wider than reported. p.find("x") on a declared percentage prints "before" and is then refused at run time. p.size and n.upper() on declared names are refused by the checker before anything runs. .find passes the checker on every declared scalar type the checker does not route to a type-specific check: number, bool, binary and percentage all give "before" followed by a run-time S301. The same holds for p.number, which the percent help itself says is refused. The cause is the of_a_string_or_number pass-through for non-container, non-file types at program_check.cpp:752. The already-confirmed lists#5 is the same pass-through in the container branch (:657), which covers only declared lists and indexes. The already-confirmed number-string-conversion#3 is float .bin/.hex. This scalar branch is not in any known list.
- Nearest known entry: none. Related but distinct: the already-confirmed lists#5 (the same pass-through inside the container branch, for declared lists and indexes) and number-string-conversion#3 (float .bin/.hex).
- Merged: One cause: the early "return success" for of_a_string_or_number in method_on_a_name (program_check.cpp:~647 in the container branch, and :751-752 for other types). lists#5 also lists smaller pass-throughs in the same function (.keys/.values on a list, .reverse(1), later chain segments), but its main case is this one.

### Also seen as: Wrong argument counts on a string's .find/.add/.number/.hex/.bin/.string are refused only after earlier lines ran (s.add() as '+ was given a string and nothing'), while .upper(1)/.lower(x, y)/.foreground() are refused before anything runs

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string s = "abc"
    satellite.console.display("before")
    satellite.console.display(s.add())
    satellite.return(satellite)
}
```

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): + was given a string and nothing, and there is no scenario for that
pair

directory: .../verify2/strings-2-0/ar7.satl:8
syntax: satellite.console.display(s.add())
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.

--------------------------------------------------------------------------------

exit 27
```

### Also seen as: On a declared list or index, .add/.find/.to_string/.to_number, .keys/.values (on a list) and a wrong argument count to .reverse or a later chain method pass the checker and are refused only after earlier lines ran, while .push/.pop/.length and .sum(1)/.sort(1)/.append() are refused before anything runs

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list a = {1, 2, 3}
    satellite.console.display("before")
    a.add(4)
    satellite.return(satellite)
}
```

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S301: TYPES_DO_NOT_MEET
satl(run): in a, a.add -- a container has no add (a list and an index have
.append, .size, .empty, .first, .last, .contains, .index_of, .search, .insert,
.remove, .remove_at, .remove_first, .remove_last, .clear, .truncate, .reserve,
.sum, .max, .min, .join, .keys, .values, .sort().by_name(), .sort().by_value()
and .reverse())

directory: .../verify/lists-5/add.satl:7
syntax: a.add(4)
        /\

this operator has no scenario for the two kinds it was given. Nothing was
guessed at, because a guess here is an answer that is wrong and does not say so.
machine code 27 types_do_not_meet -- satl exits with this.
--------------------------------------------------------------------------------

For comparison, push.satl (same program with a.push(4)) prints no "before":
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, a is satellite.container.list, and push is not
one of its methods -- only an object of a satellite.spacesuit has capsules to
call

exit 27
```
