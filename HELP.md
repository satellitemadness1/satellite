# The help lines

**One entry for every node `words.def` numbers**, in trie order, which is the
order help walks them and the order the rows sit in the file.

Each entry is three things:

- the **mark, number and path**, which are measured rather than typed;
- the **`>` line**, which is what you type to bring the entry up. Consecutive
  entries share one, because asking about a path brings up that node **and the
  words underneath it together** — `satellite.help(satellite.main)` answers for
  `1 3` and `1 3 0` at once;
- **what it is**, in a few sentences, and then **a worked line**.

**Every worked line in this file has been run.** A script pulls each one out,
wraps it in a program, and executes it against the interpreter in this tree;
140 of them run and pass. That is not a formality — writing these caught **five
statements that were plainly stated and plainly wrong**:

- `console.typed()` answers the line itself, or nothing, and not a yes-or-no.
- `floor`, `ceil`, `round` and `truncate` are number methods and refuse a float.
- `power` answers a float, so `digits` refuses its result.
- a quoted word inside `list.remove` is read as an option name, not a value.
- `satellite.returns` is optional rather than required, and no file in the tree
  uses it.

The mark comes from walking the trie with every module's handlers installed.
`H` means a handler row exists and a call to it runs today, which is **106 of
the 264**. A dot means nothing is behind it yet, which is the other **158**.

**A dot is not the same as undocumented, and that is the trap in this list.**
The front-end words — `include`, `capsule`, `main`, `return`, `statement`
and every type name — are all dotted, because the parser and the
resolver recognise them and they are never dispatched. They are the words the
language is written in. Hello world uses seven paths and exactly one of them,
`satellite.console.display`, is a handler row.

**Where a path belongs to a milestone nobody has started, the entry says which
milestone and shows no example.** Writing a worked line for
`satellite.network.https` would be inventing the language, which is the thing
help exists to stop.

The nine aliases carry no entry of their own. Each shares a node with a path
already listed, and help answers for the node.

---


## What `satellite.help()` prints

The topics that are built, one to a line, each indented one tab
and separated by a blank line.  Generated from the entries below,
so it cannot name a topic this file does not describe.

```
satellite -- the topics that are built. Ask about any of them:

    satellite.help(satellite.console)

	satellite.include     Brings a body of words into the program.

	satellite.capsule     Declares a capsule, which is what this language calls a function.

	satellite.main        Where the program starts.

	satellite.container   The things that hold other things.

	satellite.console     The terminal the program is running in, both halves of it.

	satellite.variable    Every kind of value a name can hold.

	satellite.random      Random numbers, in three grades.

	satellite.time        The clock, and waiting.

	satellite.statement   The statements that change what runs next, rather than what a value is.

	satellite.library     Where the program's own globals live, and where the machine's settings are read and written.

	satellite.return      Leaves the capsule it is written in, at once, and optionally hands a value back to whoever called it.

	satellite.bool        The two constants, true and false.

	satellite.system      The machine the program is running on, and the interpreter's own switches.

    13 topics.
```

---

## satellite

.  `1`  `satellite`
> satellite.help(satellite)

The root of the language, and the only word that is not under another
one. Every path in satellite begins here, which is why its number is `1`
and why every other number starts with a 1.

On its own it names nothing you can call. Asking help about it is how you
get the list of modules underneath, and that list is the language as it is
built today rather than as it is planned.

    satellite.help(satellite)


## include

.  `1 1`  `satellite.include`
> satellite.help(satellite.include)

Brings a body of words into the program. It is the first line of every
satellite file, and until it has run the language's own names are not in
scope, so a program without it cannot even reach `satellite.console`.

It is a declaration and not a call. It sits at the top of the file,
outside any capsule, and nothing is returned by it.

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("the language is in scope")
    }

.  `1 1 0`  `satellite.include()`
> satellite.help(satellite.include)

The bare shape, written with nothing between the parentheses. It parses,
and it includes nothing, which makes it a way of saying the line is
deliberate while the thing it will name is still being decided.

It was the shape that refused until M17 taught the parser to accept an
empty argument list here.

    satellite.include()
    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("the empty one included nothing")
    }

.  `1 1 1`  `satellite.include(satellite)`
> satellite.help(satellite.include)

Includes the language itself, and this is the line you actually type. It
puts every built-in module in scope for the rest of the file:
`satellite.console`, `satellite.container`, `satellite.variable` and the
rest.

The word `satellite` inside the parentheses is part of the path's number
rather than an argument, so nothing is evaluated and you cannot pass a
name there.

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("Hello, World!")
    }

.  `1 1 2`  `satellite.include(spaceship)`
> satellite.help(satellite.include)

Includes another satellite file, a spaceship, so its capsules can be
called from this one.

**Not built.** The path is numbered and the parser accepts it, but nothing
loads a second file yet, so a program that writes this line refuses when
it runs. M25 is the milestone that builds it.


## capsule

.  `1 2`  `satellite.capsule`
> satellite.help(satellite.capsule)

Declares a capsule, which is what this language calls a function. The
name you give it is yours, the parameters are typed, and the body is a
block in braces on the lines below.

A capsule hands a value back with `satellite.return`. One that falls off
the end of its body has simply finished, and needs no return at all.

    satellite.capsule announce(satellite.variable.string what)
    {
        satellite.console.display(what)
    }

.  `1 2 0`  `satellite.capsule()`
> satellite.help(satellite.capsule)

Calling one. You write the capsule's own name and hand it arguments in
the order it declared its parameters, so the path you type is never the
word `capsule` itself.

A capsule can call itself, and each call gets its own frame, so the
parameters of the call in progress are never the ones the caller is
holding.

    satellite.capsule double_it(satellite.variable.number n)
    {
        satellite.return(n * 2)
    }

    satellite.console.display(double_it(21))


## main

.  `1 3`  `satellite.main`
> satellite.help(satellite.main)

Where the program starts. Every program declares exactly one, written as
a capsule whose name is `satellite.main`, and running the file runs its
body from the first line to the last.

**You can also call it yourself**, from another capsule or from inside
main, and it behaves like any other capsule when you do: it gets its own
frame each time, and it counts its arguments. If it declares the argument
list, a call that hands it nothing is refused rather than run.

    satellite.include(satellite)

    satellite.library.n = 0

    satellite.capsule satellite.main()
    {
        satellite.library.n = satellite.library.n + 1
        satellite.console.display(satellite.library.n)
        satellite.statement.if (satellite.library.n < 3)
        {
            satellite.main()
        }
    }

.  `1 3 0`  `satellite.main()`
> satellite.help(satellite.main)

The parameter list, and it takes either nothing or the command line.
Written empty, the program ignores how it was started. Written with a
list of strings, that name holds the arguments the program was given, and
the name is yours to choose.

Displaying it prints the arguments as a list, so a program started with
none prints `[]`. It is also the shape a call to main takes, and calling
one that declares a parameter with no argument is refused.

    satellite.include(satellite)

    satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
    {
        satellite.console.display(arguments)
    }


## container

.  `1 4`  `satellite.container`
> satellite.help(satellite.container)

The things that hold other things. There are two: a list, which keeps
values in order, and a map, which keeps values under keys.

Both say what they hold inside angle brackets when you declare them, and
both are made by calling the bare shape rather than by the declaration
alone.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    satellite.console.display(l)

.  `1 4 0`  `satellite.container()`
> satellite.help(satellite.container)

The bare shape. There is no container called `container`; the word is the
family and `list` or `map` is what you write.

.  `1 4 1`  `satellite.container.map`
> satellite.help(satellite.container.map)

Values kept under keys. The declaration names both types, the key's and
the value's, separated by a comma inside the angle brackets.

It remembers the order things were put in, so walking its keys gives them
back in that order rather than in some order of its own.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    satellite.console.display(m)

H  `1 4 1 0`  `satellite.container.map()`  _M16_
> satellite.help(satellite.container.map)

Makes an empty map. A declaration on its own holds nothing, so this call
is what actually constructs one.

It is the same shape for every map whatever it holds; the types are in the
declaration on the left.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    satellite.console.display(m.empty())

H  `1 4 1 1`  `satellite.container.map.set(k, v)`  _M16_
> satellite.help(satellite.container.map.set)

Puts a value under a key. A key that is not there yet is added; one that
is already there has its value replaced.

The square bracket form does the same thing and reads better when the key
is a literal.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    m["washer"] = 4
    satellite.console.display(m)

H  `1 4 1 2`  `satellite.container.map.get(k)`  _M16_
> satellite.help(satellite.container.map.get)

Takes the value out from under a key.

Ask `has` first if you are not sure the key is there, since asking for a
key that is missing is a refusal rather than a quiet empty answer.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    satellite.console.display(m.get("bolt"))
    satellite.console.display(m["bolt"])

H  `1 4 1 3`  `satellite.container.map.has(k)`  _M16_
> satellite.help(satellite.container.map.has)

Answers true when the key is in the map.

This is the test to write before `get`, and it is also how you tell an
absent key from a key whose value happens to be zero.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    satellite.console.display(m.has("bolt"))
    satellite.console.display(m.has("rivet"))

H  `1 4 1 4`  `satellite.container.map.size`  _M16_
> satellite.help(satellite.container.map.size)

How many pairs the map holds.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    m.set("nut", 9)
    satellite.console.display(m.size())

H  `1 4 1 5`  `satellite.container.map.empty`  _M16_
> satellite.help(satellite.container.map.empty)

Answers true when the map holds no pairs at all.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    satellite.console.display(m.empty())

H  `1 4 1 6`  `satellite.container.map.clear`  _M16_
> satellite.help(satellite.container.map.clear)

Throws away every pair, leaving the map empty and ready to use again.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    m.clear()
    satellite.console.display(m.empty())

