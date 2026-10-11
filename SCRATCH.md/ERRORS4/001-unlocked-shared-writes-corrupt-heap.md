# 001 -- Without .lock(), two threads writing one shared object field (string, list, big number) or appending to one file take no hold at all: glibc heap corruption (134), segfault (139) or a false S999, and the appended file is left empty

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** threads  
**Kind:** crash  
**Severity:** high

## What happens

Two threads writing one string, list or big-number field of a shared object without .lock() make satl abort with glibc heap corruption (134), segfault (139) or report a false S999 OUT_OF_MEMORY.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit box()
{
    satellite.protected
    {
        satellite.variable.string v = ""
    }
    satellite.public
    {
        satellite.capsule call_put()
        {
            v = v + "d"
        }
    }
}

satellite.capsule work(box b)
{
    satellite.variable.number i = 0
    satellite.statement.while(i < 200)
    {
        b.call_put()
        i = i + 1
    }
}

satellite.capsule satellite.main()
{
    box b()
    satellite.variable.thread t1 = satellite.thread.new(work(b))
    satellite.variable.thread t2 = satellite.thread.new(work(b))
    t1.start()
    t2.start()
    t1.join()
    t2.join()
    satellite.console.display("done")
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is saved as rs200.satl. The variants race_list.satl, race_bignum.satl and race_number.satl are in the same folder, and so are the locked forms rs_lock, rl_lock and rb_lock.

## What satl does

```
free(): unaligned chunk detected in tcache 2
free(): double free detected in tcache 2
timeout: the monitored command dumped core

exit 134
```

## What it should do

'done', exit 0. The field's final value may be wrong through lost updates, as the number field shows, but satl must not abort, segfault or claim it is out of memory.

THREADS.md T2 says that with no lock turned on, 'Nothing is ever locked for it, and threads share it freely'. Lost updates are the program's to avoid, and THREADS.md measures them for the number field (12,528 of 20,000). No ruling says the interpreter may corrupt its heap. T1 review item 1 fixed an abort from two threads writing a string field as a bug. The race_bignum S999 is a refusal that says something false: the program uses almost no memory.

## Variants

String field (v = v + "d"): 6 of 6 runs abort (double free, unaligned chunk, or tcache_thread_shutdown) at 200 or 2000 iterations. List field (v.append(1)): 3 of 3 abort, including 'Fatal glibc error: malloc.c:2599 (sysmalloc): assertion failed' and 'double free or corruption (!prev)'. Big-number field: S999 OUT_OF_MEMORY 'on a thread the program started' (exit 48) in 3 of 5 runs, abort once, segfault (139) once. Small-number field: done, exit 0, 3 of 3. Every form with b.lock() added prints done, exit 0. The list case is half-mentioned in THREADS.md T2 ('not run: it can crash satl'). Nothing is said for strings or big numbers.

## Where it comes from

satellite/satellite_object/object_lock.hpp:16-21 (a lock nobody turned on takes nothing: 'the cost is one relaxed load per statement')

When an object's lock is off, a field write takes no hold at all. Two OS threads then assign the same field's heap-owning value (std::string, the list's vector, a big-number buffer) at the same moment, and the old buffer is freed twice or read while freed. A small number is stored inline, so it only loses updates.

## Notes

- Why the skeptic kept it: Reproduced: race_string aborts 3/3 (and 3/3 at 200 iterations), race_list aborts 3/3, race_bignum gives S999 twice, abort once and segfault once, and race_number prints done 3/3. With b.lock() all three print done, exit 0. None of the known lists or salvage.json's confirmed entries has a thread or race crash. THREADS.md is not a known list and has no ruling that satl may crash. It only mentions in passing, for lists, that 'it can crash satl'. Its own T1 review item 1 treated 'two threads writing a string field, satl aborted' as a bug and fixed it (for join answers), so an abort is not by design.
- Nearest known entry: none. THREADS.md T2 only notes, for lists, 'it can crash satl'. It is not a known list and not a ruling.
- Merged: One cause: an object or file lock that is off takes nothing (object_lock.hpp; ObjectHold at expression.cpp:534), so two threads change heap-owning state at the same time. One fix: a memory-safety hold that is always on, separate from the author's lock.

### Also seen as: Two threads appending to one file without f.lock() make satl abort (heap corruption, 134) every run, and the file is left empty

```satellite
satellite.include(satellite)

satellite.capsule writer(satellite.variable.file f, satellite.variable.string who)
{
    satellite.variable.number i = 0
    satellite.statement.while(i < 50)
    {
        f.append(who + " " + i)
        i = i + 1
    }
}

satellite.capsule satellite.main()
{
    satellite.variable.file f = satellite.file.new("/tmp/claude-1000/-home-madness-code-satl/b70f905b-dca1-4f92-bbad-f66d8ce80b78/scratchpad/verify2/threads-2-0/files/race_file.se")
    satellite.variable.thread a = satellite.thread.new(writer(f, "alpha"))
    satellite.variable.thread b = satellite.thread.new(writer(f, "bravo"))
    a.start()
    b.start()
    a.join()
    b.join()
    satellite.console.display(f.size)
    satellite.console.display(f.close())
    satellite.return(satellite)
}
```

```
corrupted double-linked list
timeout: the monitored command dumped core

exit 134
```
