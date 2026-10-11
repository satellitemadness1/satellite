# 014 -- satellite.access shows a string (a key, or a plain string name's value) without escaping " or \, and shows a tab or newline as \x09 / \x0a, a form satl refuses, so different strings look identical

**Found:** 2026-09-26, satl 004 revision 08 build 0099 (/home/madness/code/cxx/satellite/build/satl), by running ordinary programs against it.  
**Area:** maps and nesting, strings  
**Kind:** wrong-answer  
**Severity:** low

## What happens

satellite.access shows string keys (and a string name's value) unescaped, and a tab as \x09, a form satl refuses.

## Program

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.container.map<satellite.variable.string, satellite.variable.number> m = {"a\"b": 1, "t\tx": 2}
    satellite.console.display(m)
    satellite.access(m)
    satellite.variable.string s = "q\"r"
    satellite.access(s)
    satellite.return(satellite)
}
```

Run it with `satl program.satl`.

## What satl does

```
{"a\"b": 1, "t\tx": 2}
m is a map (string -> number), 2 keys
  value         {"a\"b": 1, "t\tx": 2}
  m["key"]      a number, "key" a string, one of "a"b", "t\x09x"
  m["key"] = 1  puts a number under "key"
  m.size        how many keys it holds
  m.keys        the keys of it, as a list
s is a string
  value  "q"r"

exit 0
```

## What it should do

one of "a\"b", "t\tx" and a string value shown as "q\"r", matching the value line and display.

satellite.access help: it shows 'the way to reach one item', 'written as a program writes it'; access_words.hpp says value_shown is 'what display shows'. The same keys are escaped correctly on the value line; writing a key as the key list shows it (m["t\x09x"]) is refused: 'satl(check): \x is not an escape satellite knows -- the six are \" \\ \n \t \r and \'' (exit 13), and "a"b" is not a string literal at all.

## Variants

Refused when copied back: display(m["t\x09x"]) -> S110 '\x is not an escape satellite knows' (exit 13). Correctly escaped: display(m) and access's own 'value' line for the map.

## Where it comes from

/home/madness/code/cxx/satellite/satellite/bytecode/access_words.cpp:172-176 (as_text) and :200-214 (value_shown -> shown)

as_text wraps a string's raw display text in quotes without escaping " or \, and value_shown then passes it through shown(), which renders control characters as \xNN rather than satellite's own escapes (\t \n \r). The map 'value' line comes from a different formatter that escapes correctly.

## Notes

- Why the skeptic kept it: Reproduced. satellite.access on a map whose keys hold a quote and a tab shows them in its 'one of' list as "a"b" and "t\x09x", while the value line two rows above shows the same keys correctly escaped as "a\"b" and "t\tx" (as display does). A string name holding a"b is shown as value "q"r". The \x09 form is refused by satl itself before the run ('\x is not an escape satellite knows'). The access help says fill lines are 'written as a program writes it' and the value is 'what display shows'; the key list and the string value line are neither. Not in any known list.
- Nearest known entry: none
- Merged: Same code and one fix: as_text and value_shown -> shown() in access_words.cpp:172-214.

### Also seen as: satellite.access shows a plain string's value unescaped, and a new line as \x0a (a form satl refuses), so different strings look identical

```satellite
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string one = "a\n"
    satellite.variable.string two = "a\\x0a"
    satellite.access(one)
    satellite.access(two)
    satellite.console.display(one == two)
    satellite.return(satellite)
}
```

```
one is a string
  value  "a\x0a"
two is a string
  value  "a\x0a"
false

exit 0
```
