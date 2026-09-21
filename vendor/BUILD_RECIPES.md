# vendor/BUILD_RECIPES.md -- how each vendored project is configured

**Generated 2026-09-20 from a 20-agent analysis** that read every project's build
files and then audited each recipe adversarially for options whose defaults would
silently link a system library. Saved here because it cost 2.3M tokens to derive and
lived only in a temporary file.

NOTHING BELOW HAS BEEN RUN. These are derived from reading, not from building. Expect
the first build to find defects -- the previous round of this took nine configure
rounds and found eight. Correct this file as they turn up.

## The rule every recipe serves

Build STATIC only, install into one shared `vendor/stage`, and take NOTHING from /usr.
`PKG_CONFIG_PATH` points only at the stage prefix -- but that only covers `dependency()`
calls. **`cc.find_library()` asks the linker directly and ignores it**, and meson itself
has a SYSTEM fallback for zlib (`mesonbuild/dependencies/dev.py:823`) that is bare
`cc.find_library('z')` and prints "found: YES". `utility/linking/link_test.cpp` exists
because of this.

---

## zlib 1.3.2

- **source** `zlib/zlib-1.3.2`
- **build system** zlib's own hand-written ./configure + Makefile.in. NOT autoconf, NOT meson. There IS a choice here and it matters: the tarball also ships CMakeLists.txt (9873 bytes, dated Feb 17 2026) and BUILD.bazel. Use ./configure. Reason, quoted from the project's own README-cmake.md: the CMake path builds minizip by default with `MINIZIP_ENABLE_BZIP2=ON -- Build minizip withj bzip2 support` and `A usable ins
- **installs** zlib.pc  -- installed to $STAGE/lib64/pkgconfig/zlib.pc (Makefile.in:337-339). Name: zlib, Version: 1.3.2 (configure substitutes @VERSION@ from zlib.h, configure:1074). This is the name every consumer asks for: meson `dependency('zlib')` resolves zlib.pc.
Also installs: libz.a -> $STAGE/lib64 (Makef
- **meson floor** n/a -- there is no meson.build in this tarball. (Note for the record: no wrapdb patch-zip meson.build has been dropped i

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage
SRC=/home/madness/code/cxx/satellite/vendor/zlib/zlib-1.3.2
B=/home/madness/code/cxx/satellite/vendor/build/zlib
CCW=/home/madness/code/cxx/satellite/vendor/build-cc   # the -w wrapper dir (cc, c++)

mkdir -p "$B" && cd "$B" && \
CC="$CCW/cc" \
CFLAGS="-O2 -fPIC" \
"$SRC/configure" \
  --static \
  --prefix="$STAGE" \
  --eprefix="$STAGE" \
  --libdir="$STAGE/lib64" \
  --sharedlibdir="$STAGE/lib64" \
  --includedir="$STAGE/include" \
  --mandir="$STAGE/share/man"

# build+install (do NOT run bare `make`, see traps):
make -j"$(nproc)" libz.a && make install
```

**Depends on:** NONE. This is the leaf of the whole stack.

- zlib.pc.in ships an empty Requires line: `Requires:` (zlib.pc.in, line 8). No Requires.private either.
- configure's probes are all libc, no AC_CHECK_LIB equivalent, no external -l anywhere: size_t (configure:617), off64_t / `-D_LARGEFILE64_SOURCE=1` (configure:596-605), fseeko (configure:615-621), unistd.h (configure:647), stdarg.h (configure:662), plus mmap/vsnprintf/snprintf checks further down.
- Makefile.in has no LIBS/LDLIBS pulling anything in; the static rule is just `libz.a: $(OBJS)` (Makefile.in:129).

Who depends on IT, since this is what fixes build order -- zlib must be first and is needed by five projects:
  glib/glib-2.90.0/meson.build:2304   `libz_dep = dependency('zlib')`                        (required, no version floor)
  harfbuzz/harfbuzz-14.4.0/meson.build:163  `zlib_dep = dependency('zlib', required: get_option('zlib'))

**Traps:** 1. THE BIG ONE -- meson will silently link /usr/lib64/libz.so if our zlib.pc is not visible, and will print "found: YES". meson-1.12.0/mesonbuild/dependencies/dev.py:823-828 registers `packages['zlib'] = DependencyFactory('zlib', [DependencyMethods.PKGCONFIG, DependencyMethods.CMAKE, DependencyMethods.SYSTEM], cmake=..., system=ZlibSystemDependency)`. If pkg-config misses, meson falls through to CMake's FindZLIB (which searches /usr) and then to ZlibSystemDependency, which is just `cc.find_library('z')` plus `has_header('zlib.h')` (dev.py:543-556) and reads the version out of the ZLIB_VERSION macro. Restricting PKG_CONFIG_PATH does NOT restrict that last method -- it is a bare compiler-and-linker probe against the default search path. This is exactly the failure mode named in constraint 2, and zlib is the one project in my three where it is live. Before configuring anything above zlib, run `pkg-config --variable=libdir zlib` and confirm it prints the stage path -- AND check that it printed something at all, because an empty result from pkg-config reads like success.

2. CMakeLists.txt is a trap, not an alternative. README-cmake.md: `MINIZIP_ENABLE_BZIP2=ON -- Build minizip withj bzip2 support` / `A usable installation of bzip2 is needed or config will fail`, and `ZLIB_BUILD_SHARED=ON`. bzip2 is not vendored. Do not reach for cmake because it looks more modern.

3. UNKNOWN OPTIONS ARE IGNORED, NOT REJECTED. configure:161 `*) unknown=1; echo "unknown option ignored: $1" | tee -a configure.log; shift;;`. A mistyped or autotools-habit flag (--enable-static, --disable-static, --

> **AUDIT [leak]** Trap 1 enumerates meson's fall-through for `dependency('zlib')` as pkg-config -> CMake -> ZlibSystemDependency and stops there. There is a FOURTH method it never mentions, and it is the one that does not merely link the wrong library but builds a second zlib inside our own tree: the subproject wrap. glib and fontconfig each ship subprojects/zlib.wrap declaring `[provide] zlib = zlib_dep`, and meso
> **FIX** Pass `--wrap-mode=nodownload` to every `meson setup` in the stack (prefer `--wrap-mode=nofallback`, which also refuses an already-unpacked subprojects/ directory), and never pass --force-fallback-for. Add "no subprojects/<name>/ directory was created" to the post-configure check alongside the existing pkg-config check.

---

## libffi 3.8.0

- **source** `libffi/libffi-3.8.0`
- **build system** autotools (GNU autoconf + automake + libtool). No choice to make: there is no meson.build and no CMakeLists.txt in the tree, only ./configure (707269 bytes, pre-generated -- no autogen needed) with Makefile.in, plus an msvc_build/ and an xcodeproj that are irrelevant here.
- **installs** libffi.pc -- installed to $STAGE/lib64/pkgconfig/libffi.pc (Makefile.am:39-40 `pkgconfigdir = $(libdir)/pkgconfig` / `pkgconfig_DATA = libffi.pc`). Name: libffi (from @PACKAGE_NAME@), Version: 3.8.0 (from @PACKAGE_VERSION@, configure.ac:5 `AC_INIT([libffi],[3.8.0],...)`). This is the name glib asks 
- **meson floor** n/a -- no meson.build in this tarball. (Also confirms no wrapdb patch-zip has been dropped into it, so it is clean under

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage
SRC=/home/madness/code/cxx/satellite/vendor/libffi/libffi-3.8.0
B=/home/madness/code/cxx/satellite/vendor/build/libffi
CCW=/home/madness/code/cxx/satellite/vendor/build-cc   # the -w wrapper dir (cc, c++)

mkdir -p "$B" && cd "$B" && \
PKG_CONFIG_PATH="$STAGE/lib64/pkgconfig:$STAGE/lib/pkgconfig" \
CC="$CCW/cc" CXX="$CCW/c++" \
CFLAGS="-O2 -fPIC" CXXFLAGS="-O2 -fPIC" CCASFLAGS="-O2 -fPIC" \
"$SRC/configure" \
  --prefix="$STAGE" \
  --libdir="$STAGE/lib64" \
  --includedir="$STAGE/include" \
  --mandir="$STAGE/share/man" \
  --disable-shared \
  --enable-static \
  --with-pic \
  --disable-multi-os-directory \
  --disable-docs \
  --disable-builddir \
  --disable-dependency-tracking \
  --enable-portable-binary

# then:
make -j"$(nproc)" && make install
```

**Depends on:** NONE. libffi is the second leaf, alongside zlib, and can be built in parallel with it.

Every check in configure.ac is a libc header or function -- there is not one AC_CHECK_LIB, not one PKG_CHECK_MODULES, not one -l in the whole file:
  configure.ac:81  `AC_CHECK_HEADERS(sys/memfd.h)`
  configure.ac:82  `AC_CHECK_FUNCS([memfd_create])`
  configure.ac:104 `AC_CHECK_FUNCS(memcpy)`
  configure.ac:105 `AC_CHECK_HEADERS(alloca.h)`
libffi.pc.in has no Requires line at all -- the whole file is prefix/exec_prefix/libdir/toolexeclibdir/includedir plus Name/Description/Version/Libs/Cflags.
Makefile.am:108 `libffi_la_LIBADD = $(TARGET_OBJ)` -- its own target objects, nothing external.

Who depends on IT (build order): exactly one consumer in this stack --
  glib/glib-2.90.0/meson.build:2302  `libffi_dep = dependency('libffi', version : '>= 3.0.0', default_options: {'werror': false, 'tests': false,

**Traps:** 1. THE .pc POINTS SOMEWHERE ELSE THAN WHERE IT LIVES. libffi.pc.in:9 is `Libs: -L${toolexeclibdir} -lffi` while the file itself installs to `$(libdir)/pkgconfig`. Those are two different variables and the default configuration is free to make them differ (configure.ac:441-447). pkg-config will cheerfully report success while handing out a -L to a directory that has no libffi.a in it -- and the link that fails is glib's, not libffi's. --disable-multi-os-directory collapses them. After installing, verify with `pkg-config --libs libffi` and confirm the -L it prints is the directory the .a is actually in.

2. NON-PIC STATIC OBJECTS, invisible until the final link. Covered above; --with-pic plus -fPIC in CFLAGS and CCASFLAGS. Nothing warns at libffi time.

3. UNSET CFLAGS SILENTLY REWRITES YOUR BUILD. AX_CC_MAXOPT (configure.ac:62). With CFLAGS unset and the vendor detected as `gnu`, you get -O3 -fomit-frame-pointer -ffast-math and a guessed -march (m4/ax_cc_maxopt.m4:151-167, configure:18487). Subtlety worth knowing: m4/ax_compiler_vendor.m4:15,77 detects clang as its OWN vendor, `clang`, and m4/ax_cc_maxopt.m4 has NO `clang)` case (the cases are dec, sun, hp, ibm, intel, nvhpc, gnu, microsoft) -- so with clang the case falls through with CFLAGS still empty and the fallback at m4/ax_cc_maxopt.m4:174-183 fires instead, printing a full-width banner reading `WARNING: Don't know the best CFLAGS for this system` and setting CFLAGS="-O3". So under clang you get an alarming-looking warning that is harmless, and you do NOT get the -march -- but that safety is an accident of vendor dete

> **AUDIT [leak]** Same wrap door as zlib, and the libffi recipe's closing claim makes it worse: "Nothing in libffi's option set can pull in a system library... PKG_CONFIG_PATH is exported on the command line above only for consistency; libffi never reads it." True of libffi's own build, and irrelevant to the risk. The risk is on the consumer side: glib ships subprojects/libffi.wrap providing `dependency_names = lib
> **FIX** `--wrap-mode=nodownload` on glib's meson setup, plus the PKG_CONFIG_LIBDIR fix from finding 1. Keep the recipe's own post-install check (`pkg-config --libs libffi` must print a -L that is the directory libffi.a is actually in) — with --disable-multi-os-directory and --libdir=$STAGE/lib64 that is $STAGE/lib64, and it is what stops the wrap from ever being reached.

> **AUDIT [missing-option]** Question (4) is answered wrong. The recipe says "No dlopen, no runtime data files... with FFI_EXEC_STATIC_TRAMP on x86_64 Linux it uses memfd rather than a temp file." memfd is the SECOND choice, not the first. On Linux the static-trampoline init reads /proc/<pid>/maps, finds the mapping that contains libffi's own trampoline text, and open()s and PROT_EXEC-mmaps that file. Once libffi.a is linked 
> **FIX** Keep the default (do not pass --disable-exec-static-tramp) but record that satl maps its own executable PROT_EXEC at first closure creation; if you want the memfd-only path instead, pass `--disable-exec-static-tramp` explicitly. Either way add the 12 GB/never-strip policy note: the mapped file is satl, so the binary must stay readable in place for the lifetime of the process.

---

## gperf 3.3

