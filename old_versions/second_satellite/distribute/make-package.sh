#!/bin/sh
# make-package.sh -- fill enterprise_satellite/ with built programs and artwork,
# then write the tarball that gets carried to the target machine.
#
# THIS RUNS ON THE BUILD MACHINE. install-satellite.sh runs on the target and
# compiles nothing; everything it installs is put here, by this. The split is
# the whole design of the package: the machine with the compiler, the CRB
# repository and the vte development files does the work once, and the machine
# that is going to run satellite needs none of them.
#
# WHAT IT PRODUCES:
#
#     enterprise_satellite/programs/     four built programs
#     enterprise_satellite/share/        the launcher, the mime packet, the
#                                        artwork and the hicolor index.theme
#     enterprise_satellite/example/      the example programs
#     satellite-<version>-almalinux10-x86_64.tar.xz
#
# THE ARTWORK IS COPIED FROM ../satellite_enterprise/icons AND IS NOT EDITED,
# EVER. Not scaled, not re-encoded, not "cleaned up", and no missing size
# generated from a larger one. Every file is verified with cmp after the copy
# and this script fails if any of them differ. That is a rule with a reason:
# those PNGs are the author's exports, and the set is deliberately MIXED --
# the 128, 256 and 512 application icons were re-exported on 2026-08-31 and the
# smaller ones were not, so a script that "helpfully" regenerated 16 through 64
# from the 512 would silently replace chosen artwork with a guess about it.
#
# THAT DIRECTORY IS A SEAM AND IT IS WORTH NAMING. The artwork belongs to the
# PROGRAM, not to Enterprise Linux, so the folder it sits in is a fact about
# which installer was written first rather than about what the files are.
# satellite_debian/install.sh reaches across to the same place for the same
# reason: twenty binary files copied into a second directory would be twenty
# files that can drift. If the artwork ever moves to a shared directory of its
# own, the artwork_src line below is the one line that changes.

set -eu

self=$0
here=$(cd -- "$(dirname -- "$self")" && pwd)
repo=$(cd -- "$here/.." && pwd)
pkg=$here/enterprise_satellite
artwork_src=$repo/satellite_enterprise/icons

me=$(basename -- "$self")
die() { printf '%s: %s\n' "$me" "$1" >&2; exit 1; }
step() { printf '\n%s: %s\n' "$me" "$1"; }

do_build=yes
case ${1:-} in
    --no-build) do_build=no ;;
    --help|-h)
        printf 'usage: %s [--no-build]\n\n' "$me"
        printf 'Fills %s and writes the tarball.\n' "enterprise_satellite/"
        printf '  --no-build   package the binaries already in the tree root\n'
        printf '               instead of running make first.\n'
        exit 0
        ;;
    '') ;;
    *) die "unknown option: $1" ;;
esac

# ---------------------------------------------------------------------------
step "building"

# ONE make AT A TIME. This tree builds IN-TREE -- the object files sit beside
# their sources -- so a second make running against it at the same time is two
# processes writing the same .o files. Checked rather than assumed, because the
# failure is a corrupt object file and a link error somewhere unrelated.
if pgrep -x make >/dev/null 2>&1; then
    die "another make is already running against this tree.
       This build is in-tree, so two of them write the same object files.
       Wait for it to finish, or use --no-build to package the binaries
       that are in the tree root now."
fi

if [ "$do_build" = yes ]; then
    # STATIC IS NOT PASSED, because make_support/048-static.mk already defaults
    # it to full and passing it again would be a second place for that default
    # to live. 050-payload.sh in the installer verifies the property on the way
    # in rather than trusting either of them.
    ( cd "$repo" && make ) || die "the build failed. Nothing was packaged."
else
    printf '  skipped (--no-build)\n'
fi

for _p in satl satl.haswell satl-cpu-level satl-term; do
    [ -f "$repo/$_p" ] || die "$repo/$_p was not built.
       satl-term needs vte-2.91-gtk4; see make_support/047-window.mk."
done

# ---------------------------------------------------------------------------
step "checking what will be shipped"

# THE TWO PROPERTIES THAT MAKE A BINARY SHIPPABLE, checked here at the source
# and again by the installer at the destination. Two checks of one fact, and
# they answer different questions: this one is about what was built, that one is
# about what arrived.
#
# 1. STATIC. satl must depend on nothing at all on the target -- no libstdc++,
#    no libc, no version of either to match.
# 2. NO RPATH. This build environment exports LD_RUN_PATH, and GNU ld silently
#    turns that into an RPATH when no -rpath is given, so a binary linked here
#    can come out pointing at a hand-built toolchain inside one user's home
#    directory. The Makefile's LINK_ENV = env -u LD_RUN_PATH is what prevents
#    it; this is the check that it is still doing so. `ldd` would not reveal it
#    -- on this machine those paths exist and it reports success.
for _p in satl satl.haswell satl-cpu-level; do
    case $(ldd "$repo/$_p" 2>&1 || :) in
        *'not a dynamic executable'*|*'statically linked'*) ;;
        *) die "$_p is dynamically linked. Build with STATIC=full (the default);
       see make_support/048-static.mk." ;;
    esac
