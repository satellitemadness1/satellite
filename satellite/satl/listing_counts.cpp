// The counting behind the prompt's directory table. See listing_counts.hpp.

#include "listing_counts.hpp"

#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace satellite004 {

namespace {

bool asked_to_stop(const volatile sig_atomic_t *stop)
{
    return stop != nullptr && *stop != 0;
}

struct CloseFolder {
    void operator()(DIR *folder) const { ::closedir(folder); }
};
using Folder = std::unique_ptr<DIR, CloseFolder>;

// THE TOP BIT OF EVERY BYTE OF `eight` THAT IS `c`, for eight bytes none of which
// has its top bit set: adding 0x7F to a byte under 0x80 carries into its top bit
// exactly when the byte is not zero, and never into the byte beside it.
std::uint64_t bytes_equal_to(std::uint64_t eight, unsigned char c)
{
    const std::uint64_t differs = eight ^ (0x0101010101010101ull * c);
    return ~(differs + 0x7F7F7F7F7F7F7F7Full) & 0x8080808080808080ull;
}

struct TextScan {
    unsigned long long int newlines = 0;
    bool after_a_return = false;   // the last byte seen was \r, so the next must be \n
};

// Scans bytes[0, size) for what makes a file NOT TEXT: a NUL, a \r not followed by
// \n, or bytes that are not UTF-8 -- strictly, Table 3-7 of the Unicode standard,
// the table conversion_loops.hpp's decode follows (no overlong form, no surrogate,
// nothing above U+10FFFF). Counts the \n it passes. Answers `size` when all of it
// is text; the offset of a character cut in half by the end of the bytes, when more
// bytes are coming to finish it; npos when it is not text.
std::size_t scan_text(const unsigned char *bytes, std::size_t size, bool at_the_end, TextScan &scan)
{
    constexpr std::size_t not_text = static_cast<std::size_t>(-1);
    std::size_t at = 0;
    while (at < size) {
        // EIGHT BYTES AT A TIME while they are plain ASCII with no NUL and no \r --
        // which is nearly every byte of nearly every text file.
        while (!scan.after_a_return && size - at >= 8) {
            std::uint64_t eight;
            std::memcpy(&eight, bytes + at, 8);
            if ((eight & 0x8080808080808080ull) != 0 || bytes_equal_to(eight, 0) != 0 ||
                bytes_equal_to(eight, '\r') != 0)
                break;
            scan.newlines += static_cast<unsigned long long int>(std::popcount(bytes_equal_to(eight, '\n')));
            at += 8;
        }
        if (at == size)
            break;

        const unsigned char first = bytes[at];
        if (scan.after_a_return) {
            if (first != '\n')
                return not_text;
            scan.after_a_return = false;
        }
        if (first < 0x80) {
            if (first == 0)
                return not_text;
            if (first == '\n')
                ++scan.newlines;
            else if (first == '\r')
                scan.after_a_return = true;
            ++at;
            continue;
        }
        if (first < 0xC2 || first > 0xF4)
            return not_text;
        const std::size_t length = first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
        if (size - at < length)
            return at_the_end ? not_text : at;
        const unsigned char low = first == 0xE0 ? 0xA0 : first == 0xF0 ? 0x90 : 0x80;
        const unsigned char high = first == 0xED ? 0x9F : first == 0xF4 ? 0x8F : 0xBF;
        if (bytes[at + 1] < low || bytes[at + 1] > high)
            return not_text;
        for (std::size_t k = 2; k < length; ++k)
            if ((bytes[at + k] & 0xC0) != 0x80)
                return not_text;
        at += length;
    }
    return size;
}

} // namespace

