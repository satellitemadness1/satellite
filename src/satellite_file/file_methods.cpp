// The ten rows under `satellite.variable.file` `1 6 2` -- PLAN M19. See
// satellite_file/handlers.hpp for the eleventh, `new` `1 6 2 1`, which is a
// number with nothing behind it.
//
// EVERY ROW BINDS ITS RECEIVER AND COUNTS IT IN THE ARITY, per DESIGN §6.4's
// written-out form; operations_dispatch.cpp subtracts it back out of any
// sentence about counts. NONE OF THEM MUTATES in the `Handler::mutates` sense,
// and that is the reference type showing through: a mutating row's answer is
// the receiver's NEW VALUE, written back to the slot -- which is how a list or
// a string changes, because their bodies are frozen. A file's body is not
// frozen and there is nothing to write back; `close` changes the descriptor
// every snapshot already shares, and the slot holds the same handle afterwards.

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error_reporter/report.hpp"
#include "satellite_bits/bits.hpp"
#include "satellite_file/file_internal.hpp"
#include "satellite_file/gzip.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cerrno>
#include <cstring>
#include <string>

namespace satellite::file {

namespace {

// What a write is being asked to put, or a refusal and false.
//
// `write(x)` `1 6 2 11` GAINED ITS BIT-RUN ARM AT M19.5 AND THE VERB DID NOT
// CHANGE, which is the reason PLAN §8 put that milestone directly after this
// one: the spelling is the same, the row is the same, and what widened is
// which values it accepts. Until it did, `write(x)` accepted only a string and
// therefore read as `write_line` with the newline left off.
//
// A BIT RUN GOES OUT AS BYTES AND NOT AS ITS TEXT. `write(b01000001)` puts one
// byte, `A`, and never the nine characters `b01000001` -- writing the text is
// what `write(x.to_string())` spells, out loud, and a verb that guessed
// between the two would be DESIGN §1.1's "behind the user's back" on the one
// operation where the difference is invisible until something reads the file
// back.
//
// AND A WIDTH THAT IS NOT A MULTIPLE OF EIGHT IS REFUSED. A file is made of
// bytes; half a byte has no representation in one. Padding to the next byte
// and left-aligning in the last one both write bits the program never wrote,
// so the refusal is the only answer that invents nothing -- and it is one the
// program can act on, `width()` `1 6 5 2` being the question it would ask.
bool bytes_to_put(eval::Machine &m, const Value *arguments, bool newline,
                  std::string *out)
{
    // A HEX RUN IS THE SAME BYTES AND THE SAME RULE, and it is written as its
    // OWN arm rather than folded into the one below because the two values
    // live in different variant arms (value.hpp) -- `as_binary` answers
    // nullptr for a hex and there is nothing to fold. What the two arms share
    // is `bits::to_bytes`, so `write(x41)` and `write(b01000001)` put the same
    // single byte `A`, and the width rule reads identically on both.
    //
    // THE MULTIPLE OF EIGHT IS BITS AND NOT DIGITS, which is exactly why
    // `hex.width()` `1 6 11 2` answers bits: `x41` is 8 bits and writes;
    // `x415` is 12 and does not. Had `width()` counted digits, this refusal
    // would have had to say "a multiple of two" for one type and "a multiple
    // of eight" for the other while meaning one thing.
    if (const bits::HexRun *run = as_hex(arguments[1])) {
        if (!bits::to_bytes(run->bits, *out)) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                m.span_of(m.here()),
                std::string(m.text_of(m.here())),
                "a `satellite.variable.hex` whose width is a whole number "
                "of bytes -- a multiple of 8",
                bits::text_of(*run) + ", which is " +
                    std::to_string(run->width()) + " bits wide"));
            return false;
        }
    } else if (const bits::BitRun *run = as_binary(arguments[1])) {
        if (!bits::to_bytes(*run, *out)) {
            m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
                m.span_of(m.here()),
                std::string(m.text_of(m.here())),
                "a `satellite.variable.binary` whose width is a whole number "
                "of bytes -- a multiple of 8",
                // THE SENTENCE IS FINISHED BY THE TEMPLATE, which reads
                // "and this one is ..." -- so this clause starts with the
                // value and never with a verb, or the two say "is" twice.
                bits::text_of(*run) + ", which is " +
                    std::to_string(run->width()) + " bits wide"));
            return false;
        }
    } else if (!path_at(m, arguments, 1, out)) {
        return false;
    }
    if (newline)
        *out += '\n';
    return true;
}

