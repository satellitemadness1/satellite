# 033 -- Passing a list<number> to a list<number> parameter or declaration re-checks every item each time: 20,000 calls with a 20,000-item list take 15+ s typed, against 0.19 s untyped

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** realistic programs  
**Kind:** performance  
**Severity:** medium

## What happens

Handing a list<number> to a list<number> parameter, or declaring a list<number> from one, re-checks every item each time: 20,000 calls with a 20,000-item list take 15+ s typed against 0.19 s untyped.

## Program

```satellite
satellite.include(satellite)

satellite.capsule first_of(satellite.container.list<satellite.variable.number> a)
{
    satellite.return(a[1])
}

satellite.capsule satellite.main()
{
    satellite.container.list<satellite.variable.number> xs = satellite.container.list()
    satellite.statement.for(satellite.variable.number i = 1; i <= 20000; i++)
    {
        xs.append(i)
    }
    satellite.variable.number total = 0
    satellite.statement.for(satellite.variable.number t = 1; t <= 20000; t++)
    {
        total = total + first_of(xs)
    }
    satellite.console.display(total)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
20000
(real 15.74 s; the same program with an untyped satellite.container.list parameter: 20000, [exit 0], real 0.19 s)

exit 0
```

## What it should do

About the same time as the untyped form, well under a second.

satellite.container.list help: 'b = a COPIES the list ... and the copy costs nothing until one of them is written to', and a typed list 'checks every item against that type on every way in', so every item of xs was already checked. Handing an already-typed list<number> to a list<number> parameter should not need an O(n) walk every call. Binary search, recursive list walks and helper capsules taking list<number> become quadratic because of it.

## Variants

p3 (`list<number> view = xs` inside the loop): 16.2 s against 0.19 s untyped. p1 binary search with a list<number> parameter, 20000 searches over 20000 items: 16-19.5 s against 0.9 s untyped. 5000 items x 20000 calls: 12 s real / 6.2 s user. 10000 items: 38 s real (under load). 20000 items timed out at 60 s once under load.

## Where it comes from

satellite/bytecode/type_shape.cpp:78-90 (value_fits walks every item of a list); called per bind from satellite/bytecode/program_walk.cpp:1216 (declaration) and about :1990-2010 (capsule arguments)

value_fits checks 'EVERY one of' a list's items against the element type. It runs on every typed declaration and every typed parameter bind, even when the value comes from a name that is already declared with the same type, whose items were checked when they went in. So each call costs O(list size).

## Notes

- Why the skeptic kept it: Reproduced with the machine heavily loaded (load average 74 on 24 cores, so wall times are inflated). p2 typed 20000 items x 20000 calls: 15.7 s (another run, 89.7 s real / 31.3 s user); p2 untyped: 0.19 s. p3 typed local copy: 16.2 s; untyped: 0.19 s. The time grows with list size: 5000 items x 20000 calls took 6.2 s user, 20000 items took 31.3 s user. Cause: value_fits walks every item of a list each time a typed declaration or a typed parameter is bound, though the items were already checked on the way in. Not in any known list: CONTAINERS.md's '20,000 appends take 7.8 s' was a rejected design for multiple, not this, and ERRORS3 #8 is string building. The output is correct, only the time is wrong.
- Nearest known entry: none
- Merged: Single finding.
