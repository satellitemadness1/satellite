// satellite/satellite_variable_file/satellite_file.cpp -- satellite.variable.file:
// a text file held as a list of lines. satellite_file.hpp says what each method
// answers; this file says how, and the few choices the header leaves open.
//
// NOTHING THROWS OUT OF HERE. Every public method catches what an allocation can
// throw and answers false (or 0, or "") with the code its kind of work uses:
// file_unreadable while reading, file_unwritable while saving or making, and
// `error` (1) for an edit in memory, which has no code of its own. Each edit makes
// its room before it changes anything, so one that fails part way changes nothing.
//
// A \r THAT ENDS NO LINE IS REFUSED ON THE WAY IN as well as on the way out
// (file_not_text): open_existing refuses a file holding one, so a handle that
// wrote one would make a file it could not read back.
//
// A READ-ONLY FILE STAYS READ-ONLY. A full rewrite renames a new file over the
// old one, which a writable folder allows whatever the file's own permissions
// say; so it asks first whether the file itself may be written, as the append
// path's open does.

#include "satellite_file.hpp"

#include "../machine/machine_codes.hpp"
#include "../satellite_variable_string/satellite_string.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace satellite004 {

namespace {

using Ending = satellite_file::Ending;

const char kByteOrderMark[] = "\xEF\xBB\xBF";
constexpr std::size_t kByteOrderMarkSize = 3;
constexpr std::size_t kWriteBuffer = 1024 * 1024;   // a full rewrite writes this much at a time

bool holds_a_nul(const std::string &path)
{
    return path.find('\0') != std::string::npos;
}

// A relative path made absolute against the working directory NOW. When there is
// none to be had (the folder was removed), the path stays as given.
std::string absolute(const std::string &path)
{
    if (path.empty() || path[0] == '/')
        return path;
    char *here = ::getcwd(nullptr, 0);
    if (here == nullptr)
        return path;
    std::string whole;
    try {
        whole = here;
    } catch (...) {
        std::free(here);
        throw;
    }
    std::free(here);
    if (whole.empty() || whole.back() != '/')
        whole += '/';
    return whole + path;
}

// strerror_r COMES IN TWO SHAPES: the GNU one answers the text, the XSI one fills
// the buffer. Whichever this library has, one of these two takes it.
[[maybe_unused]] const char *reason_text(const char *gnu, const char *) { return gnu; }
[[maybe_unused]] const char *reason_text(int xsi, const char *buffer) { return xsi == 0 ? buffer : "an unknown error"; }

std::string system_reason(int number)
{
    char buffer[256] = {};
    return reason_text(::strerror_r(number, buffer, sizeof buffer), buffer);
}

const char *what_it_is(mode_t mode)
{
    if (S_ISDIR(mode)) return "a directory";
    if (S_ISFIFO(mode)) return "a FIFO";
    if (S_ISCHR(mode)) return "a character device";
    if (S_ISBLK(mode)) return "a block device";
    if (S_ISSOCK(mode)) return "a socket";
    return "not a regular file";
}

const char *ending_bytes(Ending ending)
{
    switch (ending) {
    case Ending::lf: return "\n";
    case Ending::crlf: return "\r\n";
    case Ending::none: break;
    }
    return "";
}

std::size_t ending_size(Ending ending)
{
    return ending == Ending::crlf ? 2 : ending == Ending::lf ? 1 : 0;
}

// ROOM FOR `more`, growing by doubling. reserve(size + 1) on every append would
// allocate exactly one more each time and make a loop of appends quadratic.
template <typename Row>
void make_room(std::vector<Row> &rows, std::size_t more)
{
    const std::size_t needed = rows.size() + more;
    if (needed > rows.capacity())
        rows.reserve(std::max(needed, rows.capacity() * 2));
}

// TEXT IN -> LINES. '\n' ends a piece and a '\r' just before it is part of that
// ending; a '\n' at the very end ends the last piece and adds no empty one, so
// "a\n" is one line and "a\n\n" is "a" and ""; "" is one empty line. False, with
// nothing added, when a '\r' is not followed by '\n'.
bool split_text(const std::string &text, std::vector<std::string> &pieces)
{
    for (std::size_t r = text.find('\r'); r != std::string::npos; r = text.find('\r', r + 1))
        if (r + 1 >= text.size() || text[r + 1] != '\n')
            return false;
    pieces.clear();
    std::size_t start = 0;
    for (;;) {
        const std::size_t newline = text.find('\n', start);
        if (newline == std::string::npos) {
            pieces.emplace_back(text, start);
            return true;
        }
        std::size_t end = newline;
        if (end > start && text[end - 1] == '\r')
            --end;
        pieces.emplace_back(text, start, end - start);
        start = newline + 1;
        if (start == text.size())
            return true;
    }
}

bool all_ascii(const char *bytes, std::size_t size)
{
    unsigned char seen = 0;
    for (std::size_t k = 0; k < size; ++k)
        seen |= static_cast<unsigned char>(bytes[k]);
    return seen < 0x80;
}

std::string hex_byte(unsigned char byte)
{
    const char digits[] = "0123456789ABCDEF";
    return std::string("0x") + digits[byte >> 4] + digits[byte & 0x0F];
}

// Writes all of it; answers 0 or the errno.
int write_all(int fd, const char *bytes, std::size_t size)
{
    while (size > 0) {
        const ssize_t wrote = ::write(fd, bytes, size);
        if (wrote < 0) {
            if (errno == EINTR)
                continue;
            return errno;
        }
        if (wrote == 0)
            return EIO;
        bytes += wrote;
        size -= static_cast<std::size_t>(wrote);
    }
    return 0;
}

// A descriptor that closes itself when a read gives up part way (or throws).
class descriptor {
public:
    explicit descriptor(int fd) : fd_(fd) {}
    ~descriptor()
    {
        if (fd_ >= 0)
            ::close(fd_);
    }
    descriptor(const descriptor &) = delete;
    descriptor &operator=(const descriptor &) = delete;
    int get() const { return fd_; }

private:
    int fd_;
};

} // namespace