H  `1 4 1 7`  `satellite.container.map.remove(k)`  _M16_
> satellite.help(satellite.container.map.remove)

Takes one key and its value out of the map.

The order of what is left is unchanged, so removing from the middle does
not shuffle the rest.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    m.set("nut", 9)
    m.remove("bolt")
    satellite.console.display(m)

H  `1 4 1 8`  `satellite.container.map.keys`  _M16_
> satellite.help(satellite.container.map.keys)

Hands back every key as a list, in the order they were put in.

Because that order is kept rather than arbitrary, a walk over the keys is
something a program can rely on.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    m.set("nut", 9)
    satellite.console.display(m.keys())

H  `1 4 1 9`  `satellite.container.map.values`  _M16_
> satellite.help(satellite.container.map.values)

Hands back every value as a list, in the same order the keys come back
in, so the two lists line up position by position.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("bolt", 7)
    m.set("nut", 9)
    satellite.console.display(m.values())

H  `1 4 1 10`  `satellite.container.map.search(pattern)`  _M16_
> satellite.help(satellite.container.map.search)

Finds pairs whose key is close to the pattern, rather than exactly equal
to it. How close is close enough is a dial, `satellite.system.threshold`,
running from an exact match up to a substring either way.

What comes back is a list of maps, one per hit, each carrying the value,
the key, the path and the score.

    satellite.container.map<satellite.variable.string, satellite.variable.number> m = satellite.container.map()
    m.set("washer", 4)
    satellite.system.threshold(6)
    satellite.console.display(m.search("was"))

.  `1 4 2`  `satellite.container.list`
> satellite.help(satellite.container.list)

Values in order, counted from zero. The declaration names what it holds
inside angle brackets, and everything in it is that one type.

A position in square brackets reads or writes one value, a negative one
counts back from the end, and two positions with a colon between them
take a slice.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    satellite.console.display(l[0])
    satellite.console.display(l[-1])

H  `1 4 2 0`  `satellite.container.list()`  _M16_
> satellite.help(satellite.container.list)

Makes an empty list. The declaration holds nothing on its own, so this
call is what constructs one.

    satellite.container.list<satellite.variable.string> parts = satellite.container.list()
    satellite.console.display(parts.empty())

H  `1 4 2 1`  `satellite.container.list.append`  _M16_
> satellite.help(satellite.container.list.append)

Adds one value to the end. It changes the list you called it on rather
than handing back a new one.

This is how a list gets built, since a list constructed empty has nothing
in it and there is no literal form.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.append(2)
    satellite.console.display(l)

H  `1 4 2 2`  `satellite.container.list.size`  _M16_
> satellite.help(satellite.container.list.size)

How many values the list holds.

It counts what is there now, so it is right after appends and removals
without being told.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    satellite.console.display(l.size())

H  `1 4 2 3`  `satellite.container.list.sort()`  _M16_
> satellite.help(satellite.container.list.sort)

Puts the values in order, smallest first, changing the list in place.

Strings sort as text and numbers sort as numbers, so a list sorts the way
the type it holds compares.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.append(2)
    l.sort()
    satellite.console.display(l)

H  `1 4 2 4`  `satellite.container.list.sort(direction)`  _M16_
> satellite.help(satellite.container.list.sort)

Sorts in the direction you name, `"up"` or `"down"`.

The two spellings fold onto the plain sort and the sort_down beside it, so
this is the same work reached by a name rather than a different one.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.sort("down")
    satellite.console.display(l)
    l.sort("up")
    satellite.console.display(l)

H  `1 4 2 5`  `satellite.container.list.sort_down()`  _M16_
> satellite.help(satellite.container.list.sort_down)

Puts the values in order largest first, changing the list in place.

It is the plain sort turned round, and `sort("down")` is the same thing
said the other way.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(1)
    l.append(3)
    l.sort_down()
    satellite.console.display(l)

H  `1 4 2 6`  `satellite.container.list.sort_down(key)`  _M16_
> satellite.help(satellite.container.list.sort_down)

Sorts largest first by a key you name, for a list holding maps.

The key is the name to look under in each map, so a list of records
sorts by one of their fields.

H  `1 4 2 7`  `satellite.container.list.sort_up(key)`  _M16_
> satellite.help(satellite.container.list.sort_up)

Sorts smallest first by a key you name, for a list holding maps.

It is the pair to sort_down with a key, and the key is the name to look
under in each map.

H  `1 4 2 8`  `satellite.container.list.contains(x)`  _M16_
> satellite.help(satellite.container.list.contains)

Answers true when the value is somewhere in the list.

When you need to know where rather than whether, `index_of` says.

    satellite.container.list<satellite.variable.string> parts = satellite.container.list()
    parts.append("bolt")
    satellite.console.display(parts.contains("bolt"))

H  `1 4 2 9`  `satellite.container.list.index_of(x)`  _M16_
> satellite.help(satellite.container.list.index_of)

Answers the position of the first place the value appears, counting from
zero.

    satellite.container.list<satellite.variable.string> parts = satellite.container.list()
    parts.append("bolt")
    parts.append("nut")
    satellite.console.display(parts.index_of("nut"))

H  `1 4 2 10`  `satellite.container.list.empty`  _M16_
> satellite.help(satellite.container.list.empty)

Answers true when the list holds nothing.

It is the honest test, since an empty list is not itself false.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    satellite.console.display(l.empty())

H  `1 4 2 11`  `satellite.container.list.clear`  _M16_
> satellite.help(satellite.container.list.clear)

Throws away every value, leaving the list empty and ready to use again.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.clear()
    satellite.console.display(l.empty())

H  `1 4 2 12`  `satellite.container.list.first`  _M16_
> satellite.help(satellite.container.list.first)

The value at the front, without taking it out.

It is the same as position zero and reads better when the point is that it
is the first one.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    satellite.console.display(l.first())

H  `1 4 2 13`  `satellite.container.list.last`  _M16_
> satellite.help(satellite.container.list.last)

The value at the end, without taking it out.

It is the same as position -1 and says what it means more plainly.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    satellite.console.display(l.last())

H  `1 4 2 14`  `satellite.container.list.truncate(n)`  _M16_
> satellite.help(satellite.container.list.truncate)

Cuts the list down to the length you give, throwing away everything past
it. A length longer than the list changes nothing.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.append(2)
    l.truncate(2)
    satellite.console.display(l)

H  `1 4 2 15`  `satellite.container.list.reserve(n)`  _M16_
> satellite.help(satellite.container.list.reserve)

Says roughly how many values are coming, so the list can make room once
instead of growing as it goes.

It changes nothing you can observe except speed. The list is still empty
afterwards.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.reserve(1000)
    satellite.console.display(l.empty())

H  `1 4 2 16`  `satellite.container.list.remove_first()`  _M16_
> satellite.help(satellite.container.list.remove_first)

Takes the front value out and shuffles the rest down.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.remove_first()
    satellite.console.display(l)

H  `1 4 2 17`  `satellite.container.list.remove_last()`  _M16_
> satellite.help(satellite.container.list.remove_last)

Takes the end value out, which costs nothing since nothing has to move.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.remove_last()
    satellite.console.display(l)

H  `1 4 2 18`  `satellite.container.list.remove_at(n)`  _M16_
> satellite.help(satellite.container.list.remove_at)

Takes out the value at the position you name and closes the gap.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.append(2)
    l.remove_at(1)
    satellite.console.display(l)

H  `1 4 2 19`  `satellite.container.list.remove(x)`  _M16_
> satellite.help(satellite.container.list.remove)

Takes out the value you name rather than the position, removing the first
place it appears.

**A quoted word here is read as an option, not as a value.** `remove` has
three of them — `"first"`, `"last"` and `"at"` — which fold onto the three
methods of those names. So to remove a string, hold it in a name first and
pass the name; a literal would be looked up among the options and refused.

    satellite.container.list<satellite.variable.string> parts = satellite.container.list()
    parts.append("bolt")
    parts.append("nut")
    satellite.variable.string which = "bolt"
    parts.remove(which)
    satellite.console.display(parts)

H  `1 4 2 20`  `satellite.container.list.insert(n, x)`  _M16_
> satellite.help(satellite.container.list.insert)

Puts a value in at the position you name, pushing everything from there
on one place along.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(2)
    l.insert(1, 9)
    satellite.console.display(l)

H  `1 4 2 21`  `satellite.container.list.join(separator)`  _M16_
> satellite.help(satellite.container.list.join)

Runs the values together into one string with the separator between
them, and nothing after the last one.

It is the other half of a string's `split`.

    satellite.container.list<satellite.variable.string> parts = satellite.container.list()
    parts.append("a")
    parts.append("b")
    satellite.console.display(parts.join(", "))

H  `1 4 2 22`  `satellite.container.list.reverse`  _M16_
> satellite.help(satellite.container.list.reverse)

Turns the list back to front in place.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.reverse()
    satellite.console.display(l)

H  `1 4 2 23`  `satellite.container.list.sum`  _M16_
> satellite.help(satellite.container.list.sum)

Adds every value together. The list has to hold numbers.

Since a number here has no width, a long list of large values sums
exactly rather than wrapping.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    l.append(2)
    satellite.console.display(l.sum())

H  `1 4 2 24`  `satellite.container.list.max`  _M16_
> satellite.help(satellite.container.list.max)

The largest value in the list.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    satellite.console.display(l.max())

H  `1 4 2 25`  `satellite.container.list.min`  _M16_
> satellite.help(satellite.container.list.min)

The smallest value in the list.

    satellite.container.list<satellite.variable.number> l = satellite.container.list()
    l.append(3)
    l.append(1)
    satellite.console.display(l.min())

