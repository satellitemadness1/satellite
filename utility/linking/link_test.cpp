// utility/linking/link_test.cpp -- PROVE, FROM INSIDE THE BINARY, THAT EVERY VENDORED
// LIBRARY IS THE COPY WE VENDORED.
//
//     make link-test && ./build/link_test
//
// WHY THIS EXISTS. `make GTK=vendor` already refuses to ship a binary whose `readelf -d`
// names a library outside ALLOWED_NEEDED (make_support/050-build.mk). That gate is good
// and it is not enough, because it only sees the DYNAMIC section. Three real failures
// pass it:
//
//   1. THE WRONG COPY, LINKED STATICALLY. fontconfig 2.18.3 resolves expat like this
//      (fontconfig-2.18.3/meson.build:78-82):
//
//          xml_dep = dependency('expat', required: false)    // pkg-config: OUR expat
//          if not xml_dep.found()
//            xml_dep = cc.find_library('expat', ...)         // the linker: /usr's expat
//
//      cc.find_library() ASKS THE LINKER DIRECTLY AND IGNORES PKG_CONFIG_PATH, so the
//      isolation the whole build rests on does not cover it. Build fontconfig before
//      expat is staged and it takes the system's, quietly, and every test still passes.
//
//   2. THE WRONG HEADERS. Compile against /usr/include/expat.h (2.7.3) while linking our
//      libexpat.a (2.8.4) and nothing in the dynamic section is wrong. readelf cannot
//      see it. It is an ABI mismatch that shows up as a crash months later.
//
//   3. SOMETHING DLOPENED AT RUN TIME. readelf -d lists what the loader is TOLD to load.
//      It does not list what the process actually ends up holding.
//
// So this program does not read the build log, and it does not read the binary. IT ASKS
// THE RUNNING PROCESS, which is the only witness that cannot be mistaken about what it
// contains. The same argument tests/ makes about satl: assert on the screen, not on the
// bytes we hoped were written.
//
// WHAT IT CANNOT PROVE. That a library is free of defects, that the version we vendored
// was the right choice, or that a dlopen which has not happened yet will be clean. It
// proves identity and provenance, at this moment, for this binary. That is all, and it
// is the part that was silently wrong before.

#include <dlfcn.h>
#include <link.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// THE VENDORED STACK'S OWN HEADERS. Order matters in two places and both are upstream's
// rule, not ours: pcre2.h is a hard #error without PCRE2_CODE_UNIT_WIDTH set first
// (which is also what mangles pcre2_config into pcre2_config_8), and jpeglib.h needs
// FILE from <cstdio> and does not pull in jerror.h unless JPEG_INTERNALS is defined.
#define PCRE2_CODE_UNIT_WIDTH 8

#include <zlib.h>
#include <png.h>
#include <jpeglib.h>
#include <jerror.h>
#include <tiffio.h>
#include <expat.h>
#include <pcre2.h>
#include <ffi.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <fontconfig/fontconfig.h>
#include <hb.h>
#include <fribidi.h>
#include <pixman.h>
#include <cairo.h>
#include <graphene.h>
#include <glib.h>
#include <pango/pango.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <epoxy/common.h>
#include <xkbcommon/xkbcommon.h>

// Stringify a bare token. libjpeg-turbo needs it: LIBJPEG_TURBO_VERSION expands to the
// token sequence 3.2.0, which is not a valid C expression on its own.
#define SAT_STR2(x) #x
#define SAT_STR(x) SAT_STR2(x)

namespace
{

std::string sat_ver2(int a, int b) { return std::to_string(a) + "." + std::to_string(b); }
std::string sat_ver3(int a, int b, int c) { return sat_ver2(a, b) + "." + std::to_string(c); }

// fontconfig encodes its version as one int, major*10000 + minor*100 + micro.
std::string sat_fc_version(int v) { return sat_ver3(v / 10000, (v / 100) % 100, v % 100); }

// pcre2 writes "10.48 <release date>" rather than a bare version, which the substring
// match in versions_agree() handles. pcre2_config is a macro that mangles by code unit
// width; with PCRE2_CODE_UNIT_WIDTH 8 above it becomes pcre2_config_8.
std::string pcre2_runtime_version()
{
    char buffer[128] = {0};
    if (pcre2_config(PCRE2_CONFIG_VERSION, buffer) < 0) return "";
    return buffer;
}

// FT_Library_Version needs an initialised library -- it is the one version call in the
// stack that cannot be made cold, so it gets a helper rather than an inline expression.
std::string freetype_runtime_version()
{
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) return "";
    FT_Int major = 0, minor = 0, patch = 0;
    FT_Library_Version(library, &major, &minor, &patch);
    FT_Done_FreeType(library);
    return sat_ver3(major, minor, patch);
}