// ---------------------------------------------------------------------------
// THE BOOKKEEPING
// ---------------------------------------------------------------------------

void satellite_file::fail(signed long long int code, std::string reason) const
{
    code_ = code;
    error_ = std::move(reason);
}

void satellite_file::out_of_memory(signed long long int code) const noexcept
{
    code_ = code;
    try {
        error_ = "ran out of memory";   // short enough to need no allocation
    } catch (...) {
        error_.clear();
    }
}

// THE KIND IS ASKED FIRST: a line word on a binary file is wrong whether or not it
// is open, and file_has_no_lines is the code the interpreter stops on.
bool satellite_file::usable() const
{
    if (kind_ == Kind::binary) {
        fail(file_has_no_lines, path_ + " is a binary file, and a binary file has no lines");
        return false;
    }
    if (!open_) {
        fail(file_not_open, path_ + " is not open");
        return false;
    }
    return true;
}

void satellite_file::past_the_end(std::size_t n) const
{
    if (n == 0) {
        fail(line_past_the_end, "there is no line 0 in " + path_ + ": lines count from 1");
        return;
    }
    fail(line_past_the_end, "line " + std::to_string(n) + " is past the end of " + path_ + ", which has " +
                                std::to_string(lines_.size()) + (lines_.size() == 1 ? " line" : " lines"));
}

void satellite_file::edited()
{
    only_appended_ = false;
    changed_ = true;
}

void satellite_file::now_saved() noexcept
{
    saved_lines_ = lines_.size();
    only_appended_ = true;
    changed_ = false;
    last_line_needs_ending_on_disk_ = false;
    pending_bytes_ = 0;
    flush_after_ = kFlushBytes;
}

bool satellite_file::take_lines(const std::string &text, std::vector<std::string> &pieces) const
{
    if (split_text(text, pieces))
        return true;
    fail(file_not_text, "the text holds a \\r that does not end a line; 004 does not write lines that end in \\r "
                        "alone, since it could not read them back");
    return false;
}

