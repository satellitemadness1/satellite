// Pack a list of header files into one blob for embedding in the binary.
//
// Run at build time.  argv[1] is a file holding one absolute path per line,
// argv[2] is the blob to write.  The blob is then turned into an object file
// with `ld -r -b binary` and linked into satellite, so the standard library
// headers a satellite.cxx block needs travel INSIDE the executable instead of
// being read off the machine it happens to be running on.
//
// Format -- deliberately the simplest thing that works, because it is read
// once at startup and never seeks:
//
//     "SATHDR01"                       8 bytes
//     repeated until end of file:
//         u32  path length
//         u8[] path, the ABSOLUTE path the file had when packed
//         u64  data length
//         u8[] data, followed by one NUL
//
// Paths are kept absolute and unchanged on purpose.  clang's driver computes
// its own include search list by probing for a GCC installation, and mounting
// the files where it already expects them means that list keeps working with no
// flags to keep in sync.  The virtual filesystem answers instead of the disk.
//
// The trailing NUL is what lets the loader hand clang a zero-copy buffer:
// llvm::MemoryBuffer wants a null-terminated region, and having it already in
// the blob avoids copying 7 MB of headers at startup to add one byte.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool read_file(const std::string& path, std::string* out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    *out = ss.str();
    return true;
}

template <typename T>
void put(std::ofstream& f, T v) {
    f.write(reinterpret_cast<const char*>(&v), sizeof v);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: pack_headers <list-file> <out.blob>\n");
        return 2;
    }

    std::ifstream list(argv[1]);
    if (!list) {
        std::fprintf(stderr, "pack_headers: cannot read %s\n", argv[1]);
        return 1;
    }

    std::ofstream out(argv[2], std::ios::binary);
    if (!out) {
        std::fprintf(stderr, "pack_headers: cannot write %s\n", argv[2]);
        return 1;
    }
    out.write("SATHDR01", 8);

    std::string line;
    size_t      packed = 0, bytes = 0, missing = 0;
    while (std::getline(list, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
                                 line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] != '/') continue;

        std::string data;
        if (!read_file(line, &data)) {
            // A path in the list that is not readable is reported and skipped
            // rather than failing the build: the dependency scan can name a
            // file that a later toolchain update removed, and a missing header
            // shows up as a clear "file not found" at block-compile time.
            std::fprintf(stderr, "pack_headers: skipping unreadable %s\n",
                         line.c_str());
            ++missing;
            continue;
        }

        put<uint32_t>(out, (uint32_t)line.size());
        out.write(line.data(), (std::streamsize)line.size());
        put<uint64_t>(out, (uint64_t)data.size());
        out.write(data.data(), (std::streamsize)data.size());
        out.put('\0');

        ++packed;
        bytes += data.size();
    }

    std::printf("pack_headers: %zu files, %.1f MB", packed, bytes / 1048576.0);
    if (missing) std::printf(" (%zu skipped)", missing);
    std::printf("\n");
    return 0;
}
