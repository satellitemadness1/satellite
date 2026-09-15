#pragma once

// `satellite.variable.file` -- the handle behind DESIGN §8's "handle |
// reference type" row, and PLAN M19.
//
// A REFERENCE TYPE, WHICH IS THE ONE THING THAT MAKES THIS ARM UNLIKE EVERY
// OTHER HANDLE IN THE VALUE MODEL. `Str`, `Flo`, `Lst` and `Map` are all
// `shared_ptr<const T>`: a value two slots see can never change under either of
// them, and every mutation is a copy published whole through the receiver's
// storage slot (DESIGN §6.4). A file is the deliberate opposite. Two names for
// one open file are TWO NAMES FOR ONE OPEN FILE -- closing through either one
// closes it for both, because there is one descriptor and the kernel does not
// know about our slots. So the handle is `shared_ptr<FileHandle>` with no
// `const`, and it is the first mutable-through-shared arm the language has.
//
// THAT IS WHY THE TWO FIELDS THAT MOVE ARE ATOMIC. v1's note, kept: `fd` and
// `last_error` are what `close` and `open` change after the value has been
// published, so two snapshots racing to close must not both close the
// descriptor -- the loser would be closing a number the OS had already handed
// to something else. `exchange` and `compare_exchange_strong` in
// file_methods.cpp are that rule, and DESIGN §8's reference semantics are what
// make the race reachable at all. **M23 is when it first gets EXERCISED, not
// when it gets written** -- PLAN M19 says so in those words.
//
// AND THE READ CURSOR IS NOT ATOMIC, WHICH IS A LIMIT OF M19 AND IS NAMED HERE
// RATHER THAN DISCOVERED AT M23. `read_at`, `buffer` and `buffer_at` below are
// one logical position spread over three fields; making each atomic would make
// each field consistent and the position as a whole no more so. Two threads
// reading one handle at M23 need a lock or a per-snapshot cursor, and which of
// those it is is a design question about what a shared handle MEANS -- the same
// question DESIGN §8 leaves open for the socket. One thread reading is exact.
//
// ANSWERED AT THREAD.md T1: A LOCK. The author's Q4 of 2026-09-13; `lock` below
// is it, and every method row runs under it.

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace satellite::file {

// What a mode word buys. `readable` and `writable` are checked BEFORE the
// syscall, which is what turns "Bad file descriptor" -- a sentence about a
// descriptor in perfect health -- into S1202 naming the mode that would have
// worked.
struct Mode {
    const char *word;
    int flags;
    bool readable;
    bool writable;
};

// The four, in ONE table, because `satellite.file.open` `1 8 2` and
// `satellite.file.new` `1 8 1` ask the same question and a second copy is a
// second chance for the two to disagree about what "append" means.
const Mode *mode_named(const std::string &word);

// The four words as `"read", "write", "append" or "read_append"`, built from
// the table so S1201's sentence cannot fall behind it -- v1's hand-written list
// of three was already stale when a fourth landed.
std::string mode_list();

// The modes that can do a thing, for S1202's fourth argument.
std::string modes_that_read();
std::string modes_that_write();

// One handle. Everything but the three moving fields is written once, before
// the handle is published into a Value, and read-only afterwards.
struct FileHandle {
    // -1 is closed or never opened. The two are told apart by `last_error`:
    // a handle whose open FAILED holds the errno, and `ok` `1 6 2 8` answers
    // false for both, which is the question it exists to answer.
    std::atomic<int> descriptor{-1};

    // The errno of the most recent failure, or 0. `error` `1 6 2 10` renders
    // it. Cleared by a successful `open`, because "the most recent failure"
    // describing a state that no longer exists would make `ok` true beside an
    // `error` saying "No such file or directory" -- two true sentences that
    // read as a contradiction.
    std::atomic<int> last_error{0};

    std::string path;
    bool readable = false;
    bool writable = false;

    // THE GZIP STREAM, AND NOBODY ASKED FOR IT. A mode says READ or WRITE,
    // which is what the program means; whether the bytes on disk are
    // compressed is a fact about the FILE, and satellite finds that out itself
    // -- gzip::looks_gzipped() reads the magic two bytes at open. DESIGN §1.1
    // is doing everything for the user, and "read_gzip" was the user doing the
    // encoding by hand. A path ending `.gz` is not even consulted: the content
    // is the answer and the name is only a claim.
    //
    // NULL MEANS PLAIN, and a plain file keeps the pread path it has had since
    // M19 -- unchanged, uncopied, and not routed through a decompressor that
    // would only hand the bytes straight back.
    //
    // `void *` SO THAT THIS HEADER DOES NOT NAME zlib.
    // satellite_file/gzip.hpp carries that argument: a `gzFile` here would put
    // -isystem vendor/zlib-develop on the compile line of every unit that
    // touches a file handle, and of every test binary that links one.
    //
    // ONCE THIS IS NON-NULL IT OWNS THE DESCRIPTOR. gzdopen(3) adopts the fd,
    // so closing BOTH would close a number the OS had already handed out
    // again -- the precise race the atomics above exist to prevent, arriving
    // from a direction they cannot see. Every close site therefore branches:
    // the stream is closed, or the descriptor is, never both.
    //
    // AND IT IS NOT ATOMIC, WHICH IS THE READ CURSOR'S LIMIT AND NOT A NEW ONE.
    // The note above says `read_at`, `buffer` and `buffer_at` are one logical
    // position spread over three fields that two threads cannot share; a gzip
    // stream is a fourth, and it is strictly sequential besides -- there is no
    // pread for a compressed stream, because byte N is not findable without
    // inflating the N-1 before it. One thread reading is exact -- and since
    // THREAD.md T1 so are several, one at a time, under `lock` below.
    void *gz = nullptr;

    // WHETHER THIS HANDLE HAS EVER BEEN OPEN, which is what separates two
    // states that a descriptor of -1 collapses: a handle somebody CLOSED, and
    // a handle whose open never worked. They want different sentences -- S1203
    // sends the reader to `open`, S1208 sends them to `ok` and `error` -- and
    // without this the second reads as "the file is closed", which is a
    // sentence about something that never happened.
    //
    // FOUND BY A HELP EXAMPLE, 2026-09-08. A leftover file from an earlier run
    // made `satellite.file.new` come back EEXIST, and the next line said
    // "`write_line` was asked of a `satellite.variable.file` that is closed" --
    // about a handle three lines below the call that failed to open it. The
    // errno was the answer and the message pointed away from it.
    //
    // NOT DERIVED FROM `last_error`, which was the first attempt and is wrong:
    // a handle that opened, failed one write, and was then closed holds an
    // errno and HAS been open, so the two would be told apart by a field that
    // does not mean that.
    bool ever_open = false;

    // What `open` `1 6 2 2` reopens with, which is NOT always what the handle
    // was created with. `satellite.file.new` adds O_EXCL so that creating a
    // file that already exists is a refusal rather than a silent clobber;
    // replaying O_EXCL on a reopen would then fail with EEXIST against the very
    // file the program just made. The ACCESS flags are stored; the CREATION
    // flags are used once and dropped.
    int reopen_flags = 0;

    // THE HANDLE'S OWN READ POSITION, and it is the whole of what M19 adds to
    // v1's design. v1's `.read()` seeks to 0 on EVERY call, and its recorded
    // reason is that with no `.seek()` in the language, "read from wherever the
    // offset happens to be" is a question no satellite program can pose. That
    // reasoning is sound and it does not reach `read_line` `1 6 2 3`, because
    // DESIGN §8.7 fixes what read_line answers -- "a line, or nothing" -- and
    // that sentence has no meaning at all on a handle that rewinds every call.
    //
    // SO THE CURSOR IS THE LANGUAGE'S AND NOT THE KERNEL'S, and every read goes
    // through `pread` at this offset rather than through `read` at the shared
    // one. That is not a preference: "read_append" is `O_RDWR | O_APPEND`, and
    // a write on an O_APPEND descriptor moves the shared offset to the end of
    // the file. A read cursor riding on that offset would be silently reset by
    // every `write_line`, in the one mode whose entire purpose is writing and
    // reading back through one handle.
    long long read_at = 0;

    // What has been read ahead of the cursor, and how much of it is spent.
    // NOT AN OPTIMISATION -- without it, `read_line` either issues one syscall
    // per byte or re-reads its chunk's tail on every call, and the second is
    // quadratic in the file. `read_at` is the position of `buffer[buffer_at]`
    // in the file, so the two together are one position and either can be
    // recomputed from the other.
    std::string buffer;
    size_t buffer_at = 0;

    // THE HANDLE'S LOCK -- THREAD.md D7, the author's Q4 of 2026-09-13: "use
    // locks". Every method row on a file runs under it (file_methods.cpp's
    // `whole`), so the cursor above, the buffer, the gzip stream and a
    // write_line's bytes are each one thing to every thread. The two notes
    // above that called the cursor a limit of M19 are answered by this.
    //
    // `user` AND `shared_seen` ARE UNDER IT TOO: the first thread to use the
    // handle, and whether S1406 has been written to satellite.log for it yet.
    std::mutex lock;
    std::thread::id user;
    bool shared_seen = false;

    // RAII IS THE BACKSTOP AND NOT THE INTERFACE. DESIGN §8 requires an
    // explicit `close` `1 6 2 6` that answers a status, because close(2) is
    // where buffered writes commit and where ENOSPC and EIO are reported, and
    // a destructor has nobody left to tell.
    ~FileHandle();
};

} // namespace satellite::file