bool count_contents(const std::string &path, const volatile sig_atomic_t *stop, Contents &out)
{
    // EVERY OPEN DIRECTORY IS HELD BY WHAT CLOSES IT, so a stop, or an allocation
    // that fails part way down, closes all of them.
    std::vector<Folder> levels;
    const int top = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    Folder first(top >= 0 ? ::fdopendir(top) : nullptr);
    if (first == nullptr) {
        if (top >= 0)
            ::close(top);
        return true;
    }
    levels.push_back(std::move(first));
    out.opened = true;

    while (!levels.empty()) {
        if (asked_to_stop(stop))
            return false;
        DIR *const folder = levels.back().get();
        const bool directly = levels.size() == 1;
        errno = 0;   // readdir's null is both "ended" and "failed"; errno tells them apart
        const struct dirent *const entry = ::readdir(folder);
        if (entry == nullptr) {
            if (errno != 0) {
                out.whole = false;
                out.files_whole = out.files_whole && !directly;
            }
            levels.pop_back();
            continue;
        }
        const char *const leaf = entry->d_name;
        if (leaf[0] == '.' && (leaf[1] == '\0' || (leaf[1] == '.' && leaf[2] == '\0')))
            continue;

        bool directory = entry->d_type == DT_DIR;
        bool regular = entry->d_type == DT_REG;
        struct stat about {};
        if (regular || entry->d_type == DT_UNKNOWN) {
            if (::fstatat(::dirfd(folder), leaf, &about, AT_SYMLINK_NOFOLLOW) == 0) {
                directory = S_ISDIR(about.st_mode);
                regular = S_ISREG(about.st_mode);
            } else if (errno == ENOENT) {
                continue;
            } else {
                out.whole = false;
                out.files_whole = out.files_whole && !directly;   // it may even be a directory
                regular = false;
            }
        }

        if (!directory) {
            ++(directly ? out.files : out.below);
            if (regular)
                out.bytes += static_cast<unsigned long long int>(about.st_size);
            continue;
        }
        ++out.below;
        const int inner = ::openat(::dirfd(folder), leaf, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        Folder down(inner >= 0 ? ::fdopendir(inner) : nullptr);
        if (down != nullptr) {
            levels.push_back(std::move(down));
            continue;
        }
        const int why = errno;
        if (inner >= 0)
            ::close(inner);
        if (why != ENOENT)
            out.whole = false;
    }
    return true;
}

bool count_lines(const std::string &path, const volatile sig_atomic_t *stop, std::vector<unsigned char> &piece,
                 bool &text, unsigned long long int &lines)
{
    text = false;
    lines = 0;
    if (piece.empty())
        piece.resize(256 * 1024);
    const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOCTTY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return true;

    TextScan scan;
    std::size_t kept = 0;   // a character the last piece cut in half, moved to the front
    bool first_piece = true;
    bool any = false;       // anything after the byte-order mark
    unsigned char last = 0;
    for (;;) {
        if (asked_to_stop(stop)) {
            ::close(fd);
            return false;
        }
        std::size_t got = kept;
        bool ended = false;
        while (got < piece.size()) {
            const ssize_t count = ::read(fd, piece.data() + got, piece.size() - got);
            if (count < 0 && errno == EINTR)
                continue;
            if (count < 0) {
                ::close(fd);
                return true;
            }
            if (count == 0) {
                ended = true;
                break;
            }
            got += static_cast<std::size_t>(count);
        }
        std::size_t from = 0;
        if (first_piece) {
            first_piece = false;
            if (got >= 3 && std::memcmp(piece.data(), "\xEF\xBB\xBF", 3) == 0)
                from = 3;
        }
        const std::size_t scanned = scan_text(piece.data() + from, got - from, ended, scan);
        if (scanned == static_cast<std::size_t>(-1)) {
            ::close(fd);
            return true;
        }
        if (got > from) {
            any = true;
            last = piece[got - 1];
        }
        if (ended)
            break;
        kept = got - from - scanned;
        std::memmove(piece.data(), piece.data() + from + scanned, kept);
    }
    ::close(fd);
    if (scan.after_a_return)
        return true;
    text = true;
    lines = scan.newlines + (any && last != '\n' ? 1 : 0);
    return true;
}

} // namespace satellite004
