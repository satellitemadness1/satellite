// History management implementation.
// Milestone 11 Prototype in prototype/M11.

#include "history.hpp"

#include <cstdlib>
#include <fstream>
#include <utility>

namespace satellite {

namespace {

const std::string &nothing()
{
    static const std::string empty;
    return empty;
}

} // namespace

History::History(std::string path) : path_(std::move(path)) {}

const std::string &History::at(size_t index) const
{
    return index < entries_.size() ? entries_[index] : nothing();
}

void History::add(const std::string &line)
{
    if (line.empty())
        return;
    if (line.find('\n') != std::string::npos)
        return;
    if (!entries_.empty() && entries_.back() == line)
        return;

    entries_.push_back(line);

    if (entries_.size() > MAX_ENTRIES) {
        entries_.erase(entries_.begin(),
                       entries_.begin() + static_cast<long>(entries_.size() - MAX_ENTRIES));
    }
}

std::string History::default_path()
{
    if (const char *set = std::getenv("SATL_HISTORY")) {
        std::string value = set;
        if (value.empty() || value == "none")
            return {};
        return value;
    }

    const char *home = std::getenv("HOME");
    if (!home || !*home)
        return {};

    return std::string(home) + "/.satl_history";
}

bool History::load()
{
    if (path_.empty())
        return true;

    std::ifstream in(path_, std::ios::binary);
    if (!in)
        return true;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        add(line);
    }
    return true;
}

bool History::save() const
{
    if (path_.empty())
        return true;

    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    for (const std::string &entry : entries_)
        out << entry << '\n';

    return static_cast<bool>(out);
}

} // namespace satellite

