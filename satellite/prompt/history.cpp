// The lines already typed. See history.hpp.

#include "history.hpp"

namespace satellite004::prompt {

void History::add(const std::string &line)
{
    if (line.empty() || (!entries_.empty() && entries_.back() == line))
        return;
    entries_.push_back(line);
}

const std::string &History::at(std::size_t index) const
{
    static const std::string nothing;
    return index < entries_.size() ? entries_[index] : nothing;
}

} // namespace satellite004::prompt
