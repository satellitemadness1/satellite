// Every row of WORD_NUMBERS.md §2.2, walked. See words_test.hpp.
//
// THE COUNTS ARE CHECKED BEFORE THE ROWS ARE. 247 rows, 244 distinct numbers,
// exactly 3 aliases, 38 `(0)` markers -- if the reader below finds a different
// number of rows than the documents claim, every per-row result after it is
// answering a question nobody asked, and a suite that reports PASS over half a
// table is the green line FORMAT/CXX.md §6 warns about.
//
// AND IT REFUSES TO PASS WHEN IT CANNOT FIND THE FILE. A test whose subject is
// "does the code agree with that document" has exactly one way to be useless,
// and it is to skip quietly when the document is not there.

#include "words_test.hpp"

#include "satellite_words/words.hpp"

#include <fstream>
#include <set>
#include <string>
#include <vector>

using namespace satellite::words;

namespace {

std::vector<std::string> read_lines(const std::string &path)
{
    std::vector<std::string> lines;
    std::ifstream in(path);
    for (std::string line; std::getline(in, line);)
        lines.push_back(line);
    return lines;
}

// The lines of one `###` section, not including its heading.
std::vector<std::string> section(const std::vector<std::string> &lines,
                                 const std::string &heading)
{
    std::vector<std::string> out;
    bool inside = false;
    for (const std::string &line : lines) {
        if (line.rfind("### ", 0) == 0) {
            if (inside)
                break;
            inside = line.rfind(heading, 0) == 0;
            continue;
        }
        if (inside)
            out.push_back(line);
    }
    return out;
}

std::string trim(const std::string &s)
{
    const size_t first = s.find_first_not_of(" \t");
    if (first == std::string::npos)
        return {};
    return s.substr(first, s.find_last_not_of(" \t") - first + 1);
}

std::vector<std::string> columns(const std::string &line)
{
    std::vector<std::string> out;
    size_t at = 0;
    while (at < line.size()) {
        const size_t bar = line.find('|', at);
        if (bar == std::string::npos)
            break;
        if (at)
            out.push_back(trim(line.substr(at, bar - at)));
        at = bar + 1;
    }
    if (at < line.size())
        out.push_back(trim(line.substr(at)));
    return out;
}

// Everything between a pair of backticks, in order.
std::vector<std::string> backticked(const std::string &s)
{
    std::vector<std::string> out;
    for (size_t at = 0;;) {
        const size_t open = s.find('`', at);
        if (open == std::string::npos)
            break;
        const size_t close = s.find('`', open + 1);
        if (close == std::string::npos)
            break;
        out.push_back(s.substr(open + 1, close - open - 1));
        at = close + 1;
    }
    return out;
}

// A cell holding exactly one backticked thing, or empty.
std::string one_backticked(const std::string &cell)
{
    const std::vector<std::string> all = backticked(cell);
    return all.size() == 1 ? all[0] : std::string();
}

// "1 4 (0)" is the node 1 4 whose bare shape is 1 4 0 (WORD_NUMBERS §1.3). The
// marker is stripped here and checked separately, because it is a second number
// riding on the row of the node it belongs to.
std::string without_marker(const std::string &number, bool &marked)
{
    std::string out;
    marked = number.find("(0)") != std::string::npos;
    for (size_t i = 0; i < number.size();) {
        if (number.compare(i, 3, "(0)") == 0) {
            i += 3;
            continue;
        }
        out += number[i++];
    }
    return trim(out);
}

struct Row {
    std::string path;
    std::string number;
    bool marked;
    bool alias;
};

std::vector<Row> read_table(const std::vector<std::string> &lines)
{
    std::vector<Row> rows;
    for (const std::string &line : lines) {
        if (line.rfind("| `", 0) != 0)
            continue;
        const std::vector<std::string> cell = columns(line);
        if (cell.size() < 2)
            continue;
        const std::string path = one_backticked(cell[0]);
        const std::string number = one_backticked(cell[1]);
        if (path.empty() || number.empty())
            continue;
        Row row;
        row.path = path;
        row.marked = false;
        row.number = without_marker(number, row.marked);
        row.alias = cell.size() > 2 && cell[2].find("ALIAS") != std::string::npos;
        rows.push_back(row);
    }
    return rows;
}

std::string named(const Row &row, const std::string &what)
{
    return what + " -- " + row.path + " should be " + row.number;
}

} // namespace

