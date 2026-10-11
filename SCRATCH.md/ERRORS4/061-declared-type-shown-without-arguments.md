# 061 -- A typed-container refusal names the declared type without its <type> ("was declared satellite.container.list"), though a bare list holds anything

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A typed-container refusal names the declared type without its <type> ("was declared satellite.container.list"), though a bare list holds anything.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list<satellite.variable.number> a = {1, "x"}
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
[satellite] satl(run): a was declared satellite.container.list, and item 2 of it does not fit: it holds a string (machine_code: 27 types_do_not_meet)

exit 27
```

## What it should do

"a was declared satellite.container.list<satellite.variable.number>, and item 2 of it does not fit: it holds a string".

The list help: "with no <> a list holds anything". Naming the type without its <> says the list accepts any item, which contradicts the refusal.

## Variants

Capsule parameter (total's x, list<number>): the same wording. index<string, number> m = {"a": "x"} gets "m was declared satellite.container.index, and a value of it does not fit". a.append("x") and a[1] = "x" get S301 reports that do not name the declaration at all.

## Where it comes from

satellite/bytecode/type_shape.hpp:84-87 (shape_written returns word::spelling_of(shape.word) only); used at program_walk.cpp:1218 and 2009/2014

shape_written prints the shape's outer word (or a spacesuit's name) and never its type arguments, so list<number> and map<k, v> come out bare.

## Notes

- Why the skeptic kept it: Reproduced. When a typed container does not fit, the sentence names the declared type without its <...>: "x was declared satellite.container.list, and item 2 of it does not fit". A bare satellite.container.list holds anything, so the sentence contradicts itself. The same happens for the index. The known ERRORS2 #7 / NEW_ERROR_LIST B1 is only about the refusal being one line rather than a report. The wrong type name is a separate defect, in shape_written.
- Nearest known entry: ERRORS2 #7 / NEW_ERROR_LIST B1 cover only the one-line form, not the type named
- Merged: Single finding (shape_written, type_shape.hpp:84-87).
