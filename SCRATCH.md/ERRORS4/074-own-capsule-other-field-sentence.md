# 074 -- other.field inside the spacesuit's own capsule is refused because "fields are reached from inside the spacesuit only", though the line is already inside it

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

other.field inside the spacesuit's own capsule is refused because "fields are reached from inside the spacesuit only" / "only its own capsules can reach it", both true of the line.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit money()
{
    satellite.protected
    {
        satellite.variable.number cents = 0
    }
    satellite.public
    {
        satellite.capsule same(money other)
        {
            satellite.return(other.cents)
        }
    }
}

satellite.capsule satellite.main()
{
    money a
    satellite.console.display(a.same(a))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S230: MEMBER_IS_PROTECTED
satl(check): in money.same, cents is a field of money and fields are reached
from inside the spacesuit only -- write a capsule in money that answers it and
call that instead

directory: f15min.satl:13
syntax: satellite.return(other.cents)
        /\

this is inside a spacesuit, where only its own capsules can reach it -- every
field is, and so is a capsule written in satellite.protected. A capsule in
satellite.public that answers it is the way in from outside.
machine code 52 member_is_protected -- satl exits with this.

--------------------------------------------------------------------------------

exit 52
```

## What it should do

The same refusal, with a reason that fits the line: a field is reached by its bare name on its own object, never through another object with a dot (other.cents), so answer it through a capsule, for example other.cents_of().

satellite.help/satellite.spacesuit/help_text.txt: 'Fields are reached from inside the spacesuit only, never as host.name'. The refusal quotes only the first half, which this line satisfies. The explanation's 'only its own capsules can reach it' is contradicted by other.secret() being accepted from the same capsule.

## Variants

The same capsule returning other.secret(), with secret() protected, is accepted and prints 7 (exit 0). cents == other.cents (the hunter's form) gets the same refusal, with its caret at column 0.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/suit_reach.cpp:203-213 (the field arm of CapsuleTable::member)

The field arm fires for any dotted reach of a field, whoever the caller is. Its sentence (003's S0517 'word for word') and the S230 paragraph assume the caller is outside the spacesuit. They never account for a caller inside it that reaches another object's field.

## Notes

- Why the skeptic kept it: Reproduced. other.cents inside money.same (money's own public capsule) is refused with S230. Refusing is right: the help says fields are reached 'never as host.name'. But the reason given is 'fields are reached from inside the spacesuit only', and the explanation paragraph says 'only its own capsules can reach it'. Both are true of this line, which is inside money and in one of its own capsules, so neither explains the refusal. The contrast confirms the explanation is wrong: a protected capsule reached the same way, other.secret() from money.same, is accepted and prints 7. So 'only its own capsules can reach it' is exactly the permission this line has. The right reason, a field is never reached through another object with a dot, is the help's 'never as host.name', and the refusal leaves it out. No known entry covers it.
- Nearest known entry: none
- Merged: Single finding.
