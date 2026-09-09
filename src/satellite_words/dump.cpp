// What `satl --words` prints. See satellite_words/dump.hpp.
//
// THE COLUMN IS THE POINT. WORD_NUMBERS §2.2 is a two-column table of paths and
// numbers, and so is this, so the two can be read side by side or diffed by
// somebody who suspects a transcription error and does not want to compile a
// test to find out. tests/words_test is what PROVES they agree; this is what
// lets a person see it.

#include "satellite_words/dump.hpp"

#include "error_reporter/suggest.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

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

// The children of one node, and the number the next one would take.
//
// M20 IS WHY THIS EXISTS AND PLAN.md SAYS SO IN ITS OWN WORDS: "minting forty
// rows by opening words.def and counting is the one way this milestone can
// silently renumber something; asking the binary cannot be wrong about the
// binary." So this is M20's first commit and it lands before any of its rows --
// a tool, and then the work the tool is for.
//
// THE FREE NUMBER COMES FROM Words AND NOT FROM frozen_children() + 1, and the
// two are the same answer today for exactly one reason: a fresh Words has met
// no user names. Recomputing the sum here would be a second implementation of
// M2's allocation rule, sitting next to the first, free to drift the day
// WORD_NUMBERS §3 grows a case -- and the whole argument for this command is
// that it asks the machine rather than repeating it. words_runtime.hpp's
// next_free() is the allocator, so next_free() is what is printed.
//
// AND IT IS THAT FUNCTION'S FIRST CALLER OUTSIDE A TEST. It has been in
// words_runtime.hpp since M2, exercised only by tests/words_test/runtime.cpp --
// which PLAN M2 wrote down as deliberate ("until then the only consumer is
// tests/words_test") and PLAN M20 then overstated as "no caller anywhere in the
// tree". The test is a caller and a real one; what there was none of is a
// caller a PERSON can reach, which is what M2's own rule about a registry
// getting a consumer is asking for.
//
// THE BARE SHAPE IS LISTED AND NOT COUNTED, because that is what it is:
// WORD_NUMBERS §1.3 makes position 0 a real number, and words_numbers.hpp keeps
// the counter still when it passes one, so a parent's numbered children stay
// dense from 1 whether it has a bare shape or not. Printing it inside the count
// would make this command disagree with the numbering it reports.
std::string children_text(NodeId id)
{
    size_t numbered = 0;
    bool bare = false;
    size_t column = 0;
    for (PathId c = first_child(id); c != kNoPath; c = next_sibling(c)) {
        const NodeId child = static_cast<NodeId>(c);
        // THE COLUMN IS MEASURED OVER WHAT IS PRINTED HERE, not over the
        // language -- the same choice the alias block below makes, and for the
        // same reason: padding a six-character path out to the width of
        // satellite.window.console.new(title, width, height) is a column with a
        // gap in it rather than a column.
        column = std::max(column, path_text(child).size());
        if (is_bare(child))
            bare = true;
        else
            numbered++;
    }

    std::string out = "  children ";
    if (numbered == 0 && !bare)
        out += "none\n";
    else {
        out += numbered == 0 ? std::string("none numbered")
                             : std::to_string(numbered) + " numbered";
        out += bare ? ", and a bare shape\n" : "\n";
    }

    if (numbered != 0 || bare) {
        out += '\n';
        for (PathId c = first_child(id); c != kNoPath; c = next_sibling(c))
            row(out, "  " + path_text(static_cast<NodeId>(c)),
                number_text(static_cast<NodeId>(c)), column + 4);
        out += '\n';
    }

    // ONE Words PER CALL AND NOT A STATIC. It is a kilobyte of counters and a
    // command that answers once, and words_runtime.hpp's own reason for the
    // class -- "a run's names end with the run" -- is worth more here than the
    // allocation it saves. It also has to be FRESH: next_free() is one past the
    // frozen count only for a numbering that has defined nothing, which is a
    // property tests/words_test/runtime.cpp states and demonstrated by failing
    // on satellite.library when it was first written over a used one.
    const Words words;
    out += "  the next number free under it is " + number_text(id) + " " +
           std::to_string(words.next_free(id));

    // AND WHETHER 0 IS FREE, WHICH next_free() CANNOT SAY. It counts numbered
    // children, and a bare shape is the one child that takes no position from
    // its siblings -- so a node with no bare row has a free number the
    // allocator will never offer, and M20 needs it five times over: `system`,
    // `build`, `interpreter`, `process` and `session` are all new parents under
    // `arguments`, and every parent in words.def carries a `()` row at 0.
    //
    // A COMMAND THAT ANSWERS "WHAT IS FREE" MUST ANSWER FOR 0 TOO, or the one
    // row it stays silent about is the row somebody mints by hand.
    out += bare ? ", and 0 is taken by the bare shape\n"
                : ", and " + number_text(id) + " 0 is free for a bare shape\n";
    return out;
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
    // above exists as a number; most of it still does not run.
    //
    // AND M18 BUILT THE THING THIS PARAGRAPH USED TO PROMISE. It said "there is
    // no handler table yet" and "there is no evaluator yet", which stopped being
    // true at M9 and M10 and went on being printed until 2026-09-08. The answer
    // it pointed at is now `satellite.help`, which walks this same trie and
    // names a node only when it is BUILT -- a handler row, an assigner row, a
    // front-end word, or something built underneath it. satellite_help/built.hpp
    // is the predicate and DESIGN §4.6 carries the correction.
    //
    // THIS COMMAND IS STILL NOT MARKED, AND THAT IS A DECISION RATHER THAN A
    // LEFTOVER. Marking each row here would put `eval::Handlers` behind
    // satellite_words -- and words.hpp's whole claim is that "the numbering can
    // be read by a test, a future .satc reader or a disassembler without
    // dragging the interpreter in behind it", which words_test relies on by
    // linking no objects at all. The mark would have to arrive as a predicate
    // passed in by programs/dump_commands.cpp; it is worth doing and it is not
    // this milestone's, because the question `--words` answers is what the
    // registry SAYS and `satellite.help` is now the command that answers what
    // the language DOES. MILESTONES/M18.md §4 records it.
    out += "\n"
           "Every path above is NUMBERED and most of it is not BUILT. This is\n"
           "the registry M2 wrote; `satellite.help` is what the language answers\n"
           "about the part of it that runs today, and PLAN.md §8 says what\n"
           "lands when.\n";
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
        out += children_text(id);
        return out;
    }

    // A FAILURE NAMES THE SEGMENT AND THE NODE IT WAS LOOKED UP UNDER, because
    // those are the two things an answer needs and the first satellite's was
    // "no such module function: satellite.consle.display" -- a sentence that
    // repeats the question. M2 built the information and said M5 would turn it
    // into the caret and the "did you mean"; M5 landed 2026-08-30, and the
    // NO_SUCH_WORD arm below is the second half arriving.
    //
    // NOT THROUGH errors::render, AND THAT IS DELIBERATE. A path typed on a
    // command line is not a place inside a file: there is no source to quote,
    // no line and no column, and a Span whose offsets index an ARGUMENT rather
    // than a program would be a span that means something different from every
    // other one in the tree. What this shares with the reporter is the
    // suggester, which is the part that is actually the same fact.
    //
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
    case WalkError::NO_SUCH_WORD: {
        out += "  no such word under " + path_text(static_cast<NodeId>(found.under)) +
               "\n";
        // DESIGN §4.6's WORKED EXAMPLE, ANSWERED BY THE COMMAND IT IS WRITTEN
        // ABOUT. `satl --words satellite.consle.display` is where somebody asks
        // the question that section poses, and until M5 the answer stopped at
        // "no such word under satellite" -- which is one sentence better than
        // the first satellite's and still not the one §4.6 promises.
        //
        // THE SEGMENT IS SLICED OUT OF THE PATH RATHER THAN CARRIED IN THE
        // Walk, because `offset` plus the delimiter set is the whole of what
        // words_walk.hpp needs to say and adding a string_view to that struct
        // would make it own a pointer into the caller's text.
        const size_t end = path.find_first_of(".(", found.offset);
        const std::string_view segment =
            std::string_view(path).substr(found.offset,
                                          end == std::string::npos
                                              ? std::string::npos
                                              : end - found.offset);
        if (const std::string_view meant = errors::suggest(found.under, segment);
            !meant.empty())
            out += "  did you mean `" + std::string(meant) + "`?\n";
        break;
    }
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
