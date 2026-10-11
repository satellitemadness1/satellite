# 044 -- The checker does not judge a member reached through a map item or a nested container item (m[k].x, m[k].secret(), a wrong argument count): a field read is S201 "undeclared variable", and a protected capsule or a wrong count is refused only after earlier lines ran. Through a list item, both are refused before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** contradicts-help  
**Severity:** medium

## What happens

A protected capsule or a wrong argument count reached through a map item (or a nested container item) is refused only at run time, after earlier lines ran; through a list item it is refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit cell()
{
    satellite.protected
    {
        satellite.capsule secret()
        {
            satellite.return(1)
        }
    }
    satellite.public
    {
        satellite.capsule get()
        {
            satellite.return(2)
        }
    }
}

satellite.capsule satellite.main()
{
    cell a
    satellite.container.map<satellite.variable.string, cell> byname = {"a": a}
    satellite.console.display("before")
    satellite.console.display(byname["a"].secret())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S230: MEMBER_IS_PROTECTED
satl(run): secret is inside the satellite.protected part of cell, so only cell's
own capsules can reach it -- move it to satellite.public, or add a capsule there
that answers it

directory: f9map.satl:26
syntax: satellite.console.display(byname["a"].secret())
                                             /\

this is inside a spacesuit, where only its own capsules can reach it -- every
field is, and so is a capsule written in satellite.protected. A capsule in
satellite.public that answers it is the way in from outside.
machine code 52 member_is_protected -- satl exits with this.

--------------------------------------------------------------------------------

exit 52
```

## What it should do

satl(check) S230 before anything runs, with 'before' never printed, as for things[1].secret() on a list<cell>.

satellite.help/satellite.spacesuit/help_text.txt: 'satl refuses these before anything runs: a field or a protected capsule reached from outside (S230), a name the spacesuit does not have, an object given the wrong number of arguments ...'.

## Variants

list<cell> things[1].secret(): satl(check), no 'before', which is correct. map<string, cell> byname["a"].secret(): 'before', then satl(run) S230. index<string, cell>: the same late refusal. list<list<cell>> grid[1][1].secret(): the same late refusal. byname["a"].get(1, 2): 'before', then the one-line '[satellite] satl(run): cell.get takes 0 arguments, and was given 2 (machine_code: 13 ...)'. byname["a"].nosuch() is refused at check with 'no capsule named nosuch'.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:1304-1316

The checker judges an item's member only when the head name is in where.lists, a one-level list of spacesuit objects, and the member comes straight after a single [...]. Maps/indexes of objects and nested containers are never recorded there. Their members are left to the walker, which applies S230 and the argument count only when the line runs.

## Notes

- Why the skeptic kept it: Reproduced. byname["a"].secret() on a map<string, cell> prints 'before' and is then refused with S230 at run time. The same reach through a list<cell> item is refused by satl(check) before anything runs. The spacesuit help says: 'satl refuses these before anything runs: a field or a protected capsule reached from outside (S230) ... an object given the wrong number of arguments'. A map's value type is declared just as a list's item type is. Nested reaches (grid[1][1] on list<list<cell>>) and the index spelling get the same late refusal. A wrong argument count through a map item also runs 'before' first and gives a one-line refusal. Not in the known lists. The confirmed lists#4 is about not-built methods on list items, a different check.
- Nearest known entry: none
- Merged: Same cause and one fix: the item-member check (program_check.cpp:1304-1316) is reached only through where.lists, which records only a one-level list<spacesuit>.

### Also seen as: A field read through a map item is refused as an undeclared variable (S201), or, if a same-named variable exists, only at run time

```satellite
satellite.include(satellite)

satellite.spacesuit cell()
{
    satellite.protected
    {
        satellite.variable.number v = 0
    }
}

satellite.capsule satellite.main()
{
    cell a
    satellite.container.map<satellite.variable.string, cell> byname = {"a": a}
    satellite.console.display(byname["a"].v)
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, v has no satellite.variable line declaring it

directory: f10.satl:15
syntax: satellite.console.display(byname["a"].v)
        /\

this name was used and no satellite.variable line ever declared it. A name has
to be given a type before it can hold anything.
machine code 25 name_not_declared -- satl exits with this.

--------------------------------------------------------------------------------

exit 25
```