H  `1 4 2 26`  `satellite.container.list.search(pattern)`  _M16_
> satellite.help(satellite.container.list.search)

Finds values close to the pattern rather than exactly equal to it, and
hands back a list of maps, one per hit, carrying the value, the key, the
path and the score.

The shorter form, a pattern in square brackets, gives just the values
that matched. How loose a match may be is `satellite.system.threshold`.

    satellite.container.list<satellite.variable.string> parts = satellite.container.list()
    parts.append("washer")
    satellite.system.threshold(6)
    satellite.console.display(parts.search("was"))

.  `1 4 3`  `satellite.container.arguments`
> satellite.help(satellite.container.arguments)

The command line, as a container.

**Not built as this path.** What a program reads today is the parameter
it declares on `satellite.main`, and the wider arguments object is M20.

.  `1 4 4`  `satellite.container.result`
> satellite.help(satellite.container.result)

A result carrying either an answer or a reason it failed.

**Not built.** The path is numbered and no milestone has reached it.


## console

.  `1 5`  `satellite.console`
> satellite.help(satellite.console)

The terminal the program is running in, both halves of it. Everything the
program writes out goes through here, and everything the person types in
comes back through here.

Writing is printed by a thread of its own, so a program that prints in a
tight loop is not waiting on the terminal to keep up.

    satellite.console.display("Hello, World!")

.  `1 5 0`  `satellite.console()`
> satellite.help(satellite.console)

The bare shape. The console is a module rather than a thing you construct,
so there is nothing to call here and no value comes back.

What you want is one of the words underneath, and asking help about the
module lists them.

H  `1 5 1`  `satellite.console.display`  _M10_
> satellite.help(satellite.console.display)

Prints one value and moves to the next line. It takes any type, and it
prints the value the way that type renders itself, so a list prints its
brackets and a map prints its pairs without you formatting anything.

It is the most-called word in the language and the first one most people
type.

    satellite.variable.string greeting = "Hello"
    satellite.console.display(greeting)
    satellite.console.display(greeting.size())

H  `1 5 2`  `satellite.console.input()`  _M14_
> satellite.help(satellite.console.input)

Reads one line the person types and hands it back as a string, without
printing anything first. The newline they press is not part of what you
get.

It waits. If nothing is there to type, the program stops on this line
until something is.

    satellite.variable.string answer = satellite.console.input()
    satellite.console.display(answer)

H  `1 5 3`  `satellite.console.input(prompt)`  _M14_
> satellite.help(satellite.console.input)

Prints the prompt you give it, then reads one line back. This is the
ordinary shape, because a question and the wait for its answer belong on
the same line.

The prompt is printed as you wrote it, so put your own space or colon at
the end if you want one.

    satellite.variable.string name = satellite.console.input("what is your name? ")
    satellite.console.display(name)

H  `1 5 4`  `satellite.console.input(prompt, target)`  _M14_
> satellite.help(satellite.console.input)

Prints the prompt and reads the answer straight into a variable you
name, instead of handing it back to be assigned.

The second argument is the variable itself and not its value, so the
variable has to be declared before the line runs. It is the one place in
the language where an argument is a name rather than a thing.

    satellite.variable.string name = ""
    satellite.console.input("what is your name? ", name)
    satellite.console.display(name)

H  `1 5 5`  `satellite.console.typed()`  _M14_
> satellite.help(satellite.console.typed)

Reads a line **if one is already waiting**, and answers `nothing` if it is
not. It never waits, which is the whole point of it: a program with work to
do can check on the person between pieces of that work.

What comes back is the line itself and not a yes-or-no, so hold it in a
`satellite.variable.variant` and ask whether it is holding nothing.

    satellite.variable.variant line = satellite.console.typed()
    satellite.statement.if (line.holds("nothing"))
    {
        satellite.console.display("nobody has typed yet")
    }

H  `1 5 6`  `satellite.console.width`  _M14_
> satellite.help(satellite.console.width)

How many columns wide the terminal is right now. It is read when you ask
rather than remembered, so a window the person resizes gives a new answer
on the next line.

Use it to decide how much fits before you print it.

    satellite.console.display(satellite.console.width)

H  `1 5 7`  `satellite.console.height`  _M14_
> satellite.help(satellite.console.height)

How many rows tall the terminal is right now, read fresh the same way the
width is.

Together the two are what you need to draw anything that should fill the
screen rather than scroll off it.

    satellite.console.display(satellite.console.height)

H  `1 5 8`  `satellite.console.clear()`  _M14_
> satellite.help(satellite.console.clear)

Wipes the screen and leaves the cursor at the top left. Nothing the
program has printed is recoverable afterwards.

It is the first line of anything that redraws rather than scrolls.

    satellite.console.clear()
    satellite.console.display("a fresh screen")

H  `1 5 9`  `satellite.console.home()`  _M14_
> satellite.help(satellite.console.home)

Moves the cursor back to the top left without erasing anything. What was
on the screen stays there until something is printed over it.

This is what you want for a display that updates in place, because
clearing first is what makes a redraw flicker.

    satellite.console.home()
    satellite.console.display("printed over the top line")


## variable

.  `1 6`  `satellite.variable`
> satellite.help(satellite.variable)

Every kind of value a name can hold. A declaration names the kind first
and the name second, and the kind never changes afterwards.

A declaration on its own holds nothing. It is the assignment beside it, or
a later one, that puts a value in.

    satellite.variable.string greeting = "Hello"
    satellite.variable.number count = 3
    satellite.console.display(greeting)
    satellite.console.display(count)

.  `1 6 0`  `satellite.variable()`
> satellite.help(satellite.variable)

The bare shape. There is no type called `variable`; the word is the family
and one of the kinds underneath is what you write.

Asking help about the family lists the kinds, and the ones that are built
are the ones you can declare today.

.  `1 6 1`  `satellite.variable.string`
> satellite.help(satellite.variable.string)

Text. It is written in double quotes and it can be any length; there is
no limit here and no separate character type.

It carries sixteen methods, which are the words underneath. Every one is
called on a declared name rather than on a literal, so give the text a
name first.

    satellite.variable.string greeting = "Hello"
    greeting.append(", World!")
    satellite.console.display(greeting)

.  `1 6 1 0`  `satellite.variable.string()`
> satellite.help(satellite.variable.string)

The bare shape, which is how you make an empty string when you have
nothing to put in it yet.

A declaration with no assignment holds nothing at all, which is not the
same as holding an empty string, so write the quotes if empty is what you
mean.

    satellite.variable.string answer = ""
    satellite.console.display(answer.empty())

H  `1 6 1 1`  `satellite.variable.string.size`  _M11_
> satellite.help(satellite.variable.string.size)

How many characters the string holds. An empty one answers zero.

It counts what is there rather than remembering what was put in, so it is
correct after an append or a trim without you doing anything.

    satellite.variable.string greeting = "Hello"
    satellite.console.display(greeting.size())

H  `1 6 1 2`  `satellite.variable.string.empty`  _M11_
> satellite.help(satellite.variable.string.empty)

Answers true when the string holds no characters at all.

It is the honest test, because this language has no truthiness and an
empty string is not itself false.

    satellite.variable.string answer = ""
    satellite.statement.if (answer.empty())
    {
        satellite.console.display("nothing was typed")
    }

H  `1 6 1 3`  `satellite.variable.string.find(x)`  _M11_
> satellite.help(satellite.variable.string.find)

Answers where a piece of text first appears, counting from zero.

Use it when you need the position. When you only need to know whether the
text is in there at all, `contains` says so more plainly.

    satellite.variable.string line = "bolt,nut,washer"
    satellite.console.display(line.find("nut"))

H  `1 6 1 4`  `satellite.variable.string.contains(x)`  _M11_
> satellite.help(satellite.variable.string.contains)

Answers true when the piece of text appears anywhere in the string.

It is the question you usually mean, and it reads better in a condition
than comparing a position against a number.

    satellite.variable.string line = "bolt,nut,washer"
    satellite.statement.if (line.contains("nut"))
    {
        satellite.console.display("found it")
    }

H  `1 6 1 5`  `satellite.variable.string.substring(start, end)`  _M11_
> satellite.help(satellite.variable.string.substring)

Hands back the piece between two positions, counting from zero. The first
position is included and the second is not, so the difference between them
is the length you get.

The string you called it on is unchanged.

    satellite.variable.string greeting = "Hello, World!"
    satellite.console.display(greeting.substring(0, 5))

H  `1 6 1 6`  `satellite.variable.string.starts_with(x)`  _M11_
> satellite.help(satellite.variable.string.starts_with)

Answers true when the string begins with the text you give it.

It is the cheap way to sort lines into kinds without picking them apart
first.

    satellite.variable.string line = "error: something went wrong"
    satellite.statement.if (line.starts_with("error"))
    {
        satellite.console.display("that is a complaint")
    }

H  `1 6 1 7`  `satellite.variable.string.ends_with(x)`  _M11_
> satellite.help(satellite.variable.string.ends_with)

Answers true when the string finishes with the text you give it.

The usual use is a file name's ending, since the language has no separate
idea of an extension.

    satellite.variable.string name = "program.satl"
    satellite.statement.if (name.ends_with(".satl"))
    {
        satellite.console.display("that is satellite source")
    }

H  `1 6 1 8`  `satellite.variable.string.lower`  _M11_
> satellite.help(satellite.variable.string.lower)

Hands back the same text with every letter in lower case, and leaves the
original alone.

Use it on both sides when you want a comparison that ignores case, since
comparing strings compares them exactly.

    satellite.variable.string shout = "HELLO"
    satellite.console.display(shout.lower())

H  `1 6 1 9`  `satellite.variable.string.upper`  _M11_
> satellite.help(satellite.variable.string.upper)

Hands back the same text with every letter in upper case, and leaves the
original alone.

