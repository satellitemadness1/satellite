# 021 -- satellite.thread.new(list[i].method()) and thread.new(map[key].method()) are refused with a false S721 saying the container "names a capsule and does not call it"

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** threads  
**Kind:** valid-program-refused  
**Severity:** medium

## What happens

satellite.thread.new(list[i].method()) or thread.new(map[key].method()) is refused with a false S721: it says the container 'names a capsule and does not call it -- write all()'.

## Program

```satellite
satellite.include(satellite)

satellite.spacesuit counter()
{
    satellite.public
    {
        satellite.capsule call_seven()
        {
            satellite.return(7)
        }
    }
}

satellite.capsule satellite.main()
{
    counter a()
    satellite.container.list<counter> all = {a}
    satellite.variable.thread t = satellite.thread.new(all[1].call_seven())
    t.start()
    satellite.console.display(t.join())
    satellite.return(satellite)
}
```

Run it with `satl program.satl`. Setup: The program is saved as nl_min.satl.

## What satl does

```
--------------------------------------------------------------------------------
SATELLITE CRITICAL ERROR REPORT
--------------------------------------------------------------------------------
S721: THREAD_NEEDS_A_CAPSULE_CALL
satl(check): in satellite.main, satellite.thread.new(all) names a capsule and
does not call it -- write all(), with what it takes inside the brackets

directory: nl_min.satl:18
syntax: satellite.variable.thread t = satellite.thread.new(all[1].call_seven())
        /\

satellite.thread.new runs a capsule of your own on a thread, so what goes inside
it is a CALL -- my_capsule() or my_capsule(x). Its arguments are worked out at
new; the capsule does not begin until start().
machine code 56 thread_needs_a_capsule_call -- satl exits with this.

--------------------------------------------------------------------------------

exit 56
```

## What it should do

7, exit 0, as with thread.new(a.call_seven()). At the least, a true refusal: 'all' is a list, not a capsule, and 'write all()' is wrong advice.

THREADS.md T1 says 'a spacesuit's capsule can run on a thread, as satellite.thread.new(obj.call_x())', and a list's item is such an object. all[1].call_add(7) runs when called directly on the line before, and the checker already accepts units[i].call_x() elsewhere (program_check.cpp:1304-1316). The refusal names the wrong thing and gives a false fix.

## Variants

Map item: thread.new(named["a"].call_add(7)) gives the same S721, naming 'named' and suggesting 'write named()'. Named object: thread.new(a.call_add(7)) prints 7 and 14, exit 0. The hunter's longer program also runs display(all[1].call_add(7)) directly first, which prints nothing because the checker refuses before anything runs.

## Where it comes from

satellite/bytecode/program_check.cpp:1336-1345 (dotted_names_at stops at '[', then 'code_at(row, past) != left_parenthesis_token' gives S721 with the partial name)

The thread.new check collects only a dotted name chain, then demands '(' straight after it. For all[1].call_seven() the chain is just 'all' and the next token is '[', so it reports that 'all' names a capsule without calling it. Nothing handles an indexed item before the member call, which the ordinary call path does handle.

## Notes

- Why the skeptic kept it: Reproduced for both list and map items. The checker reads only the dotted names before the '[' ('all'), finds no '(' after them, and gives S721 saying the list 'names a capsule' and advising 'write all()'. Both claims are false. The same call runs directly, and thread.new(a.call_add(7)) on the same object prints 7 and 14. No known list covers it.
- Nearest known entry: none
- Merged: Single finding.
