// The mode table, the one constructor, and the destructor that is a backstop.
// See satellite_file/file_handle.hpp for what a handle is and why two of its
// fields are atomic.

#include "satellite_file/file_handle.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>

namespace satellite::file {

namespace {

// THE FOUR MODE WORDS. DESIGN §1.1 requires an option to be a WORD at the call
// site rather than a bitmask -- "the words say what they do at the call site,
// which a bitmask never does" -- and WORD_NUMBERS §1.5's literal fold would
// have turned these into four numbers under `1 8`. The author settled on
// 2026-09-08 that they do NOT fold, so this table is read at run time and S1201
// is what a word outside it gets.
constexpr Mode kModes[] = {
    {"read",   O_RDONLY,                     true,  false},
    {"write",  O_WRONLY | O_CREAT | O_TRUNC, false, true },
    {"append", O_WRONLY | O_CREAT | O_APPEND, false, true},

    // O_RDWR | O_APPEND, AND NEVER A BARE O_RDWR. v1's paragraph, and it is
    // forced rather than preferred: POSIX gives one file offset per open file
    // description, so on a bare O_RDWR handle a write would land wherever the
    // last read left that shared offset. O_APPEND removes the question -- a
    // write goes on the END, and the kernel does the seek-and-write atomically,
    // so two snapshots of one handle cannot interleave into each other's bytes.
    //
    // WHAT M19 CHANGES IS THE OTHER HALF OF THAT SENTENCE. v1 finished it with
    // "and .read() rewinds, so the two directions do not fight over the one
    // offset they share". `read_line` `1 6 2 3` cannot rewind -- DESIGN §8.7
    // makes it "a line, or nothing" -- so instead the read side stops using the
    // shared offset at all: every read is a `pread` at the handle's own cursor,
    // and O_APPEND moving the shared offset to the end on every write is then
    // something the read side never observes.
    {"read_append", O_RDWR | O_CREAT | O_APPEND, true, true},
};

// The words that can do a thing, listed for S1202's "open it with" clause.
std::string words_where(bool Mode::*ability)
{
    std::string out;
    size_t said = 0;
    size_t total = 0;
    for (const Mode &mode : kModes)
        total += mode.*ability ? 1 : 0;
    for (const Mode &mode : kModes) {
        if (!(mode.*ability))
            continue;
        if (said)
            out += (said + 1 == total) ? " or " : ", ";
        out += std::string("\"") + mode.word + "\"";
        said++;
    }
    return out;
}

} // namespace

const Mode *mode_named(const std::string &word)
{
    for (const Mode &mode : kModes)
        if (word == mode.word)
            return &mode;
    return nullptr;
}

// BUILT FROM THE TABLE AND NOT WRITTEN OUT. v1's message held a hand-written
// list of three words and it was already stale on the day a fourth landed --
// which is the kind of defect that is invisible until somebody trusts the
// sentence.
std::string mode_list()
{
    std::string out;
    const size_t count = sizeof kModes / sizeof kModes[0];
    for (size_t i = 0; i < count; i++) {
        if (i)
            out += (i + 1 == count) ? " or " : ", ";
        out += std::string("\"") + kModes[i].word + "\"";
    }
    return out;
}

std::string modes_that_read() { return words_where(&Mode::readable); }
std::string modes_that_write() { return words_where(&Mode::writable); }

FileHandle::~FileHandle()
{
    // THE BACKSTOP, AND IT REPORTS NOTHING BECAUSE THERE IS NOBODY TO REPORT
    // TO. That is exactly why DESIGN §8 requires `close` `1 6 2 6` to be a
    // method answering a status: this runs when the last snapshot goes out of
    // scope, and a program that never called `close` has already finished
    // deciding what to do about a failed flush.
    const int held = descriptor.exchange(-1);
    if (held >= 0)
        ::close(held);
}

} // namespace satellite::file
