# 084 -- A thread joining itself is refused as S110 LINE_NOT_UNDERSTOOD, not S728 WAIT_NEVER_ENDS

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** threads  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A thread joining itself is refused as S110 LINE_NOT_UNDERSTOOD ('spelled in a shape the language does not have a meaning for', 'in worker'), not S728 WAIT_NEVER_ENDS.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit crew()
{
    satellite.protected
    {
        satellite.variable.thread worker = satellite.thread.new(idle())
    }
    satellite.public
    {
        satellite.capsule call_wait_for_me()
        {
            worker.join()
        }
        satellite.capsule call_begin()
        {
            worker = satellite.thread.new(call_wait_for_me())
            worker.start()
        }
        satellite.capsule call_finish()
        {
            worker.join()
        }
    }
}

satellite.capsule idle()
{
    satellite.variable.number i = 0
}

satellite.capsule satellite.main()
{
    crew c()
    c.call_begin()
    c.call_finish()
    satellite.console.display("after")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is saved as self_join.satl.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S110: LINE_NOT_UNDERSTOOD
satl(run): in worker, worker.join() is this thread waiting for itself, and that
would never end

directory: self_join.satl:13
syntax: worker.join()
        /\

satellite has no scenario for this line -- it is spelled in a shape the language
does not have a meaning for.
machine code 13 satl_line_not_understood -- satl exits with this.

--------------------------------------------------------------------------------

exit 13
```

## What it should do

S728 WAIT_NEVER_ENDS (exit 63), the code for 'this wait would never end', with the capsule (call_wait_for_me) as its context.

THREADS.md says 'A circle of threads waiting for each other, through joins or locks, is caught as S728', and a thread joining itself is the one-thread circle. The source comment at thread_calls.cpp:193 calls it 003's S1407, its smallest case, and S728's paragraph names S1407. The S110 paragraph is false for a well-formed line.

## Variants

The two-thread join circle (join_circle.satl) gives S728, exit 63. Its context also names a thread variable ('in second', 'in first') rather than the capsule, so 'in <thread variable>' is common to both thread reports.

## Where it comes from

satellite/bytecode/thread_calls.cpp:193-198 (context.refuse(satl_line_not_understood, ... "is this thread waiting for itself" ...))

The self-join check refuses with the code satl_line_not_understood where wait_never_ends was meant. The comment right above it names the S728/S1407 family.

## Notes

- Why the skeptic kept it: Reproduced. A thread joining itself gets S110 LINE_NOT_UNDERSTOOD (exit 13), whose paragraph ('spelled in a shape the language does not have a meaning for') is false for a well-formed worker.join(). The source comment calls it '003's S1407, its smallest case' of the circle caught as S728, and S728's own paragraph cites 003's S1407. The context says 'in worker', the thread variable, not the capsule call_wait_for_me. Not in any known list.
- Nearest known entry: none
- Merged: Single finding.
