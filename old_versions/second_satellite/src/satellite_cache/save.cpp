// Putting a `.satc` on the disk -- SATC.md §5. See satellite_cache/cache.hpp,
// and satellite_cache/write.cpp for the thing that produces the text this file
// stores. The two are deliberately different words: write.cpp writes a PROGRAM,
// this file writes a FILE.
//
// THE WRITE IS ATOMIC, AND THAT IS NOT CAUTION -- IT IS THE DIFFERENCE BETWEEN A
// CACHE AND A WAY TO CORRUPT A MACHINE. §5: "a process killed halfway through a
// plain write leaves a truncated `.satc` that the next run would read as a
// program." The next run would find a header that matched, a body that ended
// mid-statement, and would report the user's own program as malformed. So the
// bytes go to a temporary name, are forced to the disk, and are moved into place
// with rename(2) -- which is atomic within a directory, so a reader sees either
// the whole old file or the whole new one and never a half of either.
//
// AND A FAILED WRITE SAYS NOTHING. §5 again: "a read-only directory, a full
// disk, a source tree owned by somebody else -- none of these are the program's
// problem. The cache does not get written, nothing says anything, and the
// program runs." Every failure below returns false and every caller ignores it,
// which is why this file has no note and no error type. The one thing satl does
// talk about is §4's malformed FILE, and that is read.cpp's sentence to write:
// a file that is wrong is a fact about the machine, and a permission the user
// declined to give is not.
//
// THE DIRECTORY IS NOT fsync'd, DELIBERATELY. fsync on the file makes the bytes
// durable; fsync on the directory would make the RENAME durable, and skipping it
// means a machine that loses power in the next moment may come back with no
// cache entry. That is a cache miss, which §4 says is never an error and costs
// one walk -- so the second fsync would buy nothing this format cares about and
// would be paid on every write. What must never survive is a PARTIAL file, and
// the file's own fsync before the rename is what rules that out.

#include "satellite_cache/cache.hpp"

#include <cstdio>
#include <string>
#include <thread>
#include <utility>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace satellite::cache {

namespace {

// Everything above `path`, made if it is not there. Silent about a directory
// that already exists, which is the ordinary case and is not a failure.
void make_directories(const std::string &path)
{
    for (size_t at = 1; at < path.size(); at++)
        if (path[at] == '/')
            mkdir(path.substr(0, at).c_str(), 0755);
    mkdir(path.c_str(), 0755);
}

// The whole buffer, or false.
//
// A LOOP AND NOT ONE write(2), because a short write is legal and is not an
// error -- the kernel may take fewer bytes than it was offered and say so by
// returning the count. Written as a single call this would truncate a large
// `.satc` under exactly the conditions that make one large, and the truncation
// would then be fsync'd and renamed into place as if it were whole.
bool write_all(int fd, const std::string &text)
{
    size_t at = 0;
    while (at < text.size()) {
        const ssize_t put = write(fd, text.data() + at, text.size() - at);
        if (put <= 0)
            return false;
        at += static_cast<size_t>(put);
    }
    return true;
}

} // namespace

bool save(const std::string &cache_file, const std::string &text)
{
    if (cache_file.empty())
        return false;

    const size_t slash = cache_file.find_last_of('/');
    if (slash == std::string::npos)
        return false;
    make_directories(cache_file.substr(0, slash));

    // THE PID IS IN THE TEMPORARY NAME AND THAT IS WHAT MAKES TWO satls SAFE.
    // Two processes running the same program at once both write, and a shared
    // temporary name would have each one writing into the other's file before
    // either renamed. With a pid apiece they write two files and rename them
    // one after the other onto the same name, which is the case rename(2) is
    // atomic for: the loser's bytes are simply the ones that were replaced.
    const std::string temporary =
        cache_file + "." + std::to_string(getpid()) + ".tmp";

    const int fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return false;

    bool done = write_all(fd, text) && fsync(fd) == 0;
    if (close(fd) != 0)
        done = false;
    if (done && rename(temporary.c_str(), cache_file.c_str()) == 0)
        return true;

    // THE TEMPORARY IS REMOVED ON EVERY FAILURE PATH. A cache directory that
    // accumulates one `.tmp` per full disk is a second failure caused by the
    // first, and it is the kind that outlives whatever caused it.
    unlink(temporary.c_str());
    return false;
}

void Save::start(std::string cache_file, std::string text)
{
    if (cache_file.empty())
        return;
    thread_ = std::thread(
        [file = std::move(cache_file), body = std::move(text)]() {
            save(file, body);
        });
}

Save::~Save()
{
    if (thread_.joinable())
        thread_.join();
}

} // namespace satellite::cache