It answers a new string, so assign it or print it; calling it and ignoring
the answer changes nothing.

    satellite.variable.string greeting = "Hello"
    satellite.console.display(greeting.upper())
    satellite.console.display(greeting)

H  `1 6 1 10`  `satellite.variable.string.split(separator)`  _M11_
> satellite.help(satellite.variable.string.split)

Cuts the string wherever the separator appears and hands back the pieces
as a list of strings.

The separator itself is not in any of the pieces. This is how a line of
comma-separated text becomes something you can walk.

    satellite.variable.string line = "a,b,c"
    satellite.console.display(line.split(","))

H  `1 6 1 11`  `satellite.variable.string.trim`  _M11_
> satellite.help(satellite.variable.string.trim)

Hands back the text with the blank space taken off both ends, and leaves
what is in the middle alone.

It is the first thing to do to a line somebody typed, because the space
before their answer is not part of it.

    satellite.variable.string typed = "   Ada   "
    satellite.console.display(typed.trim())

H  `1 6 1 12`  `satellite.variable.string.replace(a, b)`  _M11_
> satellite.help(satellite.variable.string.replace)

Hands back the text with every appearance of the first piece swapped for
the second.

It replaces all of them and not just the first, and the string you called
it on is unchanged.

    satellite.variable.string line = "a,b,c"
    satellite.console.display(line.replace(",", " and "))

H  `1 6 1 13`  `satellite.variable.string.to_number`  _M11_
> satellite.help(satellite.variable.string.to_number)

Reads the text as a number and hands the number back. It is the other
half of a number's `to_string`.

The text has to actually be a number. This is what you call on a line the
person typed before you can do arithmetic with it.

    satellite.variable.string typed = "42"
    satellite.variable.number n = typed.to_number()
    satellite.console.display(n * 2)

H  `1 6 1 14`  `satellite.variable.string.append(x)`  _M11_
> satellite.help(satellite.variable.string.append)

Adds text to the end, and this one changes the string you called it on
rather than handing back a new one.

It is the odd one out among the string methods for exactly that reason, so
you do not assign its answer to anything.

    satellite.variable.string greeting = "Hello"
    greeting.append(", World!")
    satellite.console.display(greeting)

H  `1 6 1 15`  `satellite.variable.string.clear`  _M11_
> satellite.help(satellite.variable.string.clear)

Empties the string, leaving it holding no characters. Like append, it
changes the string you called it on.

Afterwards `empty()` answers true and `size()` answers zero.

    satellite.variable.string greeting = "Hello"
    greeting.clear()
    satellite.console.display(greeting.empty())

H  `1 6 1 16`  `satellite.variable.string.at(n)`  _M11_
> satellite.help(satellite.variable.string.at)

Hands back the one character at the position you name, counting from
zero.

There is no character type here, so what comes back is a string holding
one character.

    satellite.variable.string greeting = "Hello"
    satellite.console.display(greeting.at(0))

.  `1 6 2`  `satellite.variable.file`
> satellite.help(satellite.variable.file)

A file on disk, held as a value you can read from and write to.

**Not built.** The path is numbered and the seven words underneath are
numbered with it, but nothing opens a file yet. M19 is the milestone that
builds this and the `satellite.directory` module beside it.

.  `1 6 2 0`  `satellite.variable.file()`
> satellite.help(satellite.variable.file)

The bare shape. **Not built** — M19.

.  `1 6 2 1`  `satellite.variable.file.new`
> satellite.help(satellite.variable.file)

Makes a file that does not exist yet. **Not built** — M19.

.  `1 6 2 2`  `satellite.variable.file.open`
> satellite.help(satellite.variable.file)

Opens a file that does. **Not built** — M19.

.  `1 6 2 3`  `satellite.variable.file.read_line`
> satellite.help(satellite.variable.file)

Reads one line. **Not built** — M19.

.  `1 6 2 4`  `satellite.variable.file.write_line(s)`
> satellite.help(satellite.variable.file)

Writes one line. **Not built** — M19.

.  `1 6 2 5`  `satellite.variable.file.read_all`
> satellite.help(satellite.variable.file)

Reads the whole file at once. **Not built** — M19.

.  `1 6 2 6`  `satellite.variable.file.close`
> satellite.help(satellite.variable.file)

Closes it. **Not built** — M19.

.  `1 6 2 7`  `satellite.variable.file.exists`
> satellite.help(satellite.variable.file)

Answers whether the file is there. **Not built** — M19.

.  `1 6 3`  `satellite.variable.time`
> satellite.help(satellite.variable.time)

A moment, held as a value.

**Not built as a type.** `satellite.time.now()` answers the clock today,
but there is no declared time variable to keep one in yet. M29 is the
calendar milestone that decides what a time value can be.

.  `1 6 3 0`  `satellite.variable.time()`
> satellite.help(satellite.variable.time)

The bare shape. **Not built** — M29.

.  `1 6 4`  `satellite.variable.number`
> satellite.help(satellite.variable.number)

A whole number, of any size. It does not overflow and it has no width, so
a factorial that runs past what a machine word holds keeps being right.

The sign is written out where it matters. Fifteen methods hang underneath
it, and each is called on a declared name.

    satellite.variable.number count = 3
    satellite.console.display(count * 1000000000000)

.  `1 6 4 0`  `satellite.variable.number()`
> satellite.help(satellite.variable.number)

The bare shape, for a number you have nowhere to get a value from yet.

A declaration holds nothing until something is assigned, so write `= 0` if
zero is what you actually mean.

    satellite.variable.number total = 0
    satellite.console.display(total)

H  `1 6 4 1`  `satellite.variable.number.shift_left(n)`  _M11_
> satellite.help(satellite.variable.number.shift_left)

Moves the number's bits up by the count you give, which doubles it once
per place.

Because a number here has no width, nothing falls off the top; the value
just gets larger.

    satellite.variable.number n = 1
    satellite.console.display(n.shift_left(10))

H  `1 6 4 2`  `satellite.variable.number.max(a, b)`  _M11_
> satellite.help(satellite.variable.number.max)

Answers whichever is larger, the number you called it on or the one you
hand it.

Neither number is changed; what comes back is a new value.

    satellite.variable.number a = 3
    satellite.console.display(a.max(9))

H  `1 6 4 3`  `satellite.variable.number.min(a, b)`  _M11_
> satellite.help(satellite.variable.number.min)

Answers whichever is smaller, the number you called it on or the one you
hand it.

It is the pair to `max`, and the usual use of the two together is holding
a value inside a range.

    satellite.variable.number a = 3
    satellite.console.display(a.min(9))

H  `1 6 4 4`  `satellite.variable.number.abs(a)`  _M11_
> satellite.help(satellite.variable.number.abs)

Answers the number without its sign, so a negative one comes back
positive and a positive one comes back unchanged.

It is what you want before comparing two distances, where the direction
does not matter.

    satellite.variable.number drift = 0 - 7
    satellite.console.display(drift.abs())

H  `1 6 4 5`  `satellite.variable.number.clamp(a, low, high)`  _M11_
> satellite.help(satellite.variable.number.clamp)

Holds the number inside a range. Give it the low end and the high end,
and anything below or above comes back as the end it passed.

It is `max` and `min` in one call, which is how it is usually wanted.

    satellite.variable.number n = 42
    satellite.console.display(n.clamp(0, 10))

H  `1 6 4 6`  `satellite.variable.number.to_string`  _M11_
> satellite.help(satellite.variable.number.to_string)

Renders the number as text. It is the other half of a string's
`to_number`.

Use it when you want to join a number onto a string, since the two types
do not add to each other.

    satellite.variable.number count = 3
    satellite.variable.string line = "there are "
    line.append(count.to_string())
    satellite.console.display(line)

H  `1 6 4 7`  `satellite.variable.number.floor`  _M11_
> satellite.help(satellite.variable.number.floor)

Rounds down, towards the smaller number, and answers a whole number.

On a value that is already whole it changes nothing, so it is safe to
call without checking first.

    satellite.variable.number f = 3.7
    satellite.console.display(f.floor())

H  `1 6 4 8`  `satellite.variable.number.ceil`  _M11_
> satellite.help(satellite.variable.number.ceil)

Rounds up, towards the larger number, and answers a whole number.

It is the pair to `floor`, and the one you want when you are asking how
many whole things are needed to cover an amount.

    satellite.variable.number f = 3.2
    satellite.console.display(f.ceil())

H  `1 6 4 9`  `satellite.variable.number.round`  _M11_
> satellite.help(satellite.variable.number.round)

Rounds to the nearest whole number. A value exactly halfway goes away
from zero, so 2.5 becomes 3 and -2.5 becomes -3.

That rule is written down rather than inherited, because halfway is the
only case where languages disagree.

    satellite.variable.number f = 2.5
    satellite.console.display(f.round())

H  `1 6 4 10`  `satellite.variable.number.power(a, b)`  _M15_
> satellite.help(satellite.variable.number.power)

Raises the number to the power you give it.

The answer is a float, so hold it in one or pass it straight to display. A
number here has no width, so a large power gets a large answer rather than
a wrong one.

    satellite.variable.number two = 2
    satellite.console.display(two.power(10))

H  `1 6 4 11`  `satellite.variable.number.shift_right(n)`  _M11_
> satellite.help(satellite.variable.number.shift_right)

Moves the number's bits down by the count you give, which halves it once
per place and throws away what falls off the bottom.

It is the pair to `shift_left`.

    satellite.variable.number n = 1024
    satellite.console.display(n.shift_right(10))

H  `1 6 4 12`  `satellite.variable.number.modulus(a, b)`  _M11_
> satellite.help(satellite.variable.number.modulus)

Answers what is left over after dividing by the number you hand it.

