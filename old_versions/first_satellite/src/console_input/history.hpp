#pragma once

#include <cstddef>
#include <string>
#include <vector>

// The previous entries, which is what the up arrow is for.
//
// WHERE IT IS KEPT, and why that is said out loud rather than decided quietly.
// A history that dies with the session is half a feature -- every prompt worth
// using remembers across runs -- but writing a file into somebody's home
// directory because they pressed up-arrow is exactly the kind of thing this
// language does not do behind their back. So the file is real, its path is
// printed by `:history`, and $SATL_HISTORY names it: set it to a path to move
// it, and to `none` (or the empty string) to keep the whole session in memory
// and write nothing at all.
//
// One entry per line, so an entry may not contain a newline -- which costs
// nothing, because the prompt reads one line at a time and an entry IS a line.
// A multi-line block typed at the prompt is remembered as the lines the user
// typed, which is also what they want back when they press up: the line, not
// the paragraph.

namespace satellite {

class History {
public:
    // An empty path means memory only. Nothing is read or written until
    // load() and save() are called; the constructor touches no filesystem.
    explicit History(std::string path = {});

    // Adds a line, oldest first. Ignores an empty line and a line identical to
    // the one already at the end -- holding down return should not push ten
    // copies of the same command out the far end of the list.
    void add(const std::string &line);

    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    // 0 is the OLDEST. The editor walks backwards from size(), so it reads
    // this the other way round; oldest-first is the order the file is in and
    // the order `:history` prints, and one order is worth more than a
    // convenience for one caller.
    const std::string &at(size_t index) const;

    const std::vector<std::string> &entries() const { return entries_; }
    const std::string &path() const { return path_; }

    // Reads the file, appending to whatever is already here. A missing file is
    // NOT a failure -- it is what the first run looks like -- so this answers
    // false only for a file that exists and could not be read.
    bool load();

    // Writes the file, replacing it. False if it could not be written, which
    // the caller is free to ignore: a history that cannot be saved is a
    // nuisance and never a reason to fail a session.
    bool save() const;

    // $SATL_HISTORY, else $HOME/.satl_history. Empty when history is switched
    // off, which is $SATL_HISTORY set to nothing or to `none`, and also what a
    // machine with no $HOME gets rather than a file in the current directory.
    static std::string default_path();

    // The cap, applied on add() and again on save(). One thousand lines is
    // about 60 KB of the kind of text a prompt sees, which is small enough to
    // read at startup without anybody noticing.
    static constexpr size_t MAX_ENTRIES = 1000;

private:
    std::vector<std::string> entries_;
    std::string path_;
};

} // namespace satellite
