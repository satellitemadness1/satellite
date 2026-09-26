# 082 -- "abc".replace("a", "b") on a string literal is refused (S210) only after earlier lines ran, while s.replace on a name, and .size/.contains on a literal, are refused before anything runs

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** strings  
**Kind:** inconsistency  
**Severity:** low

## What happens

"abc".replace("a", "b") on a string literal is refused (S210) only after earlier lines ran, while s.replace on a name and "abc".size/.contains/.append on a literal are refused before anything runs.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.console.display("abc".replace("a", "b"))
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
S210: NOT_BUILT_YET
satl(run): that string.replace is not built for a string yet -- so far it is a
file's

directory: .../verify2/strings-2-0/rp1.satl:6
syntax: satellite.console.display("abc".replace("a", "b"))
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

Refused by satl(check) before 'before' prints, as "abc".size and s.replace are.

string help_text.txt: 'not built yet in satellite 004: ... .replace(a, b) ... Each is refused before anything runs (S210)'.

## Variants

s.replace("a", "b") on a declared name: satl(check) 's.replace is not built for satellite.variable.string yet', nothing printed. "abc".size, .size(), .empty, .contains("a"), .append("x"), .clear, .clear() on a literal: satl(check), nothing printed. Only .replace on a literal is late.

## Where it comes from

satellite/bytecode/program_check.cpp:1542-1551. The literal branch refuses a not-built method only when 'container_arity(method) >= 0'. replace_token is a file/string method, not a container method, so it passes.

The checker's not-built test for a method on a literal uses the list of container methods. .replace is a file's method, not a container's, so it slips through to the walker.

## Notes

- Why the skeptic kept it: Reproduced: display("before") then display("abc".replace("a", "b")) prints 'before' and then gives S210 at run time ('satl(run): that string.replace is not built for a string yet -- so far it is a file's', exit 14). s.replace on a declared name, and "abc".size/.contains/.append/.clear/.empty on a literal, are all refused by satl(check) with nothing printed. The help says .replace is 'refused before anything runs (S210)'. The task's own rule makes a late S210 an error here. No known list has it.
- Nearest known entry: none
- Merged: Single finding.
