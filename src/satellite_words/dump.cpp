// What `satl --words` prints. See satellite_words/dump.hpp.
//
// THE COLUMN IS THE POINT. WORD_NUMBERS §2.2 is a two-column table of paths and
// numbers, and so is this, so the two can be read side by side or diffed by
// somebody who suspects a transcription error and does not want to compile a
// test to find out. tests/words_test is what PROVES they agree; this is what
// lets a person see it.

#include "satellite_words/dump.hpp"

#include "satellite_words/words.hpp"

#include <algorithm>
#include <string>

namespace satellite::words {

namespace {

// The width the numbers start at. Measured from the table rather than guessed,
// and measured over exactly what this function prints: the longest path_text is
// satellite.window.console.new(title, width, height) at 50 characters. A column
// narrower than the widest row is a column that stops being one.
//
// IT IS COMPUTED HERE RATHER THAN WRITTEN DOWN, which is why the number above is
// a note and not a constant. The first version of this comment said 48 and named
// satellite.library.main.arguments.machine.threads -- which is the longest path
// counting only WORD segments, and this column includes the argument list. The
// code was right and the comment was measuring a different thing.
size_t path_column()
{
    size_t widest = 0;
    for (PathId i = 1; i <= kNodeCount; i++) {
        const size_t width = path_text(static_cast<NodeId>(i)).size();
        if (width > widest)
            widest = width;
    }
    return widest + 2;
}

void row(std::string &out, const std::string &left, const std::string &right,
         size_t column)
{
    out += left;
    out.append(left.size() < column ? column - left.size() : 1, ' ');
    out += right;
    out += '\n';
}

} // namespace

std::string dump_text()
{
    const size_t column = path_column();
    std::string out;

    // IN FILE ORDER, WHICH IS NUMBERING ORDER, and not sorted. Sorting would
    // put 1 4 2 10 before 1 4 2 2, because the numbers are a sequence and not a
    // string -- and a dump that reorders the one thing the file's order MEANS
    // is a dump that hides the property it exists to show.
    for (PathId i = 1; i <= kNodeCount; i++) {
        const NodeId id = static_cast<NodeId>(i);
        row(out, path_text(id), number_text(id), column);
    }

    // The aliases last, each naming the node it is a second spelling of, so
    // that a reader who has just seen 254 rows with distinct numbers is not
    // left to wonder why three numbers appear twice.
    //
    // The numbers are padded to the widest of THEM rather than to the widest in
    // the language, so the `=` column lines up here without a gap the size of
    // `1 14 1 1 1 1` in a block where nothing is six numbers deep.
    out += '\n';
    size_t widest = 0;
    for (size_t i = 0; i < kAliasCount; i++)
        widest = std::max(widest, number_text(kAliases[i].of).size());
    for (size_t i = 0; i < kAliasCount; i++) {
        const NodeId of = kAliases[i].of;
        std::string number = number_text(of);
        number.append(widest - number.size() + 3, ' ');
        row(out, path_text(parent_of(of)) + "." + kAliases[i].text,
            number + "= " + path_text(of), column);
    }

    out += '\n';
    out += "  " + std::to_string(kNodeCount) + " nodes, " +
           std::to_string(kAliasCount) + " aliases\n";
    out += "  digest " + digest_text() + "\n";

    // WHAT IS NUMBERED IS NOT WHAT IS BUILT, and this line is here because
    // DESIGN §4.6 was wrong about exactly that until 2026-08-28. Everything
    // above exists as a number; almost none of it runs. satellite.help walks
    // this same trie at M8.5 and must print a node only when handlers[path_id]
    // is non-null -- there is no handler table yet, so a dump that did not say
    // so would advertise the whole language as working, which DESIGN §1.1 calls
    // doing something behind the user's back.
    out += "\n"
           "Every path above is NUMBERED. Almost none of it is BUILT -- there\n"
           "is no evaluator yet, and this is the registry M2 exists to write.\n"
           "PLAN.md §8 says what lands when.\n";
    return out;
}

std::string walk_text(const std::string &path, bool &resolved)
{
    const Walk found = walk(path);
    resolved = found.error == WalkError::NONE;
    if (resolved) {
        const NodeId id = static_cast<NodeId>(found.id);
        std::string out = "  " + path + "\n";
        out += "  number   " + number_text(id) + "\n";
        out += "  path     " + path_text(id) + "\n";
        out += "  depth    " + std::to_string(depth_of(id)) + "\n";
        return out;
    }

    // A FAILURE NAMES THE SEGMENT AND THE NODE IT WAS LOOKED UP UNDER, because
    // those are the two things an answer needs and the first satellite's was
    // "no such module function: satellite.consle.display" -- a sentence that
    // repeats the question. M5 turns this into the reporter with the caret and
    // the "did you mean"; what M2 owes it is the information, now.
    // THE PATH IS INDENTED SO THE CARET CAN BE. `offset` is an index into the
    // path, so the caret line has to start where the path line starts -- and
    // printing the path flush left while indenting the caret by two put it two
    // characters past the segment it was pointing at, which is worse than no
    // caret because it accuses the wrong word.
    std::string out = "  " + path + "\n";
    out += "  " + std::string(found.offset, ' ') + "^\n";
    switch (found.error) {
    case WalkError::NOT_ROOTED:
        out += "  not rooted at `satellite` -- DESIGN §1: a dotted path rooted\n"
               "  at `satellite` names something the language owns, and a bare\n"
               "  identifier names something the user owns.\n";
        break;
    case WalkError::NO_SUCH_WORD:
        out += "  no such word under " + path_text(static_cast<NodeId>(found.under)) +
               "\n";
        break;
    case WalkError::NO_SUCH_SHAPE:
        out += "  " + path_text(static_cast<NodeId>(found.under)) +
               " has that word, but not with those arguments\n";
        break;
    case WalkError::TRAILING:
        out += "  the path ended and the text did not\n";
        break;
    case WalkError::NONE:
        break;
    }
    return out;
}

} // namespace satellite::words
