# 030 -- A statement that only computes a method's answer (a.sort().by_value(), a.reverse(), a.sum, n.add(1)) is accepted and does nothing, while a bare a or a + 1 is refused as doing nothing

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** invalid-accepted-silently  
**Severity:** low

## What happens

A statement that only computes a method's answer (a.sort().by_value(), a.reverse(), a.sum, n.add(1)) is accepted and silently does nothing, while 'a' alone and 'a + 1' are refused as doing nothing.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list a = {3, 1, 2}
    a.sort().by_value()
    satellite.console.display(a)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
{3, 1, 2}

exit 0
```

## What it should do

Refused before anything runs, as a bare name is: "a.sort().by_value() on its own line does nothing -- give its answer to a name with =, a = a.sort().by_value()".

The list help says that everything except append/insert/remove/truncate/reserve/clear "answers a new value and leaves the list alone", so the line does nothing. The checker already refuses the matching shapes 'a' ("on its own line does nothing") and 'a + 1'. Two equivalent forms that disagree count as an error.

## Variants

a.reverse(), a.sum, a.size, n.add(1) on a number and s.upper() on a string all run silently (exit 0, no effect). 'a' alone is refused with S110 "a on its own line does nothing -- give it a value with =" (exit 13). 'a + 1' is refused with S110 "a is followed by something that is not = , and there is no statement of that shape". a.sort() alone displays {3, 1, 2}, so a.sort().reverse() = {2, 1, 3} matches the help's ".sort() orders nothing on its own".

## Where it comes from

satellite/bytecode/program_check.cpp:815-817 (a_call_to_its_end returns success for any .method run ending at the line end); compare program_check.cpp:405 (bare name refused as doing nothing)

a_call_to_its_end accepts every chain of .method / (...) / [...] that reaches the line's end. It never asks whether the last method changes the name (append, insert, remove, truncate, reserve, clear) or only answers a value, so an answer that is thrown away is accepted.

## Notes

- Why the skeptic kept it: Reproduced. A statement that is only a method chain ending on an answer (a.sort().by_value(), a.reverse(), a.sum, a.size, n.add(1)) passes the checker and runs silently with no effect. The checker refuses the equivalent shapes: 'a' alone gets S110 "a on its own line does nothing -- give it a value with =", and 'a + 1' gets "there is no statement of that shape". n + 1 and n.add(1) mean the same thing, yet one is refused as a statement and the other is accepted. For a person coming from Python, a.sort().by_value() reads as an in-place sort and they get an unsorted list without a word. The second half of the claim is not an error: the help says ".sort() orders nothing on its own", and a.sort() displays the list unchanged, so a.sort().reverse() giving the list reversed but unsorted matches the help.
- Nearest known entry: none
- Merged: Kept separate from the unrecognised-statement-start group because the cause is in a different function: a_call_to_its_end accepts any chain that reaches the line end.
