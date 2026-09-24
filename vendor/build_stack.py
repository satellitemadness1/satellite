#!/usr/bin/env python3
"""vendor/build_stack.py -- build the whole vendored stack into vendor/stage, bottom-up.

    /usr/bin/python3 vendor/build_stack.py            # build everything, watch it go
    /usr/bin/python3 vendor/build_stack.py --list     # the order, and what is done
    /usr/bin/python3 vendor/build_stack.py --only glib,cairo
    /usr/bin/python3 vendor/build_stack.py --from harfbuzz
    /usr/bin/python3 vendor/build_stack.py --check    # verify the stage, build nothing

ONE LOG FILE PER PROJECT, in vendor/build_logs/NN-<name>.log. Everything also goes to
the terminal as it happens; --quiet keeps the terminal to progress lines and shows the
tail of the log only when something fails.

It is RESUMABLE. Each project that finishes writes a marker under
vendor/stage/.build_stack_done/ holding a hash of the exact commands and environment
that produced it. A re-run skips it; a re-run after its recipe CHANGED does not.

WHY THE VERIFICATION IS NOT OPTIONAL. Every project in this stack can find a system
library instead of ours and report success: meson falls through pkg-config -> CMake ->
a bare cc.find_library() that asks the linker, and prints "found: YES" for all three.
So each step asserts its prerequisites exist BEFORE configuring, asserts its own
artifacts exist after, and reads the version back out of pkg-config -- counting what it
got rather than reading silence as success.

Written 2026-09-20. NOTHING IN vendor/build_stack_recipes.py HAS BEEN RUN; it was
derived by reading each project's build files. Expect this first pass to find defects,
and correct both files when it does.
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_stack_recipes as recipes          # noqa: E402  (needs the line above)

ROOT = Path(__file__).resolve().parent.parent
VENDOR = ROOT / "vendor"

# THE SYMLINK, NOT A VERSION DIRECTORY -- the same choice make_support/010-compiler.mk
# makes, and for its stated reason: "clang-current is the symlink each rebuilt toolchain
# is repointed at, so it cannot go stale the way a version directory would". Naming it
# here is what makes "the vendored stack and satellite itself are built by one compiler"
# true by construction instead of by somebody remembering.
#
# Context resolves it through the symlink, so the marker fingerprint holds the REAL
# toolchain directory: repoint clang-current and every project rebuilds, which is the
# correct answer and not one a symlink-valued fingerprint could give.
DEFAULT_CLANG = str(Path.home() / "opt" / "clang-current")

# CPython, explicitly. `python3` on this machine's PATH is an alpha PyPy with no
# `packaging` module, and glib's gdbus-codegen starts `#!/usr/bin/env python3` -- which
# re-resolves through PATH when ninja runs it, not when meson found it. That is how the
# 2026-09-19 build died 2235 targets in.
SYSTEM_PYTHON = "/usr/bin/python3"
MESON_PY = VENDOR / "meson" / "meson-1.12.0" / "meson.py"

# .pc files no vendored project produces. wayland-client and wayland-egl CANNOT be
# static -- two copies in one process segfaults -- so they stay the machine's, and
# these copies are how they stay visible once PKG_CONFIG_LIBDIR hides /usr.
SYSTEM_PC = ["wayland-client.pc", "wayland-egl.pc", "wayland-scanner.pc"]
LIBDRM_PC = """\
# WRITTEN by vendor/build_stack.py. GTK wants drm_fourcc.h and does not link libdrm
# (gtk-4.24.0/meson.build), so this describes the HEADER only -- no Libs line, on
# purpose, so nothing can acquire a libdrm.so through it.
Name: libdrm
Description: drm_fourcc.h only -- no library, on purpose
Version: 2.4.128
Cflags: -I/usr/include/drm
"""

SCRUB = ("PKG_CONFIG_PATH", "LD_RUN_PATH", "LD_LIBRARY_PATH", "LD_PRELOAD", "CFLAGS",
         "CXXFLAGS", "CPPFLAGS", "LDFLAGS", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH",
         "LIBRARY_PATH", "PKG_CONFIG_SYSROOT_DIR", "ACLOCAL_PATH", "MAKEFLAGS", "DESTDIR")


class Context:
    """Paths, flags, and the three command builders the recipes call."""

    def __init__(self, args):
        self.vendor, self.args = VENDOR, args
        self.stage = VENDOR / "stage"
        self.builds = VENDOR / "build"
        self.ccdir = VENDOR / "build-cc"
        self.toolpath = VENDOR / "build-tools-path"
        self.native = self.builds / "native.ini"
        self.pcsystem = self.stage / "pkgconfig-system"
        self.markers = self.stage / ".build_stack_done"
        # resolve() THROUGH the symlink: the marker fingerprint must name the real
        # toolchain, or repointing clang-current would leave every project marked
        # done against a compiler that is no longer there.
        self.clang = Path(args.clang).expanduser().resolve()
        self.jobs = args.jobs
        self.cc, self.cxx = str(self.ccdir / "cc"), str(self.ccdir / "c++")

    def meson_setup(self, build, src, *opts, no_buildtype=False):
        argv = [SYSTEM_PYTHON, str(MESON_PY), "setup",
                str(self.vendor / build), str(self.vendor / src),
                f"--prefix={self.stage}", f"--native-file={self.native}",
                "--default-library=static",
                # nofallback, NOT nodownload. nodownload only blocks DOWNLOADING a wrap;
                # it happily uses a subprojects/ directory already on disk, and several
                # of these tarballs ship one. nofallback refuses the fallback outright,
                # so a missing staged .pc is a hard error instead of a second copy.
                "--wrap-mode=nofallback",
                "-Dprefer_static=true", "-Db_staticpic=true", "-Db_lto=false"]
        if not no_buildtype:
            argv.append("--buildtype=release")
        return argv + list(opts)

    def ninja(self, build, *targets):
        return ["ninja", "-C", str(self.vendor / build), "-j", str(self.jobs), *targets]

    def meson_install(self, build, *opts):
        return [SYSTEM_PYTHON, str(MESON_PY), "install",
                "-C", str(self.vendor / build), *opts]

    def env_for(self, step):
        env = {k: v for k, v in os.environ.items() if k not in SCRUB}
        pc = step.pc_dirs or ["{stage}/lib/pkgconfig", "{stage}/lib64/pkgconfig",
                              "{stage}/share/pkgconfig"]
        env["PKG_CONFIG_LIBDIR"] = ":".join(d.format(stage=self.stage) for d in pc)
        env["PATH"] = ":".join([str(self.stage / "bin"), str(self.ccdir),
                                str(self.toolpath), os.environ.get("PATH", "/usr/bin")])
        env["CC"], env["CXX"] = self.cc, self.cxx
        env["LC_ALL"] = "C"
        env.update(step.env)
        return env


def unpack(c, steps, log, force=False):
    """vendor/new/*.tar.* IS committed; vendor/<project>/ is NOT. This is the step
    that turns one into the other, and without it a fresh clone holds every
    dependency's source and still cannot build a thing -- which is most of the way
    to the author's rule and not all of it.

    THE HASH IS CHECKED BEFORE ANYTHING IS EXTRACTED, against vendor/new/SHA256SUMS.
    That pins the file against silent change from here on; it does NOT prove the
    original download was authentic, and for twelve of the twenty-six there is
    nothing published upstream to compare against. SHA256SUMS says which tier each
    file is in and the manifest reprints it -- do not let `sha256sum -c` passing
    read as more than it is."""
    hashes = source_hashes()
    tarballs = {version_of(n): n for n in hashes}
    # meson is not a Step -- it is what CONFIGURES the steps -- but a fresh clone
    # has no more of it than of anything else, and preflight dies without it.
    wanted, seen, made = [("meson", "meson/meson-1.12.0")], set(), 0
    wanted += [(st.name, st.src) for st in steps]
    for name, src in wanted:
        if src in seen:
            continue
        seen.add(src)
        tree = c.vendor / src
        if tree.is_dir() and not force:
            continue
        tar = tarballs.get(version_of(src))
        if not tar:
            die(f"{name}: nothing in vendor/new/ matches {src}")
        path = VENDOR / "new" / tar
        if not path.exists():
            die(f"{name}: {path} is missing -- it is committed, so the clone is broken")
        digest, tier = hashes[tar]
        got = hashlib.sha256(path.read_bytes()).hexdigest()
        if got != digest:
            die(f"{name}: {tar} does not match SHA256SUMS\n  want {digest}\n  got  {got}")
        parent = (c.vendor / src).parent
        parent.mkdir(parents=True, exist_ok=True)
        log(f"# unpacking {tar}  [{tier}]")
        r = subprocess.run(["tar", "xf", str(path), "-C", str(parent)],
                           capture_output=True, text=True)
        if r.returncode != 0:
            die(f"{name}: tar failed\n{r.stderr}")
        if not tree.is_dir():
            die(f"{name}: {tar} did not produce {tree}")
        apply_journal_patches(c, src, log)
        made += 1
    made += unpack_fonts(c, log, force)
    if made:
        log(f"# unpacked {made} tree(s) from vendor/new/")
    return made


def apply_journal_patches(c, src, log):
    """vendor/edit_journal/<project>/NNN-*.patch, in name order, `patch -p1` inside the
    tree that was just unpacked. README_FIRST.md: an edit lands as a journal entry AND a
    patch, because the unpacked tree is in no repository and the next unpack would destroy
    a change made in place with no warning. This is the "command instead of an act of
    memory" that file promises -- and it runs ONLY on a fresh tree, so a patch added
    beside an already-unpacked project needs `--unpack --force` to reach it.

    The first patch (2026-09-22) is VTE's: one word, shared_library -> library, so that
    --default-library=static yields libvte-2.91-gtk4.a at all."""
    project = src.split("/")[0]
    patches = sorted((VENDOR / "edit_journal" / project).glob("*.patch"))
    for p in patches:
        log(f"# patch -p1 < {p.relative_to(VENDOR)}")
        r = subprocess.run(["patch", "-p1", "--no-backup-if-mismatch", "-i", str(p)],
                           cwd=str(c.vendor / src), capture_output=True, text=True)
        if r.returncode != 0:
            die(f"{project}: {p.name} did not apply\n{r.stdout}{r.stderr}")
    return len(patches)


def unpack_fonts(c, log, force=False):
    """IBM Plex Mono, all fourteen faces, from the committed zip.

    WITHOUT THIS THE BINARY DIFFERS BY WHO BUILT IT. make_window_data.py globs
    vendor/fonts/ibm-plex-mono/*.ttf, and .gitignore tracks exactly ONE face
    (Regular) while this machine has all fourteen -- so a fresh clone embedded one
    face and this checkout embedded fourteen, from the same commit. The zip is in
    vendor/new/ and hashes clean, so there is no reason for that to be true.

    The tracked Regular.ttf is left alone: it is what guarantees a font exists even
    if this never runs, and a GTK4 binary with no font reachable exits 139 rather
    than drawing tofu."""
    import zipfile
    fonts = c.vendor / "fonts" / "ibm-plex-mono"
    if len(list(fonts.glob("*.ttf"))) >= 14 and not force:
        return 0
    hashes = source_hashes()
    name = "IBM_Plex_Mono.zip"
    path = VENDOR / "new" / name
    if not path.exists():
        die(f"{path} is missing -- it is committed, so the clone is broken")
    if name in hashes:
        digest, tier = hashes[name]
        got = hashlib.sha256(path.read_bytes()).hexdigest()
        if got != digest:
            die(f"{name} does not match SHA256SUMS\n  want {digest}\n  got  {got}")
        log(f"# unpacking {name}  [{tier}]")
    fonts.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path) as z:
        # flat archive, but never trust that: a name with a separator in it would
        # write outside the directory we meant.
        for member in z.namelist():
            leaf = PurePosixPath(member).name
            if not leaf or (not leaf.endswith(".ttf") and leaf != "OFL.txt"):
                continue
            (fonts / leaf).write_bytes(z.read(member))
    log(f"# {len(list(fonts.glob('*.ttf')))} faces in {fonts}")
    return 1


# ----------------------------------------------------------------- preflight

def preflight(c, log, steps):
    """Unpack what is missing, then build the wrapper, the curated PATH, the native
    file and the .pc allow-list."""
    unpack(c, steps, log)
    if not (c.clang / "bin" / "clang").exists():
        die(f"no clang at {c.clang}/bin/clang -- pass --clang <dir>")

    c.ccdir.mkdir(parents=True, exist_ok=True)
    for name, real in (("cc", "clang"), ("c++", "clang++")):
        (c.ccdir / name).write_text(
            "#!/bin/sh\n"
            "# GENERATED by vendor/build_stack.py. Appends -w AFTER the caller's\n"
            "# arguments, which is the only position that defeats a project's own\n"
            "# -Werror=<name>: clang resolves a warning by the LAST flag naming it, and\n"
            "# meson puts user c_args BEFORE a project's own add_project_arguments().\n"
            f'exec "{c.clang}/bin/{real}" "$@" -w\n')
        (c.ccdir / name).chmod(0o755)

    c.toolpath.mkdir(parents=True, exist_ok=True)
    py = c.toolpath / "python3"
    if py.is_symlink() or py.exists():
        py.unlink()
    py.symlink_to(SYSTEM_PYTHON)

    c.builds.mkdir(parents=True, exist_ok=True)
    c.native.write_text(
        "# GENERATED by vendor/build_stack.py -- passed to every meson setup here.\n"
        "#\n"
        "# cmake is blocked ON PURPOSE. PKG_CONFIG_LIBDIR is not an isolation boundary:\n"
        "# meson's dependency order is pkg-config -> extraframework -> CMAKE, and CMake\n"
        "# searches /usr and ignores PKG_CONFIG_LIBDIR entirely. Naming a binary that\n"
        "# does not exist makes every CMake leg report not-found instead.\n"
        "#\n"
        "# python3 is pinned because find_program('python3') searches PATH, where an\n"
        "# alpha PyPy leads on this machine.\n"
        "[binaries]\n"
        f"c = '{c.cc}'\n"
        f"cpp = '{c.cxx}'\n"
        "cmake = 'cmake-intentionally-absent'\n"
        f"python3 = '{SYSTEM_PYTHON}'\n"
        "pkg-config = '/usr/bin/pkg-config'\n")

    c.pcsystem.mkdir(parents=True, exist_ok=True)
    for pc in SYSTEM_PC:
        src = Path("/usr/lib64/pkgconfig") / pc
        if not src.exists():
            die(f"{src} is missing -- GTK cannot be built without it")
        shutil.copy2(src, c.pcsystem / pc)
    (c.pcsystem / "libdrm.pc").write_text(LIBDRM_PC)
    c.markers.mkdir(parents=True, exist_ok=True)

    missing = [t for t in ("ninja", "cmake", "bison", "perl", "make", "pkg-config")
               if shutil.which(t) is None]
    if missing:
        die("missing build tools: " + ", ".join(missing))
    if recipes.NASM is None:
        die("libjpeg-turbo needs nasm (REQUIRE_SIMD=1) -- AlmaLinux: sudo dnf install --enablerepo=crb nasm")
    if not MESON_PY.exists():
        die(f"no vendored meson at {MESON_PY}")

    smoke = c.builds / "smoke"
    smoke.mkdir(exist_ok=True)
    (smoke / "t.c").write_text("static int x; int main(void){x=1;return 0;}\n")
    (smoke / "t.cpp").write_text("#include <string>\nint main(){std::string s;return s.size();}\n")
    for tool, src in ((c.cc, "t.c"), (c.cxx, "t.cpp")):
        # -Werror=unused-but-set-variable is the exact flag pango's boilerplate trips.
        r = subprocess.run([tool, "-Werror=unused-but-set-variable", str(smoke / src),
                            "-o", str(smoke / "out")], capture_output=True, text=True)
        if r.returncode != 0:
            die(f"the -w wrapper does not work: {tool}\n{r.stdout}{r.stderr}")

    for line in (f"clang       {c.clang}",
                 f"meson       {MESON_PY} ({run_out([SYSTEM_PYTHON, str(MESON_PY), '--version'])})",
                 f"python3     {SYSTEM_PYTHON} ({run_out([SYSTEM_PYTHON, '--version'])})",
                 f"ninja       {shutil.which('ninja')} ({run_out(['ninja', '--version'])})",
                 f"linker      {default_linker(c)}",
                 f"stage       {c.stage}",
                 f"jobs        {c.jobs}   nice {c.args.nice}"):
        log(line)
    log("")
    info = c.clang / "BUILD-INFO.txt"
    if info.exists():
        log("clang BUILD-INFO.txt:")
        log("    " + info.read_text().strip().replace("\n", "\n    "))
        log("")


def default_linker(c):
    """Which ld clang actually drives. It is binutils here, not lld -- and the two
    disagree about static links of dynamic objects, which is how X11 was caught."""
    r = subprocess.run([str(c.clang / "bin" / "clang"), "-###", "-x", "c", "/dev/null",
                        "-o", "/dev/null"], capture_output=True, text=True)
    for tok in r.stderr.replace('"', " ").split():
        if tok.endswith("/ld") or tok.endswith("ld.lld") or tok.endswith("/ld.bfd"):
            return tok
    return "unknown"


def run_out(argv):
    try:
        return subprocess.run(argv, capture_output=True, text=True).stdout.strip().splitlines()[0]
    except (OSError, IndexError):
        return "?"


# ----------------------------------------------------------------- one project

def fingerprint(c, step):
    """A hash of exactly what produced this install, so a changed recipe re-runs."""
    env = c.env_for(step)
    keep = {k: env[k] for k in ("PKG_CONFIG_LIBDIR", "PATH", "CC", "CXX") if k in env}
    keep.update({k: v for k, v in step.env.items()})
    blob = json.dumps([step.commands, keep, str(c.clang)], sort_keys=True, default=str)
    return hashlib.sha256(blob.encode()).hexdigest()[:16]


def configured(builddir):
    return any((builddir / p).exists() for p in
               ("meson-private/coredata.dat", "config.status", "CMakeCache.txt"))


def build_one(c, step, index, total, logdir):
    stage, vendor = c.stage, c.vendor
    builddir = vendor / step.build
    logpath = logdir / f"{index:02d}-{step.name}.log"
    marker = c.markers / step.name
    fp = fingerprint(c, step)

    if marker.exists() and marker.read_text().strip().split()[0] == fp and not c.args.force:
        say(f"[{index}/{total}] {step.name}: already built, recipe unchanged -- skipping")
        return ("skipped", 0.0, logpath)

    say(f"[{index}/{total}] {step.name}  ->  {logpath}")
    started = time.time()
    with open(logpath, "w", buffering=1) as fh:
        def log(line=""):
            fh.write(line + "\n")
            if not c.args.quiet:
                print(line, flush=True)

        env = c.env_for(step)
        log(f"=== {step.name} ===")
        log(time.strftime("%Y-%m-%d %H:%M:%S"))
        log(f"source  {vendor / step.src}")
        log(f"build   {builddir}")
        log(f"prefix  {stage}")
        log("")
        log("why this project is here:")
        log("    " + step.why.replace("\n", "\n    "))
        if step.deviations:
            log("")
            log("differs from vendor/BUILD_RECIPES.md:")
            log("    " + step.deviations.replace("\n", "\n    "))
        log("")
        for k in ("PKG_CONFIG_LIBDIR", "PATH", "CC", "CXX"):
            log(f"{k}={env[k]}")
        for k, v in sorted(step.env.items()):
            if k not in ("CC", "CXX"):
                log(f"{k}={v}")
        log("")

        # PRE: the prerequisites must be THERE, counted -- not merely not-complained-about.
        if not check_paths(log, "prerequisite", stage, step.requires):
            return ("failed (missing prerequisite)", time.time() - started, logpath)

        subprojects = vendor / step.src / "subprojects"
        before = {p.name for p in subprojects.iterdir() if p.is_dir()} if subprojects.is_dir() else set()

        resume = c.args.resume_build and configured(builddir)
        if not resume and builddir.exists():
            log(f"# rm -rf {builddir}")
            shutil.rmtree(builddir)
        builddir.mkdir(parents=True, exist_ok=True)

        for label, argv in step.commands:
            if resume and label in ("configure", "meson setup", "cmake"):
                log(f"--- {label}: SKIPPED, --resume-build and the directory is configured")
                continue
            log("")
            log(f"--- {label}")
            log("$ " + " ".join(shell_quote(a) for a in argv))
            log("")
            rc = stream(argv, builddir, env, log, c.args.nice)
            if rc != 0:
                log("")
                log(f"*** {step.name}: `{label}` exited {rc}")
                return (f"FAILED at {label} (exit {rc})", time.time() - started, logpath)

        # POST: a wrap that fired leaves a directory behind; find it now, not in GTK.
        after = {p.name for p in subprojects.iterdir() if p.is_dir()} if subprojects.is_dir() else set()
        if after - before:
            log("")
            log(f"*** a subproject was unpacked into upstream's tree: {sorted(after - before)}")
            log("    --wrap-mode=nofallback should have made this impossible.")
            return ("FAILED (subproject fired)", time.time() - started, logpath)

        ok = check_paths(log, "installed", stage, step.provides)
        ok &= check_paths(log, "in the build tree", builddir, step.tree_provides)
        ok &= check_modversions(log, env, step.modversions, stage)
        ok &= check_no_shared(log, stage)
        if not ok:
            return ("FAILED verification", time.time() - started, logpath)

        took = time.time() - started
        log("")
        log(f"=== {step.name}: ok in {took:.1f}s")
        marker.write_text(f"{fp} {time.strftime('%Y-%m-%dT%H:%M:%S')} {took:.1f}s\n")
        return ("ok", took, logpath)


def stream(argv, cwd, env, log, nice):
    """Run one command, every line to the log and (unless --quiet) to the terminal."""
    def child():
        os.nice(nice)
    p = subprocess.Popen(argv, cwd=str(cwd), env=env, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True, bufsize=1,
                         preexec_fn=child if nice else None)
    try:
        for line in p.stdout:
            log(line.rstrip("\n"))
    except KeyboardInterrupt:
        p.kill()
        raise
    return p.wait()


# ----------------------------------------------------------------- verification

def check_paths(log, what, base, paths):
    if not paths:
        return True
    missing = [p for p in paths if not (base / p).exists()]
    log(f"# {len(paths) - len(missing)}/{len(paths)} {what} present under {base}")
    for p in missing:
        log(f"*** MISSING {what}: {base / p}")
    return not missing


def check_modversions(log, env, wanted, stage):
    """Read the version back out of pkg-config, and check the .pc that answered is ours.

    An empty result from pkg-config reads like success at a glance; this counts it."""
    ok = True
    for mod, version in sorted(wanted.items()):
        got = subprocess.run(["pkg-config", "--modversion", mod], env=env,
                             capture_output=True, text=True)
        libdir = subprocess.run(["pkg-config", "--variable=libdir", mod], env=env,
                                capture_output=True, text=True).stdout.strip()
        v = got.stdout.strip()
        if got.returncode != 0 or not v:
            log(f"*** pkg-config cannot see {mod} at all (exit {got.returncode})")
            ok = False
        elif v != version:
            log(f"*** {mod} is {v}, expected {version} -- something else answered")
            ok = False
        elif libdir and not libdir.startswith(str(stage)):
            log(f"*** {mod} {v} resolves libdir={libdir}, which is NOT under {stage}")
            ok = False
        else:
            log(f"# pkg-config {mod} {v}" + (f"  libdir={libdir}" if libdir else ""))
    return ok


def is_shared_object(path):
    """ELF header, not the file name. glib installs gdb pretty-printers called
    libglib-2.0.so.0.9000.0-gdb.py, and a *.so* glob calls those shared libraries."""
    try:
        with open(path, "rb") as fh:
            head = fh.read(18)
    except OSError:
        return False
    return len(head) == 18 and head[:4] == b"\x7fELF" and \
        int.from_bytes(head[16:18], "little") == 3          # ET_DYN


def check_no_shared(log, stage):
    """Nothing in this stack may install a shared object. shared_module() ignores
    --default-library=static, which is how gdk-pixbuf's loaders escape -- and how
    libjpeg-turbo escaped on 2026-09-20, since cmake never saw that option at all.

    The WHOLE prefix, not just lib/ and lib64/: a shared_module() names its own
    install_dir and is under no obligation to pick one of those two."""
    found = [p for p in stage.rglob("*") if p.is_file() and not p.is_symlink()
             and ".so" in p.name and is_shared_object(p)]
    for p in found:
        log(f"*** a SHARED object is in the stage: {p}")
    return not found


# ----------------------------------------------------------------- the manifest

def source_hashes():
    """vendor/new/SHA256SUMS, with the tier each hash sits under. The tier matters:
    only the first says the download was ever compared against upstream."""
    out, tier = {}, "?"
    f = VENDOR / "new" / "SHA256SUMS"
    if not f.exists():
        return out
    for line in f.read_text().splitlines():
        if line.startswith("# ") and line[2:3].isupper() and ":" in line:
            tier = line[2:].split(":")[0].strip()
        elif line and not line.startswith("#"):
            digest, _, name = line.partition("  ")
            out[name.strip()] = (digest, tier)
    return out


def version_of(src):
    """The trailing version in an unpacked directory name: freetype-2.14.3 -> 2.14.3."""
    import re
    m = re.findall(r"\d+(?:\.\d+)+", src.split("/")[-1])
    return m[-1] if m else ""


def write_manifest(c, steps, log):
    """WHAT IS INSIDE satl, written down once.

    The plan is to freeze these versions for three to five years. Nothing else
    records what went in: the tarballs are in vendor/new, the recipe is in the two
    .py files, and the built archives are gitignored -- so in 2031 this file is the
    only thing that says which source produced the binary somebody is holding. It
    is also most of what LGPL 2.1 section 6 asks for when satellite_enterprise
    ships a binary (DEP-6)."""
    hashes = source_hashes()
    tarballs = {version_of(n): n for n in hashes}
    rows = ["# vendor/stage/BUILD_MANIFEST.txt -- what this prefix is made of.",
            "# WRITTEN by vendor/build_stack.py. Regenerate with --manifest.",
            "",
            time.strftime("built    %Y-%m-%d %H:%M:%S"),
            f"clang    {c.clang}",
            f"         {run_out([str(c.clang / 'bin' / 'clang'), '--version'])}",
            f"linker   {default_linker(c)}",
            f"meson    {run_out([SYSTEM_PYTHON, str(MESON_PY), '--version'])}",
            f"python   {SYSTEM_PYTHON} {run_out([SYSTEM_PYTHON, '--version'])}",
            "",
            "# project            version      source tarball and its sha256 tier",
            ""]
    for st in steps:
        ver = version_of(st.src)
        tar = tarballs.get(ver, "")
        digest, tier = hashes.get(tar, ("", "not in SHA256SUMS"))
        rows.append(f"{st.name:<20} {ver:<12} {tar}")
        if digest:
            rows.append(f"{'':<20} {'':<12} {digest}  [{tier}]")
    rows += ["", "# what pkg-config answers, read back from this prefix", ""]
    env = c.env_for(steps[-1])
    seen = set()
    for st in steps:
        for mod in sorted(st.modversions):
            if mod in seen:
                continue
            seen.add(mod)
            v = subprocess.run(["pkg-config", "--modversion", mod], env=env,
                               capture_output=True, text=True).stdout.strip()
            rows.append(f"{mod:<26} {v}")
    rows += ["", "# the archives, which are what actually link into satl", ""]
    for a in sorted(c.stage.rglob("*.a")):
        rows.append(f"{a.stat().st_size:>12,}  {a.relative_to(c.stage)}")
    gtkb = c.vendor / "build" / "gtk"
    if gtkb.is_dir():
        rows += ["",
                 "# GTK is NOT installed (its meson.build calls shared_library explicitly),",
                 "# so satl links these out of the build tree. They are THIN archives --",
                 "# members referenced by path -- and cannot be copied or shipped as they are.",
                 ""]
        for a in sorted(gtkb.rglob("*.a")):
            members = run_out(["ar", "t", str(a)])
            rows.append(f"{a.stat().st_size:>12,}  {a.relative_to(c.vendor)}"
                        f"   (first member: {members})")
    path = c.stage / "BUILD_MANIFEST.txt"
    path.write_text("\n".join(rows) + "\n")
    log(f"# wrote {path}")
    return path


# ----------------------------------------------------------------- driver

def shell_quote(a):
    a = str(a)
    return a if all(ch.isalnum() or ch in "-_=/.:,+@" for ch in a) else "'" + a.replace("'", "'\\''") + "'"


def say(msg):
    print(msg, flush=True)


def die(msg):
    print(f"vendor/build_stack.py: {msg}", file=sys.stderr)
    sys.exit(1)


def select(steps, args):
    names = [s.name for s in steps]
    if args.only:
        want = [n.strip() for n in args.only.split(",")]
        for n in want:
            if n not in names:
                die(f"--only: no project called {n!r}. Known: {', '.join(names)}")
        return [s for s in steps if s.name in want]
    for flag, n in (("--from", args.start), ("--to", args.stop)):
        if n and n not in names:
            die(f"{flag}: no project called {n!r}. Known: {', '.join(names)}")
    lo = names.index(args.start) if args.start else 0
    hi = names.index(args.stop) + 1 if args.stop else len(steps)
    return steps[lo:hi]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--clang", default=DEFAULT_CLANG, help="toolchain directory")
    ap.add_argument("--jobs", "-j", type=int, default=16)
    ap.add_argument("--nice", type=int, default=19,
                    help="0 to run at normal priority; 19 keeps the desktop repainting")
    ap.add_argument("--logs", default=str(VENDOR / "build_logs"))
    ap.add_argument("--only", help="comma-separated project names")
    ap.add_argument("--from", dest="start", help="start at this project")
    ap.add_argument("--to", dest="stop", help="stop after this project")
    ap.add_argument("--force", action="store_true", help="rebuild even if marked done")
    ap.add_argument("--resume-build", action="store_true",
                    help="reuse an existing build directory instead of wiping it")
    ap.add_argument("--keep-going", action="store_true", help="do not stop at the first failure")
    ap.add_argument("--quiet", action="store_true", help="terminal gets progress only")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--check", action="store_true", help="verify the stage, build nothing")
    ap.add_argument("--manifest", action="store_true",
                    help="write vendor/stage/BUILD_MANIFEST.txt and stop")
    ap.add_argument("--unpack", action="store_true",
                    help="unpack vendor/new/ into the source trees and stop")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    c = Context(args)
    steps = recipes.steps(c)

    if args.list:
        for i, s in enumerate(steps, 1):
            done = (c.markers / s.name).exists()
            print(f"{i:2d}. {'done ' if done else '     '} {s.name:<18} {s.src}")
        return 0

    logdir = Path(args.logs)
    logdir.mkdir(parents=True, exist_ok=True)

    if args.unpack:
        say(f"unpacked {unpack(c, steps, say)} tree(s)")
        return 0

    if args.manifest:
        write_manifest(c, steps, say)
        return 0

    if args.check:
        ok = True
        for s in steps:
            ok &= check_paths(say, "installed", c.stage, s.provides)
            ok &= check_modversions(say, c.env_for(s), s.modversions, c.stage)
        ok &= check_no_shared(say, c.stage)
        say("stage verifies" if ok else "*** the stage does NOT verify")
        return 0 if ok else 1

    say(f"vendor/build_stack.py -- {len(steps)} projects into {c.stage}")
    say("")
    preflight(c, say, steps)
    chosen = select(steps, args)
    if args.dry_run:
        for i, s in enumerate(chosen, 1):
            print(f"--- {i:02d} {s.name}")
            for label, argv in s.commands:
                print("  $ " + " ".join(shell_quote(a) for a in argv))
        return 0

    results, t0 = [], time.time()
    try:
        for i, s in enumerate(chosen, 1):
            status, took, logpath = build_one(c, s, i, len(chosen), logdir)
            results.append((s.name, status, took, logpath))
            if status.startswith(("FAILED", "failed")):
                say("")
                say(f"*** {s.name}: {status}")
                say(f"*** the log is {logpath}")
                if args.quiet:
                    say("--- last 30 lines " + "-" * 40)
                    say("".join(logpath.read_text().splitlines(True)[-30:]))
                if not args.keep_going:
                    break
    except KeyboardInterrupt:
        say("")
        say("interrupted -- re-run to continue; finished projects are skipped")

    say("")
    say("=" * 64)
    for name, status, took, logpath in results:
        say(f"  {name:<18} {status:<28} {took:6.1f}s  {logpath.name}")
    say(f"  {'total':<18} {'':<28} {time.time() - t0:6.1f}s")
    good = all(r[1] in ("ok", "skipped") for r in results)
    if good and len(chosen) == len(steps):
        say("")
        write_manifest(c, steps, say)
    return 0 if good else 1


if __name__ == "__main__":
    sys.exit(main())
