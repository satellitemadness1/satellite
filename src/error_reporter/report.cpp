// The one place a diagnostic becomes characters. See error_reporter/report.hpp
// for the shape DESIGN §9 asks for and why it is built whole.
//
// "RENDERING IN EXACTLY ONE PLACE" IS THE HALF OF §9 THAT IS A STRUCTURAL
// CLAIM, and the first satellite is what it is measured against: it had THREE
// format_error implementations -- one each for parse, resolve and evaluation --
// and its own header says they "render nothing alike", with the parser's caret
// block and the resolver's one-liner written years apart. §9 credits it for
// having an excerpt and a caret at all, which is true and is the point: the
// hard part was never drawing the caret, it was that three passes each drew
// their own and a program with errors from two of them taught its reader two
// vocabularies for one idea.
//
// SO THIS FILE IS THE WHOLE OF WHAT SATL LOOKS LIKE WHEN SOMETHING IS WRONG.
// Every producer -- the lexer, the parser, the `.satc` reader, and M7 and M9
// after them -- hands over a Diagnostic and gets the same block.
//
// THE FORMAT IS `path:line:column:` BECAUSE THAT IS THE ONE EVERY EDITOR ALREADY
// PARSES. It is not a style choice: an error nobody can jump to is an error
// somebody reads and then goes looking for by hand, and the existing arms in
// main.cpp were already printing `satl: %s:%u: %s` for exactly that reason.
// What this adds to that line is the column, the code, and everything under it.

#include "error_reporter/report.hpp"

#include "error_reporter/codes.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::errors {

