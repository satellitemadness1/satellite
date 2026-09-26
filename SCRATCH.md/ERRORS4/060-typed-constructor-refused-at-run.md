# 060 -- The typed constructor satellite.container.list<type>() (and index<k, v>()) is refused only at run time, after earlier lines ran, as "there is no value here to work with"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

The typed constructor satellite.container.list<type>() (and index<k, v>()) is refused only at run time, after earlier lines ran, as "there is no value here to work with".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.container.list a = satellite.container.list<satellite.variable.number>()
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
S110: LINE_NOT_UNDERSTOOD
satl(run): in a = ..., there is no value here to work with

directory: l12b.satl:6
syntax: satellite.container.list a = satellite.container.list<satellite.variable.number>()
                                                             /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

Either accept it as a list of nothing, or refuse it before anything runs with a sentence naming the constructor, like the one satellite.container.list(1, 2) gets: "satellite.container.list() takes no <type> -- the declaration says what the list holds".

The help defines satellite.container.list() as "{} said in words ... and it takes nothing". The checker already catches list(1, 2) before anything runs. The typed spelling is the natural C++ form and gets a run-time sentence that names nothing, after earlier output.

## Variants

Typed on both sides (list<number> a = list<number>()): the same refusal. Inside display(...): "satl(run): there is no value here to work with" with the caret at column 0. index<string, number> m = index<string, number>(): the same refusal. list(1, 2): refused by the checker before "before" prints.

## Where it comes from

satellite/bytecode/expression.cpp:1508-1509 (fall-through refusal); satellite/bytecode/container_calls.cpp:206 (checker rule covers only list(...))

The checker's constructor rule only checks what is inside list(...). A '<' after the constructor word is not recognised, so the expression evaluator reaches its last fallback, "there is no value here to work with".

## Notes

- Why the skeptic kept it: Reproduced. satellite.container.list<satellite.variable.number>() passes the checker. The lines above it run, and then it is refused at run time with the generic "there is no value here to work with", which names nothing the person wrote. The untyped satellite.container.list(1, 2) is refused before anything runs, with a sentence that says what to write. The typed index constructor behaves the same way. Nothing in the known lists covers it.
- Nearest known entry: none
- Merged: Single finding.
