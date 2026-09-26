# 020 -- Include paths are compared without normalising them: one file included as shapes and as ./shapes is called "two files ... rename one", and a mutual include through ./ grows the path until "cannot locate file"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** syntax and layout  
**Kind:** valid-program-refused  
**Severity:** medium

## What happens

Include paths are compared unnormalised: one file included as shapes and ./shapes (or ../dir/shapes) is called 'two files ... rename one', and a mutual include through ./ grows the path until 'cannot locate file'.

## Program

```satellite
satellite.include(satellite)
satellite.include(shapes)
satellite.include(./shapes)

satellite.capsule satellite.main()
{
    satellite.console.display(shapes.area(4, 5))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: inc/shapes.satl beside the program: satellite.capsule area(satellite.variable.number w, satellite.variable.number h) { satellite.return(w * h) } (on 4 lines, no include).

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, shapes.area could be either of two files this
file includes -- the files
shapes.satl
and
shapes.satl
are both named shapes and both declare area, so rename one of the files

directory: t_dot.satl:7
syntax: satellite.console.display(shapes.area(4, 5))
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

20, exit 0, the same as including shapes twice under one spelling. The mutual ./ include should run like the plain-spelled cycle (prints 2).

The include help says './' and '..' are available. The same file under one spelling is loaded once. The refusal's two paths are one file, so 'rename one of the files' cannot be followed.

## Variants

t_twice (shapes, shapes): 20 exit 0. t_q (shapes, "shapes.satl"): 20 exit 0. t_onlydot (./shapes alone): 20 exit 0. t_qdot (shapes, "./shapes.satl"): same refusal. t_up (shapes, ../inc/shapes): same refusal naming inc/../inc/shapes.satl. cyc/a.satl + cyc/b.satl including each other with ./: '[satellite] cannot locate file: .../cyc/./././././...' (8 KB path), exit 8. The same cycle with plain names (a2/b2): prints 2, exit 0.

## Where it comes from

satellite/bytecode/program_walk.cpp:352 (loaded-file check `done == path`); paths built in satellite/bytecode/include_shape.cpp:184 (directory + "/" + relative, not normalised)

include_at resolves a relative include as directory + "/" + relative without normalising it (no lexically_normal/canonical). load_program's already-loaded and cycle check compares these strings exactly, so ./x and x are two files. A cycle through ./ adds another "./" on each round, so it never matches until the path is too long to open.

## Notes

- Why the skeptic kept it: Reproduced. Including shapes and ./shapes (or "./shapes.satl", or ../inc/shapes) loads the same file twice. The call is then refused as 'could be either of two files ... inc/shapes.satl and inc/./shapes.satl ... rename one of the files', but it is one file, so the advice is impossible. Including `shapes` twice, or shapes plus "shapes.satl", runs and prints 20. The include help says '".." and "./" is also available'. A worse variant: two files that include each other with ./ (a.satl: include(./b), b.satl: include(./a)) fail with '[satellite] cannot locate file: .../cyc/./././././...' (a path over 8 KB), exit 8. The same cycle spelled plainly runs and prints 2. The cycle guard compares the unnormalised strings, so the path grows without end. Not in any known list.
- Nearest known entry: none (errors.md / NEW_ERROR_LIST cover only the old 'include(./x) includes nothing', since fixed)
- Merged: Single finding.
