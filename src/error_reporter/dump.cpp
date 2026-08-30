// What `satl --errors` prints. See error_reporter/dump.hpp.
//
// THE HOLES ARE PRINTED AS THEY ARE WRITTEN, `{1}` and all, and the trailing
// note says so. Filling them with something plausible would be a dump that
// shows sentences no program will ever produce; showing them empty would hide
// that a message has a variable part at all. What a person looking a code up
// wants to know is which sentence they are reading and where its holes were --
// satellite_words/dump.cpp makes the same argument about printing a numbering
// in file order rather than sorted.

#include "error_reporter/dump.hpp"

#include "error_reporter/codes.hpp"

#include <string>
#include <string_view>

namespace satellite::errors {

namespace {

// The sentence, wrapped under a hanging indent, because half of these are two
// lines wide and a dump whose rows wrap at the terminal's edge stops being a
// table. Measured against the table rather than guessed: the longest sentence
// in errors.def is NOTE_SATC_IGNORED at 158 characters.
constexpr size_t kWidth = 78;

void wrapped(std::string &out, const std::string &prefix, std::string_view text)
{
    const std::string hanging(prefix.size(), ' ');
    std::string line = prefix;
    size_t at = 0;
    while (at < text.size()) {
        size_t space = text.find(' ', at);
        if (space == std::string_view::npos)
            space = text.size();
        const std::string_view word = text.substr(at, space - at);
        if (line.size() > hanging.size() && line.size() + 1 + word.size() > kWidth) {
            out += line + "\n";
            line = hanging;
        }
        if (line.size() > hanging.size())
            line += ' ';
        line += word;
        at = space + 1;
    }
    out += line + "\n";
}

std::string row_prefix(Code code)
{
    return "  " + std::string(code_text(code).view()) + "  " +
           (severity_of(code) == Severity::ERROR ? "error  " : "note   ");
}

} // namespace

std::string dump_text()
{
    std::string out;
    for (const Code code : kCodes)
        wrapped(out, row_prefix(code), text_of(code));

    size_t errors = 0;
    for (const Code code : kCodes)
        if (severity_of(code) == Severity::ERROR)
            errors++;

    out += "\n";
    out += "  " + std::to_string(kCodeCount) + " codes, " +
           std::to_string(errors) + " errors and " +
           std::to_string(kCodeCount - errors) + " notes\n";

    // WHAT IS REGISTERED IS NOT WHAT IS RAISED, and this line is the same
    // honesty `satl --words` ends with. Almost every code above belongs to the
    // lexer, the parser or the cache, which are built; the blocks errors.def
    // reserves for resolve, numbers and the evaluator hold nothing yet, and a
    // dump with no such line would read as a list of everything that can go
    // wrong in a language where almost nothing runs.
    out += "\n"
           "`{1}` and its siblings are holes a message fills in from the\n"
           "program. errors.def reserves S05xx for resolve, S06xx for numbers\n"
           "and S07xx for the evaluator; none of those has landed, so none of\n"
           "them has a code here yet.\n";
    return out;
}

std::string explain_text(const std::string &code, bool &found)
{
    const Code which = code_of(code);
    found = which != Code::NONE;
    if (!found)
        return "satl: " + code + " is not a code this satl has. `satl --errors`\n"
               "      lists every one of them.\n";

    std::string out;
    out += "  " + std::string(code_text(which).view()) + "  " +
           (severity_of(which) == Severity::ERROR ? "error" : "note") + "\n";
    out += "  " + std::string(ident_of(which)) + "\n";
    wrapped(out, "  ", text_of(which));
    if (arity_of(which) != 0)
        out += "  " + std::to_string(arity_of(which)) +
               (arity_of(which) == 1 ? " hole, filled from the program\n"
                                     : " holes, filled from the program\n");
    return out;
}

} // namespace satellite::errors
