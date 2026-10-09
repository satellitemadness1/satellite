# satellite-004 — EXTENSIONS

**NOTHING HERE IS DECIDED.** This file keeps a conversation between the author and
Claude on 2026-09-24, so it can be picked up again later. The author set it aside
himself: *"you know I don't really like the extension idea at all that much, and we
have even more basic things to attend to, so let's leave this as an .md file with
where we got to"*. Nothing in this file is built, and nothing in it is a ruling.

It grew out of an item that was already open in MILESTONES.md M8 and M35: **a
spacesuit that extends a built-in type**, such as `satellite.variable.string`.
Spacesuits can already extend other spacesuits (`child(parent)`, M8). They cannot
extend a built-in type.

---

## 1. The author's first question

> first question, a spacesuit that extends a built-in type: so what does this look
> like in the equivalent python code? How do you extend a built in type? We would
> need a satellite.extend(type) { // code here }
>
> right?
>
> It wouldn't be a "class", it would have the syntax that I just gave

## 2. What Python does (run on this machine, CPython 3.12)

```python
class Shout(str):              # a NEW type, named Shout, built on str
    def loud(self):
        return self.upper() + "!"

s = Shout("hello")
print(s.loud())                # HELLO!
print(s.upper(), len(s))       # HELLO 5  -- everything str had still works
print(isinstance(s, str))      # True     -- a Shout is still a string

class Stack(list):             # the same with list
    def push(self, x):
        self.append(x)
    def peek(self):
        return self[-1]

st = Stack()
st.push(1); st.push(2)
print(st.peek(), len(st), st)  # 2 2 [1, 2]
```

What the run showed:

- **Only the new type gets the new method.** A plain string has no `loud()`:
  `hasattr("hello", "loud")` printed `False`.
- **Python will not add a method to `str` itself.** `str.loud = ...` failed with
  `TypeError: cannot set 'loud' attribute of immutable type 'str'`.

Some languages do allow the second thing. Swift calls it an *extension*, which is
close to the author's spelling (written from memory; there is no Swift on this
machine to run it):

```swift
extension String {
    func loud() -> String { return self.uppercased() + "!" }
}
"hello".loud()   // every string has it now
```

So there are **two different features**, and the question of which one is meant
is open:

1. **A new type built on a built-in, as in Python.** It has a name, such as
   `shout`. Only values declared as `shout` have `loud()`, and plain strings are
   unchanged. This is M8/M35's open item.
2. **New capsules for the built-in type itself, as in Swift.** It has no new name,
   and every `satellite.variable.string` has `loud()`. The author's
   `satellite.extend(type) { }` has no name in it, so it reads as this one.

## 3. The author's extensions folder

> Let's take this one step further than we already have it -- let's build an
> entire extensions folder, and this will be how we extend built in types, and
> whatever sits inside of the extensions folder is ran before any other code is
> executed, so that people can permanently extend the language...
>
> so we have "/extensions" and then in it is:
>
>     satellite.spacesuit type_name(satellite.string)
>     {
>         satellite.capsule new_method(something... as i'm typing this, isn't this just a
>         class?? Isn't an extension just a class?

## 4. Claude's answer: a class and an extension overlap, and differ in three ways

The author's sketch, `satellite.spacesuit type_name(satellite.variable.string)`,
**is** a class: a spacesuit extending a string, Python's `class Shout(str)`. An
extension makes no new type at all. The same capsule both ways (neither is built):

```
// A CLASS (a spacesuit built on string): a NEW type with a name
satellite.spacesuit shout(satellite.variable.string)
{
    satellite.public
    {
        satellite.capsule loud()
        {
            satellite.return(upper() + "!")
        }
    }
}
shout s = "hello"
s.loud()                  // works
"hello".loud()            // refused -- a plain string is not a shout

// AN EXTENSION: no new type, the string itself learns loud()
satellite.extend(satellite.variable.string)
{
    satellite.capsule loud()
    {
        satellite.return(upper() + "!")
    }
}
"hello".loud()            // works -- on every string
f[3].loud()               // a line read from a file too
```

1. **A name.** A class makes a new type that values are declared as (`shout s`). An
   extension makes nothing new, and the type is still `satellite.variable.string`.
2. **Who gets the capsule.** A class gives it only to values declared as that type.
   An extension gives it to every value of the type, including the ones satl hands
   back (a file's lines, console input, a `.join()`), with no conversion.
3. **Fields.** A class can hold fields of its own (`satellite.protected`). An
   extension cannot, because a string is only its characters, so there is nowhere
   to keep a field. It can only add capsules that work on the value.

The author's *"permanently extend the language"* fits **extensions** best: every
program would have `.loud()` on strings. Classes could live in the folder too, as
types every program has without an include.

## 5. The author's last sketch

> we need some syntax to add to the methods that the string already has I think,
>
> like this:
>
>     satellite.extension(satellite.variable.string)
>     {
>         satellite.capsule satellite.variable.string.add(args)
>         {
>             // some code to go with addition,

Two things to note about it. Neither is a ruling.

- **The spelling moved.** It is `satellite.extension(...)` here, and it was
  `satellite.extend(...)` in section 1.
- **It names the capsule by the language's full path,** `satellite.variable.string.add`.
  The language's own rule is that *a dotted path that starts with `satellite` names
  something the language owns, and a bare name is something you own* (FULL
  REFERENCE, first section). A capsule someone writes, named with a `satellite.`
  path, would be the first thing to cross that line. That may be the point of it,
  since an extension does become part of the language, but it is a choice to make
  on purpose.

---

## 6. Open questions — every one of them undecided

1. **Which feature:** a new type built on a built-in (section 2, meaning 1), new
   capsules on the built-in itself (meaning 2), or both.
2. **The spelling:** `satellite.extend(type)` or `satellite.extension(type)`, and
   whether the capsules inside are named bare (`loud`) or by the full path
   (`satellite.variable.string.add`, section 5).
3. **Where the folder lives:** next to each program, or in the install
   (`~/.satl/extensions`), so that it applies to every program on the machine.
   *"Permanently extend the language"* reads as the install.
4. **What "ran before any other code" means.** An extension is a declaration, like
   a capsule or a spacesuit, and declarations are read before `main` starts
   already. Is it enough for the folder to be read first, or should code in it
   also *run*, with statements executed before `main`?
5. **Programs on another machine:** a program that uses `.loud()` will not run
   where the extension is missing. Should satl name the missing extension in its
   refusal?
6. **Collisions:** may an extension replace something the language already has,
   such as its own `.upper()`? What happens when two extensions define the same
   capsule?
7. **Includes:** does an extension in a program's own folder reach the files that
   program includes?
8. **Fields:** section 4 point 3 says an extension cannot hold fields. Is that
   acceptable, or is it the reason to prefer a class?
