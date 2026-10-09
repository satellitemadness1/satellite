# 083 -- display(t.join()) of a thread that handed nothing back is refused at run time as S210 NOT_BUILT_YET, while display(greet()) is S240 before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** threads  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

display(t.join()) of a thread that handed nothing back is refused at run time as S210 NOT_BUILT_YET ('the library ... does not exist yet'), after earlier lines ran; the direct display(greet()) is S240 before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule idle()
{
    satellite.variable.number k = 1
}

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.variable.thread t = satellite.thread.new(idle())
    t.start()
    satellite.console.display(t.join())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is saved as jn_min.satl. direct_nothing.satl and stopped_join_display.satl are in the same folder.

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(run): satellite.console.display has no scenario for nothing

directory: jn_min.satl:13
syntax: satellite.console.display(t.join())
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

A refusal that says the joined thread handed nothing back, in the family of S240 CAPSULE_GAVE_NO_ANSWER (exit 53), which the checker gives for display(greet()) before anything runs. It should not claim display is an unbuilt library word.

THREADS.md T1 says .join() 'answers ... nothing when it handed nothing back', and after .stop() '.join() then answers nothing', so this is documented, ordinary use. The S210 paragraph ('the library behind it does not exist yet. The word is right') is false here. The equivalent direct form is caught by the checker. The hunt's S210 rule counts a not-built refusal that comes after earlier lines ran, where the other form is refused before anything runs.

## Variants

direct display(greet()): S240 CAPSULE_GAVE_NO_ANSWER at check, exit 53, nothing printed. The stopped thread (t.stop() then display(t.join())): the same S210, exit 14. Assigning it (string got = t.join()): the one-line '[satellite] satl(run): got was declared satellite.variable.string, and it holds nothing (machine_code: 27 types_do_not_meet)', exit 27. So three uses of the same nothing give three different codes.

## Where it comes from

satellite/bytecode/console_calls.cpp:460 (context.refuse(not_built_yet, spelled + " has no scenario for " + argument.kind_name() ...))

display refuses any value kind it has no printer for with not_built_yet, and that includes the 'nothing' kind. The checker judges a thread.new(...) call as a call whose answer may be absent, so it never applies S240 to t.join()'s answer.

## Notes

- Why the skeptic kept it: Reproduced. display(t.join()) of a thread whose capsule hands nothing back runs the earlier lines ('before', 'hi'), then refuses with S210 NOT_BUILT_YET, whose paragraph says 'the library behind it does not exist yet. The word is right'. The direct display(greet()) is refused before anything runs with S240. THREADS.md documents a nothing answer from join() as ordinary. ERRORS2 B (a bare string cannot be displayed) meets the same S210 sentence, but it is about bare declarations, it is marked fixed, and it says nothing about this label or about join. Low severity, because the program is at fault either way.
- Nearest known entry: Related only: SCRATCH.md/ERRORS2.md item B (a bare string holds nothing, and display refuses nothing). That item is marked fixed and concerns bare declarations, not the S210 label or join.
- Merged: Single finding.
