# 049 -- The S210 hint from so_far_whose knows only file and container methods: .width on a binary and .transparency on a non-colour say "so far no type has it" (hex and colour do have them), and x.lock() says "so far it is a file's" (objects have it too)

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, threads  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

S210 for .width on a binary (and .transparency on a non-colour) says "so far no type has it", though hex has .width and colour has .transparency.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.binary a = b0110
    satellite.console.display(a.width)
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
satl(check): in satellite.main, a.width is not built for
satellite.variable.binary yet -- so far no type has it

directory: f8a.satl:6
syntax: satellite.console.display(a.width)
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```

## What it should do

'... not built for satellite.variable.binary yet -- so far a hex has it' (for .transparency, '-- so far a color has it'), or no claim about other types.

satellite.help/satellite.variable.hex: '.width counts those bits' (example mask.width // 16). satellite.help/satellite.variable.color: 'c.transparency reads it back'. A hex's h.width prints 8 in this build.

## Variants

Same false clause: number n; n.transparency -> '... not built for satellite.variable.number yet -- so far no type has it'. PASS: hex h = x0F; h.width -> 8. For comparison, an unnumbered method (n.frobnicate) gets S110 'not one of its methods'.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/file_calls.cpp:242-256 (so_far_whose); used at program_check.cpp:753 and :1551, expression.cpp:627

so_far_whose() knows only .foreground/.background, file methods and container methods. For any other method it returns 'so far no type has it' and never consults the hex or colour method tables.

## Notes

- Why the skeptic kept it: I reproduced this. `a.width` on a binary is refused with S210 'not built for satellite.variable.binary yet -- so far no type has it'. The same program's `h.width` on a hex prints 8. `n.transparency` on a number also says 'so far no type has it', although a colour has .transparency. The binary help does say '.width ... numbered and not built yet', so S210 itself is right. The clause about other types is false, and a refusal that says something false counts as an error by the definition. It is not in any known list or among the already-confirmed.
- Nearest known entry: none
- Merged: Same function and one fix: so_far_whose (file_calls.cpp:242-256) consults only the file and container tables. threads-2#7 also notes that a missing thread method is filed under S301.

### Also seen as: x.lock() on a string or number is refused 'so far it is a file's', though an object of a spacesuit has .lock() too; on a thread, x.lock() gets S301's 'this operator ... two kinds' paragraph

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string x = "abc"
    x.lock()
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S210: NOT_BUILT_YET
satl(check): in satellite.main, x.lock is not built for
satellite.variable.string yet -- so far it is a file's

directory: lock_on_string.satl:22
syntax: x.lock()
        /\

this word is numbered in the language and the library behind it does not exist
yet. The word is right; there is nothing yet to run.
machine code 14 not_built_yet -- satl exits with this.

--------------------------------------------------------------------------------

exit 14
```