// `pieces` go in at index `at`: over line `at` when replacing, before it when not.
// Each ends in the file's default ending but the last, which ends in last_ending.
void satellite_file::put_lines(std::size_t at, bool replacing, std::vector<std::string> &pieces, Ending last_ending)
{
    const std::size_t count = pieces.size();
    const std::size_t more = count - (replacing ? 1 : 0);
    make_room(lines_, more);
    make_room(endings_, more);
    // NOTHING BELOW ALLOCATES: the room is made, and strings move without throwing.
    std::size_t from = 0;
    if (replacing) {
        lines_[at].swap(pieces[0]);
        endings_[at] = default_ending_;
        from = 1;
    }
    const auto at_line = lines_.begin() + static_cast<std::ptrdiff_t>(at + from);
    lines_.insert(at_line, std::make_move_iterator(pieces.begin() + static_cast<std::ptrdiff_t>(from)),
                  std::make_move_iterator(pieces.end()));
    endings_.insert(endings_.begin() + static_cast<std::ptrdiff_t>(at + from), count - from, default_ending_);
    endings_[at + count - 1] = last_ending;
}

// ---------------------------------------------------------------------------
// MAKING AND OPENING
// ---------------------------------------------------------------------------

std::shared_ptr<satellite_file> satellite_file::make_new(const std::string &path, Kind kind)
{
    std::shared_ptr<satellite_file> handle;
    try {
        handle.reset(new satellite_file());
    } catch (...) {
        return nullptr;   // not even a handle could be made
    }
    try {
        handle->path_ = path;
        handle->kind_ = kind;
        if (holds_a_nul(path)) {
            handle->fail(path_holds_a_nul, "the path holds a NUL, which no file's name can: nothing was made");
            return handle;
        }
        if (kind == Kind::binary) {
            handle->fail(not_built_yet, "binary files are not built yet (SATELLITE_FILE_OPERATIONS FO-8)");
            return handle;
        }
        handle->where_ = absolute(path);
        const int fd = ::open(handle->where_.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOCTTY, 0666);
        if (fd < 0) {
            const int why = errno;
            if (why == EEXIST || why == EISDIR) {
                // NOTHING THERE IS TOUCHED: it is only looked at, to say what it is.
                struct stat there {};
                if (why == EISDIR || (::stat(handle->where_.c_str(), &there) == 0 && !S_ISREG(there.st_mode)))
                    handle->fail(not_a_file, path + " is " + (why == EISDIR ? "a directory" : what_it_is(there.st_mode)) +
                                                 ", not a file: nothing was made");
                else
                    handle->fail(file_already_there, "something is already at " + path +
                                                         ", and new never replaces it: nothing was made (open opens it)");
            } else {
                handle->fail(file_unwritable, "could not make " + path + ": " + system_reason(why));
            }
            return handle;
        }
        ::close(fd);
        handle->open_ = true;
        return handle;
    } catch (...) {
        handle->out_of_memory(file_unwritable);
        return handle;
    }
}

std::shared_ptr<satellite_file> satellite_file::open_existing(const std::string &path, Kind kind)
{
    std::shared_ptr<satellite_file> handle;
    try {
        handle.reset(new satellite_file());
    } catch (...) {
        return nullptr;
    }
    try {
        handle->path_ = path;
        handle->kind_ = kind;
        if (holds_a_nul(path)) {
            handle->fail(path_holds_a_nul, "the path holds a NUL, which no file's name can: nothing was opened");
            return handle;
        }
        if (kind == Kind::binary) {
            handle->fail(not_built_yet, "binary files are not built yet (SATELLITE_FILE_OPERATIONS FO-8)");
            return handle;
        }
        handle->where_ = absolute(path);
        handle->read_from_disk();
        return handle;
    } catch (...) {
        handle->open_ = false;
        handle->out_of_memory(file_unreadable);
        return handle;
    }
}

