// §5's write, on a real disk. See tests/satc_test/satc_test.hpp.
//
// IT TOUCHES A DISK, ONCE, IN /tmp, AND THAT IS ON PURPOSE. §5's write is
// tmp-fsync-rename, and the only way to know a rename happened is to look for
// the file; the only way to know the thread was joined rather than detached is
// to read what it wrote. A test of an atomic write that never writes is a test
// of the code's shape, and the two failures §5 exists to prevent -- a truncated
// file and a `.tmp` per run left behind -- are both invisible to one.

#include "satc_test.hpp"

#include "satellite_cache/cache.hpp"

#include <cstdio>
#include <string>

#include <unistd.h>

namespace satc_test {

namespace {

// The whole of a file, or false when there is not one.
//
// A THIRD READER IN THIS TREE AND THE ONLY ONE THAT WANTS A MISSING FILE. The
// harness's read_example() and programs/source_file.hpp both belong to callers
// for whom a file that is not there is something to report; here it is half of
// what is being checked -- the temporary must be GONE after a successful write.
bool read_whole(const std::string &path, std::string &into)
{
    into.clear();
    FILE *handle = fopen(path.c_str(), "rb");
    if (handle == nullptr)
        return false;
    char buffer[4096];
    size_t got = 0;
    while ((got = fread(buffer, 1, sizeof buffer, handle)) > 0)
        into.append(buffer, got);
    const bool whole = ferror(handle) == 0;
    fclose(handle);
    return whole;
}

// §5's WRITE, ON A REAL DISK. tmp, fsync, rename -- and what is checked is that
// the final name holds the whole file and that no temporary was left behind.
void the_write()
{
    const std::string directory =
        "/tmp/satc_test." + std::to_string(getpid());
    const std::string file = directory + "/round.satc";
    const std::string text = "satc 1\nnot really a program\n";

    check(satellite::cache::save(file, text),
          "save() makes the directory it was given and writes into it");

    std::string back;
    check(read_whole(file, back), "the file is at the name it was renamed onto");
    check(back == text, "and holds every byte it was given");

    // NO `.tmp` SURVIVES A SUCCESSFUL WRITE, which is the half of the atomic
    // rule a reader never sees and a directory listing does. A cache that left
    // one file per run beside each entry would fill a disk quietly.
    check(!read_whole(file + "." + std::to_string(getpid()) + ".tmp", back),
          "and the temporary it was written through is gone");

    // §5's THREAD, WHICH IS JOINED BY ITS DESTRUCTOR AND NOT DETACHED. The
    // scope below is the whole check: a detached writer would be racing this
    // read and would usually lose, because the process it belongs to is about
    // to do something else.
    const std::string second = directory + "/threaded.satc";
    {
        satellite::cache::Save writing;
        writing.start(second, text);
    }
    std::string threaded;
    check(read_whole(second, threaded) && threaded == text,
          "a `.satc` written on its own thread is on the disk once it is joined");

    unlink(file.c_str());
    unlink(second.c_str());
    rmdir(directory.c_str());
}

} // namespace

void section_writing()
{
    the_write();
}

} // namespace satc_test