namespace words_test {

void section_authority()
{
    const std::vector<std::string> lines = read_lines(authority_path);
    check(!lines.empty(),
          "WORD_NUMBERS.md could not be read at '" + authority_path +
              "' -- this test's whole subject is whether the code agrees with "
              "that file, so not finding it is a failure and never a skip");
    if (lines.empty())
        return;

    const std::vector<Row> rows = read_table(section(lines, "### 2.2 Every number"));

    // The counts the documents claim, checked before anything is walked.
    // 222 UNTIL 2026-08-31, WHEN M8 APPENDED `digits` `1 6 4 15`, 223 until
    // 2026-09-03, when M12 appended the variant's four, 227 until
    // 2026-09-05, when M16 appended `search(pattern)` under each container,
    // and 231 until 2026-09-08, when M19 appended five: `ok`, `path` and
    // `error` `1 6 2 8`-`1 6 2 10`, `write(x)` `1 6 2 11`, and
    // `satellite.file.exists(path)` `1 8 5`.
    // That is the only kind of edit §1.2 allows to this section -- nothing
    // renumbered and nothing reused -- and it is the only kind these three
    // counts can tell apart from a transcription that dropped a row.
    //
    // M19 ALSO WROTE SIX CALL SHAPES ONTO ROWS THAT HAD NONE, and that edit
    // moves no count at all -- `open` became `open(path, mode)` on the same
    // number. It is visible to this test through the WALK rather than through
    // the counts: match_shape compares argument lists character for character,
    // so a shape written here and not in words.def is a row that stops walking.
    check(rows.size() == 247,
          "§2.2 should hold 247 rows, found " + std::to_string(rows.size()));

    std::set<std::string> numbers;
    size_t aliases = 0, markers = 0;
    for (const Row &row : rows) {
        numbers.insert(row.number);
        aliases += row.alias;
        markers += row.marked;
    }
    check(numbers.size() == 244,
          "§2.2 should carry 244 distinct numbers, found " +
              std::to_string(numbers.size()));
    check(aliases == 3, "§2.2 should declare exactly 3 aliases, found " +
                            std::to_string(aliases));
    check(markers == 38, "§2.2 should carry 38 `(0)` markers, found " +
                             std::to_string(markers));

    // AND NOW THE CONVERSE, WHICH IS THE HALF THIS FUNCTION DID NOT CHECK.
    // *(Added 2026-08-28, found by review.)* Everything above and below walks
    // the authority INTO the code, and that direction only ever catches a row
    // that words.def is MISSING. A row it has and the authority does not takes
    // no number from anybody: appending
    // `SAT_NODE(CONTAINER, CONTAINER_SET, "set", SAT_NUMBERED)` -- inventing
    // `1 4 5`, which WORD_NUMBERS §4 says is deliberately free -- compiled clean
    // and this test printed `ok`, with the digest silently moved and every
    // cached `.satc` on the machine invalidated.
    //
    // The size of the registry is derivable from the authority alone, so it
    // costs no second place for a number to live: every row is a node except an
    // alias, and every `(0)` marker is one more node the row does not count.
    check(kNodeCount == rows.size() - aliases + markers,
          "words.def declares " + std::to_string(kNodeCount) +
              " nodes and §2.2 accounts for " +
              std::to_string(rows.size() - aliases + markers) +
              " -- a row the authority does not have renumbers nothing, so this "
              "count is the only thing that can see it");

    // Every row: the path walks, and it walks to the number the authority says.
    for (const Row &row : rows) {
        const Walk found = walk(row.path);
        if (found.error != WalkError::NONE) {
            check(false, named(row, "does not walk at all"));
            continue;
        }
        const NodeId id = static_cast<NodeId>(found.id);
        check(number_text(id) == row.number,
              named(row, "walks to " + number_text(id) + ", not"));

        // The join rule reproduces the authority's path column character for
        // character -- for everything except an alias, which is a second
        // spelling of a node that has its own.
        if (!row.alias)
            check(path_text(id) == row.path,
                  "§2.2's path column is not reproduced: " + row.path +
                      " prints back as " + path_text(id));

        // A `(0)` marker is a second number on the same row, and it is the one
        // §2.2 never counts. It has to be reachable or the marker means nothing.
        if (row.marked) {
            const Walk bare = walk(row.path + "()");
            check(bare.error == WalkError::NONE &&
                      number_text(static_cast<NodeId>(bare.id)) == row.number + " 0",
                  "the `(0)` on " + row.path + " should make " + row.path +
                      "() walk to " + row.number + " 0");
        }
    }

    // §2.3: two spellings, one number. The aliases are read from the authority
    // too, because the whole of the language's aliasing is nine spellings and
    // hard-coding them here would be a second place they live.
    std::set<std::string> alias_paths;
    for (const Row &row : rows)
        if (row.alias)
            alias_paths.insert(row.path);

    const std::vector<std::string> spellings =
        section(lines, "### 2.3 Two spellings, one number");
    size_t checked = 0, expected_aliases = aliases;
    for (const std::string &line : spellings) {
        if (line.rfind("| `", 0) != 0)
            continue;
        const std::vector<std::string> cell = columns(line);
        if (cell.size() < 2)
            continue;
        const std::vector<std::string> left = backticked(cell[0]);
        const std::vector<std::string> right = backticked(cell[1]);

        if (left.size() == 1 && right.size() == 1) {
            // "<alias> | the number of <real>"
            const Walk a = walk(left[0]), b = walk(right[0]);
            check(a.error == WalkError::NONE && b.error == WalkError::NONE &&
                      a.id == b.id,
                  "§2.3: " + left[0] + " should be the same node as " + right[0]);
            // §2.3 lists only one of the three `.range` rows; the other two are
            // declared in §2.2 and are already counted.
            if (!alias_paths.count(left[0]))
                expected_aliases++;
            checked++;
        } else if (left.size() > 1) {
            // The six spellings of `arguments` (DESIGN §7.7), which are bare
            // words rather than paths: each is checked under the node that owns
            // the special variable.
            const Walk owner = walk("satellite.library.main.arguments");
            for (const std::string &spelling : left) {
                const Walk one = walk("satellite.library.main." + spelling);
                check(one.error == WalkError::NONE && one.id == owner.id,
                      "§7.7: `" + spelling +
                          "` should reach satellite.library.main.arguments");
            }
            check(left.size() == 6, "§7.7 promises six spellings, §2.3 lists " +
                                        std::to_string(left.size()));
            // One of the six is the node's own text and is not an alias.
            expected_aliases += left.size() - 1;
            checked++;
        }
    }
    check(checked == 3, "§2.3 should hold three rows, read " + std::to_string(checked));

    // The same converse, for the aliases. Derived from the two sections rather
    // than written down here, for the same reason the node count is.
    check(kAliasCount == expected_aliases,
          "words.def declares " + std::to_string(kAliasCount) +
              " aliases and §2.2 plus §2.3 account for " +
              std::to_string(expected_aliases));
}

} // namespace words_test