// STAT FIRST, AND NEVER OPEN WHAT IS NOT A REGULAR FILE: opening a FIFO waits for
// a writer. The open itself is O_NONBLOCK and is checked again with fstat, in case
// something else was put at the path between the two.
bool satellite_file::read_from_disk()
{
    open_ = false;
    struct stat about {};
    if (::stat(where_.c_str(), &about) != 0) {
        const int why = errno;
        if (why == ENOENT || why == ENOTDIR)
            fail(file_not_found, "no file is at " + path_ + " (open never makes one; new does)");
        else
            fail(file_unreadable, "could not read " + path_ + ": " + system_reason(why));
        return false;
    }
    if (!S_ISREG(about.st_mode)) {
        fail(not_a_file, path_ + " is " + what_it_is(about.st_mode) + ", not a file");
        return false;
    }
    const descriptor fd(::open(where_.c_str(), O_RDONLY | O_CLOEXEC | O_NOCTTY | O_NONBLOCK));
    if (fd.get() < 0) {
        const int why = errno;
        if (why == ENOENT || why == ENOTDIR)
            fail(file_not_found, "no file is at " + path_ + " (open never makes one; new does)");
        else
            fail(file_unreadable, "could not read " + path_ + ": " + system_reason(why));
        return false;
    }
    if (::fstat(fd.get(), &about) != 0 || !S_ISREG(about.st_mode)) {
        fail(not_a_file, path_ + " is not a regular file");
        return false;
    }

    // THE BYTES, WHOLE. One more than the size the file says, so the read that
    // finds the end needs no growing; a file that grows while it is read still
    // reads whole.
    std::string bytes;
    bytes.resize(static_cast<std::size_t>(about.st_size > 0 ? about.st_size : 0) + 1);
    std::size_t used = 0;
    for (;;) {
        if (used == bytes.size())
            bytes.resize(bytes.size() * 2);
        const ssize_t got = ::read(fd.get(), &bytes[used], bytes.size() - used);
        if (got == 0)
            break;
        if (got < 0) {
            const int why = errno;
            if (why == EINTR)
                continue;
            fail(file_unreadable, "could not read " + path_ + ": " + system_reason(why));
            return false;
        }
        used += static_cast<std::size_t>(got);
    }
    bytes.resize(used);

    // THE LINES. A byte-order mark at the very start is remembered and is no part of
    // line 1. '\n' ends a line; a '\r' just before it makes the ending \r\n; a '\r'
    // anywhere else refuses the file. The piece after the last '\n' is a last line
    // with no ending, when there is one.
    bool mark = false;
    std::size_t at = 0;
    if (bytes.size() >= kByteOrderMarkSize && bytes.compare(0, kByteOrderMarkSize, kByteOrderMark) == 0) {
        mark = true;
        at = kByteOrderMarkSize;
    }
    std::vector<std::string> lines;
    std::vector<Ending> endings;
    const std::size_t newlines = static_cast<std::size_t>(std::count(bytes.begin() + static_cast<std::ptrdiff_t>(at),
                                                                     bytes.end(), '\n'));
    lines.reserve(newlines + 1);
    endings.reserve(newlines + 1);

    satellite_string scratch;   // reused, so checking a line allocates only when a line is longer than any before
    std::size_t number = 0;
    while (at < bytes.size()) {
        ++number;
        const std::size_t newline = bytes.find('\n', at);
        std::size_t end = newline == std::string::npos ? bytes.size() : newline;
        Ending ending = newline == std::string::npos ? Ending::none : Ending::lf;
        if (ending == Ending::lf && end > at && bytes[end - 1] == '\r') {
            --end;
            ending = Ending::crlf;
        }
        const void *lone = std::memchr(bytes.data() + at, '\r', end - at);
        if (lone != nullptr) {
            const std::size_t offset = static_cast<std::size_t>(static_cast<const char *>(lone) - (bytes.data() + at));
            fail(file_not_text, path_ + " is not text 004 reads: line " + std::to_string(number) +
                                    " holds a \\r that is not followed by \\n (offset " + std::to_string(offset) +
                                    " in the line); 004 does not read files whose lines end in \\r alone");
            return false;
        }
        std::string text(bytes, at, end - at);
        if (!all_ascii(text.data(), text.size())) {
            std::size_t bad = 0;
            if (satellite_string::from_utf8(text, scratch, bad) != success) {
                fail(file_not_text, path_ + " is not UTF-8 text: line " + std::to_string(number) + ", the byte " +
                                        hex_byte(static_cast<unsigned char>(text[bad])) + " at offset " +
                                        std::to_string(bad) + " in the line (offset " + std::to_string(at + bad) +
                                        " in the file), starts no valid UTF-8 character");
                return false;
            }
        }
        lines.push_back(std::move(text));
        endings.push_back(ending);
        at = newline == std::string::npos ? bytes.size() : newline + 1;
    }

    Ending first_ending = Ending::lf;
    for (const Ending ending : endings)
        if (ending != Ending::none) {
            first_ending = ending;
            break;
        }

    // NOTHING BELOW THROWS: the handle takes the new state whole.
    lines_.swap(lines);
    endings_.swap(endings);
    byte_order_mark_ = mark;
    default_ending_ = first_ending;
    now_saved();
    open_ = true;
    return true;
}

