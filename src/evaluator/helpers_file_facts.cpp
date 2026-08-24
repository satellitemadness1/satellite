// What one file IS, for the listing that prints it: what its name says of its
// contents, what its mode bits grant, and when it came into being.
//
// Moved verbatim out of helpers.cpp, which was 779 lines, and split from
// helpers_listing.cpp so that neither half is a file nobody reads to the end.
// The reasoning for the type column -- including why glib is not linked to name
// a file type, which is the decision these tables exist because of -- is the
// comment at the top of helpers_listing.cpp, where the column is rendered.
//
// Part of src/evaluator/. See eval_internal.hpp for what these pieces share,
// and helpers_file_facts.hpp for why these five are no longer static.

#include "evaluator/eval_internal.hpp"

#include "evaluator/helpers_file_facts.hpp"

namespace satellite {

static bool ends_with_ci(const std::string &name, const char *suffix)
{
    const size_t n = strlen(suffix);
    if (name.size() < n)
        return false;
    for (size_t i = 0; i < n; i++) {
        char a = name[name.size() - n + i];
        if (a >= 'A' && a <= 'Z')
            a = static_cast<char>(a - 'A' + 'a');
        if (a != suffix[i])
            return false;
    }
    return true;
}

bool named_in(const std::string &name, const char *const *table)
{
    for (const char *const *p = table; *p; p++)
        if (ends_with_ci(name, *p))
            return true;
    return false;
}

// Archives. What is inside one is a question this listing does not open a file
// to answer, so it says what the thing IS and stops there.
const char *const COMPRESSED[] = {
    ".tar", ".gz", ".tgz", ".xz", ".txz", ".bz2", ".tbz", ".tbz2", ".zst",
    ".tzst", ".zip", ".7z", ".rar", ".lz", ".lz4", ".lzma", ".z", ".jar",
    ".war", ".apk", ".deb", ".rpm", ".cab", ".iso", ".dmg", ".whl", ".xpi",
    ".cpio", ".ar", ".shar", ".sit", ".arj", ".lzh", nullptr
};

// Text BY NAME, so that a file too big to read to the end, or one whose first
// megabyte happens to hold something unusual, is still called what it is.
// Nothing here is a guess about content: every one of these is a format whose
// entire point is that a person can read it. A .sh is listed here rather than
// under exe on purpose -- it carries the execute bit and it is still source.
const char *const TEXTUAL[] = {
    ".satl", ".txt", ".text", ".md", ".markdown", ".rst", ".log", ".csv",
    ".tsv", ".ini", ".cfg", ".conf", ".json", ".yaml", ".yml", ".toml",
    ".xml", ".html", ".htm", ".css", ".scss", ".less", ".svg", ".tex", ".bib",
    ".diff", ".patch", ".c", ".h", ".cc", ".hh", ".cpp", ".hpp", ".cxx",
    ".hxx", ".c++", ".h++", ".ipp", ".inl", ".m", ".mm", ".cs", ".java",
    ".js", ".mjs", ".cjs", ".ts", ".tsx", ".jsx", ".py", ".pyi", ".rb", ".pl",
    ".pm", ".php", ".go", ".rs", ".swift", ".kt", ".kts", ".scala", ".clj",
    ".cljs", ".lisp", ".el", ".scm", ".rkt", ".hs", ".ml", ".mli", ".ex",
    ".exs", ".erl", ".hrl", ".lua", ".r", ".jl", ".d", ".zig", ".nim", ".v",
    ".sh", ".bash", ".zsh", ".fish", ".ksh", ".csh", ".bat", ".cmd", ".ps1",
    ".vim", ".sql", ".asm", ".s", ".f", ".f90", ".f95", ".for", ".pas",
    ".vb", ".def", ".mk", ".cmake", ".gradle", ".spec", ".rules", ".desktop",
    ".service", ".in", ".am", ".ac", ".po", ".pot", ".gitignore", ".env",
    ".dockerfile", ".editorconfig", ".properties", ".gradle", ".sbt", nullptr
};

// rwxr-xr-x, the nine bits everyone can already read. The leading type
// character every ls prints is left off, because the type column beside this
// one says the same thing in a word.
std::string permission_bits(mode_t mode)
{
    static const char *const rwx[] = {"---", "--x", "-w-", "-wx",
                                      "r--", "r-x", "rw-", "rwx"};
    std::string out = rwx[(mode >> 6) & 7];
    out += rwx[(mode >> 3) & 7];
    out += rwx[mode & 7];
    return out;
}

// CREATED, where the kernel and the filesystem are willing to say: statx(2)'s
// birth time, which ext4, xfs and btrfs record and which stat(2) has never
// carried at all. Where it is missing -- an old kernel, or a filesystem that
// never kept it -- the row falls back to the modification time, because the
// nearest true thing beats an empty column, and because a listing whose date
// column is blank on half the rows is a listing nobody trusts on the other
// half. The two cannot be told apart from the row, which is the honest cost.
std::string created_on(const std::string &name, const struct stat &info)
{
    time_t when = info.st_mtime;
#if defined(__linux__) && defined(STATX_BTIME)
    struct statx sx;
    if (::statx(AT_FDCWD, name.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME,
                &sx) == 0 &&
        (sx.stx_mask & STATX_BTIME) != 0)
        when = static_cast<time_t>(sx.stx_btime.tv_sec);
#endif
    struct tm parts;
    if (!localtime_r(&when, &parts))
        return "";
    char out[32];
    if (strftime(out, sizeof out, "%Y-%m-%d %H:%M", &parts) == 0)
        return "";
    return out;
}

} // namespace satellite