// The bytes a write puts, or a refusal and false. `write_line(s)` `1 6 2 4` and
// `write(x)` `1 6 2 11` differ in exactly one character and share everything
// else, which is why they are one function and two rows.
bool put(eval::Machine &m, const Value *arguments, bool newline, Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    std::string bytes;
    if (!bytes_to_put(m, arguments, newline, &bytes))
        return false;
    if (wrong_direction(m, *handle, handle->writable, "write to",
                        modes_that_write()))
        return false;
    const int fd = descriptor_of(m, *handle);
    if (fd < 0)
        return false;

    size_t sent = 0;
    while (sent < bytes.size()) {
        const ssize_t put = ::write(fd, bytes.data() + sent, bytes.size() - sent);
        if (put < 0) {
            if (errno == EINTR)
                continue;
            // A RUNTIME FAILURE IS A VALUE, which is the other half of the split
            // this module keeps: the wrong mode and a closed handle are PROGRAM
            // mistakes and refuse, a disk that filled up is a false the caller
            // can test. `error` `1 6 2 10` says which.
            handle->last_error.store(errno);
            *answer = Value::boolean(false);
            return true;
        }
        sent += static_cast<size_t>(put);
    }
    *answer = Value::boolean(true);
    return true;
}

// `write_line(s)` `1 6 2 4` -- the bytes of `s` AND a newline.
//
// THE NAME WON, AND THE ARGUMENT IT WON AGAINST IS WRITTEN DOWN. v1's `.write`
// deliberately appended nothing, on the grounds that a file with no trailing
// newline and a line assembled from several writes both become unwritable. The
// author settled it on 2026-09-08 by keeping BOTH verbs: this one says what its
// name says, and `write(x)` below is v1's, so neither argument loses and
// nothing became unwritable.
bool file_write_line(eval::Machine &m, const Value *arguments, uint32_t,
                     Value *answer)
{
    return put(m, arguments, true, answer);
}

// `write(x)` `1 6 2 11` -- exactly the bytes of `x`, minted 2026-09-08. This is
// the verb M19.5's `satellite.variable.binary` and `.hex` values are written
// with, and it gains two arms there rather than a new spelling.
bool file_write(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    return put(m, arguments, false, answer);
}

// `open` `1 6 2 2` -- reopen a handle that is closed, or retry one whose open
// failed.
//
// DESIGN §8's "NEVER SILENTLY REOPEN" IS NOT VIOLATED BY THIS. Read the
// sentence in place: it is the answer to what the RUNTIME does when a snapshot
// uses a closed handle, and that answer is unchanged -- every row above refuses
// a closed descriptor with S1203 and not one of them reopens anything on the
// program's behalf. What the rule governs is the reopen nobody asked for. This
// one is a line of source with a status the caller reads.
//
// ALREADY OPEN IS TRUE AND DOES NOTHING, which is the answer `close` gives to a
// second close and the only one that keeps the snapshot contract intact:
// opening again would leak the live descriptor and swap the file description
// out from under every other snapshot with no close having happened.
bool file_reopen(eval::Machine &m, const Value *arguments, uint32_t,
                 Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    if (handle->descriptor.load() >= 0) {
        *answer = Value::boolean(true);
        return true;
    }

    const int fd = ::open(handle->path.c_str(), handle->reopen_flags, 0644);
    if (fd < 0) {
        handle->last_error.store(errno);
        *answer = Value::boolean(false);
        return true;
    }

    // Two snapshots racing to reopen: the loser closes the descriptor it just
    // opened rather than overwriting the winner's, for `close`'s reason one
    // step earlier -- a handle must never hold a number the OS has handed to
    // something else, and must never drop one nothing will close.
    int closed = -1;
    if (!handle->descriptor.compare_exchange_strong(closed, fd)) {
        ::close(fd);
        *answer = Value::boolean(true);
        return true;
    }

    // A REOPENED HANDLE READS FROM THE BEGINNING. The cursor is the language's
    // and belongs to the open file description it was counting through; the
    // file may have changed while the handle was closed, so carrying an old
    // offset over would be a position with no meaning. Only the winner of the
    // exchange resets it -- a snapshot that found the handle already open must
    // not move a cursor somebody else is reading through.
    handle->read_at = 0;
    handle->buffer.clear();
    handle->buffer_at = 0;
    handle->ever_open = true;

    // AND THE FILE IS ASKED AGAIN, NOT REMEMBERED. The stream was released with
    // the old descriptor, so a reopened handle needs a new one -- and whether
    // it needs one at all is re-read from the bytes rather than carried over,
    // because the file may have been REPLACED while the handle was closed. The
    // paragraph above says the cursor resets for that exact reason; what the
    // file IS deserves the same treatment as where we were in it.
    //
    // WITHOUT THIS, A REOPENED HANDLE ON A GZIP FILE WOULD FALL THROUGH TO
    // pread AND HAND BACK THE COMPRESSED BYTES AS A SUCCESSFUL READ. Not an
    // error and not a refusal -- a plausible answer that is wrong, which is the
    // one outcome DESIGN §1.1 rules out entirely.
    if (handle->readable && gzip::looks_gzipped(fd)) {
        handle->gz = gzip::open_for_reading(fd);
        if (handle->gz == nullptr) {
            handle->last_error.store(EIO);
            *answer = Value::boolean(false);
            return true;
        }
    }

    // The stale errno goes. `error` means "the most recent failure", and once
    // the handle is open an open failure describes a state that no longer
    // exists -- `ok` true beside `error` "No such file or directory" would be
    // two true sentences that read as a contradiction.
    handle->last_error.store(0);
    *answer = Value::boolean(true);
    return true;
}

