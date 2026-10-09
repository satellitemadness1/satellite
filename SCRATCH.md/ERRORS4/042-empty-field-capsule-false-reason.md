# 042 -- A capsule called on an empty spacesuit field is refused with a false math-sign / undecided-operator reason, and the "holds no object yet" refusal is unreachable

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

A capsule called on an empty spacesuit field is refused with a false math-sign / undecided-operator reason; the 'holds no object yet' refusal is unreachable.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit engine()
{
    satellite.public
    {
        satellite.capsule get_power()
        {
            satellite.return(5)
        }
    }
}

satellite.spacesuit car()
{
    satellite.protected
    {
        engine motor
    }
    satellite.public
    {
        satellite.capsule power()
        {
            satellite.console.display(motor.get_power())
        }
    }
}

satellite.capsule satellite.main()
{
    car d
    d.power()
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(run): satellite.console.display was given something it could not read to
the end of -- check that every math sign has a space on both sides

directory: m6.satl:24
syntax: satellite.console.display(motor.get_power())
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

A refusal naming the real problem, the one capsule_calls.cpp already has: "motor holds no object yet, so it has no get_power -- a field of a spacesuit's type starts empty until something gives it one".

satellite.spacesuit help: "A field of a spacesuit's type with nothing after its name starts empty, until a capsule gives it an object." The line has no operator, and its only sign is spaced correctly, so the reason given is false.

## Variants

satellite.return(motor.get_power()): one line, "satellite.return(...) could not be read to the end -- it stops at something with no meaning there yet (& | << >> !! are undecided)". Bare statement motor.get_power(): "motor... could not be read to the end -- ...undecided". satellite.variable.number n = motor.get_power(): "n = ... could not be read to the end". display(motor.get_power() + 1): the math-sign sentence. All exit 13.

## Where it comes from

satellite/bytecode/capsule_calls.hpp:55-62 (a_member_next) with satellite/bytecode/expression.cpp:366-372; the unreachable refusal is at satellite/bytecode/capsule_calls.cpp:195-199

The chain loop in expression.cpp goes into call_member only when the next member is a method code or a_member_next(row, at, *live) is true. a_member_next returns false unless the value is_user_defined(), and an empty field is not, so for a plain name like get_power the loop never runs. The expression stops at the '.', and the caller reports the generic unfinished-parse sentence. call_member's null-handle refusal only fires for a value that is user-defined but has a null handle.

## Notes

- Why the skeptic kept it: Reproduced: calling a capsule on a spacesuit field that holds no object yet is refused with the generic unfinished-parse sentence. That sentence gives a false reason: math-sign spacing, or "& | << >> !! are undecided". The dedicated sentence in capsule_calls.cpp ("motor holds no object yet ...") can never be reached. The already-confirmed number-string-conversion#2 and lists#2 share the misleading catch-all text, but their causes differ (a bare value word; [ after an answer). Here the specific refusal exists and is dead code. ERRORS3.md #10 (not a listed known file) describes the catch-all sentence in general, not this case.
- Nearest known entry: none (related in wording only: number-string-conversion#2, lists#2, SCRATCH.md/ERRORS3.md #10)
- Merged: Single finding.
