// The lines already typed. See satellite_prompt/history.hpp.

#include "satellite_prompt/history.hpp"

namespace satellite::prompt {

namespace {

// Returned by at() for an index nobody should have asked for. A reference has
// to refer to something, and the alternative -- undefined behaviour on a
// mis-indexed browse -- is the kind of thing that shows up as a crash three
// keystrokes later with nothing pointing back here.
const std::string &nothing()
{
    static const std::string empty;
    return empty;
}

} // namespace

void History::add(const std::string &line)
{
    if (line.empty())
        return;
    if (!entries_.empty() && entries_.back() == line)
        return;

    entries_.push_back(line);

    // ERASE FROM THE FRONT, ONE AT A TIME, because add() is called once per
    // line typed by a human -- there is no path that overshoots the ceiling by
    // more than one, so the loop runs at most once and a bulk erase would be
    // machinery for a case that cannot happen.
    while (entries_.size() > kMaxEntries)
        entries_.erase(entries_.begin());
}

const std::string &History::at(size_t index) const
{
    if (index >= entries_.size())
        return nothing();
    return entries_[index];
}

} // namespace satellite::prompt
