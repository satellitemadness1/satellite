// `read_line` `1 6 2 3`, `read_all` `1 6 2 5`, and the cursor they share --
// PLAN M19. Split from file_methods.cpp by SUBJECT: everything here is about
// one position in one file, which is the only thing M19 adds to v1's design and
// the only part of this module that carries state between two calls.
//
// v1 HAD NO CURSOR AND ITS REASON WAS SOUND. `.read()` seeks to 0 on every call,
// because with no `.seek()` in the language "read from wherever the offset
// happens to be" is a question no satellite program can pose. That reasoning
// does not reach `read_line`: DESIGN §8.7 fixes what it answers -- "a line, or
// nothing" -- and that sentence has no meaning at all on a handle that rewinds
// every call. So the cursor exists, it belongs to the language rather than to
// the kernel, and file_handle.hpp carries what it costs.

#include <unistd.h>

#include "error_reporter/report.hpp"
#include "satellite_file/file_internal.hpp"
#include "satellite_file/gzip.hpp"
#include "satellite_string/satellite_string.hpp"

#include <cerrno>
#include <cstring>
#include <string>

namespace satellite::file {

namespace {

// How much to read ahead. Not a limit on anything -- a line may be any length
// and `read_all` reads a file of any size; this is how many bytes one `pread`
// asks for, and the loop goes round again.
constexpr size_t kChunk = 65536;

} // namespace

Value as_text(const std::string &bytes)
{
    // encode_raw, NEVER encode. §5's escape expansion belongs to string
    // LITERALS: v1 found this defect twice, most damagingly inside its own
    // `.read()`, where reading a program containing `\home` rewrote it to the
    // reader's home directory before the lexer ever saw it. encode_raw maps one
    // byte to one code and decode maps it back, so every byte survives the round
    // trip and `satellite.file.open` on the result opens that same file.
    return Value::string(encode_raw(bytes));
}

bool fill(eval::Machine &m, FileHandle &handle, int fd)
{
    handle.buffer.resize(kChunk);
    handle.buffer_at = 0;
    for (;;) {
        // pread AND NEVER read, WHICH IS THE WHOLE OF THE CURSOR DESIGN. The
        // shared file offset belongs to the write side: "read_append" is
        // O_RDWR | O_APPEND, and every write moves that offset to the end of
        // the file. A read that rode on it would be silently reset by every
        // `write_line`, in the one mode whose entire purpose is writing and
        // reading back through one handle. pread takes its position as an
        // argument and moves nothing.
        // AND A GZIP STREAM CANNOT BE pread AT ALL, WHICH IS WHY THIS BRANCHES
        // RATHER THAN TAKING A DIFFERENT fd. Byte N of a compressed file is not
        // findable without inflating the N-1 before it, so there is no
        // positioned read to make -- the stream carries its own place, and
        // `read_at` goes on counting bytes DELIVERED, which is what the caller
        // above it means by a cursor. It stays exact because the only reader is
        // sequential: `read_line` never goes backwards.
        //
        // "read_gzip" IS READ-ONLY, so the paragraph below about the write side
        // stealing the shared offset cannot arise here -- there is no write
        // side to steal it.
        const ssize_t got =
            handle.gz != nullptr
                ? static_cast<ssize_t>(
                      gzip::read_some(handle.gz, handle.buffer.data(), kChunk))
                : ::pread(fd, handle.buffer.data(), kChunk, handle.read_at);

        // A DECOMPRESSION FAULT IS NOT AN errno, and saying so matters: the
        // file read perfectly and the bytes in it were not a gzip stream, or
        // were a damaged one. errno at this moment describes whatever syscall
        // last failed, which may be nothing to do with this file.
        if (got < 0 && handle.gz != nullptr) {
            const char *said = gzip::error_of(handle.gz);
            handle.buffer.clear();
            handle.last_error.store(EIO);
            m.refuse(errors::make<errors::Code::DIRECTORY_UNREADABLE>(
                m.span_of(m.here()), "\"" + handle.path + "\"",
                said != nullptr ? said : "the file is not a gzip stream"));
            return false;
        }
        if (got >= 0) {
            handle.buffer.resize(static_cast<size_t>(got));
            return true;
        }
        if (errno == EINTR)
            continue;
        handle.buffer.clear();
        handle.last_error.store(errno);
        m.refuse(errors::make<errors::Code::DIRECTORY_UNREADABLE>(
            m.span_of(m.here()), "\"" + handle.path + "\"",
            std::strerror(errno)));
        return false;
    }
}

// `read_line` `1 6 2 3` -- ONE LINE, OR NOTHING. DESIGN §8.7 fixes that:
// "`satellite.console.typed()` and `satellite.variable.file.read_line` answer a
// line, or nothing, assignable anywhere. The caller who handles the nothing case
// declares a `variant` and asks; the caller who declares a `string` has written
// a legal program that, at end of file, refuses by name at the line of first
// use. An empty line and nothing are two different values."
//
// THE NEWLINE IS NOT PART OF THE LINE. A reader that kept it would make every
// caller write `.trim()` and would make the last line of a file without a
// trailing newline a different shape from every other line.
bool file_read_line(eval::Machine &m, const Value *arguments, uint32_t,
                    Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    if (wrong_direction(m, *handle, handle->readable, "read",
                        modes_that_read()))
        return false;
    const int fd = descriptor_of(m, *handle);
    if (fd < 0)
        return false;

    std::string line;
    bool any = false;
    for (;;) {
        if (handle->buffer_at == handle->buffer.size()) {
            if (!fill(m, *handle, fd))
                return false;
            if (handle->buffer.empty()) {
                // End of file. NOTHING when there was nothing left at all;
                // the last line when the file did not end with a newline.
                *answer = any ? as_text(line) : Value::nothing();
                return true;
            }
        }
        const size_t at = handle->buffer.find('\n', handle->buffer_at);
        const size_t upto =
            at == std::string::npos ? handle->buffer.size() : at;
        line.append(handle->buffer, handle->buffer_at, upto - handle->buffer_at);
        any = true;
        const size_t consumed = upto - handle->buffer_at + (at != std::string::npos);
        handle->buffer_at += consumed;
        handle->read_at += static_cast<long long>(consumed);
        if (at != std::string::npos) {
            *answer = as_text(line);
            return true;
        }
    }
}

// `read_all` `1 6 2 5` -- the whole file, FROM THE BEGINNING, and the cursor is
// left at the end.
//
// THAT PAIR IS THE DECISION. Reading everything and then answering a `read_line`
// with the first line again would be two verbs disagreeing about one position;
// leaving the cursor where it was would make `read_all` mean "the whole file"
// and "and nothing happened", which is not true of a handle. So the two verbs
// share one cursor and this one moves it to the end -- a `read_line` after a
// `read_all` answers nothing, which is the honest report of a file that has been
// read.
bool file_read_all(eval::Machine &m, const Value *arguments, uint32_t,
                   Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    if (wrong_direction(m, *handle, handle->readable, "read",
                        modes_that_read()))
        return false;
    const int fd = descriptor_of(m, *handle);
    if (fd < 0)
        return false;

    handle->read_at = 0;
    handle->buffer.clear();
    handle->buffer_at = 0;

    std::string bytes;
    for (;;) {
        if (!fill(m, *handle, fd))
            return false;
        if (handle->buffer.empty())
            break;
        bytes += handle->buffer;
        handle->read_at += static_cast<long long>(handle->buffer.size());
    }
    handle->buffer.clear();
    handle->buffer_at = 0;
    *answer = as_text(bytes);
    return true;
}

} // namespace satellite::file
