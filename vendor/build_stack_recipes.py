"""vendor/build_stack_recipes.py -- the 24 configure/build/install recipes, as data.

THE REGISTRY EXCEPTION TO THE LINE RULE. This file is a table, not logic: one entry
per vendored project, in build order. vendor/build_stack.py is the engine that runs it.

Every recipe here comes from vendor/BUILD_RECIPES.md, which was derived by READING each
project's build files. Where this file differs from that one, the difference is recorded
in the entry's `deviations` field and nowhere else -- so a future reader can diff the two
documents and find every change with its reason attached.

NOTHING HERE HAS BEEN RUN. Expect the first pass to find defects; correct both files.
"""

from dataclasses import dataclass, field


@dataclass
class Step:
    """One project. `commands` run in order; the first failure stops the project."""

    name: str
    src: str                                  # under vendor/, the unpacked tree
    build: str                                # under vendor/, the build directory
    commands: list = field(default_factory=list)      # list of (label, argv)
    env: dict = field(default_factory=dict)           # added to the base environment
    pc_dirs: list = None                              # overrides PKG_CONFIG_LIBDIR
    requires: list = field(default_factory=list)      # stage-relative, must exist BEFORE
    provides: list = field(default_factory=list)      # stage-relative, must exist AFTER
    tree_provides: list = field(default_factory=list) # build-dir-relative, must exist AFTER
    modversions: dict = field(default_factory=dict)   # pkg-config module -> expected version
    why: str = ""
    deviations: str = ""