done

for _p in satl satl.haswell satl-cpu-level satl-term; do
    if readelf -d "$repo/$_p" 2>/dev/null | grep -qiE '\((RPATH|RUNPATH)\)'; then
        die "$_p carries an RPATH and would not be portable:
$(readelf -d "$repo/$_p" | grep -iE '\((RPATH|RUNPATH)\)')
       LD_RUN_PATH is exported in this environment and GNU ld bakes it in.
       Relink with: env -u LD_RUN_PATH make"
    fi
done

# satl-term IS DYNAMIC ON PURPOSE and is checked differently: it must resolve
# every library it names on THIS machine, because the machine it is going to is
# the same AlmaLinux 10.2. GTK4 and VTE ship no static libraries and declare
# shared_library() in their own build files, so there is nothing to link even in
# principle -- this is a property of the package, not a shortcut in it.
if ldd "$repo/satl-term" 2>&1 | grep -q 'not found'; then
    die "satl-term has unresolved libraries on the machine that built it:
$(ldd "$repo/satl-term" | grep 'not found')"
fi

printf '  satl           %s bytes, static, no RPATH\n' "$(stat -c%s "$repo/satl")"
printf '  satl.haswell   %s bytes, static, no RPATH\n' "$(stat -c%s "$repo/satl.haswell")"
printf '  satl-cpu-level %s bytes, static, no RPATH\n' "$(stat -c%s "$repo/satl-cpu-level")"
printf '  satl-term      %s bytes, links %s libraries from this system\n' \
    "$(stat -c%s "$repo/satl-term")" "$(ldd "$repo/satl-term" | grep -c '=>')"

# ---------------------------------------------------------------------------
step "copying the programs"

# THE PAYLOAD DIRECTORIES ARE EMPTIED FIRST, so that a program removed from the
# build does not survive in the package as a stale copy of itself. Named
# directories and not a wildcard: this deletes things.
rm -rf "$pkg/programs" "$pkg/share" "$pkg/example"
mkdir -p "$pkg/programs"

for _p in satl satl.haswell satl-cpu-level satl-term; do
    cp -- "$repo/$_p" "$pkg/programs/$_p"
    chmod 755 "$pkg/programs/$_p"
    cmp -s "$repo/$_p" "$pkg/programs/$_p" ||
        die "$_p did not copy identically."
    printf '  %s\n' "$_p"
done

# ---------------------------------------------------------------------------
step "copying the artwork, and verifying every byte of it"

mkdir -p "$pkg/share/applications" "$pkg/share/mime/packages" "$pkg/share/icons/hicolor"

# THE LAUNCHER AND THE MIME PACKET, copied unchanged. The launcher is REWRITTEN
# BY THE INSTALLER and not here -- Exec= gets the absolute path of the prefix it
# is being installed into, which this script cannot know. The mime packet is
# never edited by anything: read the comments inside it before touching it, they
# record two failures that each looked like a caching problem and were not.
cp -- "$artwork_src/org.satellite.terminal.desktop" "$pkg/share/applications/"
cp -- "$artwork_src/application-x-satellite.xml" "$pkg/share/mime/packages/"

# EIGHTEEN PNGs: nine sizes, two icons. Counted as it goes and the count is
# checked at the end, so a size that disappears from the source is a failure
# here rather than a gap discovered on the target machine.
icons=0
for _size in 16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512; do
    for _ctx in apps mimetypes; do
        case $_ctx in
            apps) _name=org.satellite.terminal.png ;;
            *)    _name=application-x-satellite.png ;;
        esac
        _src=$artwork_src/hicolor/$_size/$_ctx/$_name
        _dst=$pkg/share/icons/hicolor/$_size/$_ctx/$_name
        [ -f "$_src" ] || die "the artwork is missing $_src"
        mkdir -p "$(dirname -- "$_dst")"
        cp -- "$_src" "$_dst"
        chmod 644 "$_dst"
        # cmp AND NOT A CHECKSUM COMPARISON, because cmp is the direct question
        # and needs nothing to be right about.
        cmp -s "$_src" "$_dst" || die "$_size/$_ctx artwork did not copy identically."
        icons=$((icons + 1))
    done
done
[ "$icons" -eq 18 ] || die "expected 18 icons and copied $icons."
printf '  18 icons, all byte-identical to %s\n' "${artwork_src#$repo/}"

