#pragma once
// satellite/satellite_variable_file/satellite_file.hpp -- satellite.variable.file:
// A TEXT FILE HELD AS A LIST OF LINES. SATELLITE_FILE_OPERATIONS.md Part 3.
//
// THE AUTHOR'S MODEL (2026-09-18): *"we'll build it so you can iterate over the
// lines as if they were objects ... As if each line were a list of objects."* So
// this class is a list of lines that lives on the disk, and its methods are the
// list's own (satellite.container.list's spellings), done to lines.
//
// LINES COUNT FROM 1 (the author, 2026-09-18: *"all line counts all start at 1,
// so that is how we will build file indexing"*). Line 1 is the first line, size()
// is the last, and 0 is never a line -- so index_of and search answer 0 for "no
// such line", which a program can test with `> 0`.
//
// A FILE THAT WOULD NOT OPEN IS A VALUE, NOT AN ERROR (003's rule, kept). The
// makers always answer a handle; a failure is a handle whose ok() is false and
// whose code() and error() say why. No method throws, and no method stops a
// program -- the interpreter decides that: only `f[n]` past the end does
// (line_past_the_end), because reading a line that is not there is a mistake in
// the program, while asking where a line is, is a question.
//
// UTF-8 IN, UTF-8 OUT. Lines are held as the UTF-8 bytes of their text, which
// were checked strictly when the file was read. The interpreter converts to and
// from satellite_string at its own boundary (Value::of_utf8, text_utf8), so this
// class has no dependency on the object model. Matching on checked UTF-8 is
// character-exact: a valid needle can only match where a character begins.
//
// CHANGES LIVE IN MEMORY UNTIL save(), close() or the handle's end, and a save
// is WHOLE OR NOT AT ALL: the new text is written beside the file, fsync'd and
// renamed over it, so a crash leaves the old file or the new one and never a
// mixture. APPENDING IS THE FAST PATH: while nothing but appends has happened
// since the last save, a save only adds the new bytes to the end (O_APPEND),
// and the appended bytes are written out on their own once they pass a
// threshold, so a long loop of appends streams to the disk.
//
// THE WHOLE FILE IS IN MEMORY. DESIGN §1.2 does not allow that in the end;
// SATELLITE_FILE_OPERATIONS FO-9 replaces it with an index and a list of
// changes. Written down so it is not mistaken for the finished design.

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace satellite004 {

class satellite_file {
public:
    enum class Kind { text, binary };

    // How each line ended on the disk, so a file is written back the way it was:
    // a \r\n file stays \r\n, and a last line with no newline keeps having none
    // until something is added after it.
    enum class Ending { none, lf, crlf };

    // ---------------------------------------------------------------------
    // MAKING AND OPENING, by a path the INTERPRETER has already resolved
    // (relative to the calling .satl, or the program root first for a leading
    // /). Each answers a handle, always -- ok() says whether it opened.
    // ---------------------------------------------------------------------

    // A file that is NOT there: made empty and open (O_CREAT | O_EXCL). NEVER
    // CLOBBERS -- anything at the path gives a handle that is not ok
    // (file_already_there, or not_a_file for a directory) and the thing at the
    // path is untouched.
    static std::shared_ptr<satellite_file> make_new(const std::string &path, Kind kind);

    // A file that IS there: read and open. NEVER CREATES -- a missing file is
    // file_not_found and nothing is made, so a misspelt path cannot become an
    // empty file. A directory, FIFO or device is not_a_file. Bytes that are not
    // UTF-8 are file_not_text, with the line and the byte offset in error().
    // A file whose lines end in \r alone is file_not_text, said by name.
    static std::shared_ptr<satellite_file> open_existing(const std::string &path, Kind kind);

    // Whether a REGULAR file is at the path (a symlink to one counts; a
    // directory does not). Never an error.
    static bool exists(const std::string &path);

    // Empties a regular file that is there, by name, and leaves it there.
    // Answers success, or the code (file_not_found, not_a_file,
    // file_unwritable, path_holds_a_nul) with the reason written to `reason`.
    static signed long long int clear(const std::string &path, std::string &reason);

    // Saves, then closes (see close()). A file forgotten by a program is
    // therefore never lost: its last handle's end puts it on the disk.
    ~satellite_file();

    satellite_file(const satellite_file &) = delete;
    satellite_file &operator=(const satellite_file &) = delete;

    // ---------------------------------------------------------------------
    // WHAT WENT WRONG
    // ---------------------------------------------------------------------
    bool ok() const { return open_; }                        // open, and usable
    signed long long int code() const { return code_; }       // the last failure's machine code, 0 if none
    const std::string &error() const { return error_; }       // the last failure in words, "" if none
    const std::string &path() const { return path_; }         // the path it was opened on, as given
    // Whether the file this handle opened is still a file there -- asked of the
    // absolute path kept when it was made, so a change of directory since does not
    // turn the question into one about a different file.
    bool path_exists() const { return exists(where_.empty() ? path_ : where_); }
    Kind kind() const { return kind_; }

    // ---------------------------------------------------------------------
    // THE LIST OF LINES. n counts from 1. Every changing method answers true
    // when it changed the file and false when it did not, with code() and
    // error() set; a closed handle answers false with file_not_open, and a
    // binary handle with file_has_no_lines.
    // ---------------------------------------------------------------------
    std::size_t size() const;                                  // how many lines
    bool empty() const { return size() == 0; }

    // Line n into `out`. False (line_past_the_end) for n == 0 or n > size().
    bool line(std::size_t n, std::string &out) const;
    bool first(std::string &out) const { return line(1, out); }
    bool last(std::string &out) const { return line(size(), out); }

    // A new last line. Text holding \n becomes that many lines (a \r\n inside
    // it is one line ending, not text).
    bool append(const std::string &text);

    // `text` becomes line n and line n onward move down one. n may be
    // size() + 1, which is append. n == 0 or n > size() + 1 is false.
    bool insert(std::size_t n, const std::string &text);

    // Line n becomes `text` (which may hold \n and become several lines).
    bool replace_line(std::size_t n, const std::string &text);

    // The FIRST `from`, looking from line 1 down, becomes `to` -- inside one
    // line only, since a line's text holds no newline. False when `from` is
    // empty (empty_search_text) or in no line (text_not_found). `to` may hold
    // `from`; it is replaced once and never re-scanned.
    bool replace_text(const std::string &from, const std::string &to);

    // The number of the first line that IS exactly `text`, or 0.
    std::size_t index_of(const std::string &text) const;
    // The number of the first line that CONTAINS `text`, or 0. Empty text is 0.
    std::size_t search(const std::string &text) const;
    // Whether any line is exactly `text`.
    bool contains(const std::string &text) const { return index_of(text) != 0; }

    bool remove_at(std::size_t n);                            // lines below move up one
    bool remove(const std::string &text);                     // the first line exactly `text`
    bool remove_first() { return remove_at(1); }
    bool remove_last() { return remove_at(size()); }
    bool truncate(std::size_t n);                             // keep the first n; n >= size() changes nothing and is true
    bool clear_lines();                                       // no lines; the file stays

    // The whole file as one string, every line with its own ending (and the
    // byte-order mark if the file had one), INCLUDING changes not yet saved.
    std::string read_all() const;

    // ---------------------------------------------------------------------
    // THE DISK
    // ---------------------------------------------------------------------

    // Puts every change on the disk now. Nothing changed: true and nothing is
    // written. Only appends since the last save: the new bytes are added to the
    // end. Anything else: the whole text is written to a new file beside the
    // real one (the real one, if the path was a symlink), fsync'd, given the
    // old file's permissions, and renamed over it. False with file_unwritable
    // and the system's reason when any step fails -- and then the old file is
    // untouched and the changes are still held, so a later save can try again.
    bool save();

    // save(), then closed. True when the save landed (or there was nothing to
    // save). A closed handle keeps path(), code() and error(). Closing twice is
    // true and does nothing.
    bool close();

    // Opens a closed handle again, re-reading the disk. An open handle answers
    // true and nothing happens.
    bool reopen();

private:
    satellite_file() = default;

    // Reads path_ into lines_ (strict UTF-8, endings, BOM). Sets open_.
    bool read_from_disk();
    void fail(signed long long int code, std::string reason) const;
    bool usable() const;          // open and text; sets file_not_open / file_has_no_lines

    // ADDED WITH satellite_file.cpp. Text in -> its lines (a \r alone is file_not_text,
    // because no file holding one could be read back). put_lines makes the room first,
    // so a failure part way changes nothing, and lines_ and endings_ stay row for row.
    bool take_lines(const std::string &text, std::vector<std::string> &pieces) const;
    void put_lines(std::size_t at, bool replacing, std::vector<std::string> &pieces, Ending last_ending);
    void edited();                                // a change that is not an append: the next save rewrites
    void past_the_end(std::size_t n) const;      // line_past_the_end, with the number and the size
    bool save_appended() noexcept;               // the fast path: the new bytes, O_APPEND
    bool save_whole() noexcept;                  // the whole text beside the file, fsync'd, renamed over it
    void now_saved() noexcept;                   // the bookkeeping after a save that landed
    void out_of_memory(signed long long int code) const noexcept;

    std::string path_;            // as given
    // WHAT EVERY DISK OPERATION USES: path_ made absolute against the working
    // directory when the handle was made, so a program that changes directory
    // (satellite.directory.change) still saves to the file it opened.
    std::string where_;
    Kind kind_ = Kind::text;
    bool open_ = false;

    std::vector<std::string> lines_;     // each line's UTF-8 text, no ending
    std::vector<Ending> endings_;        // row for row with lines_
    Ending default_ending_ = Ending::lf; // what a new line ends with: the file's first ending
    bool byte_order_mark_ = false;

    // THE APPEND FAST PATH. saved_lines_ is how many leading lines are exactly
    // what is on the disk; when only appends happened since, only_appended_ is
    // true and save() writes lines saved_lines_.. with O_APPEND.
    std::size_t saved_lines_ = 0;
    bool only_appended_ = true;
    bool changed_ = false;
    bool last_line_needs_ending_on_disk_ = false;   // the disk's last line had no newline and something follows it now

    // THE APPENDED BYTES ARE WRITTEN OUT ON THEIR OWN once pending_bytes_ passes
    // flush_after_. A write that fails moves flush_after_ on by another 64 KiB, so a
    // full disk is tried again after the next 64 KiB and not after every line.
    static constexpr std::size_t kFlushBytes = 64 * 1024;
    std::size_t pending_bytes_ = 0;               // appended bytes not yet on the disk, while only_appended_
    std::size_t flush_after_ = kFlushBytes;

    mutable signed long long int code_ = 0;
    mutable std::string error_;
};

} // namespace satellite004
