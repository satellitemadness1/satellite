# 026 -- A file whose name looks like a hex or binary literal (x1, xa, b1) cannot be included: include(x1) "names no file", include("x1") then gives "no capsule named area", and include(./x1) looks for ./.satl

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** syntax and layout  
**Kind:** valid-program-refused  
**Severity:** low

## What happens

A file whose name looks like a hex or binary literal (x1, xa, b1) cannot be included: include(x1) 'names no file', include("x1") then 'no capsule named area', include(./x1) looks for './.satl'.

## Program

```satellite
satellite.include(satellite)
satellite.include(x1)

satellite.capsule satellite.main()
{
    satellite.console.display(x1.area(2, 3))
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: inc/x1.satl beside the program:
satellite.capsule area(satellite.variable.number w, satellite.variable.number h)
{
    satellite.return(w * h)
}
(the same file copied as xa.satl, b1.satl and shapes.satl)

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(check): this satellite.include names no file -- write
satellite.include(ship), satellite.include(parts/ship) or
satellite.include("parts/ship.satl")

directory: m_x1.satl:2
syntax: satellite.include(x1)
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

6, exit 0, as for the same file named shapes (m_shapes.satl prints 6).

The include names a file that exists and declares area, in the documented include(ship) spelling. 'names no file' is false. The quoted form's 'no capsule named area' is false too, and its partner refusal says to write x1.area(...), which is refused.

## Variants

m_xa and m_b1: the same 'names no file' refusal. m_shapes: 6, exit 0. q_x1: include("x1") then x1.area(2, 3) gives 'satl(check): in satellite.main, no capsule named area', exit 13. q2_x1: include("x1.satl") then area(2, 3) gives 'area is a capsule of .../inc/x1.satl, and a file's capsules are reached through its name -- write x1.area(...)', exit 13. d_x1: include(./x1) gives '[satellite] cannot locate file: .../inc/./.satl, included by .../d_x1.satl (machine_code: 8 missing_satl_file)', exit 8.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/include_shape.cpp:95-122 (include_at accepts only token::name_token for the file's name and each path part); refused at /home/madness/code/cxx/satellite/satellite/bytecode/capsule_scopes.cpp:678-682

The lexer reads x1, xa and b1 as hex or binary literals, not as name_tokens. include_at builds the file name only from name_tokens, so it finds no spelling (Kind::none) and capsule_scopes refuses the include as naming no file. For ./x1 the path part is dropped, leaving './.satl'. In x1.area(...) the x1 is again a literal, so no file prefix is seen and the capsule lookup fails.

## Notes

- Why the skeptic kept it: Reproduced. satellite.include(x1), (xa) and (b1) are refused before anything runs as 'names no file', though x1.satl, xa.satl and b1.satl exist beside the program. The same file named shapes gives 6. The quoted include("x1") loads the file, but then x1.area(2, 3) is refused 'no capsule named area'. include("x1.satl") with a bare area(2, 3) is refused with 'write x1.area(...)', the spelling that was just refused, so no working form exists. include(./x1) looks for '.../inc/./.satl', losing the name. All the refusals are false. It shares a root with the hunt's reports on x1-shaped variable and capsule names (integers#4, syntax-and-layout#7, control-flow-and-capsules#1/#2), none of which is confirmed yet, but an include is its own surface.
- Nearest known entry: none in the known lists (errors.md's include(./x) entry is about the unquoted ./ form, fixed). Related unconfirmed hunt findings on x1-shaped names: integers#4, syntax-and-layout#7, control-flow-and-capsules#1/#2.
- Merged: The lexer fact is the same as in the hex-shaped-name group, but this is a different consumer (include_at accepts only name_token) with a different fix (a file name has no literal ambiguity), so it is kept separate.