# index.theme, WHICH IS NOT ARTWORK AND IS WHAT DECIDES WHETHER ANY ARTWORK IS
# EVER DRAWN. An icon directory is only a theme if it holds one, and without it
# GTK does not look inside AT ALL. It belongs to the hicolor-icon-theme package,
# which installs it under /usr and nowhere else, so every other prefix starts
# without one and the installer has to put it there.
#
# SHIPPED AS A FALLBACK, not as the first choice: install-satellite.sh prefers
# the target machine's own copy and uses this one only when there is none.
if [ -f /usr/share/icons/hicolor/index.theme ]; then
    cp /usr/share/icons/hicolor/index.theme "$pkg/share/icons/hicolor/index.theme"
    chmod 644 "$pkg/share/icons/hicolor/index.theme"
    printf '  index.theme, from this machine'\''s hicolor-icon-theme\n'
else
    printf '  index.theme NOT shipped -- hicolor-icon-theme is not installed here.\n'
    printf '     The installer will fall back to the target machine'\''s own copy.\n'
fi

# ---------------------------------------------------------------------------
step "copying the example programs"

# WHY EXAMPLES ARE IN A PACKAGE AT ALL: this one is for somebody learning the
# language, and a file type registered on a machine that holds no file of that
# type is a registration nobody ever sees. Everything in example/ goes, not just
# the .satl files -- corpus.txt and the .ini files are inputs that the programs
# beside them read, and a program shipped without its input is a program that
# fails the first time it is run.
mkdir -p "$pkg/example"
_examples=0
for _f in "$repo"/example/*; do
    [ -f "$_f" ] || continue
    cp -- "$_f" "$pkg/example/"
    _examples=$((_examples + 1))
done
chmod 644 "$pkg"/example/*
printf '  %s files\n' "$_examples"

# ---------------------------------------------------------------------------
step "writing the tarball"

# THE VERSION COMES OUT OF THE BINARY, not out of a variable here. `satl
# --version` prints what its objects were actually compiled as, so the name of
# the tarball cannot disagree with what is inside it.
#
# SATL_NO_WINDOW=1 because satl hands itself to satl-term when it finds no
# console, and a command substitution has a pipe for stdout, which is one of the
# cases that check gets subtle about. The variable is the documented escape
# hatch and costs nothing.
version=$(SATL_NO_WINDOW=1 "$repo/satl" --version 2>/dev/null | head -1 |
          sed -n 's/^satl \([0-9]*\) revision \([0-9]*\).*/\1r\2/p' || :)
[ -n "$version" ] || version=unknown

stem=satellite-$version-almalinux10-x86_64
tarball=$here/$stem.tar.xz

# xz AND NOT gzip, AND THE REASON IS THE UPLOAD AND NOT THE COMPRESSION.
#
# The package is handed over by putting it on a WordPress site, and WordPress
# refuses a .tar.gz upload -- its allowed-types list is a fixed allowlist keyed
# on extension and mime type, and gzip is not on it. So the author was
# recompressing every build by hand, which is a manual step in a script whose
# whole job is to not have manual steps. Found 2026-09-08, the first time this
# package was actually delivered to somebody.
#
# It is also simply smaller, which is worth having when the person downloading
# it is on a home connection: 4,177,217 bytes as .tar.gz against 2,129,664 as
# .tar.xz, for byte-identical contents. Roughly half, and the reason is that
# almost all of this package is two nearly identical 4MB static binaries --
# satl and satl.haswell are the same sources compiled twice -- and xz's much
# larger dictionary window finds that redundancy across the megabytes between
# them where gzip's 32KB window cannot see it at all.
#
# CHECKED RATHER THAN ASSUMED, because `tar -J` shells out to xz and a tar that
# cannot find it fails partway through writing the archive, leaving a truncated
# file with the right name. xz is on every AlmaLinux by way of rpm itself, so
# this should never fire; it costs one line and turns a corrupt output into a
# sentence.
command -v xz >/dev/null 2>&1 ||
    die "xz is not on this machine and this package is written with it.
       On AlmaLinux: dnf install xz"

# --transform SO THAT IT UNPACKS INTO ITS OWN DIRECTORY. A tarball that
# scatters its contents into whatever directory it was opened in is the one
# thing everybody who has ever used tar has been burned by, and the person this
# package is for should not have to know that it is a risk.
rm -f "$tarball"
tar -cJf "$tarball" \
    -C "$here" \
    --transform "s,^enterprise_satellite,$stem," \
    --owner=0 --group=0 \
    enterprise_satellite

printf '  %s\n' "$tarball"
printf '  %s bytes, unpacks into %s/\n' "$(stat -c%s "$tarball")" "$stem"

printf '\n%s: done.\n' "$me"
printf '  Copy that one file to the target machine, then:\n'
printf '      tar xf %s\n' "$stem.tar.xz"
printf '      cd %s\n' "$stem"
printf '      ./install-satellite.sh\n\n'