def steps(c):
    """Build the ordered recipe list. `c` is the Context from build_stack.py."""

    S, V = c.stage, c.vendor
    meson = c.meson_setup            # (builddir, srcdir, *args) -> argv
    ninja = c.ninja                  # (builddir, *targets)      -> argv
    install = c.meson_install        # (builddir, *args)         -> argv
    cc, cxx = c.cc, c.cxx

    # ---------------------------------------------------------------- the leaves

    zlib = Step(
        name="zlib", src="zlib/zlib-1.3.2", build="build/zlib",
        why="The leaf of the whole stack: glib, libpng, libtiff, cairo and freetype all "
            "ask for it, and meson's zlib factory falls through to a bare cc.find_library "
            "('z') that would take /usr's zlib-ng and print 'found: YES'.",
        env={"CC": cc, "CFLAGS": "-O2 -fPIC"},
        commands=[
            ("configure", [f"{V}/zlib/zlib-1.3.2/configure", "--static",
                           f"--prefix={S}", f"--eprefix={S}", f"--libdir={S}/lib64",
                           f"--sharedlibdir={S}/lib64", f"--includedir={S}/include",
                           f"--mandir={S}/share/man"]),
            # NOT a bare `make`: that also builds the shared object we did not ask for.
            ("make libz.a", ["make", "-j", str(c.jobs), "libz.a"]),
            ("make install", ["make", "install"]),
        ],
        provides=["lib64/libz.a", "lib64/pkgconfig/zlib.pc"],
        modversions={"zlib": "1.3.2"},
    )

    libffi = Step(
        name="libffi", src="libffi/libffi-3.8.0", build="build/libffi",
        why="glib's only hard dependency besides zlib and pcre2.",
        env={"CC": cc, "CXX": cxx, "CFLAGS": "-O2 -fPIC", "CXXFLAGS": "-O2 -fPIC",
             "CCASFLAGS": "-O2 -fPIC"},
        commands=[
            ("configure", [f"{V}/libffi/libffi-3.8.0/configure",
                           f"--prefix={S}", f"--libdir={S}/lib64",
                           f"--includedir={S}/include", f"--mandir={S}/share/man",
                           "--disable-shared", "--enable-static", "--with-pic",
                           # collapses toolexeclibdir onto libdir: libffi.pc's
                           # `Libs: -L${toolexeclibdir}` otherwise names a directory
                           # with no libffi.a in it, and glib's link is what fails.
                           "--disable-multi-os-directory",
                           "--disable-docs", "--disable-builddir",
                           "--disable-dependency-tracking", "--enable-portable-binary"]),
            ("make", ["make", "-j", str(c.jobs)]),
            ("make install", ["make", "install"]),
        ],
        provides=["lib64/libffi.a", "lib64/pkgconfig/libffi.pc"],
        modversions={"libffi": "3.8.0"},
    )

    expat = Step(
        name="expat", src="expat/expat-2.8.4", build="build/expat",
        why="fontconfig's XML backend. Without it in the stage, fontconfig falls to "
            "cc.find_library('expat') -> /usr, then to the system libxml2.",
        env={"CC": cc, "CXX": cxx, "CFLAGS": "-O2 -fPIC"},
        commands=[
            ("configure", [f"{V}/expat/expat-2.8.4/configure",
                           f"--prefix={S}", f"--libdir={S}/lib",
                           "--disable-shared", "--enable-static", "--enable-pic",
                           "--without-xmlwf", "--without-examples", "--without-tests",
                           "--without-docbook", "--disable-symbol-versioning",
                           "--disable-maintainer-mode",
                           "--disable-dependency-tracking", "--disable-silent-rules"]),
            ("make", ["make", "-j", str(c.jobs)]),
            ("make install", ["make", "install"]),
        ],
        provides=["lib/libexpat.a", "lib/pkgconfig/expat.pc"],
        modversions={"expat": "2.8.4"},
        deviations="+ --disable-maintainer-mode (AUDIT: expat defaults maintainer mode ON, "
                   "whose rebuild rules write INTO the upstream tree -- an unjournalled edit). "
                   "- CFLAGS=-fvisibility=default (AUDIT: expat's only consumer here is "
                   "fontconfig and both land in satl's one link unit, so upstream's hidden "
                   "visibility is protection we should keep).",
    )

    pcre2 = Step(
        name="pcre2", src="pcre2/pcre2-10.48", build="build/pcre2",
        why="glib's regex engine; glib asserts the fallback is internal if it cannot "
            "find libpcre2-8, and --wrap-mode=nofallback turns that into a loud failure.",
        env={"CC": cc, "CFLAGS": "-O2 -fPIC"},
        commands=[
            ("configure", [f"{V}/pcre2/pcre2-10.48/configure",
                           f"--prefix={S}", f"--libdir={S}/lib",
                           "--disable-shared", "--enable-static", "--with-pic",
                           "--enable-pcre2-8", "--disable-pcre2-16", "--disable-pcre2-32",
                           "--enable-unicode", "--enable-jit",
                           "--disable-pcre2grep-libz", "--disable-pcre2grep-libbz2",
                           "--disable-pcre2test-libedit", "--disable-pcre2test-libreadline",
                           "--disable-valgrind", "--disable-coverage", "--disable-debug",
                           "--disable-fuzz-support", "--disable-diff-fuzz-support",
                           "--disable-rebuild-chartables", "--disable-symvers",
                           "--disable-dependency-tracking", "--disable-silent-rules"]),
            ("make", ["make", "-j", str(c.jobs)]),
            ("make install", ["make", "install"]),
        ],
        provides=["lib/libpcre2-8.a", "lib/pkgconfig/libpcre2-8.pc"],
        modversions={"libpcre2-8": "10.48"},
    )

    gperf = Step(
        name="gperf", src="gperf/gperf-3.3", build="build/gperf",
        why="Installs NO library and NO .pc -- just $STAGE/bin/gperf, found through PATH. "
            "fontconfig hard-requires it to generate fcobjshash.h and there is none on "
            "this machine, so fontconfig cannot even configure until this is staged.",
        env={"CC": cc, "CXX": cxx, "CFLAGS": "-O2", "CXXFLAGS": "-O2"},
        commands=[
            ("configure", [f"{V}/gperf/gperf-3.3/configure",
                           f"--prefix={S}", f"--bindir={S}/bin",
                           f"--infodir={S}/share/info", f"--mandir={S}/share/man",
                           f"--docdir={S}/share/doc/gperf", f"--htmldir={S}/share/doc/gperf"]),
            ("make", ["make", "-j", str(c.jobs)]),
            ("make install", ["make", "install"]),
        ],
        provides=["bin/gperf"],
        deviations="- --disable-dependency-tracking (AUDIT: the hand-rolled top-level "
                   "configure does not recognise it; the sub-configures do, but it only "
                   "earns a warning either way).",
    )

    fribidi = Step(
        name="fribidi", src="fribidi/fribidi-1.0.17", build="build/fribidi",
        why="pango's bidi engine, and a true leaf. The previous round pinned it to "
            "`revision = master`; this one is a release tarball.",
        commands=[
            ("meson setup", meson("build/fribidi", "fribidi/fribidi-1.0.17",
                                  "--libdir=lib", "--includedir=include", "--bindir=bin",
                                  "-Ddocs=false", "-Dbin=false", "-Dtests=false",
                                  "-Ddeprecated=true")),
            ("ninja", ninja("build/fribidi")),
            ("install", install("build/fribidi")),
        ],
        provides=["lib/libfribidi.a", "lib/pkgconfig/fribidi.pc"],
        modversions={"fribidi": "1.0.17"},
    )

    pixman = Step(
        name="pixman", src="pixman/pixman-0.46.4", build="build/pixman",
        why="cairo's only REQUIRED dependency. A true leaf.",
        commands=[
            ("meson setup", meson("build/pixman", "pixman/pixman-0.46.4",
                                  "--libdir=lib",
                                  "-Dtests=disabled", "-Ddemos=disabled", "-Dgtk=disabled",
                                  # load-bearing: the libpng block is a cc.find_library
                                  # loop over six sonames, which asks the LINKER and so
                                  # ignores PKG_CONFIG_LIBDIR entirely.
                                  "-Dlibpng=disabled",
                                  "-Dopenmp=disabled", "-Dtimers=false", "-Dgnuplot=false",
                                  "-Dmmx=enabled", "-Dsse2=enabled", "-Dssse3=enabled",
                                  "-Dgnu-inline-asm=enabled", "-Dtls=enabled",
                                  "-Dloongson-mmi=disabled", "-Dvmx=disabled",
                                  "-Darm-simd=disabled", "-Dneon=disabled",
                                  "-Da64-neon=disabled", "-Dmips-dspr2=disabled",
                                  "-Drvv=disabled")),
            ("ninja", ninja("build/pixman")),
            ("install", install("build/pixman")),
        ],
        provides=["lib/libpixman-1.a", "lib/pkgconfig/pixman-1.pc"],
        modversions={"pixman-1": "0.46.4"},
    )

    wayland_protocols = Step(
        name="wayland-protocols", src="wayland-protocols/wayland-protocols-1.49",
        build="build/wayland-protocols",
        why="Pure XML data, no library, no compiler. Its .pc lands in share/pkgconfig -- "
            "and /usr/share/pkgconfig already holds one, so ours is only found because "
            "PKG_CONFIG_LIBDIR names stage/share/pkgconfig and hides /usr's.",
        commands=[
            ("meson setup", meson("build/wayland-protocols",
                                  "wayland-protocols/wayland-protocols-1.49",
                                  "--libdir=lib64", "--datadir=share", "--includedir=include",
                                  "-Dtests=false")),
            ("ninja", ninja("build/wayland-protocols")),
            ("install", install("build/wayland-protocols")),
        ],
        provides=["share/pkgconfig/wayland-protocols.pc",
                  "share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"],
        modversions={"wayland-protocols": "1.49"},
    )

    xkeyboard_config = Step(
        name="xkeyboard-config", src="xkeyboard-config/xkeyboard-config-2.48",
        build="build/xkeyboard-config",
        why="The keyboard database satl must spill and point XKB_CONFIG_ROOT at. "
            "Replaces vendor/xkb/xkb-data, which was copied off this machine's /usr/share "
            "and is provenance nobody should accept.",
        commands=[
            ("meson setup", meson("build/xkeyboard-config",
                                  "xkeyboard-config/xkeyboard-config-2.48",
                                  "--libdir=lib64", "--datadir=share", "--sysconfdir=etc",
                                  "--mandir=share/man",
                                  "-Dnls=false", "-Dcompat-rules=true",
                                  "-Dxorg-rules-symlinks=false",
                                  "-Dnon-latin-layouts-list=false")),
            ("ninja", ninja("build/xkeyboard-config")),
            ("install", install("build/xkeyboard-config")),
        ],
        provides=["share/pkgconfig/xkeyboard-config-2.pc",
                  # generated by rules/generator through the literal string 'python3',
                  # so this file existing is also the proof that PyPy did not run it
                  "share/xkeyboard-config-2/rules/evdev",
                  "share/xkeyboard-config-2/rules/evdev.lst",
                  "share/xkeyboard-config-2/symbols/us"],
        modversions={"xkeyboard-config-2": "2.48"},
        deviations="Needs perl (xml2lst.pl, required:true) and CPython >= 3.11; "
                   "vendor/build-tools-path puts /usr/bin/python3 first so the generators "
                   "do not run under the alpha PyPy that leads this machine's PATH. "
                   "ACCEPTED, not fixed: xsltproc is present, so two man pages install "
                   "into the stage and no option prevents it.",
    )

    # ------------------------------------------------- image codecs and font engine

    libpng = Step(
        name="libpng", src="libpng/libpng-1.6.58", build="build/libpng",
        why="freetype, cairo and gdk-pixbuf all ask for it by the module name `libpng`.",
        requires=["lib64/pkgconfig/zlib.pc"],
        env={"CC": cc, "CXX": cxx, "CFLAGS": "-O2 -fPIC",
             "CPPFLAGS": f"-I{S}/include",
             "LDFLAGS": f"-L{S}/lib64 -L{S}/lib"},
        commands=[
            ("configure", [f"{V}/libpng/libpng-1.6.58/configure",
                           f"--prefix={S}", f"--libdir={S}/lib64",
                           "--disable-shared", "--enable-static", "--with-pic",
                           "--disable-dependency-tracking", "--disable-tools",
                           "--disable-tests", "--disable-werror",
                           "--enable-hardware-optimizations=yes",
                           "--enable-unversioned-links", "--enable-unversioned-libpng-pc",
                           "--without-binconfigs", "--disable-unversioned-libpng-config"]),
            ("make", ["make", "-j", str(c.jobs)]),
            ("make install", ["make", "install"]),
        ],
        provides=["lib64/libpng16.a", "lib64/pkgconfig/libpng16.pc",
                  "lib64/pkgconfig/libpng.pc"],
        modversions={"libpng": "1.6.58"},
        deviations="+ --disable-unversioned-libpng-config (AUDIT: --without-binconfigs "
                   "empties bin_SCRIPTS but leaves DO_INSTALL_LIBPNG_CONFIG true). "
                   "NOTE: there is no option to pin WHICH zlib is used -- "
                   "AC_CHECK_LIB([z],[zlibVersion]) would take /usr's zlib-ng silently. "
                   "Build order is the only defence, hence `requires` above.",
    )

    libjpeg = Step(
        name="libjpeg-turbo", src="libjpeg-turbo/libjpeg-turbo-3.2.0",
        build="build/libjpeg-turbo",
        why="libtiff's JPEG codec and gdk-pixbuf's jpeg loader.",
        commands=[
            ("cmake", ["cmake", "-G", "Ninja",
                       f"-DCMAKE_C_COMPILER={cc}",
                       f"-DCMAKE_INSTALL_PREFIX={S}", "-DCMAKE_INSTALL_LIBDIR=lib64",
                       "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_POSITION_INDEPENDENT_CODE=1",
                       # the trio below is what stops it compiling a SECOND, bundled zlib
                       # into libturbojpeg and never saying so.
                       "-DWITH_TURBOJPEG=0", "-DWITH_TOOLS=0", "-DWITH_TESTS=0",
                       "-DWITH_FUZZ=0", "-DWITH_JNA=", "-DWITH_PROFILE=0",
                       "-DWITH_JPEG7=0", "-DWITH_JPEG8=0",
                       "-DWITH_ARITH_DEC=1", "-DWITH_ARITH_ENC=1",
                       "-DWITH_SIMD=1", "-DREQUIRE_SIMD=1",
                       "-DCMAKE_ASM_NASM_COMPILER=/usr/local/bin/nasm",
                       "-S", f"{V}/libjpeg-turbo/libjpeg-turbo-3.2.0",
                       "-B", f"{V}/build/libjpeg-turbo"]),
            ("ninja", ninja("build/libjpeg-turbo")),
            ("install lib", ["cmake", "--install", f"{V}/build/libjpeg-turbo",
                             "--component", "lib"]),
            ("install include", ["cmake", "--install", f"{V}/build/libjpeg-turbo",
                                 "--component", "include"]),
        ],
        provides=["lib64/libjpeg.a", "lib64/pkgconfig/libjpeg.pc", "include/jpeglib.h"],
        modversions={"libjpeg": "3.2.0"},
        deviations="Installed BY COMPONENT rather than wholesale (AUDIT: the top-level "
                   "install(FILES ...) at CMakeLists.txt:2016 is unguarded and drops eight "
                   "doc files into the stage). NASM here is 3.02rc13 from /usr/local -- a "
                   "release candidate, not what 3.2.0 was tested against. If a JPEG ever "
                   "decodes wrong, reconfigure with -DWITH_SIMD=0 before looking anywhere else.",
    )

    libtiff = Step(
        name="libtiff", src="libtiff/tiff-4.7.2", build="build/libtiff",
        why="gdk-pixbuf's tiff loader. Every codec it has is ON by default and found with "
            "AC_CHECK_LIB against the compiler's own search path -- lzma, zstd and webp "
            "are all installed here, so a plain ./configure links three system .so files "
            "and writes them into libtiff-4.pc.",
        requires=["lib64/pkgconfig/zlib.pc", "lib64/pkgconfig/libjpeg.pc"],
        env={"CC": cc, "CXX": cxx, "CFLAGS": "-O2 -fPIC",
             "CPPFLAGS": f"-I{S}/include",
             "LDFLAGS": f"-L{S}/lib64 -L{S}/lib"},
        commands=[
            ("configure", [f"{V}/libtiff/tiff-4.7.2/configure",
                           f"--prefix={S}", f"--libdir={S}/lib64",
                           "--disable-shared", "--enable-static", "--with-pic",
                           "--disable-dependency-tracking", "--disable-rpath",
                           "--disable-tools", "--disable-tests", "--disable-contrib",
                           "--disable-docs", "--disable-sphinx", "--disable-cxx",
                           "--enable-zlib",
                           f"--with-zlib-include-dir={S}/include",
                           f"--with-zlib-lib-dir={S}/lib64",
                           "--enable-jpeg",
                           f"--with-jpeg-include-dir={S}/include",
                           f"--with-jpeg-lib-dir={S}/lib64",
                           "--disable-libdeflate", "--disable-jbig", "--disable-lerc",
                           "--disable-lzma", "--disable-zstd", "--disable-webp",
                           "--disable-jpeg12", "--disable-opengl", "--without-x"]),
            ("make", ["make", "-j", str(c.jobs)]),
            ("make install", ["make", "install"]),
        ],
        provides=["lib64/libtiff.a", "lib64/pkgconfig/libtiff-4.pc"],
        modversions={"libtiff-4": "4.7.2"},
        deviations="- --disable-deprecated (AUDIT: upstream wired the option BACKWARDS -- "
                   "configure.ac:209-212 defaults it off and it is --ENABLE-deprecated that "
                   "compiles the deprecated API OUT, which gdk-pixbuf's loader links against).",
    )

    def freetype(pass_no, harfbuzz):
        return Step(
            name=f"freetype-{pass_no}",
            src="freetype/freetype-2.14.3", build=f"build/freetype-{pass_no}",
            why=("PASS 1, built WITHOUT harfbuzz to break the cycle: harfbuzz wants "
                 "freetype2, freetype wants harfbuzz. The previous round gave up hinting "
                 "permanently at this point; bottom-up makes the second pass free."
                 if pass_no == 1 else
                 "PASS 2, the same source rebuilt WITH harfbuzz now that harfbuzz exists, "
                 "which is the auto-hinter quality the old build permanently lost. It "
                 "reinstalls freetype2.pc with harfbuzz in Requires.private."),
            requires=(["lib64/pkgconfig/zlib.pc", "lib64/pkgconfig/libpng.pc"] +
                      ([] if pass_no == 1 else ["lib/pkgconfig/harfbuzz.pc"])),
            commands=[
                ("meson setup", meson(f"build/freetype-{pass_no}", "freetype/freetype-2.14.3",
                                      "--libdir=lib", "--bindir=bin", "--includedir=include",
                                      "-Dzlib=system", "-Dpng=enabled",
                                      # both are cc.find_library, i.e. outside pkg-config's
                                      # reach: /usr/lib64/libbz2.so and libbrotlidec exist.
                                      "-Dbzip2=disabled", "-Dbrotli=disabled",
                                      f"-Dharfbuzz={harfbuzz}",
                                      "-Dmmap=enabled", "-Dtests=disabled",
                                      "-Derror_strings=false")),
                ("ninja", ninja(f"build/freetype-{pass_no}")),
                ("install", install(f"build/freetype-{pass_no}")),
            ],
            provides=["lib/libfreetype.a", "lib/pkgconfig/freetype2.pc"],
            deviations=("-Dharfbuzz=disabled is NOT the default: `auto` falls through to a "
                        "`dynamic` leg that defines FT_CONFIG_OPTION_USE_HARFBUZZ_DYNAMIC "
                        "and dlopens harfbuzz at run time -- invisible to readelf."
                        if pass_no == 1 else
                        "Second pass over the same source into a separate build dir; the "
                        "install overwrites pass 1's."),
        )

    # ------------------------------------------------------------- the glib layer

    glib = Step(
        name="glib", src="glib/glib-2.90.0", build="build/glib",
        why="Everything above here needs it, and it also installs the code generators "
            "(glib-compile-resources, glib-mkenums, glib-genmarshal, gdbus-codegen) that "
            "GTK runs at build time -- stage/bin must lead PATH or /usr/bin's older "
            "copies are used instead and nothing warns.",
        requires=["lib64/pkgconfig/zlib.pc", "lib64/pkgconfig/libffi.pc",
                  "lib/pkgconfig/libpcre2-8.pc"],
        commands=[
            ("meson setup", meson("build/glib", "glib/glib-2.90.0",
                                  "--libdir=lib", "--auto-features=disabled",
                                  # libmount and libselinux ARE installed here, have no
                                  # wrap, and were the two the previous round's reading
                                  # missed: glib links them and the binary comes out
                                  # "static" with two dynamic dependencies inside it.
                                  "-Dselinux=disabled", "-Dlibmount=disabled",
                                  "-Dlibelf=disabled", "-Dsysprof=disabled",
                                  "-Ddtrace=disabled", "-Dsystemtap=disabled",
                                  "-Dintrospection=disabled", "-Dman-pages=disabled",
                                  "-Ddocumentation=false", "-Dnls=disabled",
                                  "-Dtests=false", "-Dinstalled_tests=false",
                                  "-Doss_fuzz=disabled", "-Dxattr=true",
                                  "-Dfile_monitor_backend=inotify",
                                  "-Dglib_debug=disabled", "-Dglib_assert=true",
                                  "-Dglib_checks=true", "-Dbsymbolic_functions=false",
                                  "-Dmultiarch=false")),
            ("ninja", ninja("build/glib")),
            ("install", install("build/glib")),
        ],
        provides=["lib/libglib-2.0.a", "lib/libgobject-2.0.a", "lib/libgio-2.0.a",
                  "lib/pkgconfig/glib-2.0.pc", "lib/pkgconfig/gobject-2.0.pc",
                  "lib/pkgconfig/gio-2.0.pc", "lib/pkgconfig/gio-unix-2.0.pc",
                  "lib/pkgconfig/gmodule-no-export-2.0.pc",
                  "bin/glib-compile-resources", "bin/glib-mkenums",
                  "bin/glib-genmarshal", "bin/gdbus-codegen"],
        modversions={"glib-2.0": "2.90.0"},
        deviations="AUDIT, no option exists for either, so they are recorded not fixed: "
                   "(1) gio compiles in xdgmime unconditionally and reads the TARGET "
                   "machine's shared-mime-info at run time -- g_content_type_guess() "
                   "degrades to extension-only on a bare machine. (2) meson's iconv factory "
                   "must resolve as `builtin` (glibc), not `system`; the log audit checks it.",
    )

    graphene = Step(
        name="graphene", src="graphene/graphene-1.10.8", build="build/graphene",
        why="GTK's vector/matrix maths. The trap is that graphene asks for gobject-2.0 "
            "with required:false -- with no gobject in the stage it configures fine, never "
            "generates graphene-gobject-1.0.pc, and GTK fails much later asking for it.",
        requires=["lib/pkgconfig/gobject-2.0.pc"],
        commands=[
            ("meson setup", meson("build/graphene", "graphene/graphene-1.10.8",
                                  "--libdir=lib", "--auto-features=disabled",
                                  "-Dgobject_types=true", "-Dintrospection=disabled",
                                  "-Dgtk_doc=false", "-Dtests=false",
                                  # defaults to TRUE in this project, unlike the rest
                                  "-Dinstalled_tests=false",
                                  "-Dsse2=true", "-Dgcc_vector=true", "-Darm_neon=false")),
            ("ninja", ninja("build/graphene")),
            ("install", install("build/graphene")),
        ],
        provides=["lib/libgraphene-1.0.a", "lib/pkgconfig/graphene-1.0.pc",
                  "lib/pkgconfig/graphene-gobject-1.0.pc"],
        modversions={"graphene-gobject-1.0": "1.10.8"},
    )

    fontconfig = Step(
        name="fontconfig", src="fontconfig/fontconfig-2.18.3", build="build/fontconfig",
        why="cairo and pango both need it. It is also where the vendored gperf earns its "
            "place: without $STAGE/bin on PATH, meson takes /usr/bin/gperf if one exists "
            "and the build succeeds with somebody else's tool.",
        requires=["lib/pkgconfig/freetype2.pc", "lib/pkgconfig/expat.pc", "bin/gperf"],
        commands=[
            ("meson setup", meson("build/fontconfig", "fontconfig/fontconfig-2.18.3",
                                  "--libdir=lib", "--bindir=bin", "--includedir=include",
                                  "--sysconfdir=etc", "--localstatedir=var", "--datadir=share",
                                  "-Dxml-backend=expat",
                                  # load-bearing, not cosmetic: a disabled feature passed as
                                  # `required:` makes meson skip the search AND the fallback,
                                  # which is what keeps subprojects/libintl/ out of the build.
                                  "-Dnls=disabled",
                                  "-Diconv=disabled", "-Dfontations=disabled",
                                  "-Ddoc=disabled", "-Ddoc-txt=disabled", "-Ddoc-man=disabled",
                                  "-Ddoc-pdf=disabled", "-Ddoc-html=disabled",
                                  "-Dtests=disabled", "-Dtests-bwrap=disabled",
                                  "-Dtests-external-fonts=disabled", "-Dtools=disabled",
                                  "-Dcache-build=disabled",
                                  "-Ddefault-hinting=slight",
                                  "-Ddefault-sub-pixel-rendering=none",
                                  "-Dbitmap-conf=no-except-emoji",
                                  "-Dadditional-fonts-dirs=no", "-Ddefault-fonts-dirs=yes")),
            ("ninja", ninja("build/fontconfig")),
            ("install", install("build/fontconfig")),
        ],
        provides=["lib/libfontconfig.a", "lib/pkgconfig/fontconfig.pc",
                  "etc/fonts/fonts.conf"],
        modversions={"fontconfig": "2.18.3"},
        deviations="OPEN DECISION, left at the recipe's defaults on purpose: the five "
                   "*-dir options (baseconfig-dir, config-dir, template-dir, xml-dir, "
                   "cache-dir) bake THIS checkout's absolute paths into libfontconfig.a. "
                   "That is wrong for a shipped binary, and satl is expected to set "
                   "FONTCONFIG_FILE at start-up instead. It is cheap to change later -- "
                   "only fontconfig and the final satl link would be redone, because a "
                   "static archive does not absorb its dependencies.",
    )

    harfbuzz = Step(
        name="harfbuzz", src="harfbuzz/harfbuzz-14.4.0", build="build/harfbuzz",
        why="pango's shaper, and freetype's auto-hinter on the second pass.",
        requires=["lib/pkgconfig/glib-2.0.pc", "lib/pkgconfig/freetype2.pc"],
        commands=[
            ("meson setup", meson("build/harfbuzz", "harfbuzz/harfbuzz-14.4.0",
                                  "--libdir=lib", "--includedir=include", "--bindir=bin",
                                  "--auto-features=disabled",
                                  # harfbuzz's own escape hatch. hb.hh:64 promotes ~30
                                  # warnings to errors with `#pragma GCC diagnostic error`,
                                  # which NO command-line flag reaches -- not even the -w
                                  # wrapper. Carried from the 2026-09-19 build, which is
                                  # where it was found, and BUILD_RECIPES.md omits it.
                                  "-Dcpp_args=-DHB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR",
                                  "-Dglib=enabled", "-Dfreetype=enabled", "-Dsubset=enabled",
                                  "-Dgobject=disabled", "-Dicu=disabled",
                                  "-Dicu_builtin=false",
                                  # cairo is built AFTER harfbuzz, so `auto` here could
                                  # only ever find /usr's.
                                  "-Dcairo=disabled",
                                  "-Dchafa=disabled", "-Dpng=disabled", "-Dzlib=disabled",
                                  "-Dgraphite=disabled", "-Dgraphite2=disabled",
                                  "-Dwasm=disabled", "-Dfontations=disabled",
                                  "-Dharfrust=disabled", "-Dkbts=disabled", "-Dgdi=disabled",
                                  "-Ddirectwrite=disabled", "-Dcoretext=disabled",
                                  "-Draster=disabled", "-Dvector=disabled", "-Dgpu=disabled",
                                  "-Dgpu_demo=disabled", "-Dutilities=disabled",
                                  "-Dtests=disabled", "-Dbenchmark=disabled",
                                  "-Dintrospection=disabled", "-Ddocs=disabled",
                                  "-Ddoc_tests=false", "-Dexperimental_api=false",
                                  "-Dwith_libstdcxx=false", "-Dragel_subproject=false")),
            ("ninja", ninja("build/harfbuzz")),
            ("install", install("build/harfbuzz")),
        ],
        provides=["lib/libharfbuzz.a", "lib/libharfbuzz-subset.a",
                  "lib/pkgconfig/harfbuzz.pc", "lib/pkgconfig/harfbuzz-subset.pc"],
        modversions={"harfbuzz": "14.4.0"},
        deviations="+ -Dcpp_args=-DHB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR, from the 2026-09-19 "
                   "round and absent from BUILD_RECIPES.md. harfbuzz is the one project "
                   "whose own source defeats the -w wrapper.",
    )

    cairo = Step(
        name="cairo", src="cairo/cairo-1.18.4", build="build/cairo",
        why="NOT a leaf, despite pixman being one: GTK reads cairo-gobject.pc unguarded, "
            "and cairo-gobject exists only if glib AND gobject were found when cairo was "
            "configured. So cairo comes after glib, not beside pixman.",
        requires=["lib/pkgconfig/pixman-1.pc", "lib64/pkgconfig/libpng.pc",
                  "lib64/pkgconfig/zlib.pc", "lib/pkgconfig/freetype2.pc",
                  "lib/pkgconfig/fontconfig.pc", "lib/pkgconfig/gobject-2.0.pc"],
        commands=[
            ("meson setup", meson("build/cairo", "cairo/cairo-1.18.4",
                                  "--libdir=lib",
                                  "-Dpng=enabled",
                                  # keep zlib ON: it is what sets CAIRO_HAS_INTERPRETER and
                                  # so installs cairo-script-interpreter.pc. Without that
                                  # file GTK falls to cc.find_library('cairo-script-
                                  # interpreter'), which would find /usr's .so.
                                  "-Dzlib=enabled",
                                  "-Dfreetype=enabled", "-Dfontconfig=enabled",
                                  "-Dglib=enabled", "-Dtee=disabled", "-Dtests=disabled",
                                  "-Dlzo=disabled", "-Dspectre=disabled",
                                  # cc.find_library('bfd') -> binutils-devel, invisible to
                                  # pkg-config isolation; only this option stops it.
                                  "-Dsymbol-lookup=disabled",
                                  "-Dgtk2-utils=disabled",
                                  # GTK's -Dx11-backend=false does NOT stop cairo turning on
                                  # its own X backends: x11, xcb, xcb-render, xcb-shm, xext,
                                  # xrender all resolved from /usr last time with no warning.
                                  "-Dxlib=disabled", "-Dxcb=disabled", "-Dxlib-xcb=disabled",
                                  "-Dquartz=disabled", "-Ddwrite=disabled",
                                  "-Dgtk_doc=false")),
            ("ninja", ninja("build/cairo")),
            ("install", install("build/cairo")),
        ],
        provides=["lib/libcairo.a", "lib/pkgconfig/cairo.pc",
                  "lib/pkgconfig/cairo-gobject.pc",
                  "lib/pkgconfig/cairo-script-interpreter.pc"],
        modversions={"cairo": "1.18.4"},
    )

    pango = Step(
        name="pango", src="pango/pango-1.58.2", build="build/pango",
        why="The first target that links the whole lower stack: pango-view, pango-list and "
            "pango-segmentation are built and INSTALLED unconditionally, with no option to "
            "skip them. A failure in those three is a missing Libs.private further down, "
            "not a pango bug.",
        requires=["lib/pkgconfig/glib-2.0.pc", "lib/pkgconfig/fribidi.pc",
                  "lib/pkgconfig/harfbuzz.pc", "lib/pkgconfig/cairo.pc",
                  "lib/pkgconfig/fontconfig.pc", "lib/pkgconfig/freetype2.pc"],
        commands=[
            ("meson setup", meson("build/pango", "pango/pango-1.58.2",
                                  "--libdir=lib",
                                  "-Dcairo=enabled", "-Dfontconfig=enabled",
                                  # NOTE: -Dfreetype is a NO-OP on Linux -- pango overwrites
                                  # the option whenever fontconfig is found. A missing
                                  # freetype2.pc shows up as error('No Cairo font backends
                                  # found') several hundred lines later.
                                  "-Dfreetype=enabled",
                                  "-Dxft=disabled",
                                  # break-thai.c calls th_brk_new/th_uni2tis; undefined at
                                  # link time, and Thai word-breaking needs a dictionary on
                                  # the target anyway, so it could never have worked here.
                                  "-Dlibthai=disabled",
                                  "-Dsysprof=disabled", "-Dintrospection=disabled",
                                  "-Ddocumentation=false", "-Dman-pages=false",
                                  "-Dbuild-testsuite=false", "-Dbuild-examples=false")),
            ("ninja", ninja("build/pango")),
            ("install", install("build/pango")),
        ],
        provides=["lib/libpango-1.0.a", "lib/libpangocairo-1.0.a", "lib/libpangoft2-1.0.a",
                  "lib/pkgconfig/pango.pc", "lib/pkgconfig/pangocairo.pc",
                  "lib/pkgconfig/pangoft2.pc"],
        modversions={"pango": "1.58.2"},
    )

    gdk_pixbuf = Step(
        name="gdk-pixbuf", src="gdk-pixbuf/gdk-pixbuf-2.44.8", build="build/gdk-pixbuf",
        why="Two options here are the whole reason libjpeg-turbo and libtiff are in this "
            "build at all, and both defaults are wrong for a self-contained binary.",
        requires=["lib/pkgconfig/gmodule-no-export-2.0.pc", "lib64/pkgconfig/libpng.pc",
                  "lib64/pkgconfig/libjpeg.pc", "lib64/pkgconfig/libtiff-4.pc"],
        commands=[
            ("meson setup", meson("build/gdk-pixbuf", "gdk-pixbuf/gdk-pixbuf-2.44.8",
                                  "--libdir=lib",
                                  # loaders are normally dlopen()ed modules found through a
                                  # loaders.cache; shared_module() ignores
                                  # --default-library=static, so without this the prefix
                                  # still gets libpixbufloader-*.so.
                                  "-Dbuiltin_loaders=all",
                                  "-Dpng=enabled", "-Djpeg=enabled", "-Dtiff=enabled",
                                  "-Dgif=enabled", "-Dothers=disabled",
                                  "-Dlegacy_xpm=disabled",
                                  # enable_auto_if(linux) promotes this to REQUIRED, and it
                                  # is not installed -- the DEFAULT configure FAILS here.
                                  # If it were ever found it would replace builtin_loaders
                                  # entirely and switch png off underneath.
                                  "-Dglycin=disabled",
                                  "-Dandroid=disabled", "-Dnative_windows_loaders=false",
                                  # false is what makes builtin_loaders=all actually work:
                                  # left true, a loader is chosen ONLY via the system MIME
                                  # database, so the loaders compiled INTO the binary are
                                  # unreachable on a machine without shared-mime-info.
                                  "-Dgio_sniffing=false",
                                  "-Drelocatable=false", "-Dintrospection=disabled",
                                  "-Ddocumentation=false", "-Dman=false",
                                  "-Dthumbnailer=disabled", "-Dtests=false",
                                  "-Dinstalled_tests=false")),
            ("ninja", ninja("build/gdk-pixbuf")),
            ("install", install("build/gdk-pixbuf")),
        ],
        provides=["lib/libgdk_pixbuf-2.0.a", "lib/pkgconfig/gdk-pixbuf-2.0.pc"],
        modversions={"gdk-pixbuf-2.0": "2.44.8"},
        deviations="AUDIT, checked at run time not fixed here: intl_dep and medialib_dep "
                   "come from cc.find_library() and are pushed into gdk_pixbuf_deps "
                   "unconditionally. The log audit requires `Library intl found: NO` and "
                   "`Library mlib found: NO`.",
    )

    # ------------------------------------------------------- the window layer

    libepoxy = Step(
        name="libepoxy", src="libepoxy/libepoxy-1.5.10", build="build/libepoxy",
        why="GTK's GL entry-point loader. Static or not, it dlopens libEGL.so.1 at RUN "
            "time by design -- that is the GPU driver, and it is meant to be the machine's.",
        commands=[
            ("meson setup", meson("build/libepoxy", "libepoxy/libepoxy-1.5.10",
                                  "--libdir=lib64", "--datadir=share", "--includedir=include",
                                  # its own glx_static test links -static against
                                  # /usr/lib64/libX11.so and the linker refuses outright --
                                  # the clearest statement there is that X11 has no wrap.
                                  "-Dglx=no", "-Dx11=false",
                                  "-Degl=yes", "-Dtests=false", "-Ddocs=false")),
            ("ninja", ninja("build/libepoxy")),
            ("install", install("build/libepoxy")),
        ],
        provides=["lib64/libepoxy.a", "lib64/pkgconfig/epoxy.pc"],
        deviations="egl.pc is deliberately NOT put on the pkg-config path. epoxy needs the "
                   "Khronos HEADERS (which come from /usr/include, a header-only dependency "
                   "that bakes nothing into the binary) and dlopens the library itself. Let "
                   "egl.pc be found and epoxy.pc would carry it into GTK's link, adding a "
                   "NEEDED entry outside the allowed eight.",
    )

    libxkbcommon = Step(
        name="libxkbcommon", src="libxkbcommon/libxkbcommon-xkbcommon-1.13.2",
        build="build/libxkbcommon",
        why="GTK's keymap handling. xkb-config-root points at a path that will NOT exist "
            "on a target machine ON PURPOSE: satl must spill its own copy and set "
            "XKB_CONFIG_ROOT, and a build that quietly found /usr/share/X11/xkb would hide "
            "that requirement until somebody else ran the binary.",
        # DELIBERATELY BLIND. DFLT_XKB_LEGACY_ROOT is a SECOND baked-in root that no option
        # controls: it is read from xkeyboard-config's `xkb_base` pkg-config variable, and
        # src/context.c appends it to the include path whenever the real root fails to open.
        # Let this configure see any xkeyboard-config.pc and the nonexistent root above
        # becomes decoration.
        pc_dirs=["{stage}/no-such-pkgconfig-on-purpose"],
        commands=[
            ("meson setup", meson("build/libxkbcommon",
                                  "libxkbcommon/libxkbcommon-xkbcommon-1.13.2",
                                  "--libdir=lib64",
                                  # not `share`: datadir is the other branch DFLT_XKB_
                                  # LEGACY_ROOT derives from, and it installs nothing here.
                                  "--datadir=share/no-legacy-xkb-root",
                                  "--sysconfdir=etc", "--includedir=include",
                                  "-Denable-x11=false", "-Denable-xkbregistry=false",
                                  "-Denable-tools=false", "-Denable-wayland=false",
                                  "-Denable-bash-completion=false", "-Denable-docs=false",
                                  "-Denable-cool-uris=false",
                                  "-Dxkb-config-root=/nonexistent/satl-must-provide-xkb-data",
                                  "-Dxkb-config-extra-path=/nonexistent/satl-must-provide-xkb-data-extra",
                                  "-Dxkb-config-versioned-extensions-path=/nonexistent/satl-must-provide-xkb-data.d",
                                  "-Dxkb-config-unversioned-extensions-path=/nonexistent/satl-must-provide-xkb-data-unversioned.d",
                                  "-Dx-locale-root=/nonexistent/satl-must-provide-x-locale",
                                  "-Ddefault-rules=evdev", "-Ddefault-model=pc105",
                                  "-Ddefault-layout=us",
                                  "-Ddefault-variant=", "-Ddefault-options=")),
            # NOT a bare ninja: there is no option to disable tests, benchmarks or fuzzers
            # and they build by default -- a second full copy of the library among them.
            ("ninja libxkbcommon.a", ninja("build/libxkbcommon", "libxkbcommon.a")),
            ("install", install("build/libxkbcommon", "--no-rebuild")),
        ],
        provides=["lib64/libxkbcommon.a", "lib64/pkgconfig/xkbcommon.pc"],
        deviations="+ --datadir=share/no-legacy-xkb-root and a bogus PKG_CONFIG_LIBDIR "
                   "(AUDIT: both derivations of DFLT_XKB_LEGACY_ROOT otherwise produce "
                   "$STAGE/share/X11/xkb, which xkeyboard-config creates as a live absolute "
                   "symlink -- so the deliberately-nonexistent root would be silently "
                   "rescued on THIS machine and fail on the bare one). "
                   "+ -Ddefault-variant= -Ddefault-options= (AUDIT: records the audit; the "
                   "empty values were already correct).",
    )

    gtk = Step(
        name="gtk", src="gtk/gtk-4.24.0", build="build/gtk",
        why="Last, and the only project here that is NOT installed. gtk/meson.build:1074 "
            "is `shared_library('gtk-4', ...)` -- explicit, so --default-library=static "
            "does not touch it. `ninja install` would put a libgtk-4.so and four .pc files "
            "naming -lgtk-4 into the stage, which is a false green waiting to happen. satl "
            "links the ARCHIVES out of this build tree instead, which is what "
            "make_support/047-window.mk already does for gtk-old.",
        requires=["lib/pkgconfig/glib-2.0.pc", "lib/pkgconfig/pango.pc",
                  "lib/pkgconfig/pangocairo.pc", "lib/pkgconfig/pangoft2.pc",
                  "lib/pkgconfig/cairo-gobject.pc", "lib/pkgconfig/gdk-pixbuf-2.0.pc",
                  "lib/pkgconfig/graphene-gobject-1.0.pc", "lib64/pkgconfig/epoxy.pc",
                  "lib64/pkgconfig/xkbcommon.pc", "share/pkgconfig/wayland-protocols.pc",
                  "pkgconfig-system/wayland-client.pc", "pkgconfig-system/wayland-egl.pc",
                  "pkgconfig-system/wayland-scanner.pc", "pkgconfig-system/libdrm.pc"],
        # the four .pc files above that no vendored project produces. wayland-client and
        # wayland-egl CANNOT be static (two copies in one process segfaults: GDK makes its
        # wl_display with ours and EGL calls wl_list_insert in the copy the GPU driver
        # dlopened). wayland-scanner is a build tool GTK needs but never names. libdrm is
        # headers only.
        pc_dirs=["{stage}/lib/pkgconfig", "{stage}/lib64/pkgconfig",
                 "{stage}/share/pkgconfig", "{stage}/pkgconfig-system"],
        commands=[
            ("meson setup", meson("build/gtk", "gtk/gtk-4.24.0",
                                  "--libdir=lib", "--bindir=bin", "--includedir=include",
                                  # X11 is nine libraries, none of them with a wrap, so
                                  # every one would link as a system .so and quietly
                                  # un-static the binary. satl draws on Wayland only.
                                  "-Dwayland-backend=true", "-Dx11-backend=false",
                                  "-Dbroadway-backend=false", "-Dwin32-backend=false",
                                  "-Dmacos-backend=false", "-Dandroid-backend=false",
                                  "-Dmedia-gstreamer=disabled", "-Dprint-cpdb=disabled",
                                  "-Dprint-cups=disabled", "-Dvulkan=disabled",
                                  "-Dcloudproviders=disabled", "-Dsysprof=disabled",
                                  "-Dtracker=disabled", "-Dcolord=disabled",
                                  "-Daccesskit=disabled", "-Df16c=enabled",
                                  "-Dintrospection=disabled", "-Ddocumentation=false",
                                  "-Dscreenshots=false", "-Dman-pages=false",
                                  "-Dbuild-demos=false", "-Dbuild-testsuite=false",
                                  "-Dbuild-examples=false", "-Dbuild-tests=false",
                                  "-Dprofile=default",
                                  no_buildtype=True)),
            ("ninja", ninja("build/gtk")),
        ],
        tree_provides=["gtk/libgtk.a", "gdk/libgdk.a", "gsk/libgsk.a"],
        deviations="--buildtype is deliberately NOT passed: upstream's default_options say "
                   "debugoptimized and that is what the proven 4.16.7 round used. Release "
                   "would flip debug=false, adding -DG_DISABLE_CAST_CHECKS and "
                   "-DG_DISABLE_ASSERT to every GTK object. Size is not a reason here "
                   "(12 GB ceiling, never strip). "
                   "NOT INSTALLED -- see `why`.",
    )

    return [zlib, libffi, expat, pcre2, gperf, fribidi, pixman,
            wayland_protocols, xkeyboard_config,
            libpng, libjpeg, libtiff, freetype(1, "disabled"),
            glib, graphene, fontconfig, harfbuzz, freetype(2, "enabled"),
            cairo, pango, gdk_pixbuf,
            libepoxy, libxkbcommon, gtk]
