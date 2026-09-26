# 031 -- Building a string with s = s + ... in a loop is quadratic: every + copies the whole left side (40,000 passes take 9.8 s for 230 KB)

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** number string conversion  
**Kind:** performance  
**Severity:** medium

## What happens

Building a string with s = s + ... in a loop is quadratic: 40,000 passes of s = s + i + "," take 9.8 s for a 230 KB string.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string s = ""
    satellite.statement.for(satellite.variable.number i = 0; i < 40000; i++)
    {
        s = s + i + ","
    }
    satellite.console.display(s.find("39999,"))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none (the 40,000 case finishes inside the default 20 s)

## What satl does

```
228884
(wall time 9.81 s; 20,000 passes 2.35 s; 10,000 0.48 s; 5,000 0.21 s)

exit 0
```

## What it should do

Close to linear time. python3 builds the same string in a few milliseconds, and 40,000 appends to a list in satl take well under a second.

The task counts runaway time for ordinary work. The string help teaches joining with + ("n = " + 4), and the interpreter treats a quadratic append as a bug elsewhere (expression.cpp:337-347 and satellite_file.cpp:113). At 20,000 passes /usr/bin/time shows sys 1.34 s of 2.15 s, so fresh large allocations on every pass dominate.

## Variants

s = s + "abcdef": 10,000 0.33 s, 20,000 0.83 s, 40,000 2.43 s. The number-plus-comma form is worse (two + per pass, and the same fourfold growth). The hunter measured 80,000 passes at 41.5 s.

## Where it comes from

satellite/satellite_object/string_and_string_add.hpp (out = left; out.append(right)) -- every + copies the whole left side, and s = s + x is never done in place

Each + builds a brand-new satellite_string by copying the whole left operand, and the assignment then replaces s. So every pass copies the growing string at least once per +. Nothing recognises s = s + x as an append to s's own storage.

## Notes

- Why the skeptic kept it: Reproduced. s = s + i + "," in a loop takes 0.48 s for 10,000 passes, 2.35 s for 20,000 and 9.81 s for 40,000, about four times the time for twice the passes. The answer is correct (228884, which python3 agrees with). Plain s = s + "abcdef" is also superlinear: 0.33 s, 0.83 s, 2.43 s. The final string is only about 230 KB. Building a string with + is the ordinary way the string help teaches, and until the string methods land (M16) it is the only way besides list-plus-join. The source has explicit guards against this kind of quadratic append for lists and files, but none for strings. Nothing in the known lists or DESIGN.md covers it.
- Nearest known entry: none (ERROR.md #29 is to_text on one million-digit number, unrelated)
- Merged: Single finding.
