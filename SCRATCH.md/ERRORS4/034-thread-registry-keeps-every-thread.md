# 034 -- Every started thread stays in the process-wide registry until satellite.return: a joined thread keeps its answer (614 MB at 80 rounds, against 44 MB for direct calls), and a finished thread that was never joined keeps ~17 KB of stack (1 GB at 60,000)

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** threads  
**Kind:** performance  
**Severity:** medium

## What happens

Every started thread's handle and its answer are kept until the program ends: a loop that joins list answers grows without bound (614 MB at 80 rounds against 44 MB for the same calls made directly).

## Program

```satellite
satellite.include(satellite)

satellite.capsule build(satellite.variable.number n)
{
    satellite.container.list<satellite.variable.number> l = {}
    satellite.variable.number i = 0
    satellite.statement.while(i < n)
    {
        l.append(i)
        i = i + 1
    }
    satellite.return(l)
}

satellite.capsule satellite.main()
{
    satellite.variable.number total = 0
    satellite.variable.number round = 0
    satellite.statement.while(round < 80)
    {
        satellite.variable.thread t = satellite.thread.new(build(50000))
        t.start()
        t.join()
        total = total + 1
        round = round + 1
    }
    satellite.console.display(total)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is saved as ljdiscard80.satl. Also in that folder: lj40 and lj80 (list<number> got = t.join()), and ld40 (got = build(50000) called directly).

## What satl does

```
80
5.82 s maxRSS 620728 KB

exit 0
```

## What it should do

Flat memory, near the direct form's ~44 MB: once a thread has been joined and its variable is replaced, its answer is freed.

THREADS.md says join() 'answers what f handed back', so the thread form and the direct call should cost about the same memory. Memory growing linearly with the number of threads joined in an ordinary loop is runaway memory for ordinary work, and DESIGN.md §13 targets very many threads.

## Variants

lj40 (got = t.join()): 2000000, 2.89 s, 324,432 KB. lj80: 4000000, 5.62 s, 614,364 KB. ld40 (direct build): 2000000, 2.82 s, 44,008 KB. ljdiscard80 (answer not kept by the program): 620,728 KB. The hunter measured 1,508,064 KB at 200 rounds against 44,208 KB direct, and 36,576 KB when the thread answers only l.size, so the thread's working memory is freed and the answer is not. Time is the same either way.

## Where it comes from

satellite/bytecode/thread_calls.cpp:33-42 and :152 (running().threads.push_back(which) keeps every ThreadHandle until close_every_thread at :351); satellite/satellite_variable_thread/satellite_thread.hpp:60 (satelliteObject answer held on the thread)

start() pushes the thread's shared handle into the process-wide Running registry, and only close_every_thread, run at the program's end, empties it. The handle holds 'answer' (kept for a second join, S724), so each finished thread's returned list stays alive after its satellite.variable.thread is replaced.

## Notes

- Why the skeptic kept it: Reproduced. 40 rounds of a thread returning a 50,000-item list peak at 324,432 KB, and 80 rounds at 614,364 KB (about 7.3 MB per round). The direct twin at 40 rounds peaks at 44,008 KB. Discarding the answer (a plain t.join() statement, with no 'got') still grows the same way (620,728 KB at 80 rounds), so the answer is not kept by the program's variables. In the source, every started thread's handle, and with it its 'answer' field, stays in a process-wide registry until the program ends. No known list mentions it.
- Nearest known entry: none
- Merged: Same cause: start() pushes every handle into running().threads (thread_calls.cpp:152), and only close_every_thread empties it or joins/detaches the pthread.

### Also seen as: Threads that finish but are never joined keep about 17 KB of resident memory each until satellite.return: 30,000 fire-and-forget threads hold 520 MB, 60,000 hold 1 GB

```satellite
satellite.include(satellite)

satellite.capsule note(satellite.variable.number n)
{
    satellite.return(n)
}

satellite.capsule satellite.main()
{
    satellite.variable.number i = 1
    satellite.statement.while(i <= 30000)
    {
        satellite.variable.thread t = satellite.thread.new(note(i))
        t.start()
        i = i + 1
    }
    satellite.console.display("done")
    satellite.return(satellite)
}
```

```
done
1.33 s maxRSS 519604 KB

exit 0
```
