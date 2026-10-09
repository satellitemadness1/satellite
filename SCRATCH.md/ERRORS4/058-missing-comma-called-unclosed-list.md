# 058 -- A list or map literal with a comma missing between two items is refused as "opened with { and never closed with }", though the line closes it

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** lists  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A list (or map) literal with a comma missing between two items is refused as "opened with { and never closed with }", though the line closes it with }.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    satellite.container.list a = {"one" "two"}
    satellite.console.display(a)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at prog.satl (variants v1..v10 and w1..w10 .satl are in the same folder)

## What satl does

```
before
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(run): in a = ..., this list was opened with { and never closed with } --
items are separated by commas, as in {"one", "two"}

directory: prog.satl:6
syntax: satellite.container.list a = {"one" "two"}
                                     /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

A refusal, still S110 and exit 13, that says what is wrong: two items with no comma between them, with the caret at the second item (or between the two). It should not claim that the { was never closed.

The line visibly closes the list with } at its end, so "never closed with }" is false. The same message wording is used for a { that really is unclosed ({1, 2 at the end of the file, in capsule_scopes.cpp and bytecode_registry.cpp), so a reader is sent looking for a missing brace that is there. Other bad shapes are described truthfully, not as unclosed: display(1 2) says "was given something it could not read to the end of", and {1,,2} says "there is no value here to work with".

## Variants

Same false "never closed with }" sentence: list<number> a = {1 2}; display({1 2}); display({1, 2 3}); a = {1; 2}; a = {1 2 } (space before }); a = {{1, 2} {3}} (nested lists); map/index m = {"a": 1 "b": 2}, which gets the map form "this map was opened with { and never closed with } -- ... entries are separated by commas". Different but truthful: {1,,2} gives "there is no value here to work with". Correct: {"one", "two"} and {"one" , "two"} both display {"one", "two"}; {1 + 2} is accepted. A really unclosed {1, 2 / {1, 2) / {1, 2] swallows the next statement and blames satellite.return; that is the known CONTAINERS.md entry, not this one. Run time versus checker: every expression-shape error is refused only when its line runs. display(1 2), display("a" "b"), display((1 2)), n = 1 2 and a[1 2] all print "before" first (and n = 1 2 gives only the one-line [satellite] form). So the hunter's "only at run time" point is general, not specific to lists.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/expression.cpp:1133 (list; sentence at 1137-1138), and expression.cpp:998-1000 (map)

In the list-literal loop in expression.cpp, after each item it checks the next code. A comma goes on to the next item and a } ends the list. Any other code falls through to the branch commented "THE UNCLOSED LIST", which refuses with "this list was opened with { and never closed with }" at brace_at. So a second item standing where a comma should be (a string, a number, a {, or a ;) is reported as an unclosed list. map_literal's final refusal has the same fall-through for a missing comma or a missing colon.

## Notes

- Why the skeptic kept it: Reproduced exactly. When two list items have no comma between them, the refusal says the list "was opened with { and never closed with }", but the same line closes it with }. That first clause is false, which fits the task's rule "a refusal that ... says something false". The second clause ("items are separated by commas") does point at the real mistake. The same thing happens for a map with a missing comma, for nested lists, and for display({1 2}). None of the known lists has this. The nearest entry, CONTAINERS.md "a { never closed on its line swallows the next statement", is about a { that really is unclosed. Nothing in DESIGN.md or the list help rules this wording in. One caution: check.sh:802 greps for "this map was opened with { and never closed" on a map with a missing comma. That row checks that the line is refused at all (the U+0700 lexing case). It is not an author ruling that the sentence is right, but a fix must update it. I do not back the hunter's second claim, that this should come from the checker before anything runs. Every shape error inside an expression is refused only at run time: display(1 2), display("a" "b"), n = 1 2 and a[1 2] all print "before" first. So that part is not specific to lists, and I am not confirming it as part of this finding.
- Nearest known entry: none (nearest, and different: CONTAINERS.md "Found and not fixed": a { never closed on its line swallows the next statement. Note check.sh:802 pins the map form of this sentence for a missing-comma map, to show that the line is refused at all)
- Merged: Single finding.
