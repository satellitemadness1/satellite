// Reading a gzip stream. See satellite_file/gzip.hpp.

#include "satellite_file/gzip.hpp"

#include <zlib.h>

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
    const z_size_t got = gzfread(into, 1, many, static_cast<gzFile>(stream));
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
