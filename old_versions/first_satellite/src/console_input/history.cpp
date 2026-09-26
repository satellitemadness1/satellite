// The previous entries: the store, the cap, and the file. See history.hpp for
// where the file lives and how to switch it off.

#include "console_input/history.hpp"

#include <cstdlib>
#include <fstream>

namespace satellite {

namespace {

// Returned by at() for an index nobody should have asked for. A reference has
// to name something, and this names an empty line rather than undefined
// behaviour -- the editor clamps its own index, so reaching here is a bug
// somewhere else and should look like one rather than crash.
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
    // A newline cannot be stored -- one entry is one line of the file -- and
    // the prompt cannot produce one, so this is a guard against a future
    // caller rather than against anything that happens today.
    if (line.find('\n') != std::string::npos)
        return;
    if (!entries_.empty() && entries_.back() == line)
        return;

    entries_.push_back(line);

    // Trimmed from the FRONT, so what falls off is the oldest. erase() on a
    // vector shifts the rest, which would be quadratic if it happened per add
    // -- it does not: the cap is only ever exceeded by one, so this moves the
    // whole vector once per line typed past a thousand, and a thousand lines
    // is a session nobody is measuring the allocator during.
    if (entries_.size() > MAX_ENTRIES)
        entries_.erase(entries_.begin(),
                       entries_.begin() +
                           static_cast<long>(entries_.size() - MAX_ENTRIES));
}

std::string History::default_path()
{
    // The override comes first, exactly as $SATELLITE_PATH does for the
    // library directory (§9): a variable that can only ADD a behaviour is a
    // configuration mechanism, and one that can also switch the behaviour off
    // is an escape hatch. This is the second kind.
    if (const char *set = std::getenv("SATL_HISTORY")) {
        std::string value = set;
        if (value.empty() || value == "none")
            return {};
        return value;
    }

    const char *home = std::getenv("HOME");
    if (!home || !*home)
        return {};      // no home: memory only, never a dotfile in $PWD

    return std::string(home) + "/.satl_history";
}

bool History::load()
{
    if (path_.empty())
        return true;

    std::ifstream in(path_, std::ios::binary);
    if (!in)
        return true;    // no file yet is what a first run looks like

    std::string line;
    while (std::getline(in, line)) {
        // A file written on a machine that uses CRLF, or edited by hand. The
        // '\r' would otherwise be replayed into the prompt as a real
        // character and put every column in the redraw one out.
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

    // Truncating rather than appending, because add() has already applied the
    // cap and appending would grow the file without bound however small the
    // list in memory stayed. The cost is rewriting a thousand lines once per
    // session, at exit, which is not a cost.
    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    for (const std::string &entry : entries_)
        out << entry << '\n';

    return static_cast<bool>(out);
}

} // namespace satellite