// `close` `1 6 2 6`. DESIGN §8 requires a STATUS rather than a destructor:
// close(2) is where buffered writes commit and where ENOSPC and EIO are
// reported, and a destructor has nobody left to tell.
bool file_close(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    // exchange(), so two snapshots racing to close cannot both close the
    // descriptor -- the loser would be closing a number the OS had already
    // handed to something else. Closing twice is not a failure.
    const int fd = handle->descriptor.exchange(-1);
    if (fd < 0) {
        *answer = Value::boolean(true);
        return true;
    }

    // THE STREAM OWNS THE DESCRIPTOR, so a gzip handle is closed through zlib
    // and the fd is not closed again -- file_handle.hpp's note on `gz`.
    // gzclose(3) answers Z_OK or a code; anything else is a flush or a read
    // that failed on the way out, and `close` `1 6 2 6` exists to hand that
    // back rather than let a destructor swallow it.
    if (handle->gz != nullptr) {
        void *stream = handle->gz;
        handle->gz = nullptr;
        gzip::close_stream(stream);
        *answer = Value::boolean(true);
        return true;
    }

    if (::close(fd) < 0) {
        handle->last_error.store(errno);
        *answer = Value::boolean(false);
        return true;
    }
    *answer = Value::boolean(true);
    return true;
}

// `exists` `1 6 2 7` -- is THIS handle's own path still there.
//
// THE NARROW QUESTION, KEPT NARROW. `1 6 2 7` sits on the type node, so §6.4
// desugars it to a call binding a receiver -- and "does this file exist" is
// asked before a handle exists. The author settled both on 2026-09-08:
// `satellite.file.exists(path)` `1 8 5` is the question with no handle, and this
// is the question WITH one, which is what PLAN M19's done-when asks after a
// `satellite.system.delete`. It looks at the name and not at the descriptor, so
// a closed handle still answers, and an unlinked-but-open file answers false --
// both of which are the truth about the NAME, which is what the word asks.
bool file_exists_here(eval::Machine &m, const Value *arguments, uint32_t,
                      Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    struct stat info;
    *answer = Value::boolean(::stat(handle->path.c_str(), &info) == 0);
    return true;
}

// `ok` `1 6 2 8` -- did it work. DESIGN §9's failed open is a VALUE and this is
// the word that asks it, which is why the milestone could not be demonstrated
// without a number for it.
bool file_ok(eval::Machine &m, const Value *arguments, uint32_t, Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    *answer = Value::boolean(handle->descriptor.load() >= 0);
    return true;
}

// `path` `1 6 2 9` -- which file. encode_raw, for as_text's reason.
bool file_path(eval::Machine &m, const Value *arguments, uint32_t, Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    *answer = as_text(handle->path);
    return true;
}

// `error` `1 6 2 10` -- why not, in plain words, or the empty string.
//
// THE EMPTY STRING AND NOT NOTHING, deliberately. `ok` is the question with a
// yes-or-no answer; this one hands back a sentence to print, and a program that
// prints it after a success should print nothing rather than refuse by name at
// the line that was reporting somebody else's failure.
bool file_error(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    FileHandle *handle = nullptr;
    if (!handle_at(m, arguments, 0, &handle))
        return false;
    const int code = handle->last_error.load();
    *answer = as_text(code ? std::strerror(code) : "");
    return true;
}

} // namespace

void install_file_methods()
{
    using words::NodeId;
    eval::Handlers &table = eval::Handlers::table();

    struct Row {
        NodeId path;
        eval::HandlerFn fn;
        uint32_t arity;
    };
    static constexpr Row rows[] = {
        {NodeId::VARIABLE_FILE_OPEN,       file_reopen,       1},
        {NodeId::VARIABLE_FILE_READ_LINE,  file_read_line,    1},
        {NodeId::VARIABLE_FILE_WRITE_LINE, file_write_line,   2},
        {NodeId::VARIABLE_FILE_READ_ALL,   file_read_all,     1},
        {NodeId::VARIABLE_FILE_CLOSE,      file_close,        1},
        {NodeId::VARIABLE_FILE_EXISTS,     file_exists_here,  1},
        {NodeId::VARIABLE_FILE_OK,         file_ok,           1},
        {NodeId::VARIABLE_FILE_PATH,       file_path,         1},
        {NodeId::VARIABLE_FILE_ERROR,      file_error,        1},
        {NodeId::VARIABLE_FILE_WRITE,      file_write,        2},
    };
    for (const Row &row : rows)
        table.install(static_cast<words::PathId>(row.path),
                      {row.fn, true, row.arity, "M19"});
}

} // namespace satellite::file