The usual use is asking whether something divides evenly, which is a
remainder of zero.

    satellite.variable.number n = 17
    satellite.console.display(n.modulus(5))

H  `1 6 4 13`  `satellite.variable.number.truncate(a)`  _M15_
> satellite.help(satellite.variable.number.truncate)

Throws away everything after the decimal point and answers the whole part,
without rounding either way.

It differs from `floor` on negative values: truncating -3.7 gives -3, and
flooring it gives -4.

    satellite.variable.number down = 0 - 2.7
    satellite.console.display(down.truncate())

H  `1 6 4 14`  `satellite.variable.number.sqrt(a)`  _M15_
> satellite.help(satellite.variable.number.sqrt)

Answers the square root.

The answer is a float, because most square roots are not whole numbers,
so hold it in one.

    satellite.variable.number n = 144
    satellite.console.display(n.sqrt())

H  `1 6 4 15`  `satellite.variable.number.digits`  _M11_
> satellite.help(satellite.variable.number.digits)

How many digits the number is written with.

Since a number here has no width, this is a real question with a real
answer however large the value gets. It wants a whole number, so a float
is refused rather than guessed at.

    satellite.variable.number big = 123456789
    satellite.console.display(big.digits())

.  `1 6 5`  `satellite.variable.binary`
> satellite.help(satellite.variable.binary)

A number written in base two.

**Not built.** The path is numbered and no milestone has reached it.

.  `1 6 6`  `satellite.variable.bool`
> satellite.help(satellite.variable.bool)

True or false, and nothing else. It is the only type a condition will
accept, because this language has no truthiness for anything to fall back
on.

The two values it can hold are written `satellite.bool.true` and
`satellite.bool.false`. Note the difference between this path, which is
the type, and `satellite.bool`, which is the pair of constants.

    satellite.variable.bool ready = satellite.bool.true
    satellite.statement.if (ready)
    {
        satellite.console.display("ready")
    }

.  `1 6 7`  `satellite.variable.date`
> satellite.help(satellite.variable.date)

A calendar date, apart from a moment in time.

**Not built** — M29 is the calendar.

.  `1 6 8`  `satellite.variable.duration`
> satellite.help(satellite.variable.duration)

A length of time rather than a point in it.

**Not built** — M29 is the calendar.

.  `1 6 9`  `satellite.variable.expression`
> satellite.help(satellite.variable.expression)

An expression held as a value, to be evaluated later.

**Not built.** The path is numbered and no milestone has reached it.

.  `1 6 10`  `satellite.variable.float`
> satellite.help(satellite.variable.float)

A number with a decimal point, and the arithmetic on it is exact where a
machine float is not: a tenth plus two tenths really is three tenths here.

How many digits a division keeps is a dial you can turn, and a change to
it is observed by the very next division rather than at the next run.

    satellite.variable.float tenth = 0.1
    satellite.console.display(tenth + 0.2)

.  `1 6 11`  `satellite.variable.hex`
> satellite.help(satellite.variable.hex)

A number written in base sixteen. It has a second spelling,
`satellite.variable.hexadecimal`, which is the same node.

**Not built.** The path is numbered and no milestone has reached it.

.  `1 6 12`  `satellite.variable.network`
> satellite.help(satellite.variable.network)

A network connection held as a value.

**Not built** — M27 is the network.

.  `1 6 13`  `satellite.variable.thread`
> satellite.help(satellite.variable.thread)

A line of execution running beside the others.

**Not built** — M23 is threads. The arena the evaluator walks is built to
make this possible without locking, but nothing starts one yet.

.  `1 6 13 0`  `satellite.variable.thread()`
> satellite.help(satellite.variable.thread)

The bare shape. **Not built** — M23.

.  `1 6 13 1`  `satellite.variable.thread.start()`
> satellite.help(satellite.variable.thread)

Starts the thread running. **Not built** — M23.

.  `1 6 13 2`  `satellite.variable.thread.join()`
> satellite.help(satellite.variable.thread)

Waits for it to finish. **Not built** — M23.

.  `1 6 14`  `satellite.variable.variant`
> satellite.help(satellite.variable.variant)

One box that can hold any type, one at a time, and can also hold nothing.

It is what you want when the kind of thing is not known until the program
runs. Ask it what it is holding before you take the value out.

    satellite.variable.variant box = 5
    satellite.console.display(box)
    box = "Hello"
    satellite.console.display(box)

.  `1 6 14 0`  `satellite.variable.variant()`
> satellite.help(satellite.variable.variant)

The bare shape, which is a variant holding nothing.

Nothing is a real state here rather than an absence, so a variant that
has not been given a value still answers questions about itself.

    satellite.variable.variant box = 5
    box.clear()
    satellite.console.display(box.holds("nothing"))

H  `1 6 14 1`  `satellite.variable.variant.holding`  _M12_
> satellite.help(satellite.variable.variant.holding)

Answers the name of the type currently in the box, as a string.

A box that has been cleared answers `nothing`, which is a name like any
other rather than a special case you have to test for separately.

    satellite.variable.variant box = 5
    satellite.console.display(box.holding())
    box = "Hello"
    satellite.console.display(box.holding())

H  `1 6 14 2`  `satellite.variable.variant.holds(x)`  _M12_
> satellite.help(satellite.variable.variant.holds)

Answers true when the box is holding the type you name.

It takes the type's name as a string, and `"nothing"` is one of the names
you can ask about. This is the test to write before taking a value out.

    satellite.variable.variant box = "Hello"
    satellite.console.display(box.holds("string"))
    satellite.console.display(box.holds("nothing"))

H  `1 6 14 3`  `satellite.variable.variant.held`  _M12_
> satellite.help(satellite.variable.variant.held)

Takes the value out of the box so you can use it as its own type.

Ask `holds` first. Taking out a value of a type you were not expecting is
the mistake this pair of methods exists to prevent.

    satellite.variable.variant box = "Hello"
    satellite.variable.string kept = box.held()
    satellite.console.display(kept.upper())

H  `1 6 14 4`  `satellite.variable.variant.clear`  _M12_
> satellite.help(satellite.variable.variant.clear)

Empties the box, leaving it holding nothing.

Afterwards `holding()` answers `nothing` and `holds("nothing")` answers
true.

    satellite.variable.variant box = 5
    box.clear()
    satellite.console.display(box.holding())

.  `1 6 15`  `satellite.variable.window`
> satellite.help(satellite.variable.window)

A window on the screen, held as a value.

**Not built** — M24 is windows.

.  `1 6 16`  `satellite.variable.capsule`
> satellite.help(satellite.variable.capsule)

A capsule held as a value, so it can be passed to another one.

**Not built.** The path is numbered and no milestone has reached it.


## random

.  `1 7`  `satellite.random`
> satellite.help(satellite.random)

Random numbers, in three grades. `fast` is quick and good enough for a
game, `normal` is the one to reach for by default, and `ultra` is the
slowest and the most carefully made.

Each grade takes the same four shapes: bare, a number of digits, a range,
and a range with a step.

    satellite.console.display(satellite.random.normal(1, 6))

.  `1 7 0`  `satellite.random()`
> satellite.help(satellite.random)

The bare shape. There is no call called `random` on its own; pick a grade
and call that.

H  `1 7 1`  `satellite.random.fast()`  _M13_
> satellite.help(satellite.random.fast)

The bare shape, and it **draws nothing**. A random number with no bounds
and no width is not a question with an answer, so calling it refuses and
tells you the three shapes that do work: a count of digits, a low and a
high, or a low and a high and a step.

The grade itself is quick, and good enough for a game or a simulation.

    satellite.console.display(satellite.random.fast())

H  `1 7 2`  `satellite.random.normal()`  _M13_
> satellite.help(satellite.random.normal)

The bare shape, and it **draws nothing**. A random number with no bounds
and no width is not a question with an answer, so calling it refuses and
tells you the three shapes that do work: a count of digits, a low and a
high, or a low and a high and a step.

The grade itself is the ordinary grade, and the one to use unless you have a reason not to.

    satellite.console.display(satellite.random.normal())

H  `1 7 3`  `satellite.random.ultra()`  _M13_
> satellite.help(satellite.random.ultra)

The bare shape, and it **draws nothing**. A random number with no bounds
and no width is not a question with an answer, so calling it refuses and
tells you the three shapes that do work: a count of digits, a low and a
high, or a low and a high and a step.

The grade itself is the slowest and most carefully made of the three.

    satellite.console.display(satellite.random.ultra())

H  `1 7 4`  `satellite.random.fast(digits)`  _M13_
> satellite.help(satellite.random.fast)

A fast random number with the many digits you ask for.

Since a number here has no width, asking for a hundred digits gives you a
hundred digits.

    satellite.console.display(satellite.random.fast(20))

H  `1 7 5`  `satellite.random.fast(min, max)`  _M13_
> satellite.help(satellite.random.fast)

A fast random number between the two bounds you give, both included.

    satellite.console.display(satellite.random.fast(1, 6))

H  `1 7 6`  `satellite.random.fast(min, max, step)`  _M13_
> satellite.help(satellite.random.fast)

A fast random number between two bounds that lands on a multiple of the
step, so a step of 5 over 0 to 100 answers one of 0, 5, 10 and so on.

    satellite.console.display(satellite.random.fast(0, 100, 5))

H  `1 7 7`  `satellite.random.normal(digits)`  _M13_
> satellite.help(satellite.random.normal)

An ordinary-grade random number with as many digits as you ask for.

    satellite.console.display(satellite.random.normal(20))

H  `1 7 8`  `satellite.random.normal(min, max)`  _M13_
> satellite.help(satellite.random.normal)

An ordinary-grade random number between the two bounds you give, both
included. This is the shape most programs want.

    satellite.console.display(satellite.random.normal(1, 6))

