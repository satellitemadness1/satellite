# 035 -- A comment line after a capsule's last self-call stops it being a tail call: memory grows with depth (1.1 GB at 300,000 levels, 10.9 GB at 3,000,000; 18 MB without the comment)

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** control flow and capsules  
**Kind:** performance  
**Severity:** medium

## What happens

A comment line after a capsule's last self-call stops it being a tail call: memory grows with depth (1.1 GB at 300,000 levels, 10.9 GB at 3,000,000; 18 MB without the comment).

## Program

```satellite
satellite.include(satellite)

satellite.capsule down(satellite.variable.number n)
{
    satellite.statement.if(n == 0)
    {
        satellite.console.display("bottom")
        satellite.return()
    }
    down(n - 1)
    // c
}

satellite.capsule satellite.main()
{
    down(300000)
    satellite.console.display("back")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none; saved as t0_comment.satl (the plain form is t0_plain.satl); memory measured with /usr/bin/time -f '%e s %M KB' around the runner

## What satl does

```
bottom
back
(/usr/bin/time: 1.33 s, 1107296 KB peak; the same program without the '// c' line: 0.44 s, 18664 KB)

exit 0
```

## What it should do

The same memory as the version without the comment, about 18 MB at any depth, because the self-call is still the last thing the capsule does.

program_walk.cpp:423-431 records the author's 2026-09-22 ruling: a capsule whose last act is to call itself 'hands its arguments back to run_site ... the recursion never stops and never grows', and 'LAST MEANS NOTHING OF THIS CAPSULE RUNS AFTER IT'. A comment is not a statement and runs nothing. An inline comment and a blank line in the same place keep the tail call.

## Variants

A comment line then satellite.return(): same growth (1,107,552 KB at 300k). Inline '// c' after the call: 18,496 KB, fine. A blank line: 18,620 KB, fine. At 3,000,000 levels the comment form ran twice and exited 0 at 10.9 GB peak (12.9 s and 21.7 s); the reported segfault did not reproduce, but at this rate a few million more levels exceed any machine's memory.

## Where it comes from

satellite/bytecode/program_walk.cpp:551-584 (find_the_last_statements)

find_the_last_statements() skips only token::line_end_token between statements (line 556). A comment on its own line is a token::comment_token, so it is taken as a statement: 'last = here' records the comment's position as the block's last statement, and mark_last marks the comment instead of down(n - 1). is_last_in() then says the self-call is not last, and it runs as an ordinary C++-stack call. An inline comment is swallowed by past_the_statement, so it does no harm.

## Notes

- Why the skeptic kept it: I reproduced the main claim but not the crash. Adding a comment on its own line after a capsule's last self-call stops satl treating that call as a tail call. At 300,000 levels the plain version peaks at 18,664 KB (0.44 s). The comment version peaks at 1,107,296 KB (1.33 s), about 3.6 KB per level. A comment line followed by satellite.return() behaves the same (1,107,552 KB). An inline comment (down(n - 1) // c) and a blank line are both fine, at about 18.5 MB. At 3,000,000 levels I did NOT get the reported segfault: two runs with SATL_TIMEOUT=60 printed bottom/back and exited 0, peaking at 10.9 GB after 12.9 s and 21.7 s. The plain version stays near 19 MB at that depth. The hunter's 139 at 6.5 GB may depend on memory pressure. Either way, a comment turns constant-memory recursion into memory that grows with depth, which is two equivalent forms disagreeing plus runaway memory. The code comment at program_walk.cpp:423-431 records the author's ruling (2026-09-22) that a last-line self-call 'is a loop, not a deeper frame' and that 'LAST MEANS NOTHING OF THIS CAPSULE RUNS AFTER IT'. A comment runs nothing. The ruling that a middle self-call may crash does not cover this case. ERROR.md's accepted ~27,000 depth is about the C++ stack, and NEW_ERROR_LIST A8 is about middle recursion. Neither mentions comments, and none of the known lists or already-confirmed entries do either.
- Nearest known entry: none (ERROR.md's '~27,000 is enough' ruling and NEW_ERROR_LIST A8 cover middle-of-capsule recursion, not a comment defeating the tail-call rule)
- Merged: Same pattern as the if/else comment group (a comment_token is not stepped over the way a line end is), but it is in find_the_last_statements, with its own fix and symptom, so it is kept separate.
