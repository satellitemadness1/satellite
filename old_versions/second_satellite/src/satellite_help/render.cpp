// What help prints. See satellite_help/render.hpp for the shape of the walk.

#include "satellite_help/render.hpp"

#include "satellite_help/help_text.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::help {

namespace {

// The prose is wrapped where it was written -- help_lines/entries_*.json holds
// it at about seventy columns and every line of it has been read by a person --
// so nothing here re-wraps. What this does is put a margin on it, which is the
// only thing that changes when a paragraph moves from a document into a
// terminal.
void indented(std::string &out, std::string_view text, std::string_view margin)
{
    size_t at = 0;
    while (at <= text.size()) {
        const size_t end = std::min(text.find('\n', at), text.size());
        if (end != at)
            out.append(margin);
        out.append(text.substr(at, end - at));
        out += '\n';
        if (end == text.size())
            return;
        at = end + 1;
    }
}

// The first sentence of an entry, on one line, for the listings.
//
// A SENTENCE ENDS AT A FULL STOP FOLLOWED BY WHITESPACE, AND THE WHITESPACE IS
// USUALLY A NEWLINE. The entries are wrapped at about seventy columns where
// they were written, so the break after a sentence falls at a line end far more
// often than at a space -- and a version of this that looked only for ". " ran
// three sentences together in the topic listing, which is how the case was
// found. The prose is not the place to fix that: it is wrapped for a document
// and re-flowed here for a table.
//
// THE ABBREVIATION PROBLEM DOES NOT ARISE, and that is a property of the
// entries rather than of this function. Every one was written knowing its first
// sentence would be cut out and shown alone; help_lines/verify.py runs the rest
// of it, and the listing is read by a person on every ask.
std::string first_sentence(std::string_view prose)
{
    size_t end = prose.size();
    for (size_t i = 0; i + 1 < prose.size(); i++)
        if (prose[i] == '.' && (prose[i + 1] == ' ' || prose[i + 1] == '\n')) {
            end = i + 1;
            break;
        }
    std::string out;
    for (size_t i = 0; i < end; i++)
        out += prose[i] == '\n' ? ' ' : prose[i];
    return out;
}

// Every node answered for together with this one, in numbering order.
std::vector<words::PathId> group_of(words::PathId id)
{
    std::vector<words::PathId> out;
    const words::PathId head = head_of(id);
    for (words::PathId i = 1; i <= words::kNodeCount; i++)
        if (head_of(i) == head)
            out.push_back(i);
    return out;
}

// One entry: the path, its number, and what it says.
void entry(std::string &out, const BuiltSet &built, words::PathId id)
{
    const words::NodeId node = static_cast<words::NodeId>(id);
    const std::string path = std::string(words::path_text(node));
    std::string line = "  " + path;

    // THE NUMBER IS PRINTED BESIDE THE PATH, which `satl --words` also does and
    // for the same reason: WORD_NUMBERS §2.2 is a two-column table of paths and
    // numbers, so a person who suspects help of describing the wrong row can
    // read the two side by side without compiling anything.
    line.append(line.size() < 56 ? 56 - line.size() : 2, ' ');
    line += words::number_text(node);
    if (!built.contains(id))
        line += "   not built yet";
    out += line;
    out += "\n\n";

    const Entry &e = entry_of(id);
    indented(out, e.prose, "  ");
    if (e.example[0] != '\0') {
        out += '\n';
        indented(out, e.example, "      ");
    }
    out += '\n';
}

} // namespace

std::string query_text(words::PathId id)
{
    std::string path =
        std::string(words::path_text(static_cast<words::NodeId>(head_of(id))));
    const size_t open = path.find('(');
    if (open != std::string::npos)
        path.erase(open);
    while (!path.empty() && path.back() == '.')
        path.pop_back();
    return path;
}

std::string answer_for(const BuiltSet &built, words::PathId id)
{
    std::string out;
    const std::vector<words::PathId> group = group_of(id);
    for (const words::PathId member : group)
        entry(out, built, member);

    // THE WORDS UNDERNEATH, ONE TO A LINE, AND EACH ONE ONLY ONCE. A child that
    // is written in three shapes -- `input()`, `input(prompt)`,
    // `input(prompt, target)` -- is ONE word to somebody reading a list, so the
    // listing carries heads and not nodes. It is also what makes the line
    // typeable: the head with its argument list off is exactly what goes back
    // inside `satellite.help(...)`.
    std::vector<words::PathId> under;
    for (words::PathId i = 1; i <= words::kNodeCount; i++) {
        // OFFERED, WHICH IS ONE NOTCH NARROWER THAN BUILT. words.def's fourth
        // list has a column for a word that answers when asked about and is
        // never named in a listing -- `satellite.returns`, which is optional
        // and is not how a capsule is written here. `satellite.help(
        // satellite.returns)` still answers in full; it is simply not offered
        // as somewhere to go.
        if (!built.contains(i) || words::is_unlisted(i) || head_of(i) != i)
            continue;
        const words::PathId parent =
            static_cast<words::PathId>(words::parent_of(static_cast<words::NodeId>(i)));
        if (std::find(group.begin(), group.end(), parent) == group.end())
            continue;
        if (std::find(group.begin(), group.end(), i) != group.end())
            continue;
        under.push_back(i);
    }
    if (under.empty())
        return out;

    size_t width = 0;
    std::vector<std::string> paths;
    for (const words::PathId i : under) {
        paths.push_back(query_text(i));
        width = std::max(width, paths.back().size());
    }

    out += "  what is underneath, and every one of these is built:\n\n";
    for (size_t i = 0; i < under.size(); i++) {
        // A TAB AND NOT SPACES, which is the author's spec for this listing and
        // is the one piece of formatting here that was asked for rather than
        // chosen. The blank line between rows is the rest of it.
        out += '\t' + paths[i];
        out.append(width - paths[i].size() + 3, ' ');
        out += first_sentence(entry_of(under[i]).prose);
        out += "\n\n";
    }
    out += "  " + std::to_string(under.size()) +
           " of them -- ask about any by its path.\n";
    return out;
}

} // namespace satellite::help