H  `1 7 9`  `satellite.random.normal(min, max, step)`  _M13_
> satellite.help(satellite.random.normal)

An ordinary-grade random number between two bounds, landing on a multiple
of the step.

    satellite.console.display(satellite.random.normal(0, 100, 5))

H  `1 7 10`  `satellite.random.ultra(digits)`  _M13_
> satellite.help(satellite.random.ultra)

A highest-grade random number with as many digits as you ask for.

    satellite.console.display(satellite.random.ultra(20))

H  `1 7 11`  `satellite.random.ultra(min, max)`  _M13_
> satellite.help(satellite.random.ultra)

A highest-grade random number between the two bounds you give, both
included.

    satellite.console.display(satellite.random.ultra(1, 6))

H  `1 7 12`  `satellite.random.ultra(min, max, step)`  _M13_
> satellite.help(satellite.random.ultra)

A highest-grade random number between two bounds, landing on a multiple
of the step.

    satellite.console.display(satellite.random.ultra(0, 100, 5))


## file

.  `1 8`  `satellite.file`
> satellite.help(satellite.file)

Making and opening files, as a module rather than as a value you hold.

**Not built** — M19 is persistence, and it builds this alongside
`satellite.directory` and the `satellite.variable.file` type.

.  `1 8 0`  `satellite.file()`
> satellite.help(satellite.file)

The bare shape. **Not built** — M19.

.  `1 8 1`  `satellite.file.new(path)`
> satellite.help(satellite.file.new)

Makes a file at the path you give. **Not built** — M19.

.  `1 8 2`  `satellite.file.open`
> satellite.help(satellite.file.open)

Opens a file that already exists. **Not built** — M19.

.  `1 8 3`  `satellite.file.clear`
> satellite.help(satellite.file.clear)

Empties a file without removing it. **Not built** — M19.

.  `1 8 4`  `satellite.file.new(path, mode)`
> satellite.help(satellite.file.new)

Makes a file at the path you give, in the mode you name — reading,
writing, or adding to the end. **Not built** — M19.


## time

.  `1 9`  `satellite.time`
> satellite.help(satellite.time)

The clock, and waiting.

Two of its words are built: asking what time it is now, and sleeping for a
while. Holding a moment in a variable is a later milestone.

    satellite.console.display(satellite.time.now())

.  `1 9 0`  `satellite.time()`
> satellite.help(satellite.time)

The bare shape. The clock is a module rather than something you
construct; one of the words underneath is what you call.

H  `1 9 1`  `satellite.time.now`  _M13_
> satellite.help(satellite.time.now)

What time it is, read at the moment you ask.

It is one of the two sources of nondeterminism in the language, the dice
being the other, which is why they were built together and why a program
that uses neither runs the same way every time.

    satellite.console.display(satellite.time.now())

.  `1 9 2`  `satellite.time.new`
> satellite.help(satellite.time.new)

Makes a moment of your own rather than reading the clock.

**Not built** — M29 is the calendar milestone that decides what a time
value can be.

H  `1 9 3`  `satellite.time.sleep(n)`  _M13_
> satellite.help(satellite.time.sleep)

Stops the program for the number of milliseconds you give, then carries
on.

Ctrl-C still reaches a program that is sleeping, so a long wait is not a
program you have to kill.

    satellite.console.display("before")
    satellite.time.sleep(10)
    satellite.console.display("after")


## spacesuit

.  `1 10`  `satellite.spacesuit`
> satellite.help(satellite.spacesuit)

A spacesuit: a body of code kept apart, with its own names, that a
program brings in rather than writes out.

**Not built** — M26. Its neighbours `satellite.protected` and
`satellite.public` are the words that say what a spacesuit shows and what
it keeps to itself.

.  `1 10 0`  `satellite.spacesuit()`
> satellite.help(satellite.spacesuit)

The bare shape. **Not built** — M26.


## protected

.  `1 11`  `satellite.protected`
> satellite.help(satellite.protected)

Marks something as belonging to the spacesuit that declares it, and not
visible outside.

**Not built** — M26.

.  `1 11 0`  `satellite.protected()`
> satellite.help(satellite.protected)

The bare shape. **Not built** — M26.


## public

.  `1 12`  `satellite.public`
> satellite.help(satellite.public)

Marks something as visible to whatever brings the spacesuit in.

**Not built** — M26.

.  `1 12 0`  `satellite.public()`
> satellite.help(satellite.public)

The bare shape. **Not built** — M26.


## statement

.  `1 13`  `satellite.statement`
> satellite.help(satellite.statement)

The statements that change what runs next, rather than what a value is.
There are exactly four of them and every one takes a block in braces on
the lines below it.

The brace goes on its own line, under the head, which is how every file in
this language is written.

    satellite.variable.bool ready = satellite.bool.true
    satellite.statement.if (ready)
    {
        satellite.console.display("go")
    }

.  `1 13 0`  `satellite.statement()`
> satellite.help(satellite.statement)

The bare shape. There is no statement called `statement`; the word is the
family and one of the four underneath is what you write.

Asking help about the family is how you see all four at once.

.  `1 13 1`  `satellite.statement.if`
> satellite.help(satellite.statement.if)

Runs its block when the condition is true. The condition has to be a
`satellite.variable.bool` and nothing else.

**There is no truthiness here.** A number that happens to be zero is not
false, an empty string is not false, and a value that is holding nothing
is not false. Compare something, or hold a bool, and the test is honest.

    satellite.variable.number count = 3
    satellite.statement.if (count > 0)
    {
        satellite.console.display("there is at least one")
    }

.  `1 13 2`  `satellite.statement.for`
> satellite.help(satellite.statement.for)

Counts. The head holds three parts separated by semicolons: what to
declare before the first pass, the test that decides whether to go round
again, and what to do at the end of each pass.

The variable declared in the head belongs to the loop and is gone after
it.

    satellite.variable.number total = 0
    satellite.statement.for (satellite.variable.number i = 0; i < 5; i = i + 1)
    {
        total = total + i
    }
    satellite.console.display(total)

.  `1 13 3`  `satellite.statement.while`
> satellite.help(satellite.statement.while)

Goes round for as long as the condition holds, testing before each pass
rather than after, so a condition that is false to begin with runs the
block no times at all.

Something inside the block has to change what the condition reads, or the
loop does not end. Ctrl-C stops one that does not.

    satellite.variable.number countdown = 3
    satellite.statement.while (countdown > 0)
    {
        satellite.console.display(countdown)
        countdown = countdown - 1
    }

.  `1 13 4`  `satellite.statement.else`
> satellite.help(satellite.statement.else)

The other way. It follows an `if` block and runs when that condition was
false, and it takes no condition of its own.

It is written as its own statement on the line after the if block's
closing brace.

    satellite.variable.bool loud = satellite.bool.false
    satellite.statement.if (loud)
    {
        satellite.console.display("loud is true")
    }
    satellite.statement.else
    {
        satellite.console.display("loud is false")
    }


## library

.  `1 14`  `satellite.library`
> satellite.help(satellite.library)

Where the program's own globals live, and where the machine's settings
are read and written.

A name you make under `satellite.library` is a global: it is declared at
the top of a file, outside any capsule, and every capsule can see it.

    satellite.console.display(satellite.library.system.float_digits)

.  `1 14 0`  `satellite.library()`
> satellite.help(satellite.library)

The bare shape. The library is a place rather than a thing to call, and
one of the words underneath is what you write.

.  `1 14 1`  `satellite.library.main`
> satellite.help(satellite.library.main)

What the program was started with, gathered in one place.

**Not built** — M20 is the milestone that builds the arguments object, and
DESIGN §7.7 is the specification for it.

.  `1 14 1 0`  `satellite.library.main()`
> satellite.help(satellite.library.main)

The bare shape. **Not built** — M20.

.  `1 14 1 1`  `satellite.library.main.arguments`
> satellite.help(satellite.library.main.arguments)

The arguments object: the command line, and the facts about the machine
and the person running the program.

**The name is yours.** It is reached through whatever you called the
parameter on `satellite.main`, and six spellings are accepted for it —
`arguments`, `argument`, `argumentz`, `args`, `argz` and `arg`. **Not
built** — the list of command-line words works today, and everything under
it refuses until M20.

.  `1 14 1 1 0`  `satellite.library.main.arguments()`
> satellite.help(satellite.library.main.arguments)

The bare shape. **Not built** — M20.

.  `1 14 1 1 1`  `satellite.library.main.arguments.machine`
> satellite.help(satellite.library.main.arguments.machine)

What the machine is: how many cores, how many threads, and which
processor.

**Not built** — M20. The numbers themselves are already read by the
interpreter, so what is missing is the path rather than the fact.

.  `1 14 1 1 1 0`  `satellite.library.main.arguments.machine()`
> satellite.help(satellite.library.main.arguments.machine)

The bare shape. **Not built** — M20.

.  `1 14 1 1 1 1`  `satellite.library.main.arguments.machine.cores`
> satellite.help(satellite.library.main.arguments.machine.cores)

How many physical cores the machine has. **Not built** — M20.

.  `1 14 1 1 1 2`  `satellite.library.main.arguments.machine.cpu`
> satellite.help(satellite.library.main.arguments.machine.cpu)

What the processor calls itself. **Not built** — M20.

.  `1 14 1 1 1 3`  `satellite.library.main.arguments.machine.threads`
> satellite.help(satellite.library.main.arguments.machine.threads)

How many threads the machine can run at once, which is usually more than
the number of cores. **Not built** — M20.

.  `1 14 1 1 2`  `satellite.library.main.arguments.memory`
> satellite.help(satellite.library.main.arguments.memory)

How much memory the machine has. **Not built** — M20.