- **source** `gperf/gperf-3.3`
- **build system** autotools, but an unusual hand-rolled recursive one and it changes what you can pass. The top configure.ac is 50 lines: `AC_INIT`, `AC_CONFIG_FILES([Makefile])`, `AC_CONFIG_SUBDIRS([lib src tests doc])`, and an extrasub that turns one @subdir@ line into four. The top Makefile.in is not automake -- every target is literally `cd @subdir@; $(MAKE) <target>`. Only lib/ is automake; src/, tests/ and do
- **installs** NONE. This is the answer and it is the important one: gperf installs no .pc file, no library and no headers. It provides $STAGE/bin/gperf, and the next project up finds it through PATH -- not through PKG_CONFIG_PATH.
Full install manifest: $STAGE/bin/gperf (src/Makefile.in:137-139), $STAGE/share/inf
- **meson floor** n/a -- no meson.build in this tarball, and gperf is never built by meson. (It is *invoked* by meson, from fontconfig's b

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage
SRC=/home/madness/code/cxx/satellite/vendor/gperf/gperf-3.3
B=/home/madness/code/cxx/satellite/vendor/build/gperf
CCW=/home/madness/code/cxx/satellite/vendor/build-cc   # the -w wrapper dir (cc, c++)

mkdir -p "$B" && cd "$B" && \
CC="$CCW/cc" CXX="$CCW/c++" \
CFLAGS="-O2" CXXFLAGS="-O2" \
"$SRC/configure" \
  --prefix="$STAGE" \
  --bindir="$STAGE/bin" \
  --infodir="$STAGE/share/info" \
  --mandir="$STAGE/share/man" \
  --docdir="$STAGE/share/doc/gperf" \
  --htmldir="$STAGE/share/doc/gperf" \
  --disable-dependency-tracking

# then:
make -j"$(nproc)" && make install

# AND, for every project configured after this one -- this is the whole point:
export PATH="$STAGE/bin:$PATH"
```

**Depends on:** Effectively none. Two things, both trivial:

1. libm -- OPTIONAL by construction, and the only external library in the whole project:
     src/configure.ac:62  `AC_CHECK_LIB([m], [rand], [GPERF_LIBM="-lm"], [GPERF_LIBM=""])`
     src/configure.ac:63  `AC_SUBST([GPERF_LIBM])`
     src/Makefile.in:77   `LIBS     = ../lib/libgp.a @GPERF_LIBM@`
   If the probe fails the variable is simply empty. No version floor. This is a system library and it is fine: it links into the host tool, not into satl.

2. A C++ compiler:
     src/configure.ac  `AC_PROG_CXX` / `AC_PROG_CXXCPP`
   Plus a C++ VLA probe that sets HAVE_DYNAMIC_ARRAY:
     src/configure.ac:44-50  `AC_CACHE_CHECK([for stack-allocated variable-size arrays], [gp_cv_cxx_dynamic_array], ... [AC_LANG_PROGRAM([[int func (int n) { int dynamic_array[n]; }]], [[]])] ...)`

No pkg-config call anywhere. No PKG_CHECK_MODULES. No version floors of a

**Traps:** 1. THE DISCIPLINE THAT PROTECTS EVERY OTHER PROJECT DOES NOT APPLY HERE. Constraint 2 is enforced by narrowing PKG_CONFIG_PATH. gperf is found through PATH. Narrowing PKG_CONFIG_PATH does exactly nothing for it. If $STAGE/bin is not on PATH when fontconfig is configured, meson will pick up /usr/bin/gperf if the machine has one -- and fontconfig will configure and build perfectly happily, with no warning, using a gperf that is not ours. That is the same silent-substitution shape as the system-.so leak, arriving through a different door.

2. AND THEREFORE: A SUCCESSFUL FONTCONFIG CONFIGURE IS NOT EVIDENCE OUR GPERF WAS USED. Check the meson log for the `Program gperf found: YES (<path>)` line and read the path. Do not infer it from the absence of an error -- absence of an error is what the wrong gperf also produces.

3. FONTCONFIG HARD-REQUIRES IT; THE GENERATED HEADER IS NOT IN THE TARBALL. fontconfig/fontconfig-2.18.3/src/ contains only `fcobjshash.gperf.h` (the template). The real header is produced at build time: src/meson.build:54-66 preprocesses the template into fcobjshash.gperf and then runs `command: [gperf, '--pic', '-m', '100', '@INPUT@', '--output-file', '@OUTPUT@']`. And the not-found path is not graceful -- meson.build:480-485 is `if gperf_len_type == ''` -> `error('unable to determine gperf len type')`, else the fallback branch does `gperf = find_program('gperf')` with no `required: false`, which aborts. So gperf must be built and installed BEFORE fontconfig is even configured, not merely before it is compiled.

4. IT IS NOT A LIBRARY AND THERE IS NOTHING TO MA

> **AUDIT [leak]** Trap 3's conclusion is wrong in the direction that costs you. It says the not-found path "is not graceful" and that meson.build:485's `gperf = find_program('gperf')` "with no `required: false` ... aborts". It does not abort. fontconfig ships subprojects/gperf.wrap whose `[provide] program_names=gperf` makes that call resolve through the wrap, and with the default --wrap-mode meson git-clones a thi
> **FIX** Pass `--wrap-mode=nodownload` (better `nofallback`) to fontconfig's meson setup so the fallback cannot fire, in addition to `export PATH="$STAGE/bin:$PATH"`. Then read the configure log for `Program gperf found: YES (<path>)` and confirm the path is $STAGE/bin/gperf, and confirm no subprojects/gperf/ directory appeared.

> **AUDIT [ordering]** The gperf recipe owns the PATH contract for everything downstream — `export PATH="$STAGE/bin:$PATH"`, described as "the whole point" — but it only puts $STAGE/bin on PATH. `meson` on this machine's PATH is /usr/bin/meson at version 1.4.1, while constraint 8 and every line-number citation in these recipes (meson-1.12.0/mesonbuild/utils/universal.py:1254-1255 for the lib64 default, meson-1.12.0/meso
> **FIX** Extend the exported PATH so the vendored meson leads too, e.g. `export PATH="$STAGE/bin:/home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0:$PATH"`, or invoke it by absolute path in every setup line; then assert `meson --version` prints 1.12.0 before the first meson project. Same discipline for the top-level gperf configure: `--disable-dependency-tracking` is not recognised there (only the 

---

## expat

- **source** `expat/expat-2.8.4`
- **build system** autotools. There IS a choice — expat ships both a ready ./configure (754614 bytes, pre-generated) and a full CMakeLists.txt (42847 bytes) with expat.pc.cmake. Use autotools: configure is already generated so no autogen/autoreconf is needed (constraint 8), it installs expat.pc unconditionally (Makefile.am:57 `pkgconfig_DATA = expat.pc`), and its option surface for 'do not build the extra stuff' (--
- **installs** expat.pc — installed into $libdir/pkgconfig (Makefile.am:57-58 `pkgconfig_DATA = expat.pc` / `pkgconfigdir = $(libdir)/pkgconfig`), i.e. /home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig/expat.pc. Module name is `expat` (expat.pc.in: `Name: @PACKAGE_NAME@`), exactly the name fontconfig ask
- **meson floor** n/a — expat is not a meson project. It has no meson.build and no meson_options.txt (the tree is configure / configure.ac

**Configure:**
```sh
mkdir -p /home/madness/code/cxx/satellite/vendor/build/expat && \
cd /home/madness/code/cxx/satellite/vendor/build/expat && \
CC=/home/madness/code/cxx/satellite/vendor/build-cc/cc \
CXX=/home/madness/code/cxx/satellite/vendor/build-cc/c++ \
CFLAGS="-O2 -fPIC -fvisibility=default" \
PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
/home/madness/code/cxx/satellite/vendor/expat/expat-2.8.4/configure \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=/home/madness/code/cxx/satellite/vendor/stage/lib \
  --disable-shared \
  --enable-static \
  --enable-pic \
  --without-xmlwf \
  --without-examples \
  --without-tests \
  --without-docbook \
  --disable-symbol-versioning \
  --disable-dependency-tracking \
  --disable-silent-rules

(build/install: `make -j<N> && make install` from that same build directory.)
```

**Depends on:** NONE. expat is a true leaf — it declares no external library whatsoever.

Evidence of absence, which is the point: `grep -n "LIBM|AC_CHECK_LIB|AC_SEARCH_LIBS|PKG_CHECK|AC_CHECK_FUNCS" configure.ac` returns ZERO lines, and `grep -c PKG_CONFIG configure` returns 0. There is no PKG_CHECK_MODULES, no AC_CHECK_LIB and no pkg-config invocation anywhere in the generated configure.

The single library that ends up referenced is libm, and it comes from libtool's LT_LIB_M, not from configure.ac: expat.pc.in line 11 is `Libs.private: @LIBM@`, and LIBM is a real substituted variable (configure:695 lists LIBM in ac_subst_vars; configure:21440 `LIBM=-lm` is the Linux branch). -lm is glibc, so it is not a vendoring concern.

Build-time-only, all optional, all turned off above: docbook2x-man / db2x_docbook2man / docbook2man / docbook-to-man (configure.ac:449, for the xmlwf man page); a C++11 compiler (c

**Traps:** 1. THE HIDDEN-SYMBOL TRAP, and it is silent. configure.ac:164-167 adds -fvisibility=hidden ALWAYS and the counterweight -DXML_ENABLE_VISIBILITY=1 ONLY for a shared build. lib/expat_external.h:101-111 shows the consequence: with XML_ENABLE_VISIBILITY undefined it falls through to `#ifndef XMLIMPORT / #define XMLIMPORT` (empty), so nothing carries visibility("default"). Net effect: libexpat.a comes out with every XML_* symbol marked hidden. That is FINE if libexpat.a and its only caller (fontconfig) land in the same final link unit — which is the plan. It breaks the moment expat lands in one .so and a caller lands in another, which is exactly 004's one-.so-per-word shape. Check on the artifact: `readelf -sW stage/lib/libexpat.a | grep XML_Parse` — LOCAL/HIDDEN means it bit. The fix is the CFLAGS entry, not a source edit. The visibility probe itself WILL succeed under the -w wrapper: conftools/expatcfg-compiler-supports-visibility.m4 compiles with `-fvisibility=hidden -Wall -Werror -Wno-unknown-warning-option` and tests for SUCCESS (not, as pcre2 does, for a warning becoming an error), so -w does not accidentally disable it.

2. THE FONTCONFIG FALL-THROUGH — the system leak to actually fear, and it is downstream of me. fontconfig-2.18.3/meson.build:78-90: if `dependency('expat', required: false)` misses, meson does NOT stop; it tries `cc.find_library('expat', required : false)`, which resolves /usr/lib64/libexpat.so, and if that misses too it falls to `dependency('libxml-2.0', required: true)` and switches the whole XML backend to the SYSTEM libxml2. Two different system .so l

> **AUDIT [leak]** The recipe's own trap 2 names the right file but mitigates the wrong half. `-Dxml-backend=expat` closes ONLY the libxml2 branch at fontconfig meson.build:88. The silent /usr leak is meson.build:80, and -Dxml-backend=expat leaves it fully live: if `dependency('expat')` at :78 misses, :80 runs `cc.find_library('expat')`, which searches the COMPILER's default library path and is completely outside PK
> **FIX** Keep -Dxml-backend=expat, but gate fontconfig on a positive check, not on silence: before `meson setup`, run `PKG_CONFIG_LIBDIR=<stage>/lib/pkgconfig:<stage>/lib64/pkgconfig pkg-config --exists expat && pkg-config --modversion expat` and require it to print 2.8.4 — count the output, do not read an empty result as success (the same trap already in vendor/edit_journal/gtk-old/EDITS.md). After config

> **AUDIT [missing-option]** The recipe pins six options whose defaults are already correct and misses the one option whose default points the wrong way — exactly the class it caught for pcre2 ("--disable-symvers : THE ONE DEFAULT-ON OPTION IN THE LIST"). expat turns maintainer mode ON by default, which makes the autotools rebuild rules live. Two consequences. (a) Those rules write INTO the upstream tree: `$(am__cd) $(srcdir)
> **FIX** Add `--disable-maintainer-mode` to the expat configure line. (pcre2 needs no equivalent: its help reads `--enable-maintainer-mode  enable make rules...`, i.e. default off.)

> **AUDIT [wrong-option]** `-fvisibility=default` in CFLAGS is unnecessary here and it removes protection upstream put in deliberately. The recipe justifies it by 004's one-.so-per-word shape, but expat has exactly one consumer anywhere in this stack — fontconfig — and fontconfig lands in the same final link unit as expat, so hidden symbols are never asked to cross a .so boundary. Forcing default visibility puts XML_ParserC
> **FIX** Drop `-fvisibility=default`, leaving CFLAGS="-O2 -fPIC", and let upstream's -fvisibility=hidden stand. Verify with `readelf -sW stage/lib/libexpat.a | grep XML_ParserCreate` — LOCAL/HIDDEN is the wanted result for a library that only ever links into satl. Re-add the flag only if a word .so is later found to need expat directly.

---

## pcre2

- **source** `pcre2/pcre2-10.48`
- **build system** autotools. There is again a choice (a 56509-byte CMakeLists.txt, plus BUILD.bazel and build.zig). Use autotools: the ./configure is pre-generated (634008 bytes) per constraint 8, and the .pc files come straight from libpcre2-8.pc.in / libpcre2-posix.pc.in with the static -DPCRE2_STATIC flag wired in by configure itself (configure.ac:741-753 and the PCRE2_STATIC_CFLAG block). Out-of-tree VPATH buil
- **installs** With this configuration, exactly two, both into $libdir/pkgconfig (Makefile.am:912-916 `pkgconfigdir = $(libdir)/pkgconfig` / `pkgconfig_DATA =` / `pkgconfig_DATA += libpcre2-8.pc libpcre2-posix.pc`):

  libpcre2-8.pc      <- THIS is the one glib looks for, by that exact name.
  libpcre2-posix.pc  <
- **meson floor** n/a — pcre2 is not a meson project. No meson.build and no meson_options.txt in the tree (it ships configure / configure.

**Configure:**
```sh
mkdir -p /home/madness/code/cxx/satellite/vendor/build/pcre2 && \
cd /home/madness/code/cxx/satellite/vendor/build/pcre2 && \
CC=/home/madness/code/cxx/satellite/vendor/build-cc/cc \
CFLAGS="-O2 -fPIC" \
PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
/home/madness/code/cxx/satellite/vendor/pcre2/pcre2-10.48/configure \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=/home/madness/code/cxx/satellite/vendor/stage/lib \
  --disable-shared \
  --enable-static \
  --with-pic \
  --enable-pcre2-8 \
  --disable-pcre2-16 \
  --disable-pcre2-32 \
  --enable-unicode \
  --enable-jit \
  --disable-pcre2grep-libz \
  --disable-pcre2grep-libbz2 \
  --disable-pcre2test-libedit \
  --disable-pcre2test-libreadline \
  --disable-valgrind \
  --disable-coverage \
  --disable-debug \
  --disable-fuzz-support \
  --disable-diff-fuzz-support \
  --disable-rebuild-chartables \
  --disable-symvers \
  --disable-dependency-tracking \
  --disable-silent-rules

(build/install: `make -j<N> && make install` from that same build directory. If the prefix must stay free of the man pages and HTML docs, `make install dist_man_MANS= dist_doc_DATA=` — there is no configure switch for them.)
```

**Depends on:** NO REQUIRED EXTERNAL DEPENDENCY. pcre2 is a leaf: it can be built before everything except a compiler. Every library it can touch is optional, belongs to a helper PROGRAM rather than to the libraries we ship, and is switched off above. None carries a version floor — pcre2 states no minimum version for any of them.

  zlib        optional, pcre2grep only, DEFAULT OFF. configure.ac:354-357 `AC_ARG_ENABLE(pcre2grep-libz, AS_HELP_STRING([--enable-pcre2grep-libz],[link pcre2grep with libz to handle .gz files]), , enable_pcre2grep_libz=no)`. Links via configure.ac:1086 `LIBZ="-lz"` into Makefile.am:617 `pcre2grep_LDADD = $(LIBZ) $(LIBBZ2)`. No version floor stated.
  bzip2       optional, pcre2grep only, DEFAULT OFF. configure.ac:360-363 `AC_ARG_ENABLE(pcre2grep-libbz2, ..., enable_pcre2grep_libbz2=no)`; configure.ac:1099 `LIBBZ2="-lbz2"`. No version floor.
  libreadline optional, pcre2test on

**Traps:** 1. THE TWO FALSE ALARMS IN THE CONFIGURE LOG. The audit method that found the last round's leaks — grepping the log for a dependency reported as found — produces two hits here that are NOT leaks. configure.ac:648 `AC_CHECK_LIB([z], [gzopen], [HAVE_LIBZ=1])` and the hand-rolled libbz2 test at configure.ac:672-683 both run UNCONDITIONALLY, regardless of --disable-pcre2grep-libz/-libbz2, and both will print 'yes' on this machine. Neither leaves anything behind: AC_CHECK_LIB with an explicit action-if-found does NOT append -lz to LIBS, and the bz2 test does `OLD_LIBS="$LIBS"` ... `LIBS="$OLD_LIBS"`. HAVE_LIBZ/HAVE_LIBBZ2 are only consulted inside the enable_* guards at configure.ac:1080-1101. The check that actually settles it is on the artifact, not the log: after `make`, `grep -E ' -lz| -lbz2' Makefile` in the build dir, and `nm -u` the installed libpcre2-8.a for gzopen / BZ2_bzopen.

2. THE -w WRAPPER SILENTLY DISABLES PCRE2'S VISIBILITY SUPPORT, and that is FINE. m4/pcre2_visibility.m4 first asks 'whether the -Werror option is usable' by compiling a program containing `#warning e` with -Werror and expecting that to FAIL. Under the wrapper that appends -w the warning is suppressed, the compile SUCCEEDS, pcre2_cv_cc_vis_werror=no, and the entire `if test $pcre2_cv_cc_vis_werror = yes` block is skipped — VISIBILITY_CFLAGS stays empty, HAVE_VISIBILITY=0, `AC_DEFINE(PCRE2_EXPORT, [])`. You will see 'checking whether the -Werror option is usable... no' in the log. This is the OPPOSITE of expat's problem and is benign for us: pcre2's symbols come out default-visible, which is what

> **AUDIT [missing-option]** On the leak axis pcre2 is clean — I enumerated every AC_ARG_ENABLE/AC_ARG_WITH in configure.ac (lines 194-482) plus PCRE2_CHECK_VSCRIPT's --disable-symvers, and every one that can reach a system library (pcre2grep-libz, pcre2grep-libbz2, pcre2test-libedit, pcre2test-libreadline, valgrind, coverage, symvers) is already pinned off; the readline/libedit cascades are correctly guarded by `if test "$en
> **FIX** Extend the install suppression to `make install dist_man_MANS= dist_doc_DATA= bin_SCRIPTS=`, or accept stage/bin/{pcre2grep,pcre2test,pcre2-config} as harmless clutter and say so explicitly. And re-cite the static flag as configure.ac:728-738, not 741-753.

---

## libpng

- **source** `libpng/libpng-1.6.58`
- **build system** autotools — both are present (a ready `./configure` and a `CMakeLists.txt`). Use autotools. The CMake path finds zlib with `find_package(ZLIB REQUIRED)` (CMakeLists.txt:141) and offers only `ZLIB_ROOT` to steer it, so with an unset ZLIB_ROOT it goes straight to /usr and says nothing. The autotools path resolves zlib purely out of CPPFLAGS/LDFLAGS, which is the same mechanism the rest of this stack
- **installs** stage/lib64/pkgconfig/libpng16.pc — Makefile.am:211-212 `pkgconfigdir = @pkgconfigdir@` / `pkgconfig_DATA = libpng@PNGLIB_MAJOR@@PNGLIB_MINOR@.pc`. PLUS stage/lib64/pkgconfig/libpng.pc as a symlink to it, installed by the `install-libpng-pc` hook which is wired in under `if DO_INSTALL_LIBPNG_PC` (Ma
- **meson floor** n/a — libpng 1.6.58 ships no meson.build. (Note meson wrapdb has one; dropping it in would be an edit under vendor/ by R

**Configure:**
```sh
mkdir -p /home/madness/code/cxx/satellite/vendor/libpng/build-static && cd /home/madness/code/cxx/satellite/vendor/libpng/build-static && CC=/home/madness/code/cxx/satellite/vendor/gtk-old/build-cc/cc CXX=/home/madness/code/cxx/satellite/vendor/gtk-old/build-cc/c++ PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig CPPFLAGS=-I/home/madness/code/cxx/satellite/vendor/stage/include LDFLAGS="-L/home/madness/code/cxx/satellite/vendor/stage/lib64 -L/home/madness/code/cxx/satellite/vendor/stage/lib" ../libpng-1.6.58/configure --prefix=/home/madness/code/cxx/satellite/vendor/stage --libdir=/home/madness/code/cxx/satellite/vendor/stage/lib64 --disable-shared --enable-static --with-pic --disable-dependency-tracking --disable-tools --disable-tests --disable-werror --enable-hardware-optimizations=yes --enable-unversioned-links --enable-unversioned-libpng-pc --without-binconfigs
```

**Depends on:** zlib — REQUIRED, hard error, no version floor anywhere in the build files. configure.ac:175-177: `AC_CHECK_LIB([z], [zlibVersion], , [AC_CHECK_LIB([z], [${ZPREFIX}zlibVersion], , [AC_MSG_ERROR([zlib not installed])])])`. The only version statement is prose in README:17 — 'You should use zlib 1.0.4 or later to run this'. It is also declared to consumers in libpng.pc.in: `Requires.private: zlib`, so zlib.pc must be in the stage for `pkg-config --static libpng` to resolve.
libm — REQUIRED, no floor. configure.ac:161-163: `AC_CHECK_FUNCS([pow], , [AC_CHECK_LIB([m], [pow], , [AC_MSG_ERROR([cannot find pow])])])`. glibc, not a leak.
libm feenableexcept — OPTIONAL, test-only, no floor. configure.ac:182: `AC_CHECK_LIB([m], [feenableexcept])` ('it's not an error if it is not found', configure.ac:179-181).
That is the entire list. libpng declares nothing else — no fontconfig, no brotli, no bzip2 (

**Traps:** 1. `--with-zlib-prefix` IS NOT A PATH. It reads like the way to point libpng at the staged zlib and it is not. configure.ac:170-174 — `[ZPREFIX=${withval}], [ZPREFIX='z_']` — it is a SYMBOL-NAME prefix, used only as `AC_CHECK_LIB([z], [${ZPREFIX}zlibVersion])` for zlibs whose exports were renamed. Passing `--with-zlib-prefix=/home/.../stage` configures successfully and changes nothing.
2. There is NO option to pin which zlib is used. If stage/lib64/libz.a does not exist yet when configure runs, `AC_CHECK_LIB([z],[zlibVersion])` succeeds against /usr/lib64/libz.so (which on this machine is zlib-ng: `libz.so.1.3.1.zlib-ng`), reports 'yes', and there is no error branch to catch it. Build zlib into the stage FIRST, then grep config.log for the -L that won.
3. Same for the header: png.h and pngpriv.h `#include <zlib.h>` and there is no AC_CHECK_HEADER for it, so a stale /usr/include/zlib.h is used silently unless CPPFLAGS puts the stage include first.
4. The CMake path is a trap by itself: `find_package(ZLIB REQUIRED)` (CMakeLists.txt:141) with no ZLIB_ROOT, and `option(PNG_SHARED ... ON)` / `option(PNG_TESTS ... ON)` (lines 73, 87) all default ON. Do not fall back to it.
5. `install-library-links` (Makefile.am:412-422) runs `rm -f "libpng.$$ext"` in $libdir for `EXT_LIST = a dll.a so so.1.6.58 la sl dylib` before symlinking — it will delete a libpng.so sitting in the stage. Harmless here; surprising if someone ever stages a shared copy.
6. If you turn OFF --enable-unversioned-links, anything that `#include <png.h>` without going through pkg-config breaks, because the real heade

> **AUDIT [missing-option]** `--without-binconfigs` is passed but `--disable-unversioned-libpng-config` is NOT, and those two are independent switches. `--without-binconfigs` empties `bin_SCRIPTS` (configure.ac:258 `binconfigs=`, Makefile.am:122 `bin_SCRIPTS= @binconfigs@`), so nothing is installed into `$(bindir)`. But `DO_INSTALL_LIBPNG_CONFIG` is still TRUE — configure.ac:313-314 `AM_CONDITIONAL([DO_INSTALL_LIBPNG_CONFIG],
> **FIX** Add `--disable-unversioned-libpng-config` next to `--without-binconfigs`.

> **AUDIT [missing-option]** Three man pages are installed into `stage/share/man/` on every `make install`, and no configure switch exists to stop it. `dist_man_MANS` at Makefile.am:117 is at top level — it is outside the `if ENABLE_TOOLS` block (Makefile.am:25-29) and outside `if ENABLE_TESTS`, so `--disable-tools --disable-tests` do not touch it. I checked the complete option list in `ac_user_opts` (configure:822-845: enabl
> **FIX** Redirect them at install time rather than configure time: `make install mandir=/tmp/claude-1000/.../discard-man` (or accept the three files). Do not pass `--mandir` at configure — that only moves them inside the stage.

> **AUDIT [ordering]** The recipe discusses `Requires.private` for libtiff but never for libpng, and libpng has one. libpng.pc.in:9 hard-codes `Requires.private: zlib`, so the moment any consumer resolves libpng statically — cairo, freetype and gdk-pixbuf all ask for the module name `libpng` (gdk-pixbuf-2.44.8/meson.build:292 `png_dep = dependency(is_msvc_like ? 'png' : 'libpng', required: false)`) — pkg-config must fin
> **FIX** Give zlib the same `--libdir=/home/madness/code/cxx/satellite/vendor/stage/lib64` when it is built, and after staging zlib verify `stage/lib64/pkgconfig/zlib.pc` exists before configuring libpng — libpng itself never calls pkg-config (zero `PKG_CONFIG` hits in its generated `configure`), so this failure surfaces only later, in cairo or gdk-pixbuf.

---

## libjpeg-turbo

- **source** `libjpeg-turbo/libjpeg-turbo-3.2.0`
- **build system** cmake — it is the only one. There is no ./configure and no meson.build in the tree; `cmake_minimum_required(VERSION 3.15...3.28)` (CMakeLists.txt:1) and the system cmake is 3.31.8, which is inside that range's policy window. Generator: Ninja (/usr/bin/ninja, 1.11.1).
- **installs** stage/lib64/pkgconfig/libjpeg.pc — generated unconditionally (cmakescripts/BuildPackages.cmake:177 `configure_file(release/libjpeg.pc.in pkgscripts/libjpeg.pc @ONLY)`) and installed unconditionally (CMakeLists.txt:2049-2050 `install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgscripts/libjpeg.pc DESTINATION
- **meson floor** n/a — no meson.build in the tree.

**Configure:**
```sh
mkdir -p /home/madness/code/cxx/satellite/vendor/libjpeg-turbo/build-static && cd /home/madness/code/cxx/satellite/vendor/libjpeg-turbo/build-static && cmake -G Ninja -DCMAKE_C_COMPILER=/home/madness/code/cxx/satellite/vendor/gtk-old/build-cc/cc -DCMAKE_INSTALL_PREFIX=/home/madness/code/cxx/satellite/vendor/stage -DCMAKE_INSTALL_LIBDIR=lib64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DENABLE_SHARED=0 -DENABLE_STATIC=1 -DWITH_TURBOJPEG=0 -DWITH_TOOLS=0 -DWITH_TESTS=0 -DWITH_FUZZ=0 -DWITH_JNA= -DWITH_PROFILE=0 -DWITH_JPEG7=0 -DWITH_JPEG8=0 -DWITH_ARITH_DEC=1 -DWITH_ARITH_ENC=1 -DWITH_SIMD=1 -DREQUIRE_SIMD=1 -DCMAKE_ASM_NASM_COMPILER=/usr/local/bin/nasm /home/madness/code/cxx/satellite/vendor/libjpeg-turbo/libjpeg-turbo-3.2.0
```

**Depends on:** With the options above: NONE. That is the point of them. With the DEFAULT options it declares:
zlib — OPTIONAL, reached only via `if(WITH_SYSTEM_ZLIB OR WITH_SYSTEM_SPNG)` -> CMakeLists.txt:754 `find_package(ZLIB REQUIRED)`. No version floor. Default WITH_SYSTEM_ZLIB is OFF (line 743-746), in which case it builds a SECOND, BUNDLED copy of zlib out of its own tree: `set(ZLIB_STATIC_SOURCES $<TARGET_OBJECTS:zlib-static>)` (line 756).
libspng — OPTIONAL, CMakeLists.txt:739 `pkg_check_modules(spng REQUIRED spng IMPORTED_TARGET)`, gated on `option(WITH_SYSTEM_SPNG ... OFF)` (line 732-734). No version floor. Default OFF builds bundled `add_subdirectory(src/spng)` (line 748).
Build-time only: NASM or Yasm on x86_64 (simd/CMakeLists.txt:53-61, `check_language(ASM_NASM)`), and CMake >= 3.15 (CMakeLists.txt:1). Neither is linked.
Runtime/link: nothing. libjpeg.pc.in carries `Libs: -L${libdir} -ljp

**Traps:** 1. The default build compiles a SECOND ZLIB into the binary and never says so. WITH_TURBOJPEG, WITH_TOOLS and WITH_TESTS are all TRUE by default, which opens the block at line 731; WITH_SYSTEM_ZLIB defaults OFF, so instead of finding one it builds its own `zlib-static` objects and links them into libturbojpeg. Two zlibs in one executable is exactly the shape of the libwayland-client problem already recorded in this project. Turning the trio off removes the whole question.
2. Flip WITH_SYSTEM_ZLIB or WITH_SYSTEM_SPNG on and you get `find_package(ZLIB REQUIRED)` / `pkg_check_modules(spng REQUIRED spng)` resolving against /usr with nothing on the command line hinting at it.
3. NASM here is /usr/local/bin/nasm reporting `NASM version 3.02rc13 compiled on Aug 22 2026` — a RELEASE CANDIDATE of NASM 3, not the distro's, and not what libjpeg-turbo 3.2.0 was tested against. A miscompiled SIMD kernel does not fail the build, it produces wrong pixels. Pinning CMAKE_ASM_NASM_COMPILER at least records which assembler was used; if JPEG decoding ever looks wrong, reconfigure with -DWITH_SIMD=0 and compare before looking anywhere else.
4. Without REQUIRE_SIMD=1 a missing assembler is only a warning (simd/CMakeLists.txt:2-6: `if(REQUIRE_SIMD) message(FATAL_ERROR ...) else() message(WARNING "${message}. Performance will suffer.")`), and jconfig.h is regenerated after the fact because 'the value of WITH_SIMD will have changed' (CMakeLists.txt:754-756 comment) — so a non-SIMD build looks identical to a SIMD one from outside.
5. The static archive's file name is libjpeg.a — `set_target_properti

> **AUDIT [missing-option]** `ninja install` installs eight documentation files into `stage/share/doc/libjpeg-turbo/` unconditionally — README.ijg, README.md, example.c, doc/libjpeg.txt, doc/structure.txt, doc/usage.txt, doc/wizard.txt, LICENSE.md. The `install(FILES ...)` call at CMakeLists.txt:2016 sits at top level with no `if()` around it, unlike every neighbouring install which is guarded by `WITH_TOOLS`, `WITH_TURBOJPEG
> **FIX** Install by component instead of wholesale: `cmake --install build-static --component lib && cmake --install build-static --component include`. That covers the archive (CMakeLists.txt:1990-1992 `ARCHIVE DESTINATION ... COMPONENT lib`), libjpeg.pc (2049-2050 `COMPONENT lib`), the CMake package files (2055-2063 `COMPONENT lib`) and the four headers (2065-2069 `COMPONENT include`), while skipping `COM

---

## libtiff

- **source** `libtiff/tiff-4.7.2`
- **build system** autotools — both are present. Use autotools, and it is not a close call. The CMake path derives EVERY codec's default from what find_package happened to turn up on the build machine: cmake/DeflateCodec.cmake:29-30 `find_package(ZLIB)` / `option(zlib "use zlib ..." ${ZLIB_FOUND})`, cmake/ZSTDCodec.cmake:31,60 `find_package(ZSTD)` / `option(zstd ... ${ZSTD_USABLE})`, cmake/LZMACodec.cmake:29-31, cma
- **installs** stage/lib64/pkgconfig/libtiff-4.pc — Makefile.am:74-75 `pkgconfigdir = $(libdir)/pkgconfig` / `pkgconfig_DATA = libtiff-4.pc`. With this option set it will read `Requires.private: libjpeg zlib` and `Libs.private: -ljpeg -lz -lm`, so both of those .pc files must already be in the stage for `pkg-confi
- **meson floor** n/a — libtiff 4.7.2 ships no meson.build (autotools + CMake only).

**Configure:**
```sh
mkdir -p /home/madness/code/cxx/satellite/vendor/libtiff/build-static && cd /home/madness/code/cxx/satellite/vendor/libtiff/build-static && CC=/home/madness/code/cxx/satellite/vendor/gtk-old/build-cc/cc CXX=/home/madness/code/cxx/satellite/vendor/gtk-old/build-cc/c++ PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig CPPFLAGS=-I/home/madness/code/cxx/satellite/vendor/stage/include LDFLAGS="-L/home/madness/code/cxx/satellite/vendor/stage/lib64 -L/home/madness/code/cxx/satellite/vendor/stage/lib" ../tiff-4.7.2/configure --prefix=/home/madness/code/cxx/satellite/vendor/stage --libdir=/home/madness/code/cxx/satellite/vendor/stage/lib64 --disable-shared --enable-static --with-pic --disable-dependency-tracking --disable-rpath --disable-tools --disable-tests --disable-contrib --disable-docs --disable-sphinx --disable-cxx --disable-deprecated --enable-zlib --with-zlib-include-dir=/home/madness/code/cxx/satellite/vendor/stage/include --with-zlib-lib-dir=/home/madness/code/cxx/satellite/vendor/stage/lib64 --enable-jpeg --with-jpeg-include-dir=/home/madness/code/cxx/satellite/vendor/stage/include --with-jpeg-lib-dir=/home/madness/code/cxx/satellite/vendor/stage/lib64 --disable-libdeflate --disable-jbig --disable-lerc --disable-lzma --disable-zstd --disable-webp --disable-jpeg12 --disable-opengl --without-x
```

**Depends on:** libm — effectively required, no floor. configure.ac:179-182: `AC_CHECK_LIB(m,sin,[libm_lib=yes], [libm_lib=no],)` then `LIBS="-lm $LIBS"` / `tiff_libs_private="-lm ${tiff_libs_private}"`.
zlib — OPTIONAL, ENABLED BY DEFAULT, no version floor declared. configure.ac:442 `AC_CHECK_LIB(z, inflateEnd, [zlib_lib=yes], [zlib_lib=no],)` and :450 `AC_CHECK_HEADER(zlib.h, [zlib_h=yes], [zlib_h=no])`; on success :463-465 `LIBS="-lz $LIBS"` / `tiff_libs_private="-lz ..."` / `tiff_requires_private="zlib ..."`. WE WANT THIS ONE, from the stage.
libjpeg — OPTIONAL, ENABLED BY DEFAULT, no version floor declared, but a de-facto 3.0 probe. configure.ac:571 `AC_CHECK_LIB(jpeg, jpeg_read_scanlines, [jpeg_lib=yes], [jpeg_lib=no],)`, :579 `AC_CHECK_HEADER(jpeglib.h, ...)`, :587 `AC_CHECK_LIB(jpeg, jpeg12_read_scanlines, [HAVE_JPEGTURBO_DUAL_MODE_8_12=yes], ...)` with the comment 'Check for jpeg12_read_scanlin

**Traps:** 1. THE LEAK, and it is live on this machine. Every codec is found with `AC_CHECK_LIB` + `AC_CHECK_HEADER`, which is the compiler's own default search path, and every codec is ON by default. Right now /usr has a library AND a header for lzma, zstd and webp, so a plain `./configure` links three system .so files, writes them into libtiff-4.pc as Requires.private, and the only trace is one line each in the configure summary ('LZMA2 support: yes'). This is the exact failure mode in constraint 2, and for libtiff it is not hypothetical.
2. `--with-zlib-lib-dir` AND `--with-jpeg-lib-dir` DO NOT PROTECT YOU FROM THE SYSTEM COPY. They only prepend `-L$dir` to LDFLAGS and then error if the link test FAILS: `if test "$zlib_lib" = "no" -a "x$with_zlib_lib_dir" != "x"; then AC_MSG_ERROR([Zlib library not found at $with_zlib_lib_dir])` (configure.ac:444-446). If zlib is not in the stage yet, the test finds /usr/lib64/libz.so, sets zlib_lib=yes, and the error branch never runs. BUILD ORDER IS THE ONLY DEFENSE: zlib and libjpeg-turbo must both be installed into the stage BEFORE libtiff is configured. Verify afterwards from config.log, not from the summary.
3. The 12-bit probe must hit the right library. configure.ac:587 `AC_CHECK_LIB(jpeg, jpeg12_read_scanlines, ...)` sets HAVE_JPEGTURBO_DUAL_MODE_8_12 and a #define. If that probe resolves against /usr/lib64/libjpeg.so and the final link uses stage/lib64/libjpeg.a, the define and the library disagree, and the symptom is a link error or wrong output much later.
4. `--enable-jpeg12` is a different thing and the help warns about it in its own 

> **AUDIT [wrong-option]** `--disable-deprecated` does not do what the recipe says, because upstream wired the option BACKWARDS. configure.ac:209-212 defaults `enable_deprecated=no`, and then the body only fires on "yes": it is `--enable-deprecated` that adds `-DTIFF_DISABLE_DEPRECATED`. The comment inside the block even contradicts the option name ("# Disable deprecated features to ensure clean build" sits under `if test "
> **FIX** Drop `--disable-deprecated` from the command, and do NOT substitute `--enable-deprecated` — that is the flag that compiles the deprecated API OUT. Leaving the default (deprecated API present) is what gdk-pixbuf's tiff loader links against.

> **AUDIT [wrong-option]** The recipe credits `--disable-sphinx` with stopping the Python toolchain probe, citing configure.ac:1165. That is wrong on both counts, and the mistake matters because it misidentifies which flag is load-bearing. The 20-name Python search at configure.ac:1150-1156 (`for python in python3.16 ... python3 python; do AC_CHECK_PROGS(PYTHON_BIN, [$python])`) is at top level, before `AC_ARG_ENABLE(sphinx
> **FIX** Keep both flags as written, but record that `--disable-docs` is the one doing the work and `--disable-sphinx` is decorative; never drop `--disable-docs`. The Python probe runs either way and is harmless.

---

## freetype

- **source** `freetype/freetype-2.14.3`
- **build system** meson — choose it over the shipped autotools `configure`. Both exist and both work, but the autotools path has an option meson does not: `--with-librsvg=[yes|no|auto]` defaulting to `auto`, which runs `PKG_CHECK_MODULES([LIBRSVG], [librsvg-2.0 >= 2.46.0])` and is one more system-library probe to defeat. Meson also gives `--default-library=static` directly instead of libtool's `--enable-static --di
- **installs** `stage/lib/pkgconfig/freetype2.pc` — `pkgconfig.generate(ft2_lib, filebase: 'freetype2', name: 'FreeType 2', ..., subdirs: 'freetype2', version: ft2_pkgconfig_version)` (meson.build:502-509). Version is the libtool triple, **26.6.20**, from `version_info='26:6:20'` in builds/unix/configure.raw:20 — 
- **meson floor** `meson_version: '>= 0.55.0'` (meson.build:29). Satisfied by the vendored 1.12.0 and also by the system's /usr/bin/meson 

**Configure:**
```sh
env -u PKG_CONFIG_PATH \
  PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
  PATH=/home/madness/code/cxx/satellite/vendor/stage/bin:$PATH \
/usr/bin/python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/build/freetype \
  /home/madness/code/cxx/satellite/vendor/freetype/freetype-2.14.3 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --bindir=bin \
  --includedir=include \
  --buildtype=release \
  --default-library=static \
  --wrap-mode=nofallback \
  -Db_staticpic=true \
  -Db_lto=false \
  -Dzlib=system \
  -Dpng=enabled \
  -Dbzip2=disabled \
  -Dbrotli=disabled \
  -Dharfbuzz=disabled \
  -Dmmap=enabled \
  -Dtests=disabled \
  -Derror_strings=false
```

**Depends on:** Build-time, required:
- meson itself: `project('freetype2', 'c', meson_version: '>= 0.55.0', ...)` (meson.build:28-32)
- python3, hard: `python_exe = find_program('python3')` (meson.build:38) — drives `extract_freetype_version.py`, `extract_libtool_version.py`, `parse_modules_cfg.py`, `process_ftoption_h.py`

Link dependencies, all optional, each guarded by an option:
- zlib, no version floor — `zlib_dep = dependency('zlib', required: true)` (meson.build:310, the `system` branch; the `auto` branch at :289 is `required: false`). **Keep: `-Dzlib=system`.**
- libpng, no version floor — `libpng_dep = dependency('libpng', required: get_option('png'), fallback: 'libpng')` (meson.build:342). **Keep: `-Dpng=enabled`.**
- bzip2, no version floor — `bzip2_dep = dependency('bzip2', required: get_option('bzip2').disabled() ? get_option('bzip2') : false,)` (meson.build:323-326), then `bzip2_dep = cc.

**Traps:** 1. **`PKG_CONFIG_PATH` does not hide /usr — it is searched *in addition to* the default path.** Demonstrated on this machine:
   `PKG_CONFIG_PATH=/nonexistent/stage/lib/pkgconfig pkg-config --modversion zlib freetype2 fontconfig expat` → `1.3.1.zlib-ng`, `26.1.20`, `2.15.0`, `2.7.3`.
   `PKG_CONFIG_LIBDIR=/nonexistent/stage/lib/pkgconfig pkg-config --modversion zlib` → not found, exit 1.
   The brief says PKG_CONFIG_PATH will point only at stage; that is not sufficient. Use `PKG_CONFIG_LIBDIR` and clear `PKG_CONFIG_PATH` (`env -u PKG_CONFIG_PATH`), or the whole audit-the-log exercise is fighting a leak you configured in.

2. **`-Dbzip2` default `auto` bypasses pkg-config entirely** via `cc.find_library('bz2', has_headers: ['bzlib.h'])`. `/usr/lib64/libbz2.so` is present. No pkg-config setting can stop this one; only the option can.

3. **`-Dharfbuzz=auto` produces a runtime `dlopen`, not a link error.** Falling through to the `dynamic` leg defines `FT_CONFIG_OPTION_USE_HARFBUZZ_DYNAMIC` and links `-ldl`. `ldd` on the final satl would show nothing about harfbuzz; the dependency only appears when a font actually needs it. This is the failure mode the brief describes, in its most invisible form.

4. **`-Dzlib` defaults to `auto`, and `auto` silently becomes `external`** — i.e. `subproject('zlib')` — when pkg-config misses. Two zlibs in one binary. With `--wrap-mode=nofallback` the explicit `subproject()` call at meson.build:293 is *not* blocked (nofallback only disables `dependency(..., fallback:)`), so this would still fire if the wrap tree were ever populated. `-Dzlib=system

> **AUDIT [leak]** `-Dzlib=system` does NOT guarantee one zlib or a loud failure. `dependency('zlib', ...)` is not a plain pkg-config lookup: meson registers zlib as a DependencyFactory whose candidate list is PKGCONFIG -> CMAKE -> SYSTEM. If stage/lib/pkgconfig/zlib.pc is absent (stage is EMPTY right now, `ls -R vendor/stage` prints nothing), meson falls to the CMake leg -- /usr/lib64/cmake/ZLIB/zlib-config.cmake a
> **FIX** Add a native file and pass `--native-file=vendor/stage-native.ini` containing `[binaries]` / `cmake = '/nonexistent/cmake'` (CMakeExecutor.find_cmake_binary caches a NonExistingExternalProgram and every CMake dependency leg then reports not-found -- executor.py:91), and add `-Dprefer_static=true` (kills the SYSTEM leg, since no /usr/lib64/libz.a exists). Also make zlib and libpng hard build-order 

> **AUDIT [wrong-option]** The recipe's stated mitigation for the pypy problem does not do what it claims. It says invoking meson as `/usr/bin/python3 .../meson.py` "keeps meson on the tested interpreter", but meson.build:38 is `find_program('python3')`, which searches PATH and is unaffected by which interpreter runs meson. `which -a python3` on this box returns /home/madness/opt/pypy3/pypy3/bin/python3 first, and `python3 
> **FIX** Pin it in the same native file used to block cmake: `[binaries]` / `python3 = '/usr/bin/python3'` / `cmake = '/nonexistent/cmake'`, and pass `--native-file` to every meson setup in the stack. A native-file `[binaries]` entry overrides `find_program('python3')` by name, which prepending PATH does not reliably do.

---

## fontconfig

- **source** `fontconfig/fontconfig-2.18.3`
- **build system** meson — not really a choice. The autotools `configure` is present but is the derived copy (upstream runs `meson.add_dist_script('build-aux/meson-dist-autotools.py')`, meson.build:641), and its `--help` shows `--enable-static[=PKGS] build static libraries [default=no]` / `--enable-shared[=PKGS] ... [default=yes]`, i.e. libtool with `.la` files, and it offers no way to turn off tests, tools or docs.
- **installs** `stage/lib/pkgconfig/fontconfig.pc` — `pkgmod.generate(libfontconfig, description: 'Font configuration and customization library', filebase: 'fontconfig', name: 'Fontconfig', requires_private: ['freetype2 ' + freetype_req], version: fc_version, variables: ['sysconfdir=...','localstatedir=...','confd
- **meson floor** `meson_version : '>= 1.11.0'` (meson.build:3). **The system meson is 1.4.1** (`/usr/bin/meson --version` → `1.4.1`), whi

**Configure:**
```sh
env -u PKG_CONFIG_PATH \
  PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
  PATH=/home/madness/code/cxx/satellite/vendor/stage/bin:$PATH \
/usr/bin/python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/build/fontconfig \
  /home/madness/code/cxx/satellite/vendor/fontconfig/fontconfig-2.18.3 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --bindir=bin \
  --includedir=include \
  --sysconfdir=etc \
  --localstatedir=var \
  --datadir=share \
  --buildtype=release \
  --default-library=static \
  --wrap-mode=nofallback \
  -Db_staticpic=true \
  -Db_lto=false \
  -Dxml-backend=expat \
  -Dnls=disabled \
  -Diconv=disabled \
  -Dfontations=disabled \
  -Ddoc=disabled \
  -Ddoc-txt=disabled \
  -Ddoc-man=disabled \
  -Ddoc-pdf=disabled \
  -Ddoc-html=disabled \
  -Dtests=disabled \
  -Dtests-bwrap=disabled \
  -Dtests-external-fonts=disabled \
  -Dtools=disabled \
  -Dcache-build=disabled \
  -Ddefault-hinting=slight \
  -Ddefault-sub-pixel-rendering=none \
  -Dbitmap-conf=no-except-emoji \
  -Dadditional-fonts-dirs=no \
  -Ddefault-fonts-dirs=yes

# REQUIRES FIRST: vendored gperf installed into stage/bin (see traps), and
# freetype2.pc already in stage (see traps #2).
# `-Ddefault-fonts-dirs=yes` is the one option here that is a decision, not a
# fix — see options_rationale.
```

**Depends on:** Build-time, required:
- meson: `project('fontconfig', 'c', version: '2.18.3', meson_version : '>= 1.11.0', ...)` (meson.build:1-9)
- **gperf**, hard: `gperf = find_program('gperf', required: false)` (meson.build:461) and, on the else branch, `gperf = find_program('gperf')` (meson.build:485) — required, no fallback once `--wrap-mode=nofallback` is on. Used at src/meson.build:66: `command: [gperf, '--pic', '-m', '100', '@INPUT@', '--output-file', '@OUTPUT@']`.
- python3: `python3 = import('python').find_installation()` (meson.build:103), plus `find_program('fc-case.py')`, `find_program('fc-lang.py')`, `find_program('fc-const.py')`, `find_program('fc-genericfamily.py')`, `find_program('write-35-lang-normalize-conf.py')`, `src/makealias.py`, `src/cutout.py`.
- pytest, optional: `pytest = find_program('pytest', required: false)` (meson.build:104) — tests only.

Link dependencies:
- **freetype

**Traps:** 1. **`gperf` is not installed on this machine.** `command -v gperf` prints nothing. fontconfig tries `find_program('gperf', required: false)` and, on failure, `find_program('gperf')` at meson.build:485 with required=true — a hard configure error. The escape hatch upstream provides is `subprojects/gperf.wrap`, which is a **wrap-git** clone of `https://gitlab.freedesktop.org/tpm/gperf.git` revision `meson` — blocked by `--wrap-mode=nofallback`, and it would put a third-party tree under vendor/ anyway. So: build the vendored gperf-3.3, `make install` it into `stage/bin`, and prepend `stage/bin` to PATH *before* configuring fontconfig. gperf is a build-order predecessor, not a parallel leaf.

2. **A missing staged freetype2.pc silently links the system libfreetype.** meson.build:49 is `required: false`; on a miss, :56 retries with `method: 'cmake'`. `/usr/bin/cmake` exists, CMake's FindFreetype searches /usr/lib64, and `PKG_CONFIG_LIBDIR` has no effect on CMake. Worse, the system freetype2.pc reports **26.1.20**, which *passes* the `>= 21.0.15` floor — so no version check saves you. Grep the configure log for `Run-time dependency freetype2 found: YES 26.6.20`; anything else, including any mention of CMake, means the leak happened.

3. **`PKG_CONFIG_PATH` does not exclude /usr.** Same demonstration as for freetype, and it bites harder here: `PKG_CONFIG_PATH=/nonexistent pkg-config --modversion freetype2 fontconfig expat` returns `26.1.20`, `2.15.0`, `2.7.3` from the system. Use `PKG_CONFIG_LIBDIR` and clear `PKG_CONFIG_PATH`.

4. **`xml-backend=auto`'s middle leg bypasses pkg-co

> **AUDIT [leak]** `-Dxml-backend=expat` leaves a THIRD system-expat leg the recipe never noticed, and the verification it proposes cannot detect it. The recipe describes the funnel as pkg-config -> `cc.find_library('expat')` -> libxml2, and says pinning `expat` "only removes the libxml2 leg". But the first call itself, `dependency('expat', required: false)` at meson.build:78, uses method AUTO, and meson's AUTO cand
> **FIX** Block CMake for the whole stack with a native file: `[binaries]` / `cmake = '/nonexistent/cmake'`. Then a missing stage expat.pc degrades to the `cc.find_library` leg only, which `-Dprefer_static=true` at least makes prefer a .a. Verify by grepping the configure log for the literal string `(pkg-config)` on the expat line -- e.g. `Run-time dependency expat found: YES 2.8.4` sourced from stage -- an

> **AUDIT [missing-option]** The recipe claims it covers "Every option in meson.options" and then sets 19 of 24. The five it skips are all string options defaulting to 'default': `cache-dir`, `template-dir`, `baseconfig-dir`, `config-dir`, `xml-dir`. These are precisely the knobs that control the absolute-path problem the recipe's own trap #6 complains about -- trap #6 diagnoses the symptom (`/home/madness/.../stage/etc/fonts
> **FIX** Pass the deployed paths explicitly at configure time so the .a is baked with target-valid strings rather than this checkout's, e.g. `-Dbaseconfig-dir=/opt/satl/etc/fonts -Dconfig-dir=/opt/satl/etc/fonts/conf.d -Dtemplate-dir=/opt/satl/share/fontconfig/conf.avail -Dxml-dir=/opt/satl/share/xml/fontconfig -Dcache-dir=/var/cache/fontconfig` (substituting satl's real install prefix), keeping --prefix=s

> **AUDIT [wrong-option]** The recipe's log-audit rule is wrong about `threads`, and the rule is the thing the whole build is verified with. It says "json-c and `threads` are the two expected false positives; everything else is a real leak", and that threads' "only consumers" are test/meson.build:44-45. In fact `dependency('threads')` at meson.build:422 is unconditional and required (no `required:` kwarg at all -- it will h
> **FIX** Correct the audit rule to: json-c and threads are expected, threads because meson.build:422 links it into the library, not because it is test-only. Ensure satl's final link carries `-pthread`, and confirm the generated stage/lib/pkgconfig/fontconfig.pc carries it (meson promotes a static mainlib's external deps to the PUBLIC `Libs:`/`Requires:` -- pkgconfig.py:256 `isinstance(obj, build.StaticLibr

> **AUDIT [leak]** The recipe's verification string for the freetype CMake-fallback trap does not match anything, so the check it prescribes can never pass. Trap #2 says to grep the log for `Run-time dependency freetype2 found: YES 26.6.20` -- but the recipe itself reports the system freetype2.pc as 26.1.20, and the vendored 2.14.3 computes its pkg-config version at configure time from builds/unix/configure.raw, so 
> **FIX** Do not rely on a version string you have not seen. Block the leg structurally with the native file `[binaries] cmake = '/nonexistent/cmake'`, which turns line 56 into a not-found and -- with `--wrap-mode=nofallback` already blocking its `fallback: ['freetype2', 'freetype_dep']` -- makes a missing staged freetype2.pc a hard configure error instead of a silent system link. Then derive the expected v

> **AUDIT [missing-option]** The claim that `--wrap-mode=nofallback` "keeps `subprojects/` free of any third-party meson.build, so fontconfig needs no edit_journal entry" is factually wrong about what is in the tarball. `ls subprojects/` shows six .wrap files AND one real unpacked directory, `libintl/`, containing a meson.build. It is upstream fontconfig's own file rather than a wrapdb patch zip, so the journal conclusion hap
> **FIX** Correct the claim (fontconfig ships subprojects/libintl/ as a directory; nofallback is not why it is safe -- `-Dnls=disabled` is, because a disabled feature passed as `required:` makes meson skip both the search and the fallback at meson.build:231). Keep `-Dnls=disabled` and treat it as load-bearing, not cosmetic, and record subprojects/libintl/ in vendor/edit_journal as pre-existing upstream cont

> **AUDIT [missing-option]** Run-time data, beyond what trap #6 covers. The installed fonts.conf -- which the recipe correctly plans to ship in satl's payload -- bakes this checkout's paths into the XML as well as into the .a, and with `-Dtools=disabled` there is no fc-cache binary in the payload to rebuild a cache with. `<include ignore_missing="yes">@CONFIGDIR@</include>` becomes `.../vendor/stage/etc/fonts/conf.d`, which d
> **FIX** Ship a fonts.conf satl generates for itself rather than installing stage's verbatim: point `<include>` at satl's own conf.d, drop the stage `<cachedir>` line and keep only the xdg one, and set FONTCONFIG_FILE at process start. Verify on a machine with no fontconfig installed that conf.d rules actually load (e.g. that 10-hinting-slight.conf takes effect), because the failure mode is silent by desig

---

## fribidi

- **source** `fribidi/fribidi-1.0.17`
- **build system** meson — there IS a choice (a ready ./configure is present, generated 2026-09-20), but meson wins for three reasons. (1) configure.ac:48 is `LT_INIT([disable-static])`, so `./configure --help` reports `--enable-static[=PKGS] build static libraries [default=no]` and `--enable-shared [default=yes]`: the autotools default produces a .so and NO .a, and you must remember `--disable-shared --enable-stati
- **installs** fribidi.pc — installed to stage/lib/pkgconfig/fribidi.pc.

From meson.build:87-93: `pkg.generate(libfribidi, name: 'GNU FriBidi', filebase: 'fribidi', description: 'Unicode Bidirectional Algorithm Library', extra_cflags: fribidi_static_cargs, subdirs: 'fribidi', version: meson.project_version())`. `
- **meson floor** meson.build:2 — `meson_version : '>= 0.54'`. Vendored meson is 1.12.0 (verified: `meson.py --version` prints 1.12.0). Cl

**Configure:**
```sh
PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
PKG_CONFIG_PATH= \
/home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/build/fribidi \
  /home/madness/code/cxx/satellite/vendor/fribidi/fribidi-1.0.17 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --includedir=include \
  --bindir=bin \
  --buildtype=release \
  --default-library=static \
  --wrap-mode=nodownload \
  -Ddocs=false \
  -Dbin=false \
  -Dtests=false \
  -Ddeprecated=true
```

**Depends on:** NONE. fribidi is a true leaf — build it first, before zlib if you like; nothing it needs exists outside libc.

Everything it probes is libc or a build-time program:
- meson.build:37 `foreach f : ['memmove', 'memset', 'strdup']` / `cdata.set('HAVE_' + f.to_upper(), cc.has_function(f))` — libc functions.
- meson.build:44 `foreach h : ['stdlib.h', 'string.h', 'memory.h']` and :47 `foreach h : ['strings.h', 'sys/times.h']` — libc headers.
- lib/meson.build:16 `fribidi_config.set('SIZEOF_INT', cc.sizeof('int'))`.
- gen.tab/meson.build:3 `native_cc = meson.get_compiler('c')` — a NATIVE compiler, needed to build and run the table generators at build time.
- doc/meson.build:2 and bin/meson.build:47 `python3 = import('python').find_installation()` — only reached when docs/bin/tests are on; our flags skip both subdirs.
- doc/meson.build:34 `c2man = find_program('c2man', required: false)` and bin/m

**Traps:** 1. PKG_CONFIG_PATH DOES NOT BLOCK /usr. This is the stack-wide killer and it is worth fixing here because the same export is reused for every project. PKG_CONFIG_PATH only PREPENDS to pkg-config's built-in search path; /usr/lib64/pkgconfig and /usr/share/pkgconfig are still searched afterwards. Only PKG_CONFIG_LIBDIR REPLACES that path. With PKG_CONFIG_PATH alone, any dependency not yet present in stage resolves against the system copy and meson prints the same cheerful "Run-time dependency <name> found: YES" either way. Set PKG_CONFIG_LIBDIR and clear PKG_CONFIG_PATH.

2. gen.tab builds and RUNS native executables at build time. gen.tab/meson.build:32-37 `executable('gen-unicode-version', ..., native: true)` plus the six table generators driven by the `tabs` list (:49-56), each run through a custom_target whose output is compiled into libfribidi. `--default-library=static` does not apply to `native: true` executables and must not — they are host tools. If you ever put `-static` or a cross-ish flag into the environment CFLAGS/LDFLAGS for the whole stack, it hits these too and the build dies in gen.tab, nowhere near where you'd look.

3. Two installed headers are GENERATED, not shipped in the tarball: fribidi-config.h and fribidi-unicode-version.h. Anyone tempted to hand-copy lib/*.h into the prefix will produce an include dir that compiles nothing.

4. The autotools default is backwards from what we want: configure.ac:48 `LT_INIT([disable-static])` → `--enable-static [default=no]`, `--enable-shared [default=yes]`. Running the bundled ./configure with no flags gives you libf

---

## harfbuzz

- **source** `harfbuzz/harfbuzz-14.4.0`
- **build system** meson — no real choice. A CMakeLists.txt exists (47 KB) but upstream treats meson as the build system: the pkg-config files, the feature matrix (meson_options.txt has 30 options) and the gpu/raster/vector library split are all meson-side, and CMake would not generate harfbuzz-subset.pc, which GTK requires by name. There is no ./configure.
- **installs** With the command above, exactly TWO:
- harfbuzz.pc — src/meson.build:1026-1030 `pkgmod.generate(libharfbuzz, description: 'HarfBuzz text shaping library', subdirs: [meson.project_name()], version: meson.project_version())`
- harfbuzz-subset.pc — src/meson.build:1033-1039 `pkgmod.generate(libharfbuzz
- **meson floor** meson.build:2 — `meson_version: '>= 0.60.0'`. Vendored meson is 1.12.0, which also clears the two internal version gates

**Configure:**
```sh
PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
PKG_CONFIG_PATH= \
/home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/build/harfbuzz \
  /home/madness/code/cxx/satellite/vendor/harfbuzz/harfbuzz-14.4.0 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --includedir=include \
  --bindir=bin \
  --buildtype=release \
  --default-library=static \
  --wrap-mode=nofallback \
  --auto-features=disabled \
  -Dglib=enabled \
  -Dfreetype=enabled \
  -Dsubset=enabled \
  -Dgobject=disabled \
  -Dicu=disabled \
  -Dicu_builtin=false \
  -Dcairo=disabled \
  -Dchafa=disabled \
  -Dpng=disabled \
  -Dzlib=disabled \
  -Dgraphite=disabled \
  -Dgraphite2=disabled \
  -Dwasm=disabled \
  -Dfontations=disabled \
  -Dharfrust=disabled \
  -Dkbts=disabled \
  -Dgdi=disabled \
  -Ddirectwrite=disabled \
  -Dcoretext=disabled \
  -Draster=disabled \
  -Dvector=disabled \
  -Dgpu=disabled \
  -Dgpu_demo=disabled \
  -Dutilities=disabled \
  -Dtests=disabled \
  -Dbenchmark=disabled \
  -Dintrospection=disabled \
  -Ddocs=disabled \
  -Ddoc_tests=false \
  -Dexperimental_api=false \
  -Dwith_libstdcxx=false \
  -Dragel_subproject=false
```

**Depends on:** Ordered by what actually gates the build.

REQUIRED in our configuration:
- glib-2.0 >= 2.30.0 — meson.build:14 `glib_min_version = '>= 2.30.0'`; :112 `glib_dep = dependency('glib-2.0', version: glib_min_version, required: get_option('glib'))`. Required because we pass -Dglib=enabled (forced by GTK's hb-glib.h use). ⇒ glib must be built and installed into stage first.
- freetype2 >= 20.0.14 — meson.build:19 `freetype_min_version = '>= 20.0.14'`; :111 `freetype_dep = dependency('freetype2', version: freetype_min_version, required: get_option('freetype'), default_options: ['harfbuzz=disabled'])`. (That floor is freetype2.pc's libtool-style version, not 2.x.) ⇒ freetype must be in stage first, built WITHOUT harfbuzz.
- libm — meson.build:109 `m_dep = cpp.find_library('m', required: false)`. libc; unavoidable and fine.
- threads/pthread — meson.build:353 `thread_dep = dependency('threads', r

**Traps:** 1. PKG_CONFIG_PATH DOES NOT BLOCK /usr — and harfbuzz is where it bites hardest. PKG_CONFIG_PATH only prepends; pkg-config still searches /usr/lib64/pkgconfig afterwards. Every one of icu-uc, cairo, cairo-ft, chafa, libpng, zlib, glib-2.0, gobject-2.0, freetype2 is a pkg-config lookup, and meson prints the same "Run-time dependency X found: YES" whether it came from stage or from /usr. Export PKG_CONFIG_LIBDIR (which REPLACES the search path) and clear PKG_CONFIG_PATH. Note this does not cover meson.build:109/:116 `cpp.find_library('m'/'iwasm')`, which search the LINKER path — only the options protect those.

2. `--wrap-mode=nodownload` is WEAKER than what harfbuzz already sets for itself. meson.build:8 has `'wrap_mode=nofallback'` in default_options, with the comment `# Use --wrap-mode=default to revert`. Passing nodownload on the command line OVERRIDES that and re-enables subproject fallbacks for anything already on disk. subprojects/ holds 13 wraps including glib, cairo, freetype2, icu and zlib. If the parent plan's stack-wide `--wrap-mode=nodownload` is applied here verbatim, it is a downgrade. Use nofallback for harfbuzz specifically, so a missing dependency is a loud error instead of a quiet second copy of glib.

3. THE FREETYPE CYCLE, and a real dlopen inside it. harfbuzz wants freetype2; freetype wants harfbuzz. freetype-2.14.3/meson_options.txt:25-29 is `option('harfbuzz', type: 'combo', choices: ['auto', 'enabled', 'dynamic', 'disabled'], value: 'auto', ...)` and meson.build:367-380 says: if harfbuzz was not found AND the option is 'dynamic' OR 'auto', fall throug

---

## pixman

- **source** `pixman/pixman-0.46.4`
- **build system** meson — no choice: the tree ships only meson.build/meson.options (no configure, no CMakeLists.txt). `ls` of the source root shows meson.build and meson.options and nothing else buildable.
- **installs** pixman-1.pc — `pkg.generate(libpixman, name : 'Pixman', filebase : 'pixman-1', description : 'The pixman library (version 1)', subdirs: 'pixman-1', version : meson.project_version())` (meson.build:615-621). With --libdir=lib it lands at /home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig/pix
- **meson floor** >= 1.3.0 — `meson_version : '>= 1.3.0',` (meson.build:26). The vendored meson 1.12.0 clears it.

**Configure:**
```sh
env -u PKG_CONFIG_PATH PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup /home/madness/code/cxx/satellite/vendor/build/pixman /home/madness/code/cxx/satellite/vendor/pixman/pixman-0.46.4 --prefix=/home/madness/code/cxx/satellite/vendor/stage --libdir=lib --buildtype=release --default-library=static --wrap-mode=nofallback -Dprefer_static=true -Db_staticpic=true -Dtests=disabled -Ddemos=disabled -Dgtk=disabled -Dlibpng=disabled -Dopenmp=disabled -Dtimers=false -Dgnuplot=false -Dmmx=enabled -Dsse2=enabled -Dssse3=enabled -Dgnu-inline-asm=enabled -Dtls=enabled -Dloongson-mmi=disabled -Dvmx=disabled -Darm-simd=disabled -Dneon=disabled -Da64-neon=disabled -Dmips-dspr2=disabled -Drvv=disabled
```

**Depends on:** pixman has NO required external dependency. It is a true leaf and can be built first, before anything else in the stack.

Required (meson built-ins / libc only, no version floor on any of them):
- `dep_threads = dependency('threads')` (meson.build:467) — meson's built-in threads dep, i.e. -pthread. Required (dependency() defaults to required:true). Not a pkg-config lookup.
- `dep_m = cc.find_library('m', required : false)` (meson.build:466) — libm, part of glibc.

Optional, all of them test/demo-only, all turned off above:
- `dep_openmp = dependency('openmp', required : get_option('openmp'))` (meson.build:432) — no version floor. Optional. Tests only.
- `dep_gtk = dependency('gtk+-3.0', required : get_option('gtk').enabled() and get_option('demos').enabled())` (meson.build:443) — no version floor. Optional. Demos only.
- `dep_glib = dependency('glib-2.0', required : get_option('gtk').ena

**Traps:** 1. TWO dependency() CALLS THAT NO OPTION CAN TURN OFF. meson.build:443-444 call `dependency('gtk+-3.0', ...)` and `dependency('glib-2.0', ...)` unconditionally — the option expression only computes the `required` BOOL, it does not guard the lookup. So the search always runs and the configure log will print a 'Run-time dependency glib-2.0 found: YES' line once glib is in stage. That one is harmless (dep_glib is referenced only under demos/), but it is exactly the log line the leak audit greps for, so expect it and do not chase it. The only thing keeping gtk+-3.0 from resolving to /usr is PKG_CONFIG_LIBDIR.
2. cc.find_library BYPASSES pkg-config ENTIRELY. meson.build:452-456 walks ['16','15','14','13','12','10'] calling `cc.find_library('libpng16', has_headers : ['png.h'])`. That asks the LINKER, not pkg-config, so no amount of PKG_CONFIG_LIBDIR discipline stops it — it will find /usr/lib64/libpng16.so and /usr/include/png.h and report success. -Dlibpng=disabled is the only defence, because the whole block sits under `if not get_option('libpng').disabled()`.
3. 'auto' MEANS BUILD, NOT 'SKIP'. tests, demos, gtk, libpng, openmp are all `type : 'feature'` with no `value:` line, so they default to auto, and every guard in this file is written `if not get_option('X').disabled()`. Leaving them alone builds them. They must be spelled `disabled`, not omitted.
4. PKG_CONFIG_PATH IS NOT ISOLATION. Measured on this machine: `pkg-config --variable pc_path pkg-config` → `/usr/lib64/pkgconfig:/usr/share/pkgconfig`. PKG_CONFIG_PATH prepends to that. If the build script sets only PKG_CONFIG_

> **AUDIT [leak]** Same CMake fallback, and the recipe's rationale credits the wrong defence. The recipe states as trap #4 that "PKG_CONFIG_PATH IS NOT ISOLATION... Use PKG_CONFIG_LIBDIR", presenting PKG_CONFIG_LIBDIR as the isolation boundary. It is not one; meson tries CMake last (detect.py:197). For pixman the exposure happens to be closed, but not for the reason given: -Dlibpng=disabled short-circuits the whole 
> **FIX** Add the same `--native-file .../no-cmake.ini` with `[binaries]` / `cmake = 'cmake-intentionally-absent'` to the pixman setup line, so the stack has one uniform isolation boundary rather than one that happens to hold. Keep -Dlibpng=disabled — it is load-bearing for three separate lookup paths, not just the cc.find_library loop.

> **AUDIT [wrong-option]** -Dmmx=enabled and -Dsse2=enabled cannot do what the recipe says they do. The stated reason is "Enabled explicitly so a compile-probe failure becomes a loud error rather than a silently slower pixman." On x86_64 there is no compile probe to fail: meson.build:103-105 sets have_mmx = true on `host_machine.cpu_family() == 'x86_64'` before any cc.compiles(), and meson.build:183-185 does the same for ss
> **FIX** Keep the flags — they are correct and cost nothing — but fix the rationale to say that mmx/sse2 are unconditional on x86_64 and only ssse3 is probe-gated. If you want a real guarantee that the SIMD paths got compiled in, verify it after the build instead: `nm` libpixman-1.a for _pixman_implementation_create_sse2 and _pixman_implementation_create_ssse3.

---

## cairo

- **source** `cairo/cairo-1.18.4`
- **build system** meson — no choice: the source root has meson.build and meson.options and no configure or CMakeLists.txt (upstream dropped autotools at 1.17). The `INSTALL` file is a leftover.
- **installs** With the command above, into /home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig/:
- cairo.pc — `pkgmod.generate(libcairo, description: 'Multi-platform 2D graphics library', subdirs: [meson.project_name()])` (src/meson.build:264-267); filebase defaults to the target name.
- cairo-gobject.pc 
- **meson floor** >= 1.3.0 — `  meson_version: '>= 1.3.0',` (meson.build:2). The vendored meson 1.12.0 clears it.

**Configure:**
```sh
env -u PKG_CONFIG_PATH PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup /home/madness/code/cxx/satellite/vendor/build/cairo /home/madness/code/cxx/satellite/vendor/cairo/cairo-1.18.4 --prefix=/home/madness/code/cxx/satellite/vendor/stage --libdir=lib --buildtype=release --default-library=static --wrap-mode=nofallback -Dprefer_static=true -Db_staticpic=true -Dpng=enabled -Dzlib=enabled -Dfreetype=enabled -Dfontconfig=enabled -Dglib=enabled -Dtee=disabled -Dtests=disabled -Dlzo=disabled -Dspectre=disabled -Dsymbol-lookup=disabled -Dgtk2-utils=disabled -Dxlib=disabled -Dxcb=disabled -Dxlib-xcb=disabled -Dquartz=disabled -Ddwrite=disabled -Dgtk_doc=false
```

**Depends on:** REQUIRED (exactly one):
- pixman-1 >= 0.40.0 — `pixman_dep = dependency('pixman-1',` / `  version: '>= 0.40.0',` / `  fallback: ['pixman', 'idep_pixman'],` / `)` (meson.build:633-636). No `required:` kwarg, so required defaults to true. Our 0.46.4 also clears `pixman_dep.version().version_compare('>= 0.42.3')` (meson.build:640), setting HAS_PIXMAN_r8g8b8_sRGB.

OPTIONAL BUT WANTED, all from stage:
- zlib, no version floor — `zlib_dep = dependency('zlib',` / `  required: get_option('zlib'),` / `  fallback : ['zlib', 'zlib_dep'],` (meson.build:225-227). Stage has zlib-1.3.2.
- libpng >= 1.4.0 — `libpng_required_version = '>= 1.4.0'` (meson.build:12), used at `png_dep = dependency('libpng',` / `  required: get_option('png'),` / `  version: libpng_required_version,` (meson.build:238-240). Stage has libpng-1.6.58.
- fontconfig >= 2.13.0 — `fontconfig_required_version = '>= 2.13.0'` (meson.bui

**Traps:** 1. cc.find_library('bfd') IS INVISIBLE TO pkg-config ISOLATION. symbol-lookup is auto; if binutils-devel is on the box, libbfd lands in libcairo's own `deps` (meson.build:592) and you get a 'static' libcairo.a whose .pc demands /usr/lib64/libbfd.so. No PKG_CONFIG_LIBDIR setting prevents it. Only -Dsymbol-lookup=disabled does.
2. GTK CANNOT CONFIGURE WITHOUT cairo-gobject. gtk-4.24.0/meson.build:465 `cairogobj_dep = dependency('cairo-gobject', version: cairo_req)` has no `required: false` and no guard. cairo-gobject exists only when CAIRO_HAS_GOBJECT_FUNCTIONS is set, which needs BOTH gobject-2.0 and glib-2.0 found (meson.build:552-554). So glib must already be in stage when cairo is configured — cairo is NOT a leaf and must not be scheduled early with pixman.
3. DISABLING zlib WOULD OPEN A DIFFERENT LEAK. zlib is what sets CAIRO_HAS_INTERPRETER (meson.build:583-585) and so installs cairo-script-interpreter.pc. GTK tries `dependency('cairo-script-interpreter', required: false)` and, failing that, `cc.find_library('cairo-script-interpreter', required: get_option('build-tests'))` (gtk meson.build:561-563) — a raw linker lookup that would find /usr/lib64/libcairo-script-interpreter.so. Staging the .pc closes that door. Keep zlib enabled.
4. cairo-trace IS BUILT UNCONDITIONALLY AND HAS NO OPTION. CAIRO_HAS_TRACE is set whenever ld_preload-capable OS + zlib + real pthread + dlsym (meson.build:776-778) — all true on Linux. `libcairotrace = library('cairo-trace', ...)` (util/cairo-trace/meson.build:11) obeys default_library, so it comes out as libcairo-trace.a in lib/cairo/ — no .s

> **AUDIT [leak]** PKG_CONFIG_LIBDIR IS NOT ISOLATION — meson falls back to CMAKE. The recipe's entire isolation story (its trap #4, and its trap #9 claim that --wrap-mode=nofallback makes a missing stage .pc "an immediate, visible error") rests on PKG_CONFIG_LIBDIR. It is false. meson's AUTO dependency order is pkg-config, then extraframework, then CMake: mesonbuild/dependencies/detect.py:197 `methods = [Dependency
> **FIX** Disable CMake for the whole stack. mesonbuild/cmake/executor.py:74 resolves the binary through `find_external_program(environment, self.for_machine, 'cmake', 'CMake', ...)`, which consults the machine file first — so write a native file OUTSIDE vendor/ (e.g. /home/madness/code/cxx/satellite/vendor/build/no-cmake.ini) containing `[binaries]` / `cmake = 'cmake-intentionally-absent'` and add `--nativ

> **AUDIT [missing-option]** Question (4) is unanswered: -Dfontconfig=enabled gives libcairo a RUNTIME dependency on a data file that linking does not provide. meson.build:277-289 sets CAIRO_HAS_FC_FONT once fontconfig is found, so cairo-ft calls into fontconfig at run time, and fontconfig reads its config from the sysconfdir COMPILED INTO libfontconfig.a (normally /etc/fonts/fonts.conf) plus a font cache directory. A satl bi
> **FIX** Not a cairo option — it is a constraint on the fontconfig build that must be decided before cairo is configured: build fontconfig with `--sysconfdir=` pointing inside satl's own payload (or ship a fonts.conf and set FONTCONFIG_FILE/FONTCONFIG_PATH from satl before the first GTK call), and vendor at least one font. vendor/fonts/ibm-plex-mono/ already exists, so the font side is half done; record th

> **AUDIT [missing-option]** gnu_symbol_visibility: 'hidden' on libcairo is dismissed too quickly. The recipe's pixman trap #5 reasons about -fvisibility=hidden and concludes it is "fine for linking the whole stack into one executable". That is true for STATIC linking, but it also means every cairo_* symbol lands in satl's .o closure as STV_HIDDEN and therefore never appears in the executable's dynamic symbol table — not even
> **FIX** No cairo option changes it (gnu_symbol_visibility is hardcoded in src/meson.build, and it is not editable under the vendor rule). Make it a stack-wide rule instead: every GTK module, pixbuf loader, IM module and print backend must be built INTO satl, and nothing dlopened at run time may reference cairo_*. Write that down alongside the libwayland-client rule, since both are about what crosses the d

> **AUDIT [ordering]** "Stage's 2.14.3 clears it, so COLRv1 will be on" is an assumption, not a fact, and it is decided by a LINK PROBE that fails silently. meson.build:340-343 gates HAVE_FT_COLR_V1 on BOTH a version compare AND `cc.has_function('FT_Get_Color_Glyph_Paint', dependencies: freetype_dep)`. Because -Dprefer_static=true makes meson pass --static to pkg-config, that probe links against freetype2.pc's full Libs
> **FIX** Do not assert the outcome — read it back. After configure, check the generated build/cairo/config.h for `#define HAVE_FT_COLR_V1 1` and the meson summary's 'FreeType'/'Fontconfig' lines, and treat a missing COLRv1 as a freetype .pc defect to fix before moving up the stack. This is the same discipline as counting what pkg-config returned before believing what it did not print.

> **AUDIT [wrong-option]** The prefer_static rationale names a Requires.private list that cairo.pc will not contain. The recipe says "cairo's generated .pc files put pixman-1, freetype2, fontconfig, libpng, zlib, glib-2.0 and gobject-2.0 in Requires.private" and repeats the claim in trap #6. glib and gobject are never added to cairo's `deps`: meson.build:178 starts `deps = [m_dep]`, every other found dependency does `deps +
> **FIX** Correct the note: cairo.pc's Requires.private is pixman-1, freetype2, fontconfig, libpng, zlib (plus -lm -ldl in Libs.private); glib-2.0 and gobject-2.0 appear only in cairo-gobject.pc. Keep -Dprefer_static=true everywhere downstream regardless — GTK reads cairo-gobject.pc unguarded at gtk-4.24.0/meson.build:465, so that file's private section is the one that has to resolve.

---

## glib

- **source** `glib/glib-2.90.0`
- **build system** meson — there is no other choice; the tree has only meson.build + meson.options (no configure, no CMakeLists.txt). Note the options file is meson.options, NOT meson_options.txt.
- **installs** Into ${prefix}/lib/pkgconfig (glib_pkgconfigreldir = join_paths(glib_libdir, 'pkgconfig')): glib-2.0.pc, gobject-2.0.pc, gthread-2.0.pc, gmodule-no-export-2.0.pc, gmodule-export-2.0.pc, gmodule-2.0.pc, gio-2.0.pc, gio-unix-2.0.pc, girepository-2.0.pc. GTK 4.24.0 consumes glib-2.0, gio-2.0, gobject-2
- **meson floor** meson_version : '>= 1.4.0' (with the comment "NOTE: See the policy in docs/meson-version.md before changing the Meson de

**Configure:**
```sh
CC="${CC:?export CC to the clang-24 -w wrapper first — otherwise meson silently picks /usr/bin/cc (gcc)}" \
PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
PKG_CONFIG_PATH=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
PATH=/home/madness/code/cxx/satellite/vendor/stage/bin:$PATH \
python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/glib/build \
  /home/madness/code/cxx/satellite/vendor/glib/glib-2.90.0 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --buildtype=release \
  --default-library=static \
  --prefer-static \
  --auto-features=disabled \
  --wrap-mode=nofallback \
  -Dselinux=disabled \
  -Dlibmount=disabled \
  -Dlibelf=disabled \
  -Dsysprof=disabled \
  -Ddtrace=disabled \
  -Dsystemtap=disabled \
  -Dintrospection=disabled \
  -Dman-pages=disabled \
  -Ddocumentation=false \
  -Dnls=disabled \
  -Dtests=false \
  -Dinstalled_tests=false \
  -Doss_fuzz=disabled \
  -Dxattr=true \
  -Dfile_monitor_backend=inotify \
  -Dglib_debug=disabled \
  -Dglib_assert=true \
  -Dglib_checks=true \
  -Dbsymbolic_functions=false \
  -Dmultiarch=false
```

**Depends on:** REQUIRED, must already be in stage:
- pcre2 >= 10.32 — "pcre2_req = '>=10.32'" then "pcre2 = dependency('libpcre2-8', version: pcre2_req, required: false, default_options: pcre2_options)"; if that misses, "pcre2 = dependency('libpcre2-8', version: pcre2_req, allow_fallback: true, default_options: pcre2_options)" followed by "assert(pcre2.type_name() == 'internal')" — under --wrap-mode=nofallback that assert is the loud failure.
- libffi >= 3.0.0 — "libffi_dep = dependency('libffi', version : '>= 3.0.0', default_options: {'werror': false, 'tests': false, 'doc': false})". No required:false, and girepository/gobject both need it.
- zlib, NO version floor — "libz_dep = dependency('zlib')".
- gvdb — "subproject('gvdb', default_options: {'tests': false})" / "gvdb_dep = dependency('gvdb')". Not a system library: subprojects/gvdb is already unpacked in the tree and is compiled straight into glib

**Traps:** 1. zlib is the silent leak. meson's zlib factory is DependencyFactory('zlib', [PKGCONFIG, CMAKE, SYSTEM], ...) — if stage/lib/pkgconfig/zlib.pc is absent, meson falls to CMAKE and then to ZlibSystemDependency, which does "l = self.clib_compiler.find_library(lib, [], self.libtype)" for 'z' and finds /usr/lib64/libz.so. It reports "Run-time dependency zlib found: YES" either way. Confirm stage's zlib.pc EXISTS before configuring, and grep meson-logs/meson-log.txt for the pkg-config invocation next to the zlib line.
2. -latomic. "if cc.links(libatomic_test_code, args : '-latomic', ...): atomic_dep = cc.find_library('atomic')" is unconditional and not behind any option. If gcc's libatomic.so is installed it gets linked and lands in glib-2.0.pc's private libs, even though x86_64 needs nothing from it. --prefer-static makes find_library prefer libatomic.a; if that .a isn't installed, this is a shared dep with no switch to turn it off. Check the final binary with ldd.
3. -lresolv. gio probes res_query() in libc first; glibc >= 2.34 (AlmaLinux 9) has it, so nothing is added. On an older glibc "network_libs += [ cc.find_library('resolv') ]" fires and libresolv.so becomes a hard dependency. Check meson-log for 'res_query() in -lresolv'.
4. gvdb is mandatory and is a subproject. "subproject('gvdb', ...)" is an unconditional call with no option guarding it. subprojects/gvdb is present in this tree; if it were ever cleaned away, glib cannot configure at all. Use --wrap-mode=nofallback (which still allows explicit subproject()) rather than blocking subprojects outright.
5. CC must be exp

> **AUDIT [leak]** The recipe guards CC with ${CC:?...} and says nothing about CXX, but glib probes for a C++ compiler whether or not oss_fuzz is on. `required:` false only means "do not fail" — meson still runs the detection, and have_cxx becomes true for whatever C++ compiler is on PATH. On this machine `c++` resolves to /home/madness/opt/gcc-17/bin/g++ (a GCC 17 trunk build), not clang-24, so constraint 7's -w wr
> **FIX** Add a second guarded assignment to the command, immediately after the CC line: CXX="${CXX:?export CXX to the clang-24 -w C++ wrapper first — otherwise meson silently picks /usr/bin/c++ (here, gcc-17)}" \

> **AUDIT [missing-option]** GIO compiles in its own copy of xdgmime unconditionally on Linux — no option guards it — and that code reads the host machine's shared-mime-info database at RUN time, with /usr/share hardcoded as the fallback search path. A statically linked satl still calls into it for g_content_type_guess(), so on a bare target it silently degrades to extension-only guessing and returns application/octet-stream 
> **FIX** No configure option exists — decide the data question now, before gdk-pixbuf (which does have a gio_sniffing switch). Either ship a generated mime.cache inside satl's own private XDG_DATA_DIRS, the way WIN-1 already handles keyboard data and fonts, or record that content-type guessing is extension-only on the target.

> **AUDIT [leak]** The recipe states that -Dfile_monitor_backend=inotify "sidesteps gio/inotify/meson.build's dependency('libinotify', ...)". It does not. That dependency() sits at file scope in gio/inotify/meson.build, and that subdir IS entered when the backend is inotify (gio/meson.build:841: subdir('inotify')). With the backend pinned to inotify the comparison evaluates to false, so `required: false` — the looku
> **FIX** Nothing to add to the command — but never relax PKG_CONFIG_LIBDIR, and add this line to the configure-log audit: require `Run-time dependency libinotify found: NO` in meson-logs/meson-log.txt. Count the matches; a grep that returns nothing is not the same as a grep that returns NO.

> **AUDIT [leak]** An unconditional, REQUIRED dependency the recipe never mentions once. meson's iconv factory tries BUILTIN first (iconv_open probed inside libc, which succeeds on glibc) and falls through to SYSTEM, whose class runs clib_compiler.find_library('iconv'). find_library does not consult pkg-config, so it is not constrained by PKG_CONFIG_LIBDIR at all — the exact bypass that made -Dlibelf=disabled necess
> **FIX** No switch exists. Add to the post-configure audit: the meson-log line must read `Dependency iconv found: YES (builtin)`, not `(system)`; and after install, `-liconv` must be absent from stage/lib/pkgconfig/glib-2.0.pc's Libs.private.

> **AUDIT [leak]** A pkg-config lookup guarded by no option at all, whose result decides an install path OUTSIDE --prefix. If bash-completion resolves, glib reads completionsdir out of the SYSTEM .pc and installs completion scripts there — writing into /usr/share during `ninja install`. Only PKG_CONFIG_LIBDIR prevents it. The recipe presents PKG_CONFIG_LIBDIR purely as a link-leak guard ("the one setting that makes 
> **FIX** Keep PKG_CONFIG_LIBDIR exactly as written — and after `ninja install`, verify the install manifest: every path in stage/meson-logs/install-log.txt must begin with /home/madness/code/cxx/satellite/vendor/stage.

> **AUDIT [ordering]** The recipe's traps tell you to confirm only that stage's zlib.pc exists before configuring. glib has three unconditional hard prerequisites, not one: zlib (meson.build:2304), libffi (meson.build:2302, no `required:` keyword, so required, and --wrap-mode=nofallback correctly blocks subprojects/libffi.wrap), and pcre2 (meson.build:2276, allow_fallback: true, also blocked by nofallback). libffi is ne
> **FIX** Before configuring glib, require all three of stage/lib/pkgconfig/{zlib,libffi,libpcre2-8}.pc to exist, and COUNT the hits rather than looping over the result — an empty result iterates zero times and reports success by staying silent, which is the same trap as the pkg-config sweep in the gtk-old journal.

> **AUDIT [ordering]** The recipe describes gvdb as "an unconditional subproject() call" and concludes nofallback permits it. Half right, and the reason matters. The tree has BOTH calls on consecutive lines: subproject('gvdb', ...) at 2298, then gvdb_dep = dependency('gvdb') at 2299. That dependency() carries no fallback: keyword, and subprojects/gvdb.wrap declares `[provide] dependency_names = gvdb` — a wrap-provided d
> **FIX** Keep --wrap-mode=nofallback (correct) and never substitute nodownload for it here. Add a precondition to the recipe: subprojects/gvdb/meson.build must exist before configuring. It does today.

---

## graphene

- **source** `graphene/graphene-1.10.8`
- **build system** meson — meson.build + meson_options.txt only, no configure and no CMakeLists.txt.
- **installs** ${prefix}/lib/pkgconfig/graphene-1.0.pc and ${prefix}/lib/pkgconfig/graphene-gobject-1.0.pc — the second one ONLY if build_gobject came out true. graphene-1.0.pc carries the four variables GTK reads back (graphene_has_sse2, graphene_has_gcc, graphene_has_neon, graphene_has_scalar) and "extra_cflags:
- **meson floor** meson_version: '>= 0.55.3'. Vendored meson 1.12.0 clears it, but note the gap: this release is from March 2022 and preda

**Configure:**
```sh
CC="${CC:?export CC to the clang-24 -w wrapper first — otherwise meson silently picks /usr/bin/cc (gcc)}" \
PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
PKG_CONFIG_PATH=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig \
PATH=/home/madness/code/cxx/satellite/vendor/stage/bin:$PATH \
python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/graphene/build \
  /home/madness/code/cxx/satellite/vendor/graphene/graphene-1.10.8 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --buildtype=release \
  --default-library=static \
  --prefer-static \
  --auto-features=disabled \
  --wrap-mode=nofallback \
  -Dgobject_types=true \
  -Dintrospection=disabled \
  -Dgtk_doc=false \
  -Dtests=false \
  -Dinstalled_tests=false \
  -Dsse2=true \
  -Dgcc_vector=true \
  -Darm_neon=false
```

**Depends on:** - threads (required) — "threadlib = dependency('threads')"; POSIX threads from the toolchain, no external library.
- libm (optional) — "mathlib = cc.find_library('m', required: false)"; glibc.
- gobject-2.0 >= 2.30.0 (optional in graphene's eyes, MANDATORY for us) — "gobject_req_version = '>= 2.30.0'" and "gobject = dependency('gobject-2.0', version: gobject_req_version, required: false, fallback: ['glib', 'libgobject_dep'])". glib 2.90.0 clears the floor by a mile; the floor is not the issue, the required:false is.
- g-ir-scanner (optional, disabled) — "gir = find_program('g-ir-scanner', required : get_option('introspection'))".
- mutest (bundled subproject, test-only) — subprojects/mutest is already unpacked next to subprojects/mutest.wrap; never reached with -Dtests=false.
- python3 (build-time, only used by the introspection path) — "python = import('python')" at top level, "python =

**Traps:** 1. THE trap: graphene fails at finding GObject without failing the build. "gobject = dependency('gobject-2.0', ..., required: false, fallback: ['glib', 'libgobject_dep'])" then "build_gobject = gobject.found()". If stage has no gobject-2.0.pc, configure SUCCEEDS, graphene-gobject-1.0.pc is never generated, graphene-gobject.c is never compiled, and the failure surfaces only when GTK asks for graphene-gobject-1.0. Two checks, both cheap: the configure summary must print "GObject types: YES" under Features, and after install stage/lib/pkgconfig/graphene-gobject-1.0.pc must exist.
2. graphene's fallback names a subproject it does not ship. "fallback: ['glib', 'libgobject_dep']" refers to a glib wrap; subprojects/ contains only mutest and mutest.wrap. So the fallback is dead in every wrap mode — there is no rescue path, only the pkg-config lookup.
3. -Dinstalled_tests defaults to TRUE in this project ("option('installed_tests', type: 'boolean', value: true, description: 'Install tests')"), unlike almost everything else in the stack. Pass it false explicitly.
4. graphene's .pc injects SIMD flags into every consumer's CFLAGS via extra_cflags. That is intended and GTK depends on the matching graphene_has_sse2 variable — do not "clean up" sse2_cflags out of the pkgconfig call, and do not let a system graphene-1.0.pc with different SIMD variables be found instead.
5. CC must be exported, same as glib: meson defaults to /usr/bin/cc, and graphene's test_cflags list is full of -Werror=float-conversion, -Werror=redundant-decls, -Werror=shadow and friends. The -w wrapper is what defeats t

> **AUDIT [wrong-option]** I enumerated all eight options in this version's meson_options.txt one by one — gtk_doc, gobject_types, introspection, gcc_vector, sse2, arm_neon, tests, installed_tests — and the command passes every one, with names that exist in THIS version; the only library target is library() at src/meson.build:41, so --default-library=static yields libgraphene-1.0.a and nothing else; nothing is dlopened and 
> **FIX** Keep -Dinstalled_tests=false (harmless, documents intent) but treat -Dtests=false as the actual guard. No option needs adding or changing — graphene's command is correct as written.

---

## pango

- **source** `pango/pango-1.58.2`
- **build system** meson — the only build system shipped. There is no configure/CMakeLists; the tree has meson.build + meson.options (NOT meson_options.txt — it uses the newer filename, so `cat meson_options.txt` fails and looks like "no options").
- **installs** Into $prefix/lib/pkgconfig: pango.pc (filebase: 'pango', `requires: pango_pkg_requires` = ['gobject-2.0', 'harfbuzz']), pangoft2.pc (filebase: 'pangoft2', `requires: [ 'pango', freetype2_pc, 'fontconfig' ]` → pango, freetype2, fontconfig), pangoot.pc (requires: ['pangoft2']), pangofc.pc (requires: [
- **meson floor** >= 1.2.0 — meson.build:9 `meson_version : '>= 1.2.0')`. Vendored meson 1.12.0 satisfies it, and 1.12.0 is what supplies 

**Configure:**
```sh
# Environment first — these three lines are part of the configure step, not decoration.
# PKG_CONFIG_PATH ALONE IS NOT ENOUGH (see traps): pkg-config 2.1.0 here reports
#   pc_path = /usr/lib64/pkgconfig:/usr/share/pkgconfig
# and PKG_CONFIG_PATH only PREPENDS to that. PKG_CONFIG_LIBDIR REPLACES it.
export PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/share/pkgconfig
unset PKG_CONFIG_PATH
export PATH=/home/madness/code/cxx/satellite/vendor/stage/bin:$PATH   # so glib-mkenums is OURS, not /usr/bin's

python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/pango/build \
  /home/madness/code/cxx/satellite/vendor/pango/pango-1.58.2 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --buildtype=release \
  --default-library=static \
  --wrap-mode=nodownload \
  -Dprefer_static=true \
  -Db_staticpic=true \
  -Dcairo=enabled \
  -Dfontconfig=enabled \
  -Dfreetype=enabled \
  -Dxft=disabled \
  -Dlibthai=disabled \
  -Dsysprof=disabled \
  -Dintrospection=disabled \
  -Ddocumentation=false \
  -Dman-pages=false \
  -Dbuild-testsuite=false \
  -Dbuild-examples=false
```

**Depends on:** REQUIRED (no `required:` kwarg → mandatory):
- glib-2.0 / gobject-2.0 / gio-2.0, floor >= 2.88 — line 212 `glib_minor_req = 88`; line 214 `glib_req       = '>= @0@.@1@'.format(glib_major_req, glib_minor_req)`; line 233 `glib_dep = dependency('glib-2.0', version: glib_req)`, line 234 `gobject_dep = dependency('gobject-2.0', version: glib_req)`, line 235 `gio_dep = dependency('gio-2.0', version: glib_req)`. Vendored glib is 2.90.0 — satisfied. Note it also compiles with `-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_88`.
- fribidi, floor >= 1.0.6 — line 216 `fribidi_req    = '>= 1.0.6'`; line 237 `fribidi_dep = dependency('fribidi', version: fribidi_req, default_options: ['docs=false'])`. Vendored 1.0.17.
- harfbuzz, floor >= 11.0.0 — line 218 `harfbuzz_req   = '>= 11.0.0'`; line 281 `harfbuzz_dep = dependency('harfbuzz', version: harfbuzz_req, default_options: harfbuzz_fallback_options)`. Ven

**Traps:** 1. PKG_CONFIG_PATH DOES NOT ISOLATE ANYTHING. Measured here: `pkg-config --variable pc_path pkg-config` = /usr/lib64/pkgconfig:/usr/share/pkgconfig, pkg-config 2.1.0. PKG_CONFIG_PATH is searched BEFORE that list, never INSTEAD of it. So any .pc name the stage does not provide still resolves from /usr, silently. Confirmed present and probed by pango: libthai 0.1.29, xft 2.3.8. Only PKG_CONFIG_LIBDIR replaces the default list. This is the single failure mode named in constraint 2 and the environment as described in the brief does not prevent it.
2. THE `freetype` OPTION IS A NO-OP ON LINUX. Lines 305-309: `freetype_option = get_option('freetype')` / `freetype_required = fontconfig_dep.found()` / `if not freetype_option.disabled() or freetype_required` / `  freetype_option = false`. Whenever fontconfig is found (always, on Linux) the option is overwritten with `false` — meaning "not required" — so -Dfreetype=disabled does NOT disable freetype and -Dfreetype=enabled does NOT make it required. If freetype2.pc is missing from the stage you do not get a missing-dependency error; you get `error('No Cairo font backends found')` several hundred lines later, which points at cairo.
3. `subdir('utils')` (line 568) and `subdir('tools')` (line 569) are UNCONDITIONAL — there is no option to skip them, unlike tests and examples. pango-view, pango-list and pango-segmentation are always built AND installed, plus tools/gen-script-for-lang. These are the first full static link of the entire lower stack (pango → cairo → pixman/freetype/fontconfig/harfbuzz → expat/png/zlib → glib → pcre2/libffi).

> **AUDIT [leak]** RUNTIME DATA LEAK, and the recipe says nothing about it. -Dfontconfig=enabled is forced, and pango calls FcInit() at runtime to build its font map. Statically linking libfontconfig.a does NOT provide fontconfig's configuration: FcInit() loads /etc/fonts/fonts.conf (or the configdir baked into fontconfig at ITS build time) plus the font directories that file names. On a target machine with no /etc/
> **FIX** No pango option fixes this - it is a constraint on the fontconfig recipe and on satl. (a) The staged fontconfig must be configured to bake in its own config rather than point at /etc/fonts, and satl must ship a fonts.conf plus at least one font and set FONTCONFIG_FILE (or FONTCONFIG_PATH) before the first pango call. (b) Add a post-build check that actually renders text with /etc/fonts temporarily

> **AUDIT [missing-option]** --wrap-mode=nodownload is the weaker of the two modes and does not deliver what the recipe claims for it. The recipe states nodownload 'guarantees nothing is cloned into subprojects/ and that a missing staged dependency is a loud error'. Only the first half is true. nodownload blocks DOWNLOADING a wrap; it does not block USING a subproject directory that already exists on disk. pango has nine wrap
> **FIX** Use --wrap-mode=nofallback in place of --wrap-mode=nodownload. It refuses dependency() fallbacks outright, downloaded or already present, so a missing staged .pc is always a hard error. Verified safe for pango: its only subproject() call is guarded by `if cairo_found_type == 'internal'`, which cannot be reached once cairo comes from the stage as pkgconfig.

> **AUDIT [ordering]** The recipe documents twelve traps but never states pango's build order, and pango has five dependencies that are REQUIRED with no option and no `required:` kwarg at all - glib-2.0, gobject-2.0, gio-2.0, fribidi and harfbuzz - plus cairo, fontconfig and freetype2 which the options cannot actually make optional on Linux. The gdk-pixbuf agent did call its ordering constraint out (libtiff before gdk-p
> **FIX** State the prerequisite set explicitly in the recipe: glib, fribidi, harfbuzz, freetype, fontconfig and cairo must all be installed into stage before pango is configured, and cairo must have been built WITH freetype and fontconfig support or pango dies several hundred lines later at meson.build:465 `error('@0@ does not have the required FontConfig support')`.

---

## gdk-pixbuf

- **source** `gdk-pixbuf/gdk-pixbuf-2.44.8`
- **build system** meson — the only build system shipped (meson.build + meson_options.txt; no configure, no CMakeLists).
- **installs** One file: gdk-pixbuf-2.0.pc, into $prefix/lib/pkgconfig. `pkgconfig.generate(gdkpixbuf, name: 'GdkPixbuf', description: 'Image loading and scaling', ... requires: 'gobject-2.0', subdirs: gdk_pixbuf_api_name, filebase: gdk_pixbuf_api_name,)` where gdk_pixbuf_api_name = 'gdk-pixbuf-2.0'. It also defin
- **meson floor** >= 1.5 — meson.build:9 `  meson_version: '>= 1.5',`. Vendored meson 1.12.0 satisfies it.

**Configure:**
```sh
# Same environment prologue as pango — PKG_CONFIG_LIBDIR, not PKG_CONFIG_PATH.
export PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/lib/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/lib64/pkgconfig:/home/madness/code/cxx/satellite/vendor/stage/share/pkgconfig
unset PKG_CONFIG_PATH
export PATH=/home/madness/code/cxx/satellite/vendor/stage/bin:$PATH

python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
  /home/madness/code/cxx/satellite/vendor/gdk-pixbuf/build \
  /home/madness/code/cxx/satellite/vendor/gdk-pixbuf/gdk-pixbuf-2.44.8 \
  --prefix=/home/madness/code/cxx/satellite/vendor/stage \
  --libdir=lib \
  --buildtype=release \
  --default-library=static \
  --wrap-mode=nodownload \
  -Dprefer_static=true \
  -Db_staticpic=true \
  -Dbuiltin_loaders=all \
  -Dpng=enabled \
  -Djpeg=enabled \
  -Dtiff=enabled \
  -Dgif=enabled \
  -Dothers=disabled \
  -Dlegacy_xpm=disabled \
  -Dglycin=disabled \
  -Dandroid=disabled \
  -Dnative_windows_loaders=false \
  -Dgio_sniffing=false \
  -Drelocatable=false \
  -Dintrospection=disabled \
  -Ddocumentation=false \
  -Dman=false \
  -Dthumbnailer=disabled \
  -Dtests=false \
  -Dinstalled_tests=false
```

**Depends on:** REQUIRED:
- glib-2.0, gobject-2.0, gmodule-no-export-2.0, gio-2.0 — floor >= 2.56.0. Line 61 `glib_req_version = '>= 2.56.0'`; lines 62-69 `glib_dep = dependency('glib-2.0', version: glib_req_version,\n                      fallback : ['glib', 'libglib_dep'])` / `gobject_dep = dependency('gobject-2.0', version: glib_req_version, ...)` / `gmodule_dep = dependency('gmodule-no-export-2.0', version: glib_req_version, ...)` / `gio_dep = dependency('gio-2.0', version: glib_req_version, ...)`. Vendored glib 2.90.0. gmodule-no-export-2.0.pc must also carry the variable `gmodule_supported` — meson.build reads it: `build_modules = gmodule_dep.get_variable(pkgconfig: 'gmodule_supported') == 'true'`.
- libpng, no version floor — line 292 `png_dep = dependency(is_msvc_like ? 'png' : 'libpng', required: false)` then line 299 `png_dep = dependency('libpng', fallback: ['libpng', 'libpng_dep'], required:

**Traps:** 1. SHARED MODULES SURVIVE --default-library=static. gdk-pixbuf/meson.build emits `mod = shared_module('pixbufloader-@0@'.format(name), ... install: true, install_dir: gdk_pixbuf_loaderdir)` for every enabled loader that is not builtin. shared_module() is always a .so regardless of default_library. The default -Dbuiltin_loaders=['default'] resolves to only ['png','jpeg'], so a build that looks correctly configured still installs libpixbufloader-gif.so and libpixbufloader-tiff.so into the prefix and violates constraint 1. -Dbuiltin_loaders=all is the fix.
2. GLYCIN ABORTS THE DEFAULT CONFIGURE. `.enable_auto_if(host_machine.system() == 'linux')` promotes 'auto' to 'enabled', making glycin-2 >= 2.2.alpha.7 a hard requirement; it is not installed. And if anyone ever installs it, the builtin_loaders default silently flips from ['png','jpeg'] to ['glycin'] — `if glycin_dep.found()` / `    builtin_loaders = ['glycin']` — and png/jpeg/gif/tiff are all disable_auto_if'd away underneath you.
3. gio_sniffing IS A RUNTIME DATA DEPENDENCY, not just a link one. Default true → `dependency('shared-mime-info')` (installed here, version 2.3) → GDK_PIXBUF_USE_GIO_MIME is set → the finished binary needs the system MIME database on the TARGET machine. Nothing in ldd or a static-linkage audit will ever reveal it. This is the named example in constraint 2 and the trap that matters most for "needs nothing installed on the target machine".
4. libtiff at 'auto' silently takes system libtiff 4.6.0. Measured: `pkg-config --print-requires-private libtiff-4` → libwebp libzstd libjpeg zlib, and `pkg-conf

> **AUDIT [leak]** TWO UNGUARDED SYSTEM-LIBRARY PROBES THAT PKG_CONFIG_LIBDIR CANNOT COVER, and neither recipe's option enumeration caught them because they are not options. intl_dep and medialib_dep come from cc.find_library(), which bypasses pkg-config entirely and searches the linker's default path (/usr/lib64). Both are pushed into gdk_pixbuf_deps unconditionally, so they land in every loader static_library, in 
> **FIX** Nothing to pass - instead extend the post-configure audit. The recipes only grep the log for 'Run-time dependency <name> found: YES' (pkg-config); also require the two lines `Library intl found: NO` and `Library mlib found: NO` in meson-log.txt before accepting the configure, and re-check them on any machine other than this one.

> **AUDIT [missing-option]** Same --wrap-mode weakness as pango, and it bites harder here exactly as the recipe's own trap 10 argues. gdk-pixbuf's libpng and libjpeg-turbo wraps are [wrap-file] entries carrying wrapdb patch zips - third-party meson.build files dropped into upstream source, which vendor/README_FIRST.md classes as an edit - and their pins (libpng 1.6.55, libjpeg-turbo 2.1.2) are OLDER than what is being staged 
> **FIX** Use --wrap-mode=nofallback in place of --wrap-mode=nodownload. Verified safe: the one subproject('glib') call sits in the else branch of `if gmodule_dep.type_name() == 'pkgconfig'`, which is unreachable once gmodule-no-export-2.0 comes from the stage.

> **AUDIT [ordering]** THREE EXECUTABLES ARE BUILT AND INSTALLED UNCONDITIONALLY and the recipe never mentions them. The pango agent flagged the equivalent (its trap 3, pango-view/pango-list/pango-segmentation); the gdk-pixbuf recipe has no counterpart, and it turned off tests, thumbnailer, docs and introspection in the belief that nothing but the library gets linked. gdk-pixbuf-csource, gdk-pixbuf-pixdata and gdk-pixbu
> **FIX** No option removes them - plan for it instead: build gdk-pixbuf only after libtiff, libpng, libjpeg-turbo, zlib and glib are fully staged, and read a failure in these three targets as a missing Libs.private in a lower project, not as a gdk-pixbuf bug. Same expectation the pango recipe already sets for pango-view.

> **AUDIT [wrong-option]** -Dlegacy_xpm=disabled silently breaks a public API function, and the recipe's rationale ('already the defaults; they pull no external library, so they are safe to add later; off is smaller') does not mention it. gdk_pixbuf_new_from_xpm_data() is compiled unconditionally into gdk-pixbuf-io.c - so this is NOT a link error that would announce itself - but its body resolves the loader by name at runti
> **FIX** If satl or GTK 4.24 calls gdk_pixbuf_new_from_xpm_data (grep for it before deciding), pass -Dlegacy_xpm=enabled - io-xpm.c is pure in-tree code and pulls in no external library, so it costs nothing against the static-only constraint. Otherwise keep it disabled but record that the function is dead.

> **AUDIT [wrong-option]** The rationale for -Dothers=disabled is self-contradicting once -Dbuiltin_loaders=all is also passed. It says the other loaders 'are safe to add later'. They are not addable later in any drop-in sense: with builtin_all_loaders true the recipe's own trap 1 shows that ZERO loader .so files are produced and loaders.cache is empty, so there is no module directory to add a loader to. The image formats t
> **FIX** Decide the format list now. If anything in satl's assets or GTK's icon path is not png/jpeg/gif/tiff, pass -Dothers=enabled (all of them are in-tree C with no external library, so the static-only and no-/usr constraints are unaffected) rather than planning to add it afterwards.

---

## wayland-protocols

- **source** `wayland-protocols/wayland-protocols-1.49`
- **build system** meson — it is the only build system in the tree (`meson.build`, `meson_options.txt`; no configure, no CMakeLists.txt). `project('wayland-protocols', version: '1.49', meson_version: '>= 0.58.0', license: 'MIT/Expat')` declares NO language, so meson never looks for a compiler here.
- **installs** share/pkgconfig/wayland-protocols.pc  — NOT lib64/pkgconfig.

  meson.build:188-203  pkgconfig.generate(filebase: 'wayland-protocols', ..., dataonly: true, variables: {...'pkgdatadir': '${pc_sysrootdir}${datarootdir}/wayland-protocols'})

No install_dir is given, and meson's pkgconfig module redirec
- **meson floor** >= 0.58.0  — meson.build:3 `meson_version: '>= 0.58.0'`. Vendored meson is 1.12.0. Fine.

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage; \
env -u PKG_CONFIG_PATH \
  PKG_CONFIG_LIBDIR="$STAGE/lib64/pkgconfig:$STAGE/lib/pkgconfig:$STAGE/share/pkgconfig" \
  /usr/bin/python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
    /home/madness/code/cxx/satellite/vendor/wayland-protocols/build-static \
    /home/madness/code/cxx/satellite/vendor/wayland-protocols/wayland-protocols-1.49 \
    --prefix="$STAGE" \
    --libdir=lib64 \
    --datadir=share \
    --includedir=include \
    --default-library=static \
    --buildtype=release \
    --wrap-mode=nodownload \
    -Dtests=false

# then:
#   ninja -C /home/madness/code/cxx/satellite/vendor/wayland-protocols/build-static
#   /usr/bin/python3 .../meson.py install -C .../build-static
```

**Depends on:** ONE, and it is optional under our options:

  meson.build:11-16
    dep_scanner = dependency('wayland-scanner',
        version: get_option('tests') ? '>=1.25.0' : '>=1.22.90',
        required: get_option('tests'),
        native: true,
        fallback: 'wayland'
    )

  With -Dtests=false: required=false, floor >=1.22.90, machine kind = NATIVE (build machine).
  With -Dtests=true: required=true, floor >=1.25.0 — unsatisfiable here.

That is the whole list. No library dependency of any kind. Nothing else calls dependency(), find_library() or cc.anything.

Consequence for build order: wayland-protocols has NO dependency on any other vendored project and can be built FIRST, in parallel with anything.

**Traps:** 1. THE .pc LANDS IN share/pkgconfig, AND /usr ALREADY HAS ONE. `ls /usr/share/pkgconfig/` shows `wayland-protocols.pc` on this machine. The task's stated search path is only $STAGE/lib64/pkgconfig and $STAGE/lib/pkgconfig — neither contains ours. GTK would then find the SYSTEM wayland-protocols.pc, read its pkgdatadir=/usr/share/wayland-protocols, scan the system XML, and build perfectly. Nothing warns you; our vendored copy is simply never read. $STAGE/share/pkgconfig must be on the path. Same trap hits xkeyboard-config.

2. PKG_CONFIG_PATH IS ADDITIVE, PKG_CONFIG_LIBDIR IS NOT. Setting only PKG_CONFIG_PATH leaves /usr/lib64/pkgconfig and /usr/share/pkgconfig in the default search list; PKG_CONFIG_PATH entries are merely searched first. For a project whose whole job is to shadow a system package, that is the difference between shadowing it and shadowing it only until one of ours fails to install. Use PKG_CONFIG_LIBDIR (set it to the three stage dirs) and unset PKG_CONFIG_PATH. Caveat: PKG_CONFIG_LIBDIR also hides wayland-client.pc / wayland-egl.pc, which constraint 5 says must stay system — so add a fourth, explicit allow-list directory holding copies of just those .pc files. The previous round did exactly this (configure-static.sh:80 `PKG_CONFIG_PATH="$here/build-pkgconfig"`, with a guard on line 28 that libdrm.pc is in it).

3. Whether the enum headers get built depends on something you did not set. `if dep_scanner.found(): subdir('include/wayland-protocols')`. Under PKG_CONFIG_LIBDIR=stage-only, wayland-scanner is NOT found, so no headers install and configure prints `H

> **AUDIT [missing-option]** The prescribed pkg-config allow-list directory — 'copies of just those .pc files', wayland-client.pc and wayland-egl.pc — omits wayland-scanner.pc, and without it GTK cannot generate a single line of Wayland protocol code. This is invisible to a grep of GTK: across all 67 meson.build files in gtk-4.24.0 the strings 'wayland-scanner' and 'wayland_scanner' appear ZERO times (the only 'scanner' hits 
> **FIX** Add `/usr/lib64/pkgconfig/wayland-scanner.pc` to the GTK-stage allow-list directory alongside wayland-client.pc and wayland-egl.pc (or supply a meson native file with `[binaries]` / `wayland-scanner = '/usr/bin/wayland-scanner'`), and keep that directory off wayland-protocols' own PKG_CONFIG_LIBDIR. GTK 4.24 needs only wayland-client, wayland-protocols and wayland-egl by .pc name (meson.build:584-

---

## xkeyboard-config

- **source** `xkeyboard-config/xkeyboard-config-2.48`
- **build system** meson — only build system present (`meson.build`, `meson_options.txt`, plus `rules/`, `symbols/`, `po/` subdir meson files; no configure script). `project('xkeyboard-config', version: '2.48', license: 'MIT/Expat', meson_version: '>= 0.61.0')` declares NO language, so no compiler is needed; this is a pure data package.
- **installs** share/pkgconfig/xkeyboard-config-2.pc   (filebase is versioned)
share/pkgconfig/xkeyboard-config.pc     (an install_symlink to the above, for consumers older than 2.45)

  meson.build:13  dir_pkgconfig = join_paths(dir_data, 'pkgconfig')
  meson.build:18-33  pkgconfig.generate(filebase: 'xkeyboard-c
- **meson floor** >= 0.61.0  — meson.build:4 `meson_version: '>= 0.61.0'`. Needed for `install_symlink`, which this project uses three tim

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage; \
env -u PKG_CONFIG_PATH \
  PATH=/usr/bin:"$PATH" \
  PKG_CONFIG_LIBDIR="$STAGE/lib64/pkgconfig:$STAGE/lib/pkgconfig:$STAGE/share/pkgconfig" \
  /usr/bin/python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
    /home/madness/code/cxx/satellite/vendor/xkeyboard-config/build-static \
    /home/madness/code/cxx/satellite/vendor/xkeyboard-config/xkeyboard-config-2.48 \
    --prefix="$STAGE" \
    --libdir=lib64 \
    --datadir=share \
    --sysconfdir=etc \
    --mandir=share/man \
    --buildtype=release \
    --wrap-mode=nodownload \
    -Dnls=false \
    -Dcompat-rules=true \
    -Dxorg-rules-symlinks=false \
    -Dnon-latin-layouts-list=false

# then:
#   ninja -C /home/madness/code/cxx/satellite/vendor/xkeyboard-config/build-static
#   /usr/bin/python3 .../meson.py install -C .../build-static
#
# PATH=/usr/bin:$PATH is load-bearing — see traps. To also suppress the man page,
# add a PATH with no xsltproc on it; there is no option for it.
```

**Depends on:** No library dependencies at all. Every dependency is a build-time TOOL:

  rules/meson.build:8-16   REQUIRED, python3 >= 3.11
      MINIMUM_PYTHON_VERSION = '3.11'
      pymod = import('python')
      python = pymod.find_installation('python3')
      if python.language_version().version_compare('<@0@'.format(MINIMUM_PYTHON_VERSION))
          error('Minimum required Python version: @0@, but got: @1@'.format(...))

  rules/meson.build:17     REQUIRED (find_program defaults to required:true), needs perl
      xml2lst = find_program('xml2lst.pl')
      (rules/xml2lst.pl line 1: `#!/usr/bin/env perl`; mode 0755. perl here is v5.40.2.)

  meson.build:56           OPTIONAL
      xsltproc = find_program('xsltproc', required: false)

  meson.build:90-93        OPTIONAL
      python = pymod.find_installation('python3', modules: ['pytest'], required: false)

  meson.build:133          OPTIONAL
    

**Traps:** 1. `python3` MEANS WHATEVER IS FIRST ON PATH, AND ON THIS MACHINE THAT IS AN ALPHA PyPy. `which -a python3` gives /home/madness/opt/pypy3/pypy3/bin/python3 first, and it reports:
     Python 3.11.15 (808488746c6a, Aug 24 2026, 15:34:20) [PyPy 8.0.0-alpha0 ...]
   It passes the >= 3.11 floor, so meson accepts it silently. Worse, rules/meson.build:23-34 and symbols/meson.build:18-24 hardcode the literal string `'python3'` in the command rather than `python.full_path()`, so the generator that writes the keyboard rules and the compat symbols runs under an alpha PyPy. /usr/bin/python3 is CPython 3.12.14 and also clears the floor. Hence `PATH=/usr/bin:$PATH` in the command above. (Harmless for the compilers: libepoxy/libxkbcommon get CC/CXX from the environment, and this project compiles nothing.)

2. THE MAN PAGE CANNOT BE TURNED OFF BY AN OPTION. meson.build:56-75: if `xsltproc` is found it builds and installs share/man/man7/xkeyboard-config-2.7 plus a symlink. xsltproc IS at /usr/bin/xsltproc here, so constraint 4's "no man pages" is violated by default and no flag prevents it. Two files; either accept them or run `meson setup` with a PATH that has no xsltproc.

3. THE X11 SYMLINK IS ABSOLUTE. meson.build:126-130 installs $STAGE/share/X11/xkb pointing to `dir_xkb_base` — the absolute configure-time path $STAGE/share/xkeyboard-config-2. If satl spills `share/X11/xkb` into a target machine's home, the link dangles. Spill `share/xkeyboard-config-2/` and point XKB_CONFIG_ROOT at THAT. (This is the same shape as the recorded LD_RUN_PATH-bakes-an-RPATH problem: an absolute developer

> **AUDIT [ordering]** The recipe states no ordering constraint between xkeyboard-config and libxkbcommon, and the prior analysis recommends installing xkeyboard-config into the stage FIRST as a defence. That order is what publishes `xkb_base=$STAGE/share/X11/xkb` in the .pc and creates a live absolute symlink at that path to the full database — the two ingredients of the libxkbcommon leak above. Note also that the X11 
> **FIX** Configure libxkbcommon BEFORE `meson install` of xkeyboard-config (or with the stage's share/pkgconfig hidden from libxkbcommon, per the fix above), then install xkeyboard-config. Have satl spill `share/xkeyboard-config-2/` and point XKB_CONFIG_ROOT at that directory — never spill `share/X11/xkb`, whose symlink dangles off the build machine.

> **AUDIT [wrong-option]** The command's `PATH=/usr/bin:"$PATH"` — added to fix the PyPy python3 trap — guarantees that /usr/bin/xsltproc is found, so the man page and its symlink are built by default and installed, in direct contradiction to the recipe's own trap 2 advice ('run meson setup with a PATH that has no xsltproc') and to constraint 4's 'NO man pages'. The recipe notes the conflict but ships the command that loses
> **FIX** Replace `PATH=/usr/bin:$PATH` with a curated directory containing symlinks to only python3, perl and msgfmt (no xsltproc) and put that first: `PATH=/home/madness/code/cxx/satellite/vendor/build-tools-path`. Or accept the two files and drop the 'no man pages' claim for this project — but decide explicitly, because as written the command silently violates it.

> **AUDIT [missing-option]** perl is a hard, required, unvendored build-time dependency of this project and is named nowhere in the recipe. `find_program('xml2lst.pl')` defaults to required:true, resolves to the in-tree script, and that script's first line is `#!/usr/bin/env perl`. Its output (`base.lst`, `evdev.lst`) is `install: true`, so it is not skippable. The recipe correctly names bison as an unvendored build dep for l
> **FIX** Add perl, xsltproc and the gettext tools to the stack's unvendored-build-tool list beside bison, and have check.sh assert each is present before configure rather than discovering it as a mid-build failure.

---

## libepoxy

- **source** `libepoxy/libepoxy-1.5.10`
- **build system** meson — only build system present (`meson.build`, `meson_options.txt`, `src/meson.build`, `include/epoxy/meson.build`; no configure, no CMakeLists.txt). `project('libepoxy', 'c', version: '1.5.10', default_options: ['buildtype=debugoptimized', 'c_std=gnu99', 'warning_level=1'], license: 'MIT', meson_version: '>= 0.54.0')`.
- **installs** lib64/pkgconfig/epoxy.pc

  src/meson.build:108-121  pkg.generate(libraries: libepoxy, name: 'epoxy', filebase: 'epoxy', ...,
                             variables: ['epoxy_has_glx=@0@', 'epoxy_has_egl=@0@', 'epoxy_has_wgl=@0@'],
                             requires_private: ' '.join(gl_reqs))

No
- **meson floor** >= 0.54.0  — meson.build:8 `meson_version: '>= 0.54.0'`. This is the oldest floor of the four and the only one old enoug

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage; \
env -u PKG_CONFIG_PATH \
  PKG_CONFIG_LIBDIR="$STAGE/lib64/pkgconfig:$STAGE/lib/pkgconfig:$STAGE/share/pkgconfig" \
  CC=/home/madness/code/cxx/satellite/vendor/build-cc/cc \
  CXX=/home/madness/code/cxx/satellite/vendor/build-cc/c++ \
  /usr/bin/python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
    /home/madness/code/cxx/satellite/vendor/libepoxy/build-static \
    /home/madness/code/cxx/satellite/vendor/libepoxy/libepoxy-1.5.10 \
    --prefix="$STAGE" \
    --libdir=lib64 \
    --datadir=share \
    --includedir=include \
    --default-library=static \
    --buildtype=release \
    --wrap-mode=nodownload \
    -Dglx=no \
    -Dx11=false \
    -Degl=yes \
    -Dtests=false \
    -Ddocs=false

# then:
#   ninja -C /home/madness/code/cxx/satellite/vendor/libepoxy/build-static
#   /usr/bin/python3 .../meson.py install -C .../build-static
# produces $STAGE/lib64/libepoxy.a  (src/meson.build:69 library('epoxy', ...))
#
# CC/CXX must be the -w wrapper (constraint 7); meson.build:100-136 adds
# -Werror=implicit, -Werror=nonnull, -Werror=init-self, -Werror=main,
# -Werror=missing-braces, -Werror=sequence-point, -Werror=return-type,
# -Werror=trigraphs, -Werror=array-bounds, -Werror=write-strings,
# -Werror=address, -Werror=int-to-pointer-cast, -Werror=pointer-to-int-cast
# through cc.get_supported_arguments(), i.e. clang 24 will accept every one of them.
```

**Depends on:** REQUIRED — none. Not one dependency in this project is required on Linux.

  meson.build:166   dl_dep      = cc.find_library('dl', required: false)          no floor
  meson.build:167   gl_dep      = dependency('gl',      required: false)          no floor
  meson.build:168   egl_dep     = dependency('egl',     required: false)          no floor
  meson.build:172   x11_dep     = dependency('x11',     required: false)          no floor   (tests only, per its own comment on :171)
  meson.build:178   gles2_dep   = dependency('glesv2',  required: false)          no floor
  meson.build:180     fallback: cc.find_library('libGLESv2', required: false)
  meson.build:183   gles1_dep   = dependency('glesv1_cm', required: false)        no floor
  meson.build:185     fallback: cc.find_library('libGLESv1_CM', required: false)
  meson.build:192   opengl32_dep = cc.find_library('opengl32', required: tru

**Traps:** 1. epoxy/egl.h INCLUDES A HEADER THAT IS NOT VENDORED AND NOT DECLARED. src/gen_dispatch.py:493 emits `#include "EGL/eglplatform.h"` into the generated, INSTALLED epoxy/egl.h. There is no EGL/ directory in this tarball (`ls include/epoxy` is common.h egl.h gl.h glx.h meson.build wgl.h), the file comes from /usr/include/EGL/eglplatform.h (mesa/libglvnd headers), and nothing in meson.build requires or even mentions it. So libepoxy compiles, and so does every later consumer of epoxy/egl.h — including GTK's GDK Wayland code — only because /usr/include happens to hold Khronos headers. It is a HEADER-only dependency: no .so is linked and no /usr path is baked into the binary, so it does not violate the static rule. But it does mean this project cannot build on a machine without mesa's EGL headers, and nothing in the build files will tell you that. If it must be closed, the fix is to vendor the three Khronos headers (EGL/eglplatform.h, KHR/khrplatform.h) and add -I for them, not to edit epoxy.
   Checked and SAFE by default: /usr/include/EGL/eglplatform.h only reaches for X11/Xlib.h under `#elif defined(USE_X11)` (line 106); with USE_X11 undefined it takes the `#elif defined(__unix__)` branch (line 116) and typedefs EGLNativeDisplayType to void*. So including epoxy/egl.h does NOT drag in X11 headers here. Do not define USE_X11 anywhere in the stack.

2. epoxy dlopens its GL AT RUN TIME, BY DESIGN — a static libepoxy.a does not make the binary independent of the GPU stack. src/dispatch_common.c:193-197 `#define GLX_LIB "libGL.so.1"`, `#define EGL_LIB "libEGL.so.1"`, `#define OPENGL

> **AUDIT [leak]** `/usr/lib64/pkgconfig/epoxy.pc` exists on this machine. The recipe raises the shadowing trap for libxkbcommon (its trap 6, xkbcommon.pc/xkbregistry.pc) and for wayland-protocols, but never for epoxy — yet epoxy is the one of the four that actually produces a library GTK links. If the stage install ever fails or lands under lib/ instead of lib64/, GTK's `dependency('epoxy')` resolves the system .pc
> **FIX** Before GTK configures, assert the stage copy exists and is the one that will be found: `test -f "$STAGE/lib64/pkgconfig/epoxy.pc"` and `pkg-config --variable=libdir epoxy` must print a path under $STAGE — extend the existing guard pattern at vendor/gtk-old/configure-static.sh:28. Keep /usr/lib64/pkgconfig off PKG_CONFIG_LIBDIR for every project, and keep the wayland allow-list directory to wayland

---

## libxkbcommon

- **source** `libxkbcommon/libxkbcommon-xkbcommon-1.13.2`
- **build system** meson — only build system present (single top-level `meson.build` of ~1500 lines plus `meson_options.txt`; no configure, no CMakeLists.txt). `project('libxkbcommon', 'c', version: '1.13.2', default_options: ['c_std=c11', 'warning_level=3'], meson_version : '>= 1.4.0')`.
- **installs** lib64/pkgconfig/xkbcommon.pc   — and, with our options, ONLY that one.

  meson.build:446-454  pkgconfig.generate(libxkbcommon, name: 'xkbcommon', filebase: 'xkbcommon', version: meson.project_version(), description: 'XKB API common to servers and clients', variables: pkgconfig_variables)

NOT insta
- **meson floor** >= 1.4.0  — meson.build:9 `meson_version : '>= 1.4.0', # Released on March 2024`. The highest floor of the four; the ven

**Configure:**
```sh
STAGE=/home/madness/code/cxx/satellite/vendor/stage; \
env -u PKG_CONFIG_PATH \
  PKG_CONFIG_LIBDIR="$STAGE/lib64/pkgconfig:$STAGE/lib/pkgconfig:$STAGE/share/pkgconfig" \
  CC=/home/madness/code/cxx/satellite/vendor/build-cc/cc \
  CXX=/home/madness/code/cxx/satellite/vendor/build-cc/c++ \
  /usr/bin/python3 /home/madness/code/cxx/satellite/vendor/meson/meson-1.12.0/meson.py setup \
    /home/madness/code/cxx/satellite/vendor/libxkbcommon/build-static \
    /home/madness/code/cxx/satellite/vendor/libxkbcommon/libxkbcommon-xkbcommon-1.13.2 \
    --prefix="$STAGE" \
    --libdir=lib64 \
    --datadir=share \
    --sysconfdir=etc \
    --includedir=include \
    --default-library=static \
    --buildtype=release \
    --wrap-mode=nodownload \
    -Denable-x11=false \
    -Denable-xkbregistry=false \
    -Denable-tools=false \
    -Denable-wayland=false \
    -Denable-bash-completion=false \
    -Denable-docs=false \
    -Denable-cool-uris=false \
    -Dxkb-config-root=/nonexistent/satl-must-provide-xkb-data \
    -Dxkb-config-extra-path=/nonexistent/satl-must-provide-xkb-data-extra \
    -Dxkb-config-versioned-extensions-path=/nonexistent/satl-must-provide-xkb-data.d \
    -Dxkb-config-unversioned-extensions-path=/nonexistent/satl-must-provide-xkb-data-unversioned.d \
    -Dx-locale-root=/nonexistent/satl-must-provide-x-locale \
    -Ddefault-rules=evdev \
    -Ddefault-model=pc105 \
    -Ddefault-layout=us

# then — NOT a bare ninja, see trap 2:
#   ninja -C /home/madness/code/cxx/satellite/vendor/libxkbcommon/build-static libxkbcommon.a
#   /usr/bin/python3 .../meson.py install --no-rebuild -C .../build-static
# produces $STAGE/lib64/libxkbcommon.a  (meson.build:411 library('xkbcommon', ...))
```

**Depends on:** REQUIRED UNCONDITIONALLY (1):
  meson.build:320   bison = find_program('bison', 'win_bison', required: true, version: '>= 3.6')
                    A build tool, not a library; it generates src/xkbcomp/parser.y. `bison --version` here is 3.8.2, so the floor is met. This is the only unconditional hard requirement in the project.

REQUIRED WHEN AN OPTION WE TURN OFF IS ON:
  meson.build:532   dep_libxml = dependency('libxml-2.0')            no version floor, REQUIRED, under enable-xkbregistry (default TRUE)
  meson.build:458   xcb_dep     = dependency('xcb',     version: '>=1.10', required: false)   ...but
  meson.build:459   xcb_xkb_dep = dependency('xcb-xkb', version: '>=1.10', required: false)   ...and
  meson.build:460-463   if not xcb_dep.found() or not xcb_xkb_dep.found() / error('X11 support requires xcb-xkb >= 1.10 which was not found. ...')
                    i.e. effectively REQ

**Traps:** 1. DFLT_XKB_LEGACY_ROOT IS A SILENT RUNTIME FALLBACK TO A PATH NO OPTION CONTROLS. meson.build:59-73 sets XKB_LEGACY_ROOT from `dependency('xkeyboard-config-2')`/`dependency('xkeyboard-config')`'s `xkb_base` variable, falling back to prefix/datadir/X11/xkb. It is compiled in as DFLT_XKB_LEGACY_ROOT (meson.build:178) and src/context.c:330-337 APPENDS IT TO THE INCLUDE PATH whenever the canonical root fails to open:
       if (!has_root && root[0] != '\0') { log_warn(...); ret |= context_include_path_append(ctx, DFLT_XKB_LEGACY_ROOT); }
   Since we deliberately point the canonical root at a nonexistent path, THIS FALLBACK FIRES ON EVERY RUN where satl has not spilled its data. If configure saw a system xkeyboard-config.pc, that fallback is /usr/share/X11/xkb — and satl would then work perfectly on any machine with xkeyboard-config installed and fail only on the bare machine, which is the exact bug the nonexistent root was chosen to expose. There is NO option for it. The only defences are (a) build xkeyboard-config into the stage first so OUR .pc wins, and (b) PKG_CONFIG_LIBDIR, not PKG_CONFIG_PATH, so the system one is invisible. Verified on this machine there is no system xkeyboard-config.pc at all today (`pkg-config --list-all | grep -i xkeyboard` is empty, /usr/share/pkgconfig has none), so the fallback currently derives to $STAGE/share/X11/xkb — safe here, unsafe on the next machine.

2. THERE IS NO OPTION TO DISABLE TESTS, BENCHMARKS OR FUZZERS, AND THEY BUILD BY DEFAULT. meson_options.txt has no tests entry, and meson.build:886 (libxkbcommon-test-internal, a SECOND full

> **AUDIT [leak]** The whole `-Dxkb-config-root=/nonexistent/...` strategy is silently defeated on this machine. DFLT_XKB_LEGACY_ROOT is a SECOND baked-in root with no option controlling it, and BOTH of its derivation branches produce the identical path `$STAGE/share/X11/xkb`: the pkg-config branch reads xkeyboard-config's `xkb_base` variable (meson.build:63), which xkeyboard-config/meson.build:29 sets to `dir_xkb_b
> **FIX** For libxkbcommon's setup only, add `--datadir=share/no-legacy-xkb-root` (libxkbcommon installs nothing into datadir — headers go to includedir, the .pc to libdir/pkgconfig, and every other datadir consumer at meson.build:140/160/651 is already pinned or gated off), AND set `PKG_CONFIG_LIBDIR=/home/madness/code/cxx/satellite/vendor/stage/no-such-pkgconfig` so `dependency('xkeyboard-config-2')` at m

> **AUDIT [missing-option]** Two of the five compiled-in XKB defaults are left unpinned while the other three are pinned on the stated principle that compiled-in values must be explicit. `default-variant` and `default-options` both exist in this version's meson_options.txt and both become preprocessor macros. Their '' defaults map to the macro value NULL, which is the correct behaviour, so this is not a leak — but it leaves t
> **FIX** Add `-Ddefault-variant= -Ddefault-options=` to the setup command. The values are already correct, so this changes nothing and only records the audit.

---

## gtk

- **source** `gtk/gtk-4.24.0`
- **build system** meson — there is no choice. `ls vendor/gtk/gtk-4.24.0/` shows `meson.build` and `meson.options` and NO `configure`, NO `Makefile.am`, NO `CMakeLists.txt`. Note the options file is named `meson.options`, not `meson_options.txt` (GTK renamed it; a `cat meson_options.txt` returns 'No such file or directory').
- **installs** Installed into $STAGE/lib/pkgconfig by pkg_config.generate() at meson.build:969-1015, with exactly this backend set:
  gtk4.pc             — filebase 'gtk4', Requires: pango >= 1.58, pangocairo >= 1.58, gdk-pixbuf-2.0 >= 2.30.0, cairo >= 1.18.2, cairo-gobject >= 1.18.2, graphene-gobject-1.0 >= 1.10.
- **meson floor** meson_version : '>= 1.8.0'  (meson.build:11). Vendored meson is 1.12.0 — satisfied. ninja 1.11.1 and cmake 3.31.8 are on

**Configure:**
```sh
# --- the environment IS part of the recipe; without it the options below are decoration ---
V=/home/madness/code/cxx/satellite/vendor
STAGE=$V/stage

# stage/bin FIRST so glib-compile-resources / glib-mkenums / glib-genmarshal /
# gdbus-codegen come from the VENDORED glib 2.90.0 and not from /usr/bin's glib 2.8x
# (all four exist in /usr/bin on this machine and find_program() takes the first hit).
export PATH="$STAGE/bin:$V/build-cc:$PATH"

# PKG_CONFIG_LIBDIR, **NOT** PKG_CONFIG_PATH. PKG_CONFIG_PATH is PREPENDED to the
# default path; `pkg-config --variable pc_path pkg-config` on this machine prints
# /usr/lib64/pkgconfig:/usr/share/pkgconfig and those stay visible. Only
# PKG_CONFIG_LIBDIR REPLACES the search path. stage/share/pkgconfig is required
# because wayland-protocols installs dataonly and lands in datadir/pkgconfig.
# stage/pkgconfig-system is OUR OWN directory (not upstream, not an edit) holding
# four .pc files that no vendored project produces — see traps.
export PKG_CONFIG_LIBDIR="$STAGE/lib/pkgconfig:$STAGE/lib64/pkgconfig:$STAGE/share/pkgconfig:$STAGE/pkgconfig-system"

export CC="$V/build-cc/cc" CXX="$V/build-cc/c++"   # the -w wrapper

python3 "$V/meson/meson-1.12.0/meson.py" setup \
  "$V/gtk/build-static" "$V/gtk/gtk-4.24.0" \
  --prefix="$STAGE" \
  --libdir=lib --bindir=bin --includedir=include \
  --default-library=static \
  -Db_staticpic=true \
  --wrap-mode=nofallback \
  \
  `# WAYLAND ONLY` \
  -Dwayland-backend=true \
  -Dx11-backend=false \
  -Dbroadway-backend=false \
  -Dwin32-backend=false -Dmacos-backend=false -Dandroid-backend=false \
  \
  `# media + print` \
  -Dmedia-gstreamer=disabled \
  -Dprint-cpdb=disabled \
  -Dprint-cups=disabled \
  \
  `# optional features, every one a system .so or a missing tool` \
  -Dvulkan=disabled \
  -Dcloudproviders=disabled \
  -Dsysprof=disabled \
  -Dtracker=disabled \
  -Dcolord=disabled \
  -Daccesskit=disabled \
  -Df16c=enabled \
  \
  `# nothing that is not the library` \
  -Dintrospection=disabled \
  -Ddocumentation=false \
  -Dscreenshots=false \
  -Dman-pages=false \
  -Dbuild-demos=false \
  -Dbuild-testsuite=false \
  -Dbuild-examples=false \
  -Dbuild-tests=false \
  -Dprofile=default

# buildtype is deliberately NOT passed: upstream's default_options say
# 'buildtype=debugoptimized' and that is what the proven gtk-old round used.
# Add -Dbuildtype=release only as a considered decision — it flips debug=false,
# which adds -DG_DISABLE_CAST_CHECKS -DG_DISABLE_ASSERT to every GTK object
# (meson.build:88-99). Size is not a reason here (12 GB ceiling, never strip).
```

**Depends on:** VERSION FLOORS, all declared together at meson.build:15-36 and consumed at :451-500:
  glib_req = '>= 2.89.3'   → REQUIRED, four times: dependency('glib-2.0', version: glib_req) :451; dependency('gio-2.0', version: glib_req) :452; dependency('gobject-2.0', version: glib_req) :454; dependency('gmodule-2.0', version: glib_req) :461. Vendored glib is 2.90.0 ✓
  glib, unix extra: dependency('gio-unix-2.0', version: glib_req, required: false) :459 — OPTIONAL but sets HAVE_GIO_UNIX and is platform_gio_dep in gdk_deps; you want it, so glib must be built with its unix gio.
  pango_req = '>= @0@.@1@'.format(1, 58) i.e. '>= 1.58'  → REQUIRED: dependency('pango', version: pango_req) :462; dependency('pangocairo', version: pango_req) :481; dependency('pangoft2', version: pango_req, required: wayland_enabled or x11_enabled) :473 — REQUIRED for us because wayland is on. Vendored pango 1.58.2 ✓
  cairo

**Traps:** 1. **GTK DOES NOT INSTALL A STATIC LIBRARY, AND --default-library=static DOES NOT CHANGE THAT.** gtk/meson.build:1074 reads `libgtk = shared_library('gtk-4', ... install: true)`. `shared_library()` is explicit and meson's default_library option only governs `library()` and `both_libraries()`. `grep -rn shared_library --include=meson.build .` over the whole tree returns exactly ONE hit — that line. `libgtk_static = static_library('gtk', ...)` at :1065 has no `install:` key, so it defaults to false. Consequence: `ninja install` puts `$STAGE/lib/libgtk-4.so.1.2400.0` in the prefix and NO libgtk-4.a, and all four .pc files say -lgtk-4. This is the same in 4.16.7 (gtk-old/gtk-4.16.7/gtk/meson.build:1124) and is why the previous round never installed GTK at all: make_support/047-window.mk points GTK_BUILD at `vendor/gtk-old/build-static` and links the archives out of the BUILD TREE. Two ways forward, and this is a decision for the author, not for me: (a) keep doing what works — link the six build-tree archives $V/gtk/build-static/{gtk/libgtk.a, gtk/css/libgtk_css.a, gtk/svg/libgtk_svg.a, gdk/libgdk.a, gsk/libgsk.a, gsk/libgsk_f16c.a} in a --start-group with libgtk.a first, and take cflags from build-static/meson-uninstalled; or (b) a one-line journalled edit turning :1074 into `library('gtk-4', ...)`, which vendor/README_FIRST.md permits only with an EDITS.md entry written the same sitting. NOTE for (a): libgdk.a already CONTAINS the wayland backend, because gdk/meson.build:302 uses `link_whole: gdk_backends` — but libgtk.a uses `link_with:` (:1071), which does NOT merge, so all 

---
