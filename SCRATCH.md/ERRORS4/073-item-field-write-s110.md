# 073 -- Writing a field through a container item (things[1].v = 5, m["a"].v = 5) gets an unrelated S110 "not a method call" sentence, while reading it, or writing a.v, gets S230

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

Writing a field through a container item (things[1].v = 5, m["a"].v = 5) gets an unrelated S110 'not a method call' sentence, while reading it or writing a.v gets S230.

## Program

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
    satellite.container.list<cell> things = {a}
    things[1].v = 5
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
satl(check): in satellite.main, things is followed by something that is not a
method call -- a call's answer cannot be given a value, a bracket must close on
its line, and one statement is one line

directory: f11min.satl:15
syntax: things[1].v = 5
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

S230 (exit 52): 'v is a field of cell and fields are reached from inside the spacesuit only -- write a capsule in cell that answers it and call that instead', which a.v = 3 and display(things[1].v) both get.

The same mistake (reaching a field from outside) gets S230 when read through the item and when written on a named object. Every reason the S110 lists is false for this line.

## Variants

a.v = 3: S230 at check. display(things[1].v): S230 at check. things[1].v = 5: S110 as shown. map<string, cell> m["a"].v = 5: the same S110 'm is followed by something that is not a method call'.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:780-826 (a_call_to_its_end; the sentence at :823)

a_call_to_its_end walks name[...].v as a run of index then 'method or name after a dot'. It treats .v like a call and sets ended_on_an_index = false. The `=` after it is then allowed only when the run ended on an index, so the line falls to the generic sentence. The function never asks whether the dotted name is a field of the item's spacesuit, which would give S230.

## Notes

- Why the skeptic kept it: Reproduced. things[1].v = 5, which writes a protected field through a list item, is refused with S110 'things is followed by something that is not a method call -- a call's answer cannot be given a value, a bracket must close on its line, and one statement is one line'. Reading the same field, display(things[1].v), gets the correct S230 'v is a field of cell ...'. So does a.v = 3. The line holds no call, its bracket closes and it is one line, so none of the reasons given apply. The same fallback sentence appears in confirmed lists#10 (a[1] += 2). That entry is about compound-assign tokens, and its fix (mapping += after ] to S210) would not touch this shape, so this is a separate error at the same fallback.
- Nearest known entry: Related, not the same: the confirmed lists#10 (a[1] += 2 gets the same generic S110 sentence from the same fallback in a_call_to_its_end, but for a compound-assign token).
- Merged: Single finding. See item-compound-assign-s110.
