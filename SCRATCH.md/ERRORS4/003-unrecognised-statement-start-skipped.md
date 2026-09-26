# 003 -- A statement that starts with a code that cannot start one (a sign, a literal, !, /*, )) is stepped over one code at a time and accepted: a continuation line starting with - or + is silently dropped (100 / - 30 holds 100), -n = 9 runs as n = 9, and a /* ... */ block runs its contents

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** integers, syntax and layout  
**Kind:** wrong-answer  
**Severity:** high

## What happens

A line that is only a value or starts with an operator is skipped without a word, so a declaration continued with a leading - / + silently keeps only its first line (100\n - 30 gives 100).

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number total = 100
        - 30
    satellite.console.display(total)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
100

exit 0
```

## What it should do

Either 70, with the leading operator joining the line above as a trailing operator already does under A4, or a refusal before anything runs saying the line starts with - and has nothing to work on. Never a silent 100.

A4 ruling (NEW_ERROR_LIST.md): 'accept anything valid across any number of lines'. `100 -\n 30` gives 70, and a bare-name line (`x + 1`) is refused by the checker. A line whose value does nothing, and which changes what the line above means, is invalid and accepted silently.

## Variants

`100\n - 30\n + 5` gives 100. A line holding only `2 + 3` or `- 2` between two displays prints a, b, exit 0. `100 -\n 30` gives 70 (trailing operator joined). `100\n .add(5)` gives 105 (leading dot joined). `x + 1` alone is refused with S110 by satl(check).

## Where it comes from

satellite/bytecode/program_check.cpp:2242-2249 (the 'ONE CODE, NOT THE LINE' fallthrough: ++at; return success). run_statements steps over the same code.

The checker's statement dispatcher falls through to a default that steps over any code that cannot start a statement (a number literal, a minus sign) and returns success, and the walker does the same. join_statements_across_lines joins a trailing operator and a leading '.', but not a leading operator, so the `- 30` line becomes its own statement and is silently skipped.

## Notes

- Why the skeptic kept it: Reproduced. A line that holds only a value or starts with an operator (`- 30`, `2 + 3`, `- 2`) is skipped without a word, both by the checker and by the walker. A continued declaration therefore silently loses its tail: 100 is printed where 70 was written. A4 joins a TRAILING operator (100 -\n 30 gives 70) and a leading `.` (100\n .add(5) gives 105), but not a leading operator, and nothing refuses the stray line. By contrast `x + 1` on its own line is refused before anything runs. None of the known lists has it: A5's 'stepped over' is about characters with no meaning, and that item is built.
- Nearest known entry: none. NEW_ERROR_LIST A4 lists what is joined across lines (a trailing operator or comma, a leading .), and a leading operator is not among them.
- Merged: All four come from the check_statement fallback at program_check.cpp:2242-2249 ("ONE CODE, NOT THE LINE ... ++at; return success") and its twin in run_statements. One fix: refuse (or join) a statement whose first code cannot start one.

### Also seen as: A line that holds only an operator or literal (`+ 2`, `2`, `"text"`, `1 + 2`, `== 3`) is accepted and does nothing, so a leading-operator continuation silently drops terms: total = 1 / + 2 / + 3 holds 1

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 1
    + 2
    satellite.console.display(n)
    satellite.return(satellite)
}
```

```
1

exit 0
```

### Also seen as: A stray value or sign at the start of a statement is silently dropped: `-n = 9`, `2 n = 9`, `"x" n = 9`, `!n = 9` all run as `n = 9`, and `-satellite.console.display(n)` runs the display

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.number n = 5
    -n = 9
    satellite.console.display(n)
    satellite.return(satellite)
}
```

```
9

exit 0
```

### Also seen as: Inside a capsule, a line that is only signs, brackets or a bare value (/*, */, + 3, 5, "text", )) is accepted silently, so a C-style /* ... */ block 'comments out' nothing and the code inside it runs

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("kept")
    /*
    satellite.console.display("commented out")
    */
    satellite.return(satellite)
}
```

```
kept
commented out

exit 0
```
