# 005 -- A spacesuit capsule named lock() or unlock() never runs: the built-in object lock silently takes its place, and the declaration is not refused

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** spacesuits and library  
**Kind:** wrong-answer  
**Severity:** high

## What happens

A spacesuit capsule named lock() or unlock() is never run: the built-in object lock silently takes its place.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit door()
{
    satellite.public
    {
        satellite.capsule lock()
        {
            satellite.console.display("my lock ran")
        }
    }
}

satellite.capsule satellite.main()
{
    door d
    d.lock()
    satellite.console.display("after")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
after

exit 0
```

## What it should do

"my lock ran" then "after". The declared capsule should run, as a user capsule does for other method words (display, size, clear...). Failing that, declaring a capsule named lock/unlock should be refused before anything runs.

satellite.spacesuit help: "From outside, an object's public capsules are called on it, host.greet()". lock/unlock appear in no help topic, so a user cannot know the name is taken.

## Variants

The hunter's form (lock sets a field, is_locked() answers it) displays false. A capsule unlock(satellite.variable.number c) called as d.unlock(5) is refused before anything runs: "S110 ... d.unlock() takes nothing, in its brackets" (v0b.satl, exit 13). Other method-word names (display, size, clear, ...) run the user capsule, per the hunter's sweep.

## Where it comes from

satellite/bytecode/capsule_calls.cpp:201-212 (run time) and satellite/bytecode/program_check.cpp:987-996 (checker)

Both call_member and member_of_an_object check for lock_token/unlock_token and handle them as the author's object lock ("every object has .lock() and .unlock(), whatever its spacesuit declares") BEFORE they look up the spacesuit's own capsules (capsules.member). A user capsule of that name can therefore never be reached. Also, the capsule declaration is never refused.

## Notes

- Why the skeptic kept it: Reproduced: a public capsule named lock() is never run; d.lock() silently runs the built-in object lock instead, and the program exits 0. unlock() behaves the same way, and a user unlock(number) called as d.unlock(5) is refused before anything runs as 'd.unlock() takes nothing'. No help topic or DESIGN.md mentions lock/unlock, and nothing refuses or warns about the declaration. Not in any known list and not among the already-confirmed.
- Nearest known entry: none
- Merged: Single finding. The cause differs from the spelling folding: lock_token is handled before capsules.member is consulted.