bool satellite_file::exists(const std::string &path)
{
    if (holds_a_nul(path))
        return false;
    struct stat about {};
    return ::stat(path.c_str(), &about) == 0 && S_ISREG(about.st_mode);
}

signed long long int satellite_file::clear(const std::string &path, std::string &reason)
{
    try {
        reason.clear();
        if (holds_a_nul(path)) {
            reason = "the path holds a NUL, which no file's name can: nothing was emptied";
            return path_holds_a_nul;
        }
        struct stat about {};
        if (::stat(path.c_str(), &about) != 0) {
            const int why = errno;
            if (why == ENOENT || why == ENOTDIR) {
                reason = "no file is at " + path + " (clear never makes one)";
                return file_not_found;
            }
            reason = "could not empty " + path + ": " + system_reason(why);
            return file_unwritable;
        }
        if (!S_ISREG(about.st_mode)) {
            reason = path + " is " + what_it_is(about.st_mode) + ", not a file";
            return not_a_file;
        }
        const descriptor fd(::open(path.c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC | O_NOCTTY | O_NONBLOCK));
        if (fd.get() < 0) {
            const int why = errno;
            reason = "could not empty " + path + ": " + system_reason(why);
            return file_unwritable;
        }
        if (::fstat(fd.get(), &about) != 0 || !S_ISREG(about.st_mode)) {
            reason = path + " is not a regular file";
            return not_a_file;
        }
        return success;
    } catch (...) {
        reason.clear();
        try {
            reason = "ran out of memory";
        } catch (...) {
        }
        return file_unwritable;
    }
}

satellite_file::~satellite_file()
{
    if (open_)
        (void)close();
}

// ---------------------------------------------------------------------------
// THE LIST OF LINES
// ---------------------------------------------------------------------------

std::size_t satellite_file::size() const
{
    try {
        return usable() ? lines_.size() : 0;
    } catch (...) {
        out_of_memory(satellite004::error);
        return 0;
    }
}

