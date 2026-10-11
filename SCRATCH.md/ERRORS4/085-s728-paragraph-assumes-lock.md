# 085 -- S728 WAIT_NEVER_ENDS for a circle made only of joins explains it as "the object's lock it needs is held by a thread", though no lock is involved

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** threads  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

S728 WAIT_NEVER_ENDS for a circle made only of joins explains it as 'the object's lock it needs is held by a thread', and no lock is involved.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit pair()
{
    satellite.protected
    {
        satellite.variable.thread first = satellite.thread.new(idle())
        satellite.variable.thread second = satellite.thread.new(idle())
    }
    satellite.public
    {
        satellite.capsule call_wait_second()
        {
            satellite.variable.number i = 0
            satellite.statement.while(i < 20000)
            {
                i = i + 1
            }
            second.join()
        }
        satellite.capsule call_wait_first()
        {
            satellite.variable.number i = 0
            satellite.statement.while(i < 20000)
            {
                i = i + 1
            }
            first.join()
        }
        satellite.capsule call_begin()
        {
            first = satellite.thread.new(call_wait_second())
            second = satellite.thread.new(call_wait_first())
            first.start()
            second.start()
        }
        satellite.capsule call_finish()
        {
            first.join()
        }
    }
}

satellite.capsule idle()
{
    satellite.variable.number i = 0
}

satellite.capsule satellite.main()
{
    pair p()
    p.call_begin()
    p.call_finish()
    satellite.console.display("after")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is saved as join_circle.satl.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S728: WAIT_NEVER_ENDS
satl(run): in first, first.join() would never return -- call_wait_second is
waiting, through locks and joins, for this thread

directory: join_circle.satl:28
syntax: first.join()
        /\

this line would wait forever: the object's lock it needs is held by a thread
that is itself waiting -- through locks and joins -- for this one. Two threads
each waiting for the other never end, so satellite stops this line instead of
the program freezing (003's S1407 and S1408). A line that holds an object's lock
-- writing it, or reading it -- should not wait for a thread that needs the same
object's lock.
machine code 63 wait_never_ends -- satl exits with this.

--------------------------------------------------------------------------------

exit 63
```

## What it should do

S728 and exit 63 as now, with a paragraph that fits a join circle, for example 'this thread is waiting for a thread that is itself waiting for this one'.

No object is locked in the program, so 'the object's lock it needs is held by a thread' is false for this line, and the advice about lines holding an object's lock does not apply. THREADS.md says S728 covers circles 'through joins or locks', but the paragraph explains only locks.

## Variants

Run 1 named line 19 ('in second, second.join()') and then printed a notice after the critical report: '[satellite] S724 THREAD_ALREADY_JOINED: this thread had already been joined, so this join gave back the same answer the first one did ... (join_circle.satl:28 first.join())'. Run 2 named line 28 ('in first, first.join()') and printed no notice. Which line is blamed depends on timing. The context names a thread variable ('in first'), not the capsule.

## Where it comes from

satellite/machine/s_codes.hpp:296-302 (the S728 paragraph)

S728 has a single fixed paragraph, written for the lock circle (tests/threads_lock_circle.satl). The join-only circle reaches the same code through start_waiting_for, and nothing words the paragraph for a wait that involves no lock.

## Notes

- Why the skeptic kept it: Reproduced in 2 of 2 runs. A circle made only of joins, with no .lock() anywhere, gets S728 with the paragraph 'the object's lock it needs is held by a thread ...' and advice about 'a line that holds an object's lock'. Neither applies. The code and exit are right, and the paragraph is written for the lock case only. Not in any known list. Separate from #5, which is a wrong code, while this is a wrong paragraph under the right code.
- Nearest known entry: none
- Merged: Single finding.