// glib and gtk publish their runtime version as exported VARIABLES and as three
// separate calls respectively, so neither is a single expression.
std::string glib_runtime_version()
{
    return sat_ver3(glib_major_version, glib_minor_version, glib_micro_version);
}

std::string gtk_runtime_version()
{
    return sat_ver3(gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version());
}

}   // namespace

// The libraries, as data. One row per library; see linked_libraries.def for the columns
// and for why each is there. Everything below is driven from that one list, so a library
// cannot be checked for its version and forgotten for its provenance.
namespace
{

struct library_row
{
    const char* name;
    const char* vendored;      // what vendor/new/ says we shipped
    std::string header;        // the version the HEADER claimed, at compile time
    std::string runtime;       // the version the LINKED code reports, right now
    void*       symbol;        // any real exported symbol, for dladdr()
};

// Shared objects this binary is allowed to hold. Mirrors ALLOWED_NEEDED in
// make_support/050-build.mk and must be changed in both places or not at all.
//
// libwayland-client and libwayland-egl ARE ALLOWED ON PURPOSE and must never be made
// static: GDK makes its wl_display with one copy and the GPU driver dlopens another,
// and wl_list_insert then walks lists the second copy never initialised. Measured, in
// both link modes -- so it is duplicate-library state, not the static-glibc hazard.
// linux-vdso is the kernel's own virtual object. Every Linux process has one, it is not
// on disk, and nothing links against it -- it is not a dependency in any sense that
// matters here. Leaving it out of this list is a false FAIL, which is how it got here.
const char* const allowed_objects[] = {
    "libc.so", "libm.so", "libresolv.so", "ld-linux", "libstdc++.so", "libgcc_s.so",
    "libwayland-client.so", "libwayland-egl.so", "libdl.so", "libpthread.so", "librt.so",
    "linux-vdso.so",
};

bool object_is_allowed(const char* path)
{
    if (path == nullptr || path[0] == '\0') return true;   // the main executable itself
    for (const char* allowed : allowed_objects)
        if (std::strstr(path, allowed) != nullptr) return true;
    return false;
}

// --- check A: what is this process actually holding? ------------------------------
//
// dl_iterate_phdr walks the loader's own list of mapped objects. This is STRONGER than
// `readelf -d` on the file, which lists only what the loader was ASKED for: a library
// arriving through a dependency of a dependency, or through a dlopen, appears here and
// does not appear there.
int collect_object(struct dl_phdr_info* info, size_t, void* user)
{
    auto* found = static_cast<std::vector<std::string>*>(user);
    found->emplace_back(info->dlpi_name != nullptr ? info->dlpi_name : "");
    return 0;
}

int check_loaded_objects()
{
    std::vector<std::string> found;
    dl_iterate_phdr(collect_object, &found);

    int bad = 0;
    std::printf("  objects mapped into this process: %zu\n", found.size());
    for (const std::string& path : found)
    {
        if (object_is_allowed(path.c_str())) continue;
        std::printf("  FAIL  not allowed to be here: %s\n", path.c_str());
        ++bad;
    }
    if (bad == 0) std::printf("  ok    every mapped object is in the allowed set\n");
    return bad;
}

// --- check B: did this symbol come from inside the binary? ------------------------
//
// THE DIRECT QUESTION, ASKED DIRECTLY. dladdr() reports the FILE a symbol was resolved
// from. Compiled into this executable, dli_fname is this executable. Resolved from a
// shared object, it is that object's path -- which is the leak, named, with the file
// that caused it.
// COMPARE LOAD ADDRESSES, NOT PATHS, and the first version of this function is the
// reason. It compared dladdr's dli_fname against readlink("/proc/self/exe"), which
// reports the RESOLVED absolute path while dladdr reports the path as invoked. Run the
// test as ./link_test and a symbol genuinely compiled into the binary is declared a leak
// -- a false FAIL, on the one check the whole utility exists for.
//
// dli_fbase is the load address of the object holding the symbol. Asking "is this symbol
// in the same object as this function?" needs no filesystem, no /proc, and no opinion
// about how the path is spelled.
bool symbol_is_compiled_in(void* symbol, std::string& where)
{
    static void* own_base = []() -> void* {
        Dl_info self{};
        // The address of a function defined HERE is by definition in the main executable.
        return dladdr(reinterpret_cast<void*>(&object_is_allowed), &self) != 0
                   ? self.dli_fbase : nullptr;
    }();

    Dl_info info{};
    if (symbol == nullptr) { where = "(no symbol available)"; return false; }
    if (dladdr(symbol, &info) == 0 || info.dli_fname == nullptr)
    {
        where = "(dladdr could not resolve it)";
        return false;
    }
    where = info.dli_fname;
    return own_base != nullptr && info.dli_fbase == own_base;
}

// --- check C and D: is it the version we vendored, and do header and library agree? --
//
// C catches the wrong copy. D catches the wrong HEADERS with the right copy, which
// nothing outside the process can see, because the dynamic section is correct in that
// case -- the mismatch is entirely between what the compiler was told and what the
// linker supplied.
bool versions_agree(const std::string& a, const std::string& b)
{
    if (a.empty() || b.empty()) return true;            // library publishes no such thing
    if (a == b) return true;
    // Some libraries render one side as "2.8.4" and the other as "expat_2.8.4".
    return a.find(b) != std::string::npos || b.find(a) != std::string::npos;
}

int check_row(const library_row& row)
{
    int bad = 0;

    std::string where;
    const bool inside = symbol_is_compiled_in(row.symbol, where);
    if (!inside)
    {
        std::printf("  FAIL  %-16s resolved from %s\n", row.name, where.c_str());
        std::printf("        expected it compiled INTO this binary. It is being taken\n");
        std::printf("        from outside, which is the leak this whole build prevents.\n");
        ++bad;
    }

    if (!row.runtime.empty() && !versions_agree(row.runtime, row.vendored))
    {
        std::printf("  FAIL  %-16s reports %s at run time, we vendored %s\n",
                    row.name, row.runtime.c_str(), row.vendored);
        std::printf("        a DIFFERENT COPY is linked in than the one in vendor/new/.\n");
        ++bad;
    }

    if (!row.header.empty() && !row.runtime.empty() && !versions_agree(row.header, row.runtime))
    {
        std::printf("  FAIL  %-16s header says %s, linked library says %s\n",
                    row.name, row.header.c_str(), row.runtime.c_str());
        std::printf("        compiled against one copy and linked against another --\n");
        std::printf("        an ABI mismatch that readelf can never see.\n");
        ++bad;
    }

    if (bad == 0)
        std::printf("  ok    %-16s %-12s compiled in, header and library agree\n",
                    row.name, row.runtime.empty() ? row.vendored : row.runtime.c_str());
    return bad;
}

}   // namespace

