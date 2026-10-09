# 002 -- A bare block ({} or { ... }) ends the enclosing body at its }: the rest of main (satellite.return included) is skipped with exit 0, and inside a while the loop never advances (hang)

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** syntax and layout  
**Kind:** wrong-answer  
**Severity:** high

## What happens

A bare block ({}, { }, or { newline ... newline }) ends the enclosing body at its }: the rest of main (satellite.return included) is skipped with exit 0, and inside a while the loop never advances (hang).

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("one")
    {}
    satellite.console.display("two")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
one

exit 0
```

## What it should do

Either 'one' then 'two', exit 0, with a bare block running as a block, or a refusal before anything runs saying a bare block is not part of the language. It must never silently skip the rest of main and exit 0 without reaching satellite.return(satellite).

The checker accepts the program and requires main to end with satellite.return(satellite), but the run never reaches that line. The same } is treated as a block by the check (program_check.cpp steps over { and } as depth changes) and as the end of the whole body by the run. Inside a loop the skipped lines include the counter's step, so an ordinary loop becomes an endless one with no message.

## Variants

a1 '{}' alone on a line: prints only 'one', exit 0. a3 '{ }': same. a4 '{' newline '}': same. a6 '{' newline '// nothing' newline '}': same. a5 '{' newline display("inside") newline '}': prints 'one', 'inside' and never 'two', exit 0 (the hunter said this form worked; it does not). a2, the hunter's while with '{}' inside the body: prints 0 forever (1,241,087 lines in 3 s), exit 124. a7, an empty if body over two lines, is fine ('two', exit 0), so only a bare block is affected.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/program_walk.cpp:1677 (the body loop: 'if (code == token::right_brace_token) return success;'); /home/madness/code/cxx/satellite/satellite/bytecode/program_check.cpp:1680 and 2455-2462 (the checker steps over a bare { and } as depth)

The walker's body loop has no case for a { that starts a statement. It steps past it, runs the block's statements as part of the enclosing body, then meets the block's } and returns success as if the enclosing body had ended. The checker keeps a brace depth, sees nothing wrong, and checks the lines after the block, which never run.

## Notes

- Why the skeptic kept it: Reproduced, and the problem is wider than reported. ANY bare block ends the body around it, not only an empty {}. The closing } of a bare block, whether empty, one-line or multi-line, and even one with a statement inside, is taken as the closing } of the body that holds it. In main, everything after it is skipped, satellite.return(satellite) included, and satl exits 0. Inside a while, the counter never moves, so it hangs. The checker accepts the program. This contradicts the hunter's claim that p32b ran the line after a multi-line block: here a5.satl printed 'one', 'inside' and never 'two'. This does not match the known one-line '{ statement }' refusal or the 'never-closed { swallows the next statement' entry, since nothing is refused here.
- Nearest known entry: none. Not ERRORS2 #5 (a one-line '{ statement }' is refused; here nothing is refused and the multi-line form fails too). Not CONTAINERS 'a { never closed on its line swallows the next statement'.
- Merged: Single finding.
