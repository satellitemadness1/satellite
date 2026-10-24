# ERROR_BLOCK_PLAN — satellite.error(), the block that runs when the block above it fails

Written 2026-10-23 from the author's messages that day. Snapshot before the first edit:
`~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-23-before-error-block` (BUILD 0097).
This is not SATELLITE_ERROR.md, which is how satl reports an error; this is how a program catches one.

---

## 1. The author's design

> *"after the end of each block, we allow the user to type this: satellite.error() { // this code runs
> if an error has occured in the block above the line "satellite.error()" ... every block, before the
> block runs, we check for the syntax "satellite.error()" to see if that syntax exists directly outside
> the block, and this can even work on satellite.main"*

> *"the only stipulation is that the next line, no matter how much blank lines are there, as long as the
> next command is "satellite.error()" then it catches the errors in that block"*

> *"let's just do any error runs this code, make it any error will run certain code, and we have certain
> errors where they cannot be caught, like... satellite.return(satellite) missing, or satellite.main
> missing, these errors cannot be caught"*

```
satellite.capsule satellite.main()
{
    satellite.console.display(10 / 0)
    satellite.return(satellite)
}
satellite.error()
{
    satellite.console.display("that didn't work")
}
```

---

## 2. The rules

1. **`satellite.error()` written right after a block's `}` guards that block.** Blank lines and
   comments may stand between them. The brackets stay empty: any error runs it.
2. **The blocks it may follow:** a capsule's (satellite.main's, a namespace's, a spacesuit's, and a
   constructor), and a statement's: if (with its else chain), while and for. An if's or a loop's
   condition is part of what it guards.
3. **An error stops the block at its line, and the error block runs instead.** The block never resumes;
   the program carries on after the error block. Nothing is shown and nothing goes to satellite.log:
   the program handled it.
4. **An error travels up to the nearest error block**, through every capsule between: a capsule called
   from a guarded block is guarded too, unless it has an error block of its own, which runs first.
5. **What cannot be caught:**
   - everything the checker refuses before the run: satellite.include(satellite), satellite.main or
     main's satellite.return(satellite) missing (the author's examples), a name never declared, a
     call with the wrong number of arguments;
   - Ctrl-C (S810), a thread's `.stop()`, the display buffer overrunning (S840), and the S9xx band,
     which is satl itself in trouble (S999 is no memory);
   - satellite.return(satellite), which is the program ending, not an error.
6. **An error inside an error block** goes to the next error block out, or stops the program with its
   report, as today.
7. **What an error block can see.** A capsule's error block sees the capsule's parameters (and a
   spacesuit's fields), but not the names its body declared, because those ended with the body.
   `satellite.return(x)` there hands x back; without one the capsule hands back nothing. A statement's
   error block sees the names the statement could see.
   With `satellite.arguments.access` off, names live to the end of their capsule, so a capsule's error
   block may name what its body declared. If the error came before that line, the name is not there
   when the error block runs, just as with a name declared inside an if.
8. **The checker refuses, before the run:** something between the brackets, no brackets, no body, and
   a `satellite.error()` with no block right above it (the first line of a body, after a plain line,
   after another error block, or after a namespace's or a spacesuit's `}`). It also refuses an else
   written after an error block, deleting a parameter of a capsule that has an error block (the error
   block sees the parameters), and `satellite.error()` typed at the prompt, where a block runs as soon
   as its `}` is typed.
9. **Next to no cost.** A capsule's error block and every guarded if, while and for are found once,
   when the capsule table is built. A program with no `satellite.error` pays one bool a block statement
   and a compare or two a capsule call. A program with one pays a short search a block statement in a
   file that writes one.
10. **Nothing is caught once the run is ending.** After `satellite.return(satellite)`, a file that
    cannot be saved on the way out is reported as before (exit 44). A capsule with an error block is
    never made a loop when it calls itself last; the next call has to stay guarded.

---

## 3. The milestones

- **EB-1:** the word (`satellite.error`, `1 37`); the checker's rules; error blocks after capsules
  (all four kinds, main included); holding the report while an error block waits; the help page; and
  check.sh rows.
- **EB-2:** error blocks after if/else, while and for.

**The gate (the author's lean one):** the snapshot above, each milestone's own check.sh rows, all of
check.sh, the help examples, then ONE fresh reader of the whole diff, and the final fix.

---

## 4. Taken without the author's word

- **A handled error in main exits 0.** The program dealt with it, and Python does the same. The author
  left this open.
- **The block does not see which error happened.** The author: *"I don't know man, does it?"* Not in
  this version. Most error blocks print a sentence, use a fallback value, or tidy up, and none of those
  need it.
- **A caught error is not written to satellite.log.** A loop that catches the same error a million
  times would otherwise fill the log.

## 5. For the author

- **A thread's error.** A thread's capsule with no error block of its own fails as before: its report
  prints and goes to satellite.log. If main then joins it inside a guarded block, the join's failure is
  caught too, so satl exits 0 under a report that ends "satl exits with this". Should a thread's report
  wait for its join, and say nothing when the joiner catches it? Or is it right as it is, because the
  thread's report is the only place that says what went wrong?
