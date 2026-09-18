#pragma once
// satellite/machine/source_position.hpp -- WHERE IT WENT WRONG, IN THE PERSON'S
// OWN TEXT. SATELLITE_ERROR E1-E6.
//
// The author's sketch of the report has a row nothing has ever filled in:
//
//     directory: path
//     syntax: code here
//                          /\
//                        however you had 003
//
// This is what fills it. Three questions, and SATELLITE_ERROR Part 3 measured
// what each one costs before any of it was written:
//
//   WHICH FILE   free. BytecodeFilenames is kept row for row with the registry
//                and one row is one FILE, so filenames[which_row] is the answer.
//   WHICH LINE   a count of line_end_tokens before the position. Already built,
//                in statement_ring.hpp, for the same reason it is built at all:
//                O(n) is unthinkable per statement and nothing at all per report.
//   WHICH TEXT   RE-READ THE FILE. The registry holds codes and should keep
//                holding codes -- "a row stays nothing but codes and a name never
//                has to be spelled in tokens to be carried". One open and one
//                getline on a path that is already failing costs nothing, and it
//                shows what the person ACTUALLY TYPED rather than a
//                reconstruction from tokens that would differ in its spacing.
//   WHICH COLUMN the one real piece of work, and it is done by re-tokenising the
//                single line just re-read, with the lexer handing out its own
//                byte offsets. See tokenise_one_line's `offsets`.
//
// EVERYTHING HERE RUNS ONLY AFTER SOMETHING HAS ALREADY FAILED. There is no
// field added to a token, no store on the hot path, and nothing kept in memory
// against the possibility of an error. That is the whole design: keep what is
// free while running, compute what is expensive only when it is already over.

#include "critical_report.hpp"
#include "../bytecode/bytecode_registry.hpp"
#include "../bytecode/statement_ring.hpp"
#include "../bytecode/token_codes.hpp"

#include <bitset>
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace satellite004 {

// E3 -- ONE LINE OF A FILE, AS THE PERSON WROTE IT. 1-based, to match line_of()
// and to match what every editor calls that line. Answers "" when the file
// cannot be re-read, which is a real state: it was read once at load and a
// program can run long enough for somebody to move it.
inline std::string source_line(const std::string &filename, std::size_t line)
{
    if (filename.empty() || line == 0) return std::string();
    std::ifstream file(filename);
    if (!file.is_open()) return std::string();
    std::string text;
    for (std::size_t n = 0; n < line; ++n)
        if (!std::getline(file, text)) return std::string();
    return text;
}

// Where the line holding `at` starts in the row: one past the line_end_token
// before it, or 0. The registry is every line of a file concatenated, each ended
// by tokenise_one_line's own line_end_token, so a line's tokens sit between two
// of them and nothing else divides them.
inline std::size_t line_starts_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const std::size_t stop = at < row.size() ? at : row.size();
    std::size_t start = 0;
    for (std::size_t i = 0; i < stop; ++i)
        if (static_cast<token::Code>(row[i].to_ulong()) == token::line_end_token)
            start = i + 1;
    return start;
}

// E4 -- THE CARET'S COLUMN, 0-based, in the text source_line() answers.
//
// HOW IT IS KNOWN TO BE RIGHT: the row is every line tokenised one after
// another, so the tokens between the line's start and `at` are exactly the
// tokens tokenise_one_line makes from that line's text -- index for index. So
// re-tokenising the re-read text and taking offset[at - line_start] is not an
// estimate, it is the same lexer answering the same question a second time.
//
// Answers 0 when the text cannot be had or the position is past its tokens,
// which puts the caret under the first character rather than nowhere.
inline std::size_t column_of(const std::vector<std::bitset<16>> &row, std::size_t at,
                             const std::string &line_text)
{
    if (line_text.empty()) return 0;
    const std::size_t start = line_starts_at(row, at);
    if (at < start) return 0;
    const std::size_t which = at - start;

    std::vector<std::bitset<16>> again;
    std::vector<std::size_t> offsets;
    tokenise_one_line(line_text, again, &offsets);
    if (which >= offsets.size()) return 0;
    return offsets[which];
}

// WHERE ONE POSITION IS, all four answers together. Everything a report's
// `directory:` and `syntax:` rows need, gathered in one call so no caller has to
// know the order they are worked out in.
struct SourcePlace {
    std::string filename;
    std::size_t line = 0;      // 1-based
    std::size_t column = 0;    // 0-based, into `text`
    std::string text;          // the line as the person wrote it; "" if unreadable
    bool known = false;        // false means the position named no row we hold
};

inline SourcePlace place_of(const BytecodeRegistry &registry, const BytecodeFilenames &filenames,
                            std::size_t which_row, std::size_t at)
{
    SourcePlace place;
    if (which_row >= registry.size()) return place;
    place.known = true;
    place.filename = which_row < filenames.size() ? filenames[which_row] : std::string();
    place.line = line_of(registry[which_row], at);
    place.text = source_line(place.filename, place.line);
    place.column = column_of(registry[which_row], at, place.text);
    return place;
}

// E6 -- FILL A REPORT'S `directory:` AND `syntax:` ROWS FROM A POSITION, so a
// caller passes a place and never a file, a line, a column or a line of text.
//
// THE LINE IS TRIMMED OF ITS LEADING BLANKS AND THE CARET MOVES WITH IT. A
// statement eight levels deep is indented past the report's eighty columns, and a
// syntax row that wraps puts the caret under the wrong character -- which is
// worse than no caret, because it is confidently wrong.
inline void report_at(CriticalReport &report, const SourcePlace &place)
{
    if (place.known == false) return;

    report.directory = place.filename.empty() ? std::string("(a file with no name)") : place.filename;
    if (place.line != 0)
        report.directory += ":" + std::to_string(place.line);

    if (place.text.empty()) {
        // SAID, RATHER THAN LEFT BLANK. Part 7's rule 3: a section that could
        // not be gathered prints as that and never as empty, so nobody reads a
        // missing line as an empty line.
        report.syntax = "(the file could not be re-read to show this line)";
        report.caret_at = std::string::npos;
        return;
    }

    std::size_t blanks = 0;
    while (blanks < place.text.size() && (place.text[blanks] == ' ' || place.text[blanks] == '\t'))
        ++blanks;
    report.syntax = place.text.substr(blanks);
    report.caret_at = place.column >= blanks ? place.column - blanks : 0;
}

inline void report_at(CriticalReport &report, const BytecodeRegistry &registry,
                      const BytecodeFilenames &filenames, std::size_t which_row, std::size_t at)
{
    report_at(report, place_of(registry, filenames, which_row, at));
}

// WHICH ROW A ROW IS, BY ITS ADDRESS.
//
// Half the walker's functions are handed one row and not its index, and the
// index is what names the file. Rather than thread a `which_row` through every
// one of them for a path that runs at most once a run, the row is found by
// WHERE IT IS: a row lives in the registry and nothing copies one, so its
// address identifies it exactly.
//
// A linear walk over the FILES -- there are as many of these as the program has
// includes, not as many as it has lines -- and it happens after something has
// already failed. Answers the registry's size for a row that is in no registry,
// which place_of() then reads as "no position known".
inline std::size_t row_index_of(const BytecodeRegistry *registry,
                                const std::vector<std::bitset<16>> &row)
{
    if (registry == nullptr) return 0;
    for (std::size_t i = 0; i < registry->size(); ++i)
        if (&(*registry)[i] == &row) return i;
    return registry->size();
}

} // namespace satellite004
