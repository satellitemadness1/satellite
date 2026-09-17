#pragma once
// The lines already typed at this prompt, and nothing else (PLAN M0.6, ported
// from 003's satellite_prompt/history).
//
// IN MEMORY, FOR ONE SESSION. A history that outlives the session needs a file,
// and where it lives is undecided: ~/.satl is an install directory, not a place
// for a person's data. Adding a path later changes this class and no caller.
//
// NO CEILING ON ITS ENTRIES (DESIGN §1). 003 kept the newest 1,000; a pasted
// block of 100,000 lines is 100,000 entries here, and Up reaches the first.
//
// A LINE IS NOT STORED TWICE IN A ROW: a person who runs the same line three
// times wants one entry, so Up reaches the line before it in two presses.

#include <cstddef>
#include <string>
#include <vector>

namespace satellite004::prompt {

class History {
public:
    // Adds a line, unless it is empty or the same as the line before it.
    void add(const std::string &line);

    std::size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    // Index 0 is the OLDEST; the editor browses from the end backwards. An index
    // past the end answers an empty line rather than undefined behaviour.
    const std::string &at(std::size_t index) const;

private:
    std::vector<std::string> entries_;
};

} // namespace satellite004::prompt