namespace {

// A byte in the middle of a UTF-8 character. Skipped when counting columns, so
// that a caret under a line with an accented letter in it lands under the right
// character rather than one place right per extra byte.
//
// SATELLITE SOURCE IS BYTES AND THE CODE TABLE MAPS ONE BYTE TO ONE SatChar
// (satellite_string.hpp), so nothing above this file has a character count to
// hand -- and a Span is byte offsets, which is what makes it sliceable out of
// the source at all. The conversion therefore happens here, once, in the only
// place that needs it.
bool is_continuation(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

// Characters, not bytes, between two byte offsets.
size_t columns_between(std::string_view text, size_t from, size_t to)
{
    size_t columns = 0;
    for (size_t i = from; i < to && i < text.size(); i++)
        if (!is_continuation(text[i]))
            columns++;
    return columns;
}

struct Line {
    size_t begin = 0;
    size_t end = 0;      // one past the last byte, not counting the newline
};

// The line a byte offset falls on.
Line line_around(std::string_view text, size_t at)
{
    Line out;
    if (at > text.size())
        at = text.size();
    const size_t previous = at == 0 ? std::string_view::npos : text.rfind('\n', at - 1);
    out.begin = previous == std::string_view::npos ? 0 : previous + 1;
    const size_t next = text.find('\n', out.begin);
    out.end = next == std::string_view::npos ? text.size() : next;
    // A FILE WRITTEN ON WINDOWS STILL GETS A CARET IN THE RIGHT PLACE. The `\r`
    // is not part of the line as anybody reads it, and left in the excerpt it
    // returns the cursor to column 0 and paints the caret row over the source.
    if (out.end > out.begin && text[out.end - 1] == '\r')
        out.end--;
    return out;
}

// The line as it is printed: tabs to one space each.
//
// ONE SPACE AND NOT A TAB STOP, which is a decision and not a shortcut. How wide
// a tab is depends on the terminal the output lands in, and satl cannot know
// that -- so expanding to eight would put the caret under the wrong character on
// a terminal set to four, and both would be confidently wrong. One space keeps
// the caret's column and the character's column the same number, which is also
// the number the header line reports, so all three agree whatever the file is
// indented with.
std::string excerpt_of(std::string_view text, Line line)
{
    std::string out;
    out.reserve(line.end - line.begin);
    for (size_t i = line.begin; i < line.end; i++)
        out += text[i] == '\t' ? ' ' : text[i];
    return out;
}

std::string digits_of(uint32_t value)
{
    return std::to_string(value);
}

// The widest line number this diagnostic will print, so every bar lines up.
//
// FOUR IS THE FLOOR AND IT IS NOT AESTHETIC. A gutter as wide as the number it
// holds puts a one-line file's excerpt flush against the left margin, where it
// runs into the `satl:` of the header above it and reads as part of the
// sentence rather than as a quotation of the program. Four is also the width
// almost every file in this tree needs anyway -- a thousand lines -- so the
// margin stops moving between one diagnostic and the next in the same run,
// which is what makes a column of them scannable.
constexpr size_t kNarrowestGutter = 4;

size_t gutter_width(const Diagnostic &problem)
{
    size_t width = kNarrowestGutter;
    if (problem.at.somewhere())
        width = std::max(width, digits_of(problem.at.line).size());
    for (const Note &remark : problem.notes)
        if (remark.at.somewhere())
            width = std::max(width, digits_of(remark.at.line).size());
    return width;
}

// A sentence with its `{n}` holes filled.
//
// A MISSING ARGUMENT RENDERS AS `{n}` AND IS NOT DROPPED. It cannot happen
// through make() or note(), which check the count at compile time -- what can
// reach here is a Diagnostic built field by field, which is what a test does
// and what a future reader of a serialised diagnostic would do. Printing the
// hole is the answer that says a hole is what is missing.
std::string fill(std::string_view text, const std::vector<std::string> &arguments)
{
    std::string out;
    out.reserve(text.size() + 32);
    for (size_t i = 0; i < text.size(); i++) {
        const bool hole = text[i] == '{' && i + 2 < text.size() &&
                          text[i + 2] == '}' && text[i + 1] >= '1' &&
                          text[i + 1] <= '9';
        if (!hole) {
            out += text[i];
            continue;
        }
        const size_t which = static_cast<size_t>(text[i + 1] - '1');
        if (which < arguments.size())
            out += arguments[which];
        else
            out.append(text, i, 3);
        i += 2;
    }
    return out;
}

const char *severity_word(Code code)
{
    return severity_of(code) == Severity::ERROR ? "error" : "note";
}

// `hello.satl:4:25: `, or `line 4: ` with no path, or `hello.satl: ` with no
// span, or nothing at all.
//
// EACH PART IS DROPPED RATHER THAN FILLED WITH A PLACEHOLDER, and all four
// cases are real: a `.satc` that is not one has a path and no place in it, and
// a test renders text with no path. The first satellite's `span_location` made
// the same split for the same reason and its comment is the one to keep --
// WHERE an error happened must read the same everywhere.
std::string location(const Diagnostic &problem, const Source &source)
{
    std::string out;
    if (!source.path.empty())
        out += std::string(source.path);
    if (problem.at.somewhere()) {
        const Line line = line_around(source.text, problem.at.start);
        const size_t column = columns_between(source.text, line.begin,
                                              std::min<size_t>(problem.at.start,
                                                               line.end)) + 1;
        if (out.empty())
            out += "line " + digits_of(problem.at.line);
        else
            out += ":" + digits_of(problem.at.line) + ":" + std::to_string(column);
    }
    if (!out.empty())
        out += ": ";
    return out;
}

// The source line and the caret under it, or nothing when there is no place or
// no text to quote.
//
// THE CARET IS CLAMPED TO THE LINE, which is the first satellite's hardest-won
// line in this file and it says why in its own words: a span is an offset into
// the text it is rendered against, so a caller with a stale span and a short
// text "would pad the caret line with thousands of spaces". Clamping draws it
// at the end of the line instead, "which is wrong but bounded and visibly
// wrong". Reached here by rendering a `.satc`'s parse error against the
// SOURCE's text, which is a mistake somebody will make.
std::string caret_block(Span at, const Source &source, size_t width)
{
    if (!at.somewhere() || source.text.empty())
        return {};

    const Line line = line_around(source.text, at.start);
    const size_t start = std::min<size_t>(at.start, line.end);
    const size_t stop = std::min<size_t>(std::max(at.end, at.start + 1), line.end);
    const size_t column = columns_between(source.text, line.begin, start);
    const size_t span = std::max<size_t>(columns_between(source.text, start, stop), 1);

    const std::string number = digits_of(at.line);
    std::string out;
    out += std::string(width - std::min(width, number.size()), ' ') + number;
    out += " | " + excerpt_of(source.text, line) + "\n";
    out += std::string(width, ' ') + " | " + std::string(column, ' ') +
           std::string(span, '^') + "\n";
    return out;
}

} // namespace

std::string sentence(const Diagnostic &problem)
{
    return fill(text_of(problem.code), problem.arguments);
}

std::string sentence(const Note &remark)
{
    return fill(text_of(remark.code), remark.arguments);
}

std::string render(const Diagnostic &problem, const Source &source)
{
    const size_t width = gutter_width(problem);
    // Everything under the header lines up with the excerpt's TEXT rather than
    // with its bar, so a note reads as part of the block above it instead of as
    // a second thing that happens to be indented.
    const std::string indent(width + 3, ' ');

    std::string out = "satl: " + location(problem, source) +
                      severity_word(problem.code) + " " +
                      std::string(code_text(problem.code).view()) + ": " +
                      sentence(problem) + "\n";
    out += caret_block(problem.at, source, width);

    // DESIGN §4.6's payoff, and it goes under the caret because it is about the
    // word the caret is under. The sentence is here and the WORD came from the
    // trie -- suggest.hpp says why that split matters.
    if (!problem.suggestion.empty())
        out += indent + "did you mean `" + problem.suggestion + "`?\n";

    for (const Note &remark : problem.notes) {
        out += indent + severity_word(remark.code) + " " +
               std::string(code_text(remark.code).view()) + ": " +
               sentence(remark) + "\n";
        out += caret_block(remark.at, source, width);
    }

    // The call stack, innermost first. Nothing produces one at M5; report.hpp
    // says why it is drawn anyway and tests/reporter_test is what keeps this
    // arm working until M9 has a frame to put in it.
    for (const FrameRef &frame : problem.frames) {
        out += indent + "in " +
               (words::is_language_word(frame.capsule)
                    ? std::string(words::path_text(
                          static_cast<words::NodeId>(frame.capsule)))
                    : std::string("a capsule this program declared"));
        if (frame.at.somewhere())
            out += ", called at line " + digits_of(frame.at.line);
        out += "\n";
    }
    return out;
}

std::string render(const std::vector<Diagnostic> &problems, const Source &source)
{
    std::string out;
    for (const Diagnostic &problem : problems)
        out += render(problem, source);
    return out;
}

bool any_error(const std::vector<Diagnostic> &problems)
{
    for (const Diagnostic &problem : problems)
        if (problem.is_error())
            return true;
    return false;
}

} // namespace satellite::errors