.  `1 14 1 1 2 0`  `satellite.library.main.arguments.memory()`
> satellite.help(satellite.library.main.arguments.memory)

The bare shape. **Not built** — M20.

.  `1 14 1 1 2 1`  `satellite.library.main.arguments.memory.total`
> satellite.help(satellite.library.main.arguments.memory.total)

How much memory the machine has in total. **Not built** — M20.

Free memory is not a path here yet, and which of the two spellings it
gets is something M20 has to settle.

.  `1 14 1 1 3`  `satellite.library.main.arguments.username`
> satellite.help(satellite.library.main.arguments.username)

Who is running the program. **Not built** — M20.

.  `1 14 2`  `satellite.library.system`
> satellite.help(satellite.library.system)

The four dials that change how the interpreter itself behaves.

Each one reads like a value and is written like an assignment, and a
change takes effect at once rather than at the next run. They are seeded
from `satellite_config.ini` at startup.

    satellite.console.display(satellite.library.system.float_digits)
    satellite.library.system.float_digits = 5
    satellite.console.display(satellite.library.system.float_digits)

.  `1 14 2 0`  `satellite.library.system()`
> satellite.help(satellite.library.system)

The bare shape. There is nothing to call; write one of the four dials by
name.

H  `1 14 2 1`  `satellite.library.system.division_digits`  _M15_
> satellite.help(satellite.library.system.division_digits)

How many digits a division keeps.

Retuning it is observed by the very next division rather than at the next
run, which is what makes it a dial rather than a setting.

    satellite.console.display(1 / 3)
    satellite.library.system.division_digits = 40
    satellite.console.display(1 / 3)

H  `1 14 2 2`  `satellite.library.system.max_depth`  _M15_
> satellite.help(satellite.library.system.max_depth)

How deep the interpreter will go before it decides a program has run
away with itself.

It is a ceiling on memory rather than a limit on the language: the walkers
keep their own stacks, so depth here is bounded by what the machine has
rather than by a number somebody picked.

    satellite.console.display(satellite.library.system.max_depth)

H  `1 14 2 3`  `satellite.library.system.min_free_mb`  _M15_
> satellite.help(satellite.library.system.min_free_mb)

How much memory the interpreter insists on leaving free, so a runaway
program is stopped before the machine is.

It reads as `nothing` when no floor has been set, which is not the same as
a floor of zero.

    satellite.console.display(satellite.library.system.min_free_mb)

H  `1 14 2 4`  `satellite.library.system.float_digits`  _M15_
> satellite.help(satellite.library.system.float_digits)

How many digits a float carries by default.

Precision travels with the value, so narrowing this dial changes the
floats made after it rather than the ones already held.

    satellite.library.system.float_digits = 5
    satellite.variable.float narrow = 1
    satellite.console.display(narrow / 3)


## return

.  `1 15`  `satellite.return`
> satellite.help(satellite.return)

Leaves the capsule it is written in, at once, and optionally hands a
value back to whoever called it.

A capsule that runs off the end of its body has finished on its own, so
this is only needed when you want to leave early or to answer with
something.

    satellite.capsule biggest(satellite.variable.number a, satellite.variable.number b)
    {
        satellite.statement.if (a > b)
        {
            satellite.return(a)
        }
        satellite.return(b)
    }

    satellite.console.display(biggest(3, 9))

.  `1 15 0`  `satellite.return()`
> satellite.help(satellite.return)

The bare shape, with nothing between the parentheses. It leaves the
capsule and hands nothing back, which is what you want for a capsule that
does something rather than answers something.

It is the shape to use for an early exit out of a loop's enclosing
capsule.

    satellite.capsule announce(satellite.variable.string what)
    {
        satellite.console.display(what)
        satellite.return()
    }

    announce("done")

.  `1 15 1`  `satellite.return(satellite)`
> satellite.help(satellite.return)

Ends the program successfully. Written at the end of `satellite.main`, it
is how a file says it finished on purpose rather than by running out of
lines.

The word `satellite` inside the parentheses is part of the path's number
and not a value you are returning, so nothing is handed back.

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("Hello, World!")
        satellite.return(satellite)
    }

.  `1 15 2`  `satellite.return(value)`
> satellite.help(satellite.return)

Hands a value back to the caller.

Anything can go here that is a value: a literal, a name, or the answer to
another call. Nothing has to be declared in advance for it to work — a
capsule that returns a value needs no announcement on its head line.

    satellite.capsule double_it(satellite.variable.number n)
    {
        satellite.return(n * 2)
    }

    satellite.console.display(double_it(21))


## analyze

.  `1 16`  `satellite.analyze`
> satellite.help(satellite.analyze)

Reads a satellite file and says what is in it, without running it.

**Not built**, and unlike most of the paths here it has no milestone
either. It is the one word in the language nobody has scheduled.


## bool

.  `1 17`  `satellite.bool`
> satellite.help(satellite.bool)

The two constants, true and false. They are the only two values a
`satellite.variable.bool` can hold and the only two things a condition
will accept.

They are written out in full. A bare `true` is a name you own, so typing
it asks for a variable of yours by that name.

    satellite.variable.bool ready = satellite.bool.true
    satellite.console.display(ready)

.  `1 17 0`  `satellite.bool()`
> satellite.help(satellite.bool)

The bare shape. There is nothing to call here; the module holds two
constants and you write one of them by name.

H  `1 17 1`  `satellite.bool.false`  _M11_
> satellite.help(satellite.bool.false)

False, written out. It is a value like any other, so it can be assigned,
passed to a capsule, and stored in a list.

Write it in full. `false` on its own is a name the program owns, and
nothing declares it.

    satellite.variable.bool loud = satellite.bool.false
    satellite.statement.if (loud)
    {
        satellite.console.display("loud is true")
    }
    satellite.statement.else
    {
        satellite.console.display("loud is false")
    }

H  `1 17 2`  `satellite.bool.true`  _M11_
> satellite.help(satellite.bool.true)

True, written out, and the other half of the pair.

A condition is a bool and there is no truthiness in this language, so
this constant and its opposite are what every test eventually comes down
to.

    satellite.variable.bool ready = satellite.bool.true
    satellite.statement.while (ready)
    {
        satellite.console.display("once")
        ready = satellite.bool.false
    }


## directory

.  `1 18`  `satellite.directory`
> satellite.help(satellite.directory)

Directories: which one you are in, moving between them, and what is
inside.

**Not built** — M19 is persistence.

.  `1 18 0`  `satellite.directory()`
> satellite.help(satellite.directory)

The bare shape. **Not built** — M19.

.  `1 18 1`  `satellite.directory.change`
> satellite.help(satellite.directory.change)

Moves to another directory. **Not built** — M19.

.  `1 18 2`  `satellite.directory.current`
> satellite.help(satellite.directory.current)

Which directory the program is in. **Not built** — M19.

.  `1 18 3`  `satellite.directory.exists`
> satellite.help(satellite.directory.exists)

Whether a directory is there. **Not built** — M19.

.  `1 18 4`  `satellite.directory.list()`
> satellite.help(satellite.directory.list)

What is inside the current directory. **Not built** — M19.

.  `1 18 5`  `satellite.directory.list(d)`
> satellite.help(satellite.directory.list)

What is inside the directory you name. **Not built** — M19.


## help

.  `1 19`  `satellite.help`
> satellite.help(satellite.help)

The language's account of itself. It walks the same tree of words the
interpreter dispatches through, so it cannot describe a word that does
not exist and cannot leave one out that does.

The parentheses are optional when you want everything. Write a path in
them to ask about one part of the language, or the name of one of your own
variables to ask about its type.

    satellite.help

.  `1 19 0`  `satellite.help()`
> satellite.help(satellite.help)

Everything, from the root down: the topics that are built, one to a line.

Each line is indented one tab and carries the topic's path and a sentence
saying what it is, and the lines are separated by a blank one so a long
list stays readable on a narrow terminal. **Only what is built is
listed** — a path no milestone has reached is not offered as somewhere to
go.

This is the one to type when you do not yet know the name of the thing you
are looking for. Then ask about a topic by its path.

    satellite.help()

.  `1 19 1`  `satellite.help(x)`
> satellite.help(satellite.help)

One part of the language, or one of your own variables.

Give it a path and you get that node and the words underneath it, so
asking about `satellite.console` brings up display and input and the rest
together. Give it the name of a variable you declared and you get the help
for its type, which is why it still answers after the program that
declared it has finished.

**Nothing between the parentheses is evaluated.** The argument is a path
or a name, never a value, so asking about a module does not try to call
it.

    satellite.help(satellite.console)


## network

.  `1 20`  `satellite.network`
> satellite.help(satellite.network)

Talking to other machines: serving, connecting, sending and receiving.

**Not built** — M27 is the network, and it is the last of the language's
modules to be scheduled.

.  `1 20 0`  `satellite.network()`
> satellite.help(satellite.network)

The bare shape. **Not built** — M27.

.  `1 20 1`  `satellite.network.http(port)`
> satellite.help(satellite.network.http)

Serves HTTP on a port. **Not built** — M27.

.  `1 20 2`  `satellite.network.https(host, port)`
> satellite.help(satellite.network.https)

Connects to a host over HTTPS. **Not built** — M27.

.  `1 20 3`  `satellite.network.new`
> satellite.help(satellite.network.new)

Makes a connection. **Not built** — M27.

.  `1 20 4`  `satellite.network.open`
> satellite.help(satellite.network.open)

Opens one. **Not built** — M27.

.  `1 20 5`  `satellite.network.receive`
> satellite.help(satellite.network.receive)

Reads what arrived. **Not built** — M27.

.  `1 20 6`  `satellite.network.http(host, port)`
> satellite.help(satellite.network.http)