// The table. Built from ONE list so a row cannot be half-checked.
#define SAT_LINKED(ident, name, vendored, header_expr, runtime_expr, symbol_expr) \
    library_row{ name, vendored, header_expr, runtime_expr, symbol_expr },

static std::vector<library_row> linked_libraries()
{
    return {
#include "linked_libraries.def"
    };
}

#undef SAT_LINKED

int main()
{
    std::printf("satellite link test -- what is ACTUALLY in this binary\n");
    std::printf("-------------------------------------------------------------------\n");

    std::printf("\nA. objects mapped into this process\n");
    int bad = check_loaded_objects();

    std::printf("\nB/C/D. per library: compiled in, right version, header agrees\n");
    const std::vector<library_row> rows = linked_libraries();
    for (const library_row& row : rows) bad += check_row(row);

    std::printf("\n-------------------------------------------------------------------\n");
    if (bad == 0)
    {
        std::printf("PASS -- %zu libraries, all compiled in and all the vendored version.\n",
                    rows.size());
        return 0;
    }
    std::printf("FAIL -- %d problem%s across %zu libraries.\n",
                bad, bad == 1 ? "" : "s", rows.size());
    std::printf("A failure here means the binary is not self-contained, whatever the\n");
    std::printf("build said. vendor/README_FIRST.md and DEP-1 say why that matters.\n");
    return 1;
}
