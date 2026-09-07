#pragma once

// The lines already typed at this prompt, and nothing else.
//
// IN MEMORY, FOR THE LIFE OF ONE SESSION, AND THAT IS A DECISION RATHER THAN A
// STAGE LEFT UNFINISHED. A history that outlives the session has to live in a
// file, and where that file goes is a question about the user's home directory
// that PLAN §8's M22 entry does not answer and this milestone has no standing to
// answer for it: `~/.satl` is the INSTALL directory (an install puts a binary
// there), so user data does not belong in it, and inventing a second dotfile is
// exactly the kind of decision this tree writes down before making. The class is
// shaped so that adding a path later changes this file and no caller -- see
// MILESTONES/M22.md §6, where it is named as open.
//
// DUPLICATES ARE NOT STORED TWICE IN A ROW, which is the one filtering rule
// worth having: a user who runs the same line three times to watch something
// change wants one entry, not three, and pressing Up should reach the line
// BEFORE it in two presses rather than four.

#include <cstddef>
#include <string>
#include <vector>

namespace satellite::prompt {

class History {
public:
    // Adds a line, unless it is empty or the same as the line before it.
    void add(const std::string &line);

    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    // Index 0 is the OLDEST. The editor browses from the end backwards.
    const std::string &at(size_t index) const;

    const std::vector<std::string> &entries() const { return entries_; }

    // A ceiling on the entries kept, and it is not a limit on the LANGUAGE --
    // SCRATCH.md/NO_LIMITS.md's rule is about what a satellite program may do,
    // and this is a scrollback buffer for a terminal. The oldest go first.
    static constexpr size_t kMaxEntries = 1000;

private:
    std::vector<std::string> entries_;
};

} // namespace satellite::prompt