Connects to a host over HTTP. **Not built** — M27.

.  `1 20 7`  `satellite.network.https(port, cert, key)`
> satellite.help(satellite.network.https)

Serves HTTPS on a port, with a certificate and a key. **Not built** —
M27.


## returns

.  `1 21`  `satellite.returns`
> satellite.help(satellite.returns)

An optional note on a capsule's head line saying what kind of value it
hands back, written after the parameters and before the brace.

**It is optional and it is not how capsules are written here.** A capsule
without it returns values perfectly well, no file under `example/` uses
it, and DESIGN calls it optional where it specifies it. What you actually
write is `satellite.return(value)` in the body, and in
`satellite.main` that is `satellite.return(satellite)` on the last line.

    satellite.capsule name_of(satellite.variable.number n)
    {
        satellite.return(n.to_string())
    }

    satellite.console.display(name_of(7))


## system

.  `1 22`  `satellite.system`
> satellite.help(satellite.system)

The machine the program is running on, and the interpreter's own switches.

Thirty numbered paths sit under here and most of them describe memory. Two
pairs are built: how loose a search may be, and whether the prompt keeps
what a line declares.

    satellite.console.display(satellite.system.threshold())

.  `1 22 0`  `satellite.system()`
> satellite.help(satellite.system)

The bare shape. There is nothing to call; one of the words underneath is
what you write.

.  `1 22 1`  `satellite.system.delete`
> satellite.help(satellite.system.delete)

Removes a file or a directory from the machine.

**Not built.** The path is numbered and no milestone has reached it.

.  `1 22 2`  `satellite.system.environment`
> satellite.help(satellite.system.environment)

The environment the program was started in.

**Not built.** The path is numbered and no milestone has reached it.

.  `1 22 3`  `satellite.system.home`
> satellite.help(satellite.system.home)

The home directory of whoever is running the program.

**Not built** as a path. The interpreter already reads it; what is missing
is the word for it.

.  `1 22 4`  `satellite.system.memory`
> satellite.help(satellite.system.memory)

How much memory there is, in every sense of the question: the machine's,
the swap file's, and this program's own.

**Not built.** Twenty-eight numbered paths sit under here and no milestone
has reached any of them. Each of the counted ones has a second shape
taking a unit, so a number can come back in bytes or in something
readable.

.  `1 22 4 0`  `satellite.system.memory()`
> satellite.help(satellite.system.memory)

The bare shape. **Not built.**

.  `1 22 4 1`  `satellite.system.memory.bit`
> satellite.help(satellite.system.memory.bit)

Whether the machine counts in 32 bits or 64. **Not built.**

.  `1 22 4 2`  `satellite.system.memory.frequency`
> satellite.help(satellite.system.memory.frequency)

How fast the memory runs. **Not built.**

.  `1 22 4 3`  `satellite.system.memory.main()`
> satellite.help(satellite.system.memory.main)

The machine's main memory. **Not built.**

.  `1 22 4 4`  `satellite.system.memory.swap`
> satellite.help(satellite.system.memory.swap)

The swap file, which is the disk the machine uses when memory runs out.

**Not built.**

.  `1 22 4 4 0`  `satellite.system.memory.swap()`
> satellite.help(satellite.system.memory.swap)

The bare shape. **Not built.**

.  `1 22 4 4 1`  `satellite.system.memory.swap.free()`
> satellite.help(satellite.system.memory.swap.free)

How much swap is unused. **Not built.**

.  `1 22 4 4 2`  `satellite.system.memory.swap.total()`
> satellite.help(satellite.system.memory.swap.total)

How much swap there is altogether. **Not built.**

.  `1 22 4 4 3`  `satellite.system.memory.swap.used`
> satellite.help(satellite.system.memory.swap.used)

How much swap is in use. **Not built.**

.  `1 22 4 4 4`  `satellite.system.memory.swap.free(unit)`
> satellite.help(satellite.system.memory.swap.free)

How much swap is unused, in the unit you name. **Not built.**

.  `1 22 4 4 5`  `satellite.system.memory.swap.total(unit)`
> satellite.help(satellite.system.memory.swap.total)

How much swap there is altogether, in the unit you name. **Not built.**

.  `1 22 4 4 6`  `satellite.system.memory.swap(unit)`
> satellite.help(satellite.system.memory.swap)

The swap file, reported in the unit you name. **Not built.**

.  `1 22 4 5`  `satellite.system.memory.this`
> satellite.help(satellite.system.memory.this)

This program's own memory, as against the machine's. **Not built.**

.  `1 22 4 5 0`  `satellite.system.memory.this()`
> satellite.help(satellite.system.memory.this)

The bare shape. **Not built.**

.  `1 22 4 5 1`  `satellite.system.memory.this.available()`
> satellite.help(satellite.system.memory.this.available)

How much this program could still take. **Not built.**

.  `1 22 4 5 2`  `satellite.system.memory.this.free()`
> satellite.help(satellite.system.memory.this.free)

How much this program has asked for and is not using. **Not built.**

.  `1 22 4 5 3`  `satellite.system.memory.this.used`
> satellite.help(satellite.system.memory.this.used)

How much this program is using. **Not built.**

.  `1 22 4 5 4`  `satellite.system.memory.this.available(unit)`
> satellite.help(satellite.system.memory.this.available)

How much this program could still take, in the unit you name. **Not
built.**

.  `1 22 4 5 5`  `satellite.system.memory.this.free(unit)`
> satellite.help(satellite.system.memory.this.free)

How much this program has asked for and is not using, in the unit you
name. **Not built.**

.  `1 22 4 6`  `satellite.system.memory.free()`
> satellite.help(satellite.system.memory.free)

How much of the machine's memory is unused. **Not built.**

.  `1 22 4 7`  `satellite.system.memory.total()`
> satellite.help(satellite.system.memory.total)

How much memory the machine has altogether. **Not built.**

.  `1 22 4 8`  `satellite.system.memory.used()`
> satellite.help(satellite.system.memory.used)

How much of the machine's memory is in use. **Not built.**

.  `1 22 4 9`  `satellite.system.memory.main(unit)`
> satellite.help(satellite.system.memory.main)

The machine's main memory, in the unit you name. **Not built.**

.  `1 22 4 10`  `satellite.system.memory.free(unit)`
> satellite.help(satellite.system.memory.free)

How much of the machine's memory is unused, in the unit you name. **Not
built.**

.  `1 22 4 11`  `satellite.system.memory.total(unit)`
> satellite.help(satellite.system.memory.total)

How much memory the machine has altogether, in the unit you name. **Not
built.**

.  `1 22 4 12`  `satellite.system.memory.used(unit)`
> satellite.help(satellite.system.memory.used)

How much of the machine's memory is in use, in the unit you name. **Not
built.**

H  `1 22 5`  `satellite.system.threshold()`  _M16_
> satellite.help(satellite.system.threshold)

Answers how loose a search is allowed to be at the moment.

The scale runs from 1, which is an exact match and nothing else, up to
10. At 6 a substring matches either way round.

    satellite.console.display(satellite.system.threshold())

H  `1 22 6`  `satellite.system.threshold(n)`  _M16_
> satellite.help(satellite.system.threshold)

Sets how loose a search may be, and answers what it now is.

It is what `search` on a list or a map reads, so turn it up to find near
misses and down to insist on the exact thing.

    satellite.system.threshold(6)
    satellite.console.display(satellite.system.threshold())

H  `1 22 7`  `satellite.system.persist()`  _M22_
> satellite.help(satellite.system.persist)

Answers whether the prompt is keeping what each line declares.

It is on to begin with, which is why a variable you declare at the prompt
is still there on the next line.

    satellite.console.display(satellite.system.persist())

H  `1 22 8`  `satellite.system.persist(x)`  _M22_
> satellite.help(satellite.system.persist)

Turns the prompt's memory of declarations on or off, and answers what it
now is.

Turning it off does not throw away what is already held; it stops the next
line adding to it, so a program can switch it off around one line and back
on afterwards. It wants a real bool, so write
`satellite.bool.false` rather than a bare word.

    satellite.console.display(satellite.system.persist(satellite.bool.false))
    satellite.console.display(satellite.system.persist(satellite.bool.true))


## thread

.  `1 23`  `satellite.thread`
> satellite.help(satellite.thread)

Running more than one thing at once.

**Not built** — M23. The evaluator walks an arena rather than a linked
structure, which is what is meant to make threads possible here without
locking, but nothing starts one yet.

.  `1 23 0`  `satellite.thread()`
> satellite.help(satellite.thread)

The bare shape. **Not built** — M23.

.  `1 23 1`  `satellite.thread.new`
> satellite.help(satellite.thread.new)

Makes a thread. **Not built** — M23.


## window

.  `1 24`  `satellite.window`
> satellite.help(satellite.window)

Windows on the screen, drawn by the program rather than by the terminal.

**Not built** — M24. It is meant to arrive as a library loaded the first
time a program asks for a window, so a program that never opens one never
pays for it.

.  `1 24 0`  `satellite.window()`
> satellite.help(satellite.window)

The bare shape. **Not built** — M24.

.  `1 24 1`  `satellite.window.new`
> satellite.help(satellite.window.new)

Makes a window. **Not built** — M24.

.  `1 24 2`  `satellite.window.console`
> satellite.help(satellite.window.console)

A window that behaves like the terminal, so a program that prints can be
given one without being rewritten.

**Not built** — M24.

.  `1 24 2 0`  `satellite.window.console()`
> satellite.help(satellite.window.console)

The bare shape. **Not built** — M24.

.  `1 24 2 1`  `satellite.window.console.new(title, width, height)`
> satellite.help(satellite.window.console.new)

Makes a console window with the title and size you give. **Not built** —
M24.

