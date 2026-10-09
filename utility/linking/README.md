# utility/linking -- is this binary really self-contained?

    make link-test && ./build/link_test

Exit 0 means every vendored library is compiled into the binary and is the copy
`vendor/new/` says it should be. Exit 1 names what is wrong and which file caused it.

---

## The question it answers

`make GTK=vendor` already refuses to ship a binary whose `readelf -d` names anything
outside `ALLOWED_NEEDED` (`make_support/050-build.mk`). **That gate is necessary and it
is not sufficient.** It reads the file's dynamic section, and three real failures leave
the dynamic section perfectly correct.

### 1. The wrong copy, taken from the system

`fontconfig-2.18.3/meson.build:78-82` resolves expat like this:

    xml_dep = dependency('expat', required: false)     # pkg-config -- OUR expat 2.8.4
    if not xml_dep.found()
      xml_dep = cc.find_library('expat', required: false)   # the linker -- /usr's 2.7.3

**`cc.find_library()` asks the linker directly and ignores `PKG_CONFIG_PATH`.** The
isolation the whole vendored build rests on does not cover it. Build fontconfig before
expat has been staged into `vendor/stage` and it takes the system's copy, silently, and
every existing check still passes.

It is not only fontconfig. Sweeping the stack for `find_library()` on Linux:

| project | call | what it would take |
|---|---|---|
| cairo | `find_library('bfd')` | binutils libbfd, via `symbol-lookup` |
| harfbuzz | `find_library('iwasm')` | a WebAssembly runtime, via `wasm` |
| fontconfig | `find_library('expat')` | the system expat, if ordering is wrong |
| gdk-pixbuf | `find_library('mlib')` | Sun medialib |
| glib | `find_library('xattr')` | libattr |

### 2. The wrong headers with the right library

Compile against `/usr/include/expat.h` (2.7.3) while linking our `libexpat.a` (2.8.4) and
**nothing about the dynamic section is wrong**. `readelf` cannot see it, no test fails,
and the ABI mismatch surfaces as a crash months later. Comparing the version the HEADER
claimed at compile time against the version the LINKED code reports at run time is the
only way to catch it, and it is check D below.

### 3. Something dlopened at run time

`readelf -d` lists what the loader is *told* to load. It does not list what the process
*ends up holding*.

---

## What it does

It does not read the build log and it does not read the binary. **It asks the running
process**, which is the only witness that cannot be mistaken about what it contains --
the same argument `tests/` makes about satl: assert on the screen, not on the bytes you
hoped were written.

| | check | mechanism |
|---|---|---|
| **A** | every mapped object is allowed | `dl_iterate_phdr` -- the loader's own list, which includes anything arriving indirectly or through a `dlopen`, and so sees more than `readelf -d` |
| **B** | each library is compiled IN | `dladdr()` on one real symbol per library, comparing load addresses against this program's own object |
| **C** | it is the version we vendored | the library's runtime version call vs `vendor/new/` |
| **D** | header and library agree | the compile-time version macro vs the runtime call |

A failure in B prints the exact file the symbol came from:

    FAIL  expat            resolved from /lib64/libexpat.so.1

## What it does not prove

That a library is free of defects, that the version vendored was the right choice, or
that a `dlopen` which has not happened yet will be clean. **It proves identity and
provenance, at this moment, for this binary.** That is the part that was silently wrong
before, and it is all this claims.

---

## The table

`linked_libraries.def` is an X-macro list, the same mechanism `errors.def` and
`words.def` use: one row per library, and every check is driven from it, so a library
cannot be checked for its version and forgotten for its provenance. A library with no
runtime version API gets an empty string for that column and is still checked for
provenance -- say so honestly rather than inventing a version call that does not exist.

## Two bugs this found in itself, kept as a warning

Written and then run against a throwaway table with one symbol known to be compiled in
and one known to come from libc -- because **a check that has only ever passed proves
nothing**. It failed both:

- `linux-vdso.so.1` was not in the allowed list. It is the kernel's virtual object,
  present in every Linux process, on no disk and linked by nothing. A false FAIL.
- The compiled-in symbol was reported as a leak. `dladdr()` returns the path as
  invoked (`./link_test`); `readlink("/proc/self/exe")` returns the resolved absolute
  path. They never compare equal. **A false FAIL on the one check the utility exists
  for** -- now fixed by comparing load addresses, which need no filesystem and hold no
  opinion about how a path is spelled.
