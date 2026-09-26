# 032 -- Each map removal is O(n) (take_entry_out renumbers every row of the hash, and .remove(key) scans the entries), so emptying a map is quadratic: 20,000 remove_last() take 3.5-4 s, against 0.2 s for a list

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** performance  
**Severity:** medium

## What happens

Removing from a map costs O(n) per call, so emptying one is quadratic: 20,000 remove_last() take 3.5-4 s (a list's take 0.2 s).

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.number, satellite.variable.number> m
    satellite.statement.for(satellite.variable.number i = 1; i <= 20000; i++)
    {
        m[i] = i
    }
    satellite.statement.for(satellite.variable.number i = 1; i <= 20000; i++)
    {
        m.remove_last()
    }
    satellite.console.display(m.size)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: none. Saved at p2_remove_last_20000.satl. Size variants p2_{remove_last,remove_first,fill}_{5000,10000,20000}.satl and the list comparison p2_list.satl are in the same folder.

## What satl does

```
0
(wall time 3.54 s; 3.96 s on the first run; the fill-and-read-back program at 20,000 takes 0.19 s)

exit 0
```

## What it should do

Each removal costs about what an insert does, so emptying a 20,000-key map takes a fraction of a second, as filling it does and as a list's remove_last does (0.2 s).

satellite.container.map help: "It is a python dict rather than a std::map ... finding a key costs one hash." Python's dict.popitem() and del d[k] run in amortised constant time.

## Variants

remove_last: 5k 0.27 s, 10k 0.98 s, 20k 3.96 s (x4 per doubling). remove_first: 5k 0.59 s, 10k 1.72 s, 20k 5.91 s, 40k 24.3 s (perf2.satl with SATL_TIMEOUT=60). Filling plus reading every key: 5k 0.12 s, 10k 0.16 s, 20k 0.19 s. list<number> remove_last at 20k: 0.2 s. The hunter measured .remove(i) at 10k 1.41 s and 20k 5.73 s; it goes through the same take_entry_out after a linear scan.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/container_calls.cpp:160-168 (take_entry_out); callers at :440-441 (remove_first/remove_last) and :450-456 (.remove(key) scans entries linearly instead of using `where`)

take_entry_out erases the key from `where`, erases the entry from the `entries` vector, and then loops over EVERY row of the `where` hash table, decrementing each position after the hole. It does this even for the last entry, where no position moves. .remove(key) first finds the entry by scanning `entries` one by one, comparing key names, and never looks it up in `where`. Both make every removal O(n), so emptying a map is O(n^2). The method also returns *home, which may copy the whole map on each call.

## Notes

- Why the skeptic kept it: I reproduced it, and the time grows quadratically. Emptying a map<number, number> with remove_last() took 0.27 s for 5,000 keys, 0.98 s for 10,000 and 3.96 s for 20,000 (3.54 s on a re-run). remove_first() took 0.59, 1.72 and 5.91 s; 40,000 of them (perf2.satl) took 24.3 s. Filling the map and reading every key back took 0.12, 0.16 and 0.19 s for the same sizes. A list<number> emptied with remove_last() at 20,000 took 0.2 s. The map help says a map is "a python dict rather than a std::map" and that "finding a key costs one hash", and Python's popitem and del run in constant time. The cause is that each removal walks the whole lookup table to renumber positions, even when it removes the last entry, and .remove(key) also scans the entries one by one before that. ERROR.md #29 (to_text quadratic) is the only quadratic entry in the known lists, and it is unrelated. DESIGN.md has nothing about map removal.
- Nearest known entry: none (ERROR.md #29 is to_text, unrelated)
- Merged: Single finding.
