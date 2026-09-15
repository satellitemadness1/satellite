// The arguments object as the text a person reads -- DESIGN §7.7's
// "displaying it bare prints all of it".
//
// WHY THIS IS NOT IN satellite_value/render.cpp WITH EVERY OTHER ARM. The
// renderer there turns a value into characters and knows nothing about the
// registry; this is the module's own subject. The value renderer keeps one
// line -- the arm -- and calls in here.
//
// AND THE WALK IS NOT HERE EITHER, SINCE THE TEN SELECTORS LANDED. What the
// object holds is `entries_of()` in rows.cpp, because `count()`, `keys()`,
// `has(k)` and `get(k)` ask the same question this printer does and a second
// walk would be a second answer to it. What is left in this file is the two
// columns.
//
// TWO COLUMNS, ONE ENTRY PER LINE, NAMES LEFT-ALIGNED -- v1's printer, ported.
// It is the one rendering in the language that is not a single line, and the
// reason is what §1.1's tie-breaker says it is for: this is the whole of what
// the runtime knows, handed over at once, and thirty-odd facts on one line
// would be a thing nobody could read.

#include "satellite_arguments/rows.hpp"

#include "satellite_value/render.hpp"

#include <string>
#include <vector>

namespace satellite::arguments {

std::string object_text(const Arguments &body)
{
    const std::vector<Entry> entries = entries_of(body);

    size_t width = 0;
    for (const Entry &entry : entries)
        if (entry.name.size() > width)
            width = entry.name.size();

    std::string out;
    for (size_t i = 0; i < entries.size(); i++) {
        if (i != 0)
            out += "\n";
        out += entries[i].name;
        out.append(width - entries[i].name.size() + 2, ' ');
        out += text_of(entries[i].value);
    }
    return out;
}

} // namespace satellite::arguments
