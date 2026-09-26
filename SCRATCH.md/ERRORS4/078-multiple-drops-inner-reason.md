# 078 -- A multiple refusing a container whose items are the wrong kind says only "it holds a map/list, and this name takes ...", dropping the real reason

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting  
**Kind:** misleading-refusal  
**Severity:** low

## What happens

A multiple refusing a container whose items are the wrong kind says 'it holds a map/list, and this name takes ... satellite.container.map/list', dropping the real reason.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.multiple<satellite.variable.number, satellite.container.map<satellite.variable.string, satellite.variable.number>> m = {"a": "b"}
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
[satellite] satl(run): m was declared satellite.container.multiple, and it holds an index, and this name takes satellite.variable.number or satellite.container.map (machine_code: 27 types_do_not_meet)

exit 27
```

## What it should do

A refusal naming the real mismatch, e.g. 'it holds a map, and a value of it does not fit: it holds a string' -- as the plain map<string, map<string, number>> write already says -- and the word map, not index.

As written the sentence says the value is a map and the name takes a map, so it states no reason and reads as the machine arguing with itself -- the very thing type_shape.cpp:64-69's comment says the refusal wording is meant to avoid. Map help: 'a refusal says the word you wrote'.

## Variants

Same wrong sentence: the assignment form (m = 1 then m = {"a": "b"}); a list arm, multiple<number, list<number>> m = 1 then m = {"b"} -> 'it holds a list, and this name takes satellite.variable.number or satellite.container.list'; per the hunter, xs[4] = {"a": "b"} in a list of multiples. Correct for contrast: map<string, map<string, number>> m; m["x"] = {"a": "b"} -> 'the value does not fit: a value of it does not fit: it holds a string'. (The one-line run-time format itself is the known ERRORS2 #7 and not claimed.)

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/type_shape.hpp:153-167 (any_of_fits)

any_of_fits tries value_fits against each arm with a throwaway `ignored` reason, and when none fits it writes only 'it holds <kind_name>, and this name takes <arms>'. When the value's kind matches an arm but an item inside it fails, the inner reason from that arm is discarded, and kind_name() says 'index' regardless of the word written.

## Notes

- Why the skeptic kept it: Reproduced. A multiple<number, map<string, number>> given {"a": "b"} is refused with 'it holds an index, and this name takes satellite.variable.number or satellite.container.map' -- the sentence names a kind the name does take, so it gives no reason; the real reason (a value of the map is a string where the map holds numbers) is dropped. Same for a list arm: multiple<number, list<number>> m = 1 then m = {"b"} says 'it holds a list, and this name takes satellite.variable.number or satellite.container.list'. A plain map<string, map<string, number>> write gives the right reason ('a value of it does not fit: it holds a string'). It also says 'index' where the declaration wrote map, against the map help's 'a refusal says the word you wrote'. Not in any known list; lists#7's multiple case (x1F) is a real kind mismatch and reads correctly.
- Nearest known entry: none
- Merged: Single finding (any_of_fits).
