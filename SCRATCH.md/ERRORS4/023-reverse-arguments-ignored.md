# 023 -- s.reverse("x"), "abc".reverse(1, 2, 3) and n.reverse(99) are accepted silently with the argument ignored, while s.upper(1) is refused before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** strings  
**Kind:** invalid-accepted-silently  
**Severity:** medium

## What happens

s.reverse("x"), "abc".reverse(1, 2, 3) and n.reverse(99) are accepted silently with the argument ignored, while s.upper(1) is refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string s = "abc"
    satellite.console.display(s.reverse("x"))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
before
cba

exit 0
```

## What it should do

Refused before anything runs, the way s.upper(1) is: 's.reverse() takes nothing in its brackets' (S110, exit 13).

The help writes the method as '.reverse()', taking nothing, and says it 'turns a string round by character'. The equivalent no-argument methods .upper()/.lower() refuse an argument in the checker. A program with a wrong argument should not run with that argument silently dropped.

## Variants

ar1 s.reverse("x") gives cba. ar2 "abc".reverse(1, 2, 3) gives cba. ar3 n.reverse(99) with n = 123 gives 321. All exit 0. By contrast, s.upper(1) and s.lower("x", "y") are refused by satl(check), exit 13. (The observed output shows 'before' because ar1 on disk has two extra lines, a number declaration and display("before"), before the reverse line.)

## Where it comes from

satellite/bytecode/program_check.cpp:601 ('if (method == token::reverse_token) return success;' before any count check). satellite/bytecode/expression.cpp:452-466 calls reverse_of without looking at the arguments.

The checker passes .reverse on every receiver without counting its arguments, and the walker's non-container reverse path never checks that the brackets are empty.

## Notes

- Why the skeptic kept it: Reproduced: s.reverse("x") prints cba, "abc".reverse(1, 2, 3) prints cba and n.reverse(99) on 123 prints 321, all exit 0, with the argument silently thrown away. s.upper(1) and s.lower("x", "y") are refused by the checker ('s.upper() takes nothing in its brackets'). The verifier of lists#5 saw this string and number case and wrote it off as 'A related case, separate from this finding'. lists#5 itself is about list and index receivers, where a wrong .reverse count is at least refused at run time. Here nothing refuses it at all. It is in no known list.
- Nearest known entry: The verifier's note inside lists#5's variants mentions it as 'a related case, separate from this finding'. It was not confirmed there.
- Merged: Kept separate from number-string-conversion#4. Both get past the checker's early "reverse -> success" line, but here the walker also ignores the brackets, and the fix is an argument-count check in both places.
