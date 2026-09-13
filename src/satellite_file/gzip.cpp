// Reading a gzip stream. See satellite_file/gzip.hpp.

#include "satellite_file/gzip.hpp"

#include <zlib.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <cstddef>

namespace satellite::file::gzip {

bool looks_gzipped(int descriptor)
{
    if (descriptor < 0)
        return false;
    unsigned char magic[2] = {0, 0};
    const ssize_t peeked = ::pread(descriptor, magic, sizeof magic, 0);
    // A FILE SHORTER THAN TWO BYTES IS NOT GZIP AND IS NOT AN ERROR. An empty
    // file is what `satellite.file.new` just made, and answering false for it
    // is the honest answer rather than a failure to report.
    return peeked == 2 && magic[0] == 0x1f && magic[1] == 0x8b;
}

void *open_for_reading(int descriptor)
{
    // "rb" AND NOT "r", because zlib's text mode would translate line endings
    // on the platforms that have one. A WARC is bytes.
    return gzdopen(descriptor, "rb");
}

long read_some(void *stream, char *into, std::size_t many)
{
    if (stream == nullptr)
        return -1;
    // gzfread RATHER THAN gzread, which is the one call worth explaining. gzread
    // takes an unsigned INT and a read of more than 2GB is undefined through it;
    // gzfread takes size_t members and is the documented way to ask for a large
    // block. Nothing here asks for 2GB today -- the caller's buffer decides --
    // but a buffer size is the kind of constant that grows without anybody
    // rereading this function.
    //
    // AND THE THREAD WAKE IS HELD OFF FOR THE LENGTH OF THE READ -- THREAD.md
    // D15. satellite_thread/thread_handle.cpp wakes a thread being closed with
    // SIGUSR2 and no SA_RESTART, so a blocked read(2) comes back EINTR.
    // satellite's own reads retry that; vendored zlib's gz_load does not, and
    // calls it a stream error -- and drops the bytes it had already read in
    // that call, so retrying from out here could not be exact either. A gzip
    // handle is always a regular file (looks_gzipped needs pread, which a pipe
    // refuses), so this read ends on its own; the wake is left pending and
    // lands the moment the mask is restored, where the stop is noticed.
    sigset_t wake, before;
    sigemptyset(&wake);
    sigaddset(&wake, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &wake, &before);
    const z_size_t got = gzfread(into, 1, many, static_cast<gzFile>(stream));
    pthread_sigmask(SIG_SETMASK, &before, nullptr);
    if (got == 0 && gzeof(static_cast<gzFile>(stream)) == 0) {
        int problem = 0;
        gzerror(static_cast<gzFile>(stream), &problem);
        if (problem != Z_OK)
            return -1;
    }
    return static_cast<long>(got);
}

const char *error_of(void *stream)
{
    if (stream == nullptr)
        return nullptr;
    int problem = 0;
    const char *said = gzerror(static_cast<gzFile>(stream), &problem);
    return problem == Z_OK ? nullptr : said;
}

void close_stream(void *stream)
{
    if (stream != nullptr)
        gzclose(static_cast<gzFile>(stream));
}

} // namespace satellite::file::gzip
