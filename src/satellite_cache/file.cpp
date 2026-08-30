// Where a `.satc` lives, and what a source's identity is. See
// satellite_cache/cache.hpp.
//
// ONE DIRECTORY AND NOT TWO, DECIDED BY THE AUTHOR ON 2026-08-30. SATC.md §6
// asked where a `.satc` goes when the source directory is not writable and
// listed three answers: beside the source, under $HOME, or both. Both is the
// one the bullet itself names as bad -- "two places to look and a rule about
// which wins" -- and beside-the-source cannot be the only answer, because a
// program run out of /usr/share or a read-only checkout would never get a cache
// at all. So every `.satc` on this machine is under $HOME/.satl/cache, and
// SATC.md §1's opening sentence was corrected in the same edit rather than left
// saying a `.satc` sits beside its source.
//
// WHAT THAT COSTS IS A NAME COLLISION, and it is paid here rather than
// discovered later: two programs called `hello_world.satl` in two directories
// are two different sources and must not share a file. The name carries a
// digest of the source's ABSOLUTE path for that, and the basename in front of
// it so the directory can be read by a person -- §1.1's argument about the
// comment column, applied to the file listing.
//
// AND THE HEADER IS STILL CHECKED. The digest makes a collision unlikely; §2's
// `source <name> <mtime> <size>` line is what makes one harmless. A reader that
// opened the wrong file finds a name that does not match and treats it as a
// miss, which is the same answer it gives a stale one.

#include "satellite_cache/cache.hpp"

#include <cstdint>
#include <cstdlib>
#include <string>

#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace satellite::cache {

namespace {

// FNV-1a over a string, which is what names the file.
//
// NOT words_digest.hpp's FUNCTION, AND THAT IS NOT A SECOND COPY OF ONE FACT.
// That one is `constexpr` and answers "is this the same NUMBERING"; this one
// runs at run time and answers "which SOURCE is this". They share an algorithm
// and nothing else -- neither would change if the other did -- and reaching
// into another module's `detail` namespace to save four lines would make a
// change to the numbering's identity able to rename every cache file on the
// machine.
uint64_t digest_of(const std::string &text)
{
    uint64_t hash = 1469598103934665603ull;
    for (const char c : text) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ull;
    }
    return hash;
}

std::string hex_of(uint64_t value)
{
    static const char kHex[] = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 0; i < 16; i++)
        out[15 - i] = kHex[(value >> (i * 4)) & 0xf];
    return out;
}

// The last path segment, with a trailing `.satl` removed.
std::string stem_of(const std::string &path)
{
    const size_t slash = path.find_last_of('/');
    std::string name =
        slash == std::string::npos ? path : path.substr(slash + 1);
    const std::string suffix = ".satl";
    if (name.size() > suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
        name.resize(name.size() - suffix.size());
    return name;
}

} // namespace

std::string cache_directory()
{
    // NO HOME IS NO CACHE, AND THAT IS NOT AN ERROR. SATC.md §5 says a failed
    // write is silent and harmless -- "a read-only directory, a full disk, a
    // source tree owned by somebody else -- none of these are the program's
    // problem" -- and a process started without HOME is the same class of
    // fact about the machine. The program runs; it just runs from source.
    const char *home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
        return std::string();
    return std::string(home) + "/.satl/cache";
}

std::string cache_path(const std::string &source_path)
{
    const std::string directory = cache_directory();
    if (directory.empty())
        return std::string();

    // THE ABSOLUTE PATH AND NOT THE ONE THAT WAS TYPED. `satl hello_world.satl`
    // and `satl ./hello_world.satl` and `satl ~/code/hello_world.satl` are one
    // source, and hashing what the user typed would give that source three
    // cache files -- two of which are written on every run and read on none.
    char resolved[PATH_MAX];
    const char *absolute = realpath(source_path.c_str(), resolved);
    const std::string identity = absolute != nullptr ? std::string(absolute)
                                                     : source_path;
    return directory + "/" + stem_of(source_path) + "." +
           hex_of(digest_of(identity)) + ".satc";
}

bool stamp(const std::string &source_path, Source &into)
{
    struct stat facts;
    if (stat(source_path.c_str(), &facts) != 0)
        return false;

    // THE MTIME IS WHOLE SECONDS, DELIBERATELY. st_mtim.tv_nsec is available
    // and is not used: the header is compared as TEXT by a reader that has not
    // parsed it yet (words_digest.hpp makes the same argument for a fixed-width
    // digest), and a filesystem that does not carry sub-second times -- an old
    // ext3, a FAT stick, a network mount -- would then write a nanosecond field
    // that reads back as a different value and miss on every run. A second is
    // the resolution every filesystem here agrees on, and §2's `size` is what
    // catches an edit made inside one.
    into.name = stem_of(source_path) + ".satl";
    into.mtime = static_cast<uint64_t>(facts.st_mtime);
    into.size = static_cast<uint64_t>(facts.st_size);
    return true;
}

} // namespace satellite::cache
