# satellite.variable.* — where to start

> "everything except user defined classes are a `satellite.variable.something`,
> so that is a large part, I just don't know where to start"

Start with the **mechanism**, not with the types. There are going to be a dozen
of these and they are not a dozen problems — they are three problems, and only
one of them is hard.

## The three tiers

| Tier | Types | Where the value lives | Cleanup |
|---|---|---|---|
| **1 — values** | int, big, real, bool, string, list | inside the Container | refcount, done |
| **2 — inline values** | `time`, later `duration`, `point` | inside the Container | none at all |
| **3 — handles** | `file`, `thread`, `gui`, socket, process | outside the language | the hard one |

Tier 1 is finished. Tier 2 is nearly free. **Tier 3 is one piece of work that
delivers every type in it at once**, and that is where to start.

## Why tier 3 is one problem and not five

A file, a thread, a GTK window and a socket need exactly the same four things:

1. a tag saying which kind of native thing this is
2. a pointer to the thing
3. something to run when the last reference dies — `close(fd)`, join the
   thread, `g_object_unref(widget)`
4. a way to call methods on it — `.read()`, `.close()`, `.join()`, `.show()`

That is one struct and one table:

```cpp
struct NativeClass {
    const char* name;                                    // "file"
    void      (*destroy)(void* handle);                  // close, join, unref
    Container (*call)(void* handle, const std::string& method,
                      const Container* args, size_t n, std::string* err);
    std::string (*describe)(void* handle);               // for to_string()
    Container (*construct)(const Container* args, size_t n, std::string* err);
};

struct Native : Obj {
    const NativeClass* klass;
    void*              handle;
    Native() : Obj(Type::Native) {}
};
```

Build `satellite.variable.file` on its own and you write a lifetime bug. Build
it three times for file, thread and gui and you write three. Build the table
once and every future type is a row.

## `satellite.variable.X` and `satellite.X.new` are the same table row

This is the part that makes the design hold together. Both spellings resolve
through one registry:

```cpp
registry.add("file",   &kFileClass);      // satellite.variable.file  +  satellite.file.new
registry.add("thread", &kThreadClass);    // satellite.variable.thread + satellite.thread.new
registry.add("time",   &kTimeClass);      // satellite.variable.time  +  satellite.time.new
registry.add("gui",    &kGuiClass);       // satellite.variable.gui   +  satellite.gui.new_window
```

- `satellite.variable.file log` — a **declaration**. The lexer already produces
  `[Satellite][Dot][Ident "variable"][Dot][Ident "file"][Ident "log"]`, so the
  type name is just `tokens[i+4].text`, looked up in the registry.
- `satellite.file.new("out.txt", "w")` — a **construction**. Same lookup, then
  `klass->construct(...)`.

You cannot add one without the other, which is the property worth having. A
declared type that has no constructor, or a constructor for a type you cannot
declare, both become impossible rather than merely discouraged.

## Time should NOT be a handle

A point in time is a 64-bit integer of nanoseconds. Making it a `Native` means
a heap allocation and a refcount for something that fits in a register, and
benchmarking code creates these in loops.

So `time` gets its own inline tag and lives in the union like an int:

```cpp
Container t;  t.type = Type::Time;  t.i = nanoseconds_since_epoch;
```

`satellite.time.new()` still registers in the same table, so the language stays
uniform. Only the storage differs, and only where it is measurably worth it.

**There is room to do this for free.** Measured on this machine:

```
sizeof(Container) = 16
offset of type    = 0
offset of union   = 8
--> 7 spare bytes between the tag and the payload
```

Seven bytes in every Container are currently padding. A `Native` can keep its
class index in one of them instead of chasing `klass` through a pointer, and a
`time` can keep a unit or timezone tag in another. Nothing grows.

## Where the hard part actually is

**File** is the right first one. It is the simplest thing with a real lifetime
and its failure modes are all obvious and testable:

- open fails → must produce an error value, not a half-built handle
- the same fd must never be closed twice — two Containers pointing at one
  `Native` is normal, and the refcount is what makes that safe
- a file still open when the program ends → close it, do not leak it
- `.read()` on a closed file → an error, not a crash

**Thread is blocked, and it is worth being clear about why.** A thread needs
something to *run*, and that means a capsule and a virtual machine. Neither
exists yet. What can be built today is the type, `satellite.thread.new`, and a
thread that runs a `satellite.cxx` block — because that path already works. The
rest lands when the VM does.

This is also where the 20×10 worker design gets its real footing: a
`satellite.variable.thread` is a *handle to a worker*, not a fresh OS thread per
call. `satellite.thread.new` hands out a worker from the pool. That is what
makes 200 of them cost nothing when they are idle.

**GUI** is file with a different destructor. `g_object_unref` instead of
`close`. Once the table exists, GTK is a row plus method bindings.

## Why this is the piece that cashes in the clang work

Embedding clang gives satellite the whole C++ language. But today a
`satellite.cxx` block can only hand back what a Container can hold — a number,
a string, a list. It cannot hand back an object.

With `Type::Native` it can. A cxx block returns a `std::fstream*`, a socket, a
`GtkWidget*`, a Python object, an LLVM module — anything — wrapped in a handle
with a destructor, and satellite stores it in a variable like any other value.
That is the difference between "you can call C++" and "C++ is part of the
language".

## Order to build

1. `Type::Native`, `NativeClass`, the registry, the destructor path — the
   mechanism, with ASan proof that a handle is closed exactly once
2. `satellite.variable.file` — first real user, proves the mechanism
3. `satellite.variable.time` — inline tag, no allocation
4. `satellite.cxx` returning a handle — the payoff above
5. `satellite.variable.gui` — same shape as file, GTK's refcount as destroy
6. `satellite.variable.thread` — the type now, real capsule execution when the
   VM lands

## Sizing

`native.{hpp,cpp}` ~250 lines, `registry` ~120, `file` ~200, `time` ~120, tests
~400. The mechanism is the small part; the types are rows.