bool satellite_file::line(std::size_t n, std::string &out) const
{
    try {
        if (!usable()) {
            out.clear();
            return false;
        }
        if (n == 0 || n > lines_.size()) {
            out.clear();
            past_the_end(n);
            return false;
        }
        out = lines_[n - 1];
        return true;
    } catch (...) {
        out.clear();
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::append(const std::string &text)
{
    try {
        if (!usable())
            return false;
        const std::size_t before = lines_.size();
        std::size_t added = 0;   // the bytes the new lines take on the disk
        if (text.find('\n') == std::string::npos && text.find('\r') == std::string::npos) {
            // THE LIKELY CASE, ONE LINE, with no list of pieces to make.
            make_room(lines_, 1);
            make_room(endings_, 1);
            std::string copy(text);
            lines_.push_back(std::move(copy));
            endings_.push_back(default_ending_);
            added = text.size() + ending_size(default_ending_);
        } else {
            std::vector<std::string> pieces;
            if (!take_lines(text, pieces))
                return false;
            for (const std::string &piece : pieces)
                added += piece.size() + ending_size(default_ending_);
            put_lines(before, false, pieces, default_ending_);
        }
        // A LAST LINE WITH NO NEWLINE IS ENDED FIRST, so the new line is a line of its
        // own. When that line is already on the disk, the next append-only save writes
        // its ending before the new lines.
        if (before > 0 && endings_[before - 1] == Ending::none) {
            endings_[before - 1] = default_ending_;
            added += ending_size(default_ending_);
            if (before == saved_lines_)
                last_line_needs_ending_on_disk_ = true;
        }
        changed_ = true;
        if (only_appended_) {
            pending_bytes_ += added;
            // THE STREAM. The line is held whatever the write answers, so append is
            // still true; a failed write is kept in code() and error().
            if (pending_bytes_ > flush_after_ && !save_appended())
                flush_after_ = pending_bytes_ + kFlushBytes;
        }
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::insert(std::size_t n, const std::string &text)
{
    try {
        if (!usable())
            return false;
        if (n == lines_.size() + 1)
            return append(text);
        if (n == 0 || n > lines_.size()) {
            fail(line_past_the_end, "insert: " + std::to_string(n) + " is no place for a line in " + path_ +
                                        ", which has " + std::to_string(lines_.size()) + " lines: 1 to " +
                                        std::to_string(lines_.size() + 1) + " are");
            return false;
        }
        std::vector<std::string> pieces;
        if (!take_lines(text, pieces))
            return false;
        put_lines(n - 1, false, pieces, default_ending_);
        edited();
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::replace_line(std::size_t n, const std::string &text)
{
    try {
        if (!usable())
            return false;
        if (n == 0 || n > lines_.size()) {
            past_the_end(n);
            return false;
        }
        if (text.find('\n') == std::string::npos && text.find('\r') == std::string::npos) {
            if (lines_[n - 1] == text)
                return true;   // the same text: nothing to save
            std::string copy(text);
            lines_[n - 1].swap(copy);
            edited();
            return true;
        }
        // SEVERAL LINES: the last keeps line n's own ending, the rest take the file's.
        std::vector<std::string> pieces;
        if (!take_lines(text, pieces))
            return false;
        put_lines(n - 1, true, pieces, endings_[n - 1]);
        edited();
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::replace_text(const std::string &from, const std::string &to)
{
    try {
        if (!usable())
            return false;
        if (from.empty()) {
            fail(empty_search_text, "replace: the text to replace is empty, so there is nothing to replace");
            return false;
        }
        std::size_t which = 0;
        std::size_t where = std::string::npos;
        for (; which < lines_.size(); ++which) {
            where = lines_[which].find(from);
            if (where != std::string::npos)
                break;
        }
        if (where == std::string::npos) {
            fail(text_not_found, "replace: the text to replace is in no line of " + path_);
            return false;
        }
        if (from == to)
            return true;   // found, and replacing it changes nothing
        std::string changed = lines_[which];
        changed.replace(where, from.size(), to);   // once: what `to` brings is never looked at again
        if (to.find('\n') == std::string::npos && to.find('\r') == std::string::npos) {
            lines_[which].swap(changed);
            edited();
            return true;
        }
        // `to` HOLDS A NEWLINE, so the line becomes several, as replace_line makes them.
        std::vector<std::string> pieces;
        if (!take_lines(changed, pieces))
            return false;
        put_lines(which, true, pieces, endings_[which]);
        edited();
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

std::size_t satellite_file::index_of(const std::string &text) const
{
    try {
        if (!usable())
            return 0;
        for (std::size_t k = 0; k < lines_.size(); ++k)
            if (lines_[k] == text)
                return k + 1;
        return 0;
    } catch (...) {
        out_of_memory(satellite004::error);
        return 0;
    }
}

std::size_t satellite_file::search(const std::string &text) const
{
    try {
        if (!usable() || text.empty())
            return 0;
        for (std::size_t k = 0; k < lines_.size(); ++k)
            if (lines_[k].find(text) != std::string::npos)
                return k + 1;
        return 0;
    } catch (...) {
        out_of_memory(satellite004::error);
        return 0;
    }
}

bool satellite_file::remove_at(std::size_t n)
{
    try {
        if (!usable())
            return false;
        if (n == 0 || n > lines_.size()) {
            past_the_end(n);
            return false;
        }
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(n - 1));
        endings_.erase(endings_.begin() + static_cast<std::ptrdiff_t>(n - 1));
        edited();
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::remove(const std::string &text)
{
    try {
        if (!usable())
            return false;
        const std::size_t n = index_of(text);
        if (n == 0) {
            fail(text_not_found, "remove: no line of " + path_ + " is exactly that text");
            return false;
        }
        return remove_at(n);
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::truncate(std::size_t n)
{
    try {
        if (!usable())
            return false;
        if (n >= lines_.size())
            return true;
        lines_.resize(n);
        endings_.resize(n);
        edited();
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

bool satellite_file::clear_lines()
{
    try {
        if (!usable())
            return false;
        if (lines_.empty())
            return true;
        lines_.clear();
        endings_.clear();
        edited();
        return true;
    } catch (...) {
        out_of_memory(satellite004::error);
        return false;
    }
}

std::string satellite_file::read_all() const
{
    try {
        if (!usable())
            return std::string();
        std::size_t total = byte_order_mark_ ? kByteOrderMarkSize : 0;
        for (std::size_t k = 0; k < lines_.size(); ++k)
            total += lines_[k].size() + ending_size(endings_[k]);
        std::string all;
        all.reserve(total);
        if (byte_order_mark_)
            all.append(kByteOrderMark, kByteOrderMarkSize);
        for (std::size_t k = 0; k < lines_.size(); ++k) {
            all += lines_[k];
            all += ending_bytes(endings_[k]);
        }
        return all;
    } catch (...) {
        out_of_memory(satellite004::error);
        return std::string();
    }
}

// ---------------------------------------------------------------------------
// THE DISK
// ---------------------------------------------------------------------------

bool satellite_file::save()
{
    try {
        if (!open_) {
            fail(file_not_open, "save: " + path_ + " is not open");
            return false;
        }
    } catch (...) {
        out_of_memory(file_not_open);
        return false;
    }
    if (!changed_)
        return true;
    return only_appended_ ? save_appended() : save_whole();
}

// THE FAST PATH: only what was appended since the last save (and the ending of the
// disk's old last line, when it had none), added to the end. A write that fails
// part way is cut back off, so the next save starts from the same place.
bool satellite_file::save_appended() noexcept
{
    int fd = -1;
    try {
        std::string bytes;
        bytes.reserve(pending_bytes_);
        if (last_line_needs_ending_on_disk_ && saved_lines_ > 0)
            bytes += ending_bytes(endings_[saved_lines_ - 1]);
        for (std::size_t k = saved_lines_; k < lines_.size(); ++k) {
            bytes += lines_[k];
            bytes += ending_bytes(endings_[k]);
        }
        fd = ::open(where_.c_str(), O_WRONLY | O_APPEND | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
            const int why = errno;
            fail(file_unwritable, "could not add to " + path_ + ": " + system_reason(why));
            return false;
        }
        struct stat held {};
        if (::fstat(fd, &held) != 0 || !S_ISREG(held.st_mode)) {
            ::close(fd);
            fd = -1;
            fail(file_unwritable, "could not add to " + path_ + ": it is no longer a regular file");
            return false;
        }
        int why = write_all(fd, bytes.data(), bytes.size());
        if (why != 0) {
            if (::ftruncate(fd, held.st_size) != 0) {
                // the reason given is the write's; the file may keep a part of the new bytes
            }
            ::close(fd);
            fd = -1;
            fail(file_unwritable, "could not add to " + path_ + ": " + system_reason(why));
            return false;
        }
        const int closed = ::close(fd);
        why = errno;
        fd = -1;
        if (closed != 0 && why != EINTR) {
            if (::truncate(where_.c_str(), held.st_size) != 0) {
                // as above
            }
            fail(file_unwritable, "could not add to " + path_ + ": " + system_reason(why));
            return false;
        }
        now_saved();
        return true;
    } catch (...) {
        if (fd >= 0)
            ::close(fd);
        out_of_memory(file_unwritable);
        return false;
    }
}

// THE WHOLE TEXT, WHOLE OR NOT AT ALL: written to .<name>.saving.<pid> beside the
// real file (the target, when the path is a symlink, so the link stays a link),
// fsync'd, given the old file's permissions and renamed over it. Any failure
// removes the new copy and leaves the old file as it was.
bool satellite_file::save_whole() noexcept
{
    std::string temp;
    bool made = false;
    int fd = -1;
    try {
        auto give_up = [&](const std::string &reason) {
            if (fd >= 0)
                ::close(fd);
            fd = -1;
            if (made)
                ::unlink(temp.c_str());
            made = false;
            fail(file_unwritable, reason);
            return false;
        };

        std::string real;
        {
            char *resolved = ::realpath(where_.c_str(), nullptr);
            if (resolved == nullptr) {
                const int why = errno;
                return give_up("could not save " + path_ + ": " + system_reason(why));
            }
            try {
                real = resolved;
            } catch (...) {
                std::free(resolved);
                throw;
            }
            std::free(resolved);
        }
        struct stat about {};
        if (::stat(real.c_str(), &about) != 0) {
            const int why = errno;
            return give_up("could not save " + path_ + ": " + system_reason(why));
        }
        if (!S_ISREG(about.st_mode))
            return give_up("could not save " + path_ + ": it is no longer a regular file");
        if (::faccessat(AT_FDCWD, real.c_str(), W_OK, AT_EACCESS) != 0) {
            const int why = errno;
            return give_up("could not save " + path_ + ": " + system_reason(why));
        }

        const std::size_t slash = real.rfind('/');   // realpath answers an absolute path
        temp = real.substr(0, slash + 1) + "." + real.substr(slash + 1) + ".saving." + std::to_string(::getpid());
        fd = ::open(temp.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOCTTY, 0600);
        if (fd < 0) {
            const int why = errno;
            return give_up("could not save " + path_ + ": could not make " + temp + " beside it: " + system_reason(why));
        }
        made = true;

        std::string buffer;
        buffer.reserve(kWriteBuffer);
        if (byte_order_mark_)
            buffer.append(kByteOrderMark, kByteOrderMarkSize);
        for (std::size_t k = 0; k < lines_.size(); ++k) {
            buffer += lines_[k];
            buffer += ending_bytes(endings_[k]);
            if (buffer.size() >= kWriteBuffer) {
                const int why = write_all(fd, buffer.data(), buffer.size());
                if (why != 0)
                    return give_up("could not save " + path_ + ": " + system_reason(why));
                buffer.clear();
            }
        }
        int why = write_all(fd, buffer.data(), buffer.size());
        if (why != 0)
            return give_up("could not save " + path_ + ": " + system_reason(why));
        if (::fchmod(fd, about.st_mode & 07777) != 0) {
            why = errno;
            return give_up("could not save " + path_ + ": could not give the new copy the file's permissions: " +
                           system_reason(why));
        }
        // THE OWNER TOO, WHEN IT CAN BE KEPT (only root can give a file away); a copy
        // that cannot is still the user's own file, as any editor's would be.
        struct stat mine {};
        if (::fstat(fd, &mine) == 0 && (mine.st_uid != about.st_uid || mine.st_gid != about.st_gid)) {
            if (::fchown(fd, about.st_uid, about.st_gid) != 0) {
                // best effort, as above
            }
        }
        if (::fsync(fd) != 0) {
            why = errno;
            return give_up("could not save " + path_ + ": " + system_reason(why));
        }
        const int closed = ::close(fd);
        why = errno;
        fd = -1;
        if (closed != 0 && why != EINTR)
            return give_up("could not save " + path_ + ": " + system_reason(why));
        if (::rename(temp.c_str(), real.c_str()) != 0) {
            why = errno;
            return give_up("could not save " + path_ + ": " + system_reason(why));
        }
        made = false;

        // THE RENAME ITSELF, TO THE DISK. Best effort: the new file is in place.
        const std::string folder = slash == 0 ? std::string("/") : real.substr(0, slash);
        const descriptor directory(::open(folder.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
        if (directory.get() >= 0 && ::fsync(directory.get()) != 0) {
            // the save has landed; a folder that cannot be synced changes nothing here
        }
        now_saved();
        return true;
    } catch (...) {
        if (fd >= 0)
            ::close(fd);
        if (made)
            ::unlink(temp.c_str());
        out_of_memory(file_unwritable);
        return false;
    }
}

bool satellite_file::close()
{
    if (!open_)
        return true;
    if (!save())
        return false;   // STILL OPEN, with the changes held, so a later close can try again
    open_ = false;
    std::vector<std::string>().swap(lines_);   // the memory goes; reopen reads the disk again
    std::vector<Ending>().swap(endings_);
    return true;
}

bool satellite_file::reopen()
{
    if (open_)
        return true;
    try {
        if (holds_a_nul(path_)) {
            fail(path_holds_a_nul, "the path holds a NUL, which no file's name can: nothing was opened");
            return false;
        }
        if (kind_ == Kind::binary) {
            fail(not_built_yet, "binary files are not built yet (SATELLITE_FILE_OPERATIONS FO-8)");
            return false;
        }
        if (!read_from_disk())
            return false;
        code_ = success;
        error_.clear();
        return true;
    } catch (...) {
        open_ = false;
        out_of_memory(file_unreadable);
        return false;
    }
}

} // namespace satellite004
