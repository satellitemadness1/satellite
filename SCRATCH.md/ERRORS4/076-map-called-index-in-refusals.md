# 076 -- Refusals about a name declared satellite.container.map call it an "index" ("the index is empty"), though the help says a refusal uses the word that was written

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** contradicts-help  
**Severity:** low

## What happens

Refusals about a name declared satellite.container.map call it an 'index' ('the index is empty'), though the help says a refusal says the word you wrote.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.string, satellite.variable.number> age
    satellite.console.display(age.first)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at p8a.satl (variants p8b-p8g alongside)

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S501: LINE_PAST_THE_END
satl(run): age.first: the index is empty

directory: p8a.satl:6
syntax: satellite.console.display(age.first)
        /\

a line was read by its number and the file has no line with that number. Lines
count from 1, and the last one is the file's size.
machine code 47 line_past_the_end -- satl exits with this.

--------------------------------------------------------------------------------

exit 47
```

## What it should do

"age.first: the map is empty", and "map" in every refusal about a name declared as a map or a value made with satellite.container.map() or a map literal.

satellite.container.map help: "It is the same container as satellite.container.index ... either word declares it, and a refusal says the word you wrote."

## Variants

p8b age.remove_last() -> "in age, age.remove_last: the index is empty" (47). p8c age.sum -> satl(check) "an index holds keys and values, so say which" (27). p8d list l = satellite.container.map() -> "it holds an index" (27). p8e "a" + age -> "+ was given a string and an index" (27). The map age.append / age.insert refusals (p6a, p6e) say "an index has keys and not positions". p8g age.oops -> "age is satellite.container.map", which does use the word written. The S501 paragraph about files is ERRORS2 item E, a separate problem.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/container_calls.cpp:372 ("an index has keys and not positions"), :437 and :634 ("the index is empty"), :173-176 index_refuses ("an index holds keys and values"); expression.cpp:777; the value kind name "an index" used by kind_name()

These refusal strings hard-code "index", and the value's kind_name() is always "an index". The declared word (is_an_index_word covers both map and index) is never passed down to the sentence, so a map is always described as an index.

## Notes

- Why the skeptic kept it: I reproduced it. With every name declared satellite.container.map, the refusals call it an index. age.first on an empty map gives "the index is empty", and so does age.remove_last(). age.sum gets "an index holds keys and values" (from check). "a" + age gets "+ was given a string and an index". list l = satellite.container.map() gets "it holds an index". The map help states: "either word declares it, and a refusal says the word you wrote", and CONTAINERS.md step 1 records the same ("a refusal says the word written"). A refusal that knows the name's declaration still says index. Only the check-time "age is satellite.container.map, and oops is not one of its methods" uses the word written. It is in none of the known lists. ERRORS2 item E is about S501's file-lines paragraph, a separate problem that also shows up here.
- Nearest known entry: none (ERRORS2 E is the unrelated S501 paragraph)
- Merged: Single finding.
