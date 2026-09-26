# 050 -- A misspelled satellite word gets "did you mean" only at the start of a statement: as a type inside <...> or as a parameter type it is "<prefix> is not a type", and in value position (satellite.bool.flase) it is an undeclared variable

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** binary hex color infinity bool, lists  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A misspelled type word read as a type (inside list/map/index/multiple <...>, or as a capsule parameter's type) is refused as "<its prefix> is not a type a name can be declared as", with no did-you-mean; the same misspelling at the start of a declaration line gets "has no word named X -- did you mean ...?".

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.list<satellite.variable.strng> names = {"a"}
    satellite.console.display(names)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl. The bare comparison is bare.satl in the same folder, variants are v1-v10.satl, and the capsule-parameter cases are p1.satl and p2.satl.

## What satl does

```
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.variable is not a type a name can be
declared as

directory: prog.satl:5
syntax: satellite.container.list<satellite.variable.strng> names = {"a"}
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

Comparison, bare.satl (line 5 `satellite.variable.strng name = "a"`):
S110: LINE_NOT_UNDERSTOOD
satl(check): in satellite.main, satellite.variable has no word named strng --
did you mean satellite.variable.string?

exit 13
```

## What it should do

It should be refused the same way as the bare spelling: "satellite.variable has no word named strng -- did you mean satellite.variable.string?". At the least, the refusal should name the word the person actually wrote (satellite.variable.strng), not the prefix satellite.variable.

These are two equivalent forms that disagree. The same misspelled type word gets a correct did-you-mean refusal at the start of a statement (bare.satl, v9 `satellite.variable.numbr x = 5`) and a misleading one when it is read as a type anywhere else. NEW_ERROR_LIST.md fixed item 9 says "A misspelled word gets did you mean". A misspelled container at the start of a statement also gets it (v4 `satellite.container.lst<...>` -> "satellite.container has no word named lst -- did you mean satellite.container.list?"). Blaming satellite.variable is also a poor pointer: the person never wrote satellite.variable as a type.

## Variants

These also FAIL the same way (the refusal names the prefix, with no did-you-mean):
- v1: map<satellite.variable.strng, satellite.variable.number>
- v2: multiple<satellite.variable.numbr, satellite.variable.string>
- v3: list<list<satellite.variable.strng>> (nested)
- v8: index<satellite.variable.string, satellite.variable.numbr> (second parameter)
- v5: list<satellite.varible.string> -> "satellite is not a type a name can be declared as"
- p1: capsule parameter with no <> at all, `satellite.capsule show(satellite.variable.strng s)` -> "satellite.capsule show -- satellite.variable is not a type a name can be declared as"
- p2: capsule parameter `list<satellite.variable.strng>`, same sentence.

These PASS or behave correctly:
- v7: list<satellite.variable.string> prints {"a"}.
- v10: list<satellite.console.display> -> "satellite.console.display is not a type..." (correct, because the whole word is named).
- v6: list<strng> -> S201 "no spacesuit named strng -- a type is a satellite.variable or satellite.container word, or the name of a satellite.spacesuit" (reasonable).
- v4 and v9: misspellings at the start of a statement get did-you-mean.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/type_shape.cpp:170-171 (read_type_shape); the did-you-mean exists only at /home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:2031-2041

`satellite.variable.strng` lexes as the word satellite.variable followed by `.` and the name strng. In read_type_shape (type_shape.cpp:170), `!is_a_type_word(word)` is true for satellite.variable. The code then sets why = spelling_of(word) + " is not a type a name can be declared as" and never looks at the following `.name` tokens. The "has no word named X -- did you mean" logic (a word, then method_token, then name_token, then word_it_most_likely_meant) lives only in the statement-start arm of program_check.cpp (around lines 2031-2041). Capsule parameter lists and <...> type arguments go through read_type_shape, so they never reach that logic.

## Notes

- Why the skeptic kept it: Reproduced exactly on BUILD 0099. A misspelled type word inside <...> is refused as "satellite.variable is not a type a name can be declared as". That sentence names only the prefix the lexer split off and says nothing about the misspelled part (strng). The same misspelling at the start of a statement gets "has no word named strng -- did you mean satellite.variable.string?". None of the known lists (ERROR.md, errors.md, ERRORS2.md, NEW_ERROR_LIST.md, CONTAINERS.md) has an entry for this, and neither does DESIGN.md. NEW_ERROR_LIST.md fixed item 9 promises "A misspelled word gets did you mean", and that fix covers only the statement-start path. The problem is wider than the hunter reported: a misspelled type in a capsule's parameter list, even without any <>, gets the same misleading sentence.
- Nearest known entry: none. The nearest entry is NEW_ERROR_LIST.md fixed item 9 ("A misspelled word gets did you mean"). That fix covers only the statement-start path. CONTAINERS.md line 94 is about the map spelling, not this.
- Merged: Same cause: word_it_most_likely_meant is called only on the statement-start path (program_check.cpp ~2031-2081). read_type_shape and the value-position name check never call it.

### Also seen as: A misspelled satellite value word (satellite.bool.flase, satellite.console.widht) is refused as an undeclared variable, with no did-you-mean

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.bool done = satellite.bool.flase
    satellite.return(satellite)
}
```

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S201: NAME_NOT_DECLARED
satl(check): in satellite.main, flase has no satellite.variable line declaring
it

directory: n13a.satl:5
syntax: satellite.variable.bool done = satellite.bool.flase
        /\

this name was used and no satellite.variable line ever declared it. A name has
to be given a type before it can hold anything.
machine code 25 name_not_declared -- satl exits with this.

--------------------------------------------------------------------------------

exit 25
```
