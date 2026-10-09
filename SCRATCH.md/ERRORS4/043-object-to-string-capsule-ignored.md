# 043 -- Displaying or joining a list of objects whose spacesuit declares to_string() is refused because "an object has no to_string capsule"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** misleading-refusal  
**Severity:** medium

## What happens

Displaying (or joining) a list of objects whose spacesuit declares to_string() is refused because "an object has no to_string capsule".

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit box()
{
    satellite.public
    {
        satellite.capsule to_string()
        {
            satellite.return("a box")
        }
    }
}

satellite.capsule satellite.main()
{
    box a
    satellite.container.list<box> l = {a}
    satellite.console.display(l)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(run): satellite.console.display was given a list, and an object has no
to_string capsule, and satellite does not invent one

directory: f7b.satl:18
syntax: satellite.console.display(l)
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

Either {a box}, using the capsule the spacesuit declares, as the source comment on this arm describes, or an honest not-built refusal that does not claim the capsule is missing (for example 'printing an object inside a list is not built yet').

satellite_object.cpp's own comment says 'AN OBJECT PRINTS ITSELF THROUGH A CAPSULE ITS SPACESUIT DECLARES'. This spacesuit declares one, and a.to_string() answers 'a box', so 'an object has no to_string capsule' is false for this program.

## Variants

l.join(", ") gives S301 (exit 27): 'l.join: item 1 has no text to join -- an object has no to_string capsule, and satellite does not invent one'. display(a) for the single object gives S210 'satellite.console.display has no scenario for an object', which is accurate. "x" + a gives the generic S301 'no scenario for that pair'.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/satellite_object/satellite_object.cpp:755-760 (case user_defined in the to-string conversion)

The user_defined arm always returns not_built_yet with the fixed sentence 'an object has no to_string capsule'. It never looks at the object's spacesuit to see whether a to_string capsule exists, and it cannot call one, so the sentence is written as though every spacesuit lacks one.

## Notes

- Why the skeptic kept it: Reproduced. box declares a public to_string() capsule, and a.to_string() answers 'a box'. Displaying a list<box> is then refused with 'an object has no to_string capsule, and satellite does not invent one', and l.join(", ") is refused with 'item 1 has no text to join -- an object has no to_string capsule'. Refusing is acceptable: printing objects is S210 not built, and nothing in the help promises it. But this spacesuit declares the capsule the refusal says is missing, so the refusal is false. Displaying a single object gets a different, accurate sentence ('display has no scenario for an object'). No known entry and no help text covers this.
- Nearest known entry: none
- Merged: Single finding.
