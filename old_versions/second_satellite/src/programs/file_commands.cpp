// The lexer's consumer and the parser's. See programs/file_commands.hpp for the
// rule the two of them share, and programs/dump_commands.hpp for the seam.
//
// MOVED OUT OF main.cpp AT M7 AND OTHERWISE UNCHANGED, comments included. Every
// paragraph below was written by the milestone that added the arm -- including
// the two corrections M5 made to them -- and rewriting them while moving them
// would have thrown that away in a commit about line count.

#include "programs/file_commands.hpp"

#include "abstract_syntax_tree/unparse.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/dump.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "parser/parser.hpp"
#include "programs/check_command.hpp"
#include "programs/opening.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>

namespace satellite {

// THE LEXER'S CONSUMER, AND THE REASON M3 HAS ONE -- the same rule `--words`
// records, one milestone on. M3 produces a token stream that no parser reads
// until M4, so this is the only way to see what it decided.
//
// NOT AN ARM THAT SAYS "not built yet". The lexer IS built, so it answers.
int tokens_command(const std::string &path)
{
    // A file that cannot be READ is a different failure from a file that cannot
    // be LEXED, and they get different words, different codes and different
    // streams. The first is the user's command line; the second is their
    // program.
    std::string source;
    if (!open_source(path, source))
        return EXIT_USAGE;

    // THE DUMP GOES TO STDOUT EVEN WHEN THE PROGRAM IS MALFORMED, and
    // file_commands.hpp carries the whole argument for that -- including the
    // empty tokens.txt this arm produced on its first day.
    //
    // THE EXIT STATUS WAS WRONG UNTIL M5 AND IS THE ORIGINAL OF THE THREE.
    // MILESTONES/M3.md §6 item 2 opened it: non-zero was right, and EXIT_USAGE
    // was the only non-zero code there was, and its own definition is "the
    // command line did not name something satl can do", which a bad program is
    // not. EXIT_MALFORMED is the code, and programs/opening.hpp carries why it
    // is 1.
    bool clean = false;
    const std::string dump = tokens_text(source, clean);
    fputs(dump.c_str(), stdout);
    report(path, source, diagnostics_of(lex(source)));
    return clean ? EXIT_FINE : EXIT_MALFORMED;
}

// THE PARSER'S CONSUMER, AND THE REASON M4 HAS ONE -- and it is the strongest of
// the arms: `--words` prints a table and `--tokens` prints a list, while this
// prints a SATELLITE PROGRAM, which satl can read back. PLAN M4 states the
// milestone in this command -- "satl --unparse file.satl round-trips, which is
// how we know the parser is right before anything can run" -- because nothing
// runs until M10 and a tree is otherwise only visible to whoever wrote the code
// that built it.
//
// WHAT COMES BACK IS NOT THE FILE. Comments are gone (DESIGN §5.6 discards
// them), blank lines were never tokens, and brackets a program wrote around a
// single value are gone too. What is guaranteed is that printing this output and
// parsing it again gives the same text -- a fixpoint, which
// abstract_syntax_tree/unparse.hpp argues is the strongest statement available
// and a real one.
int unparse_command(const std::string &path)
{
    std::string source;
    if (!open_source(path, source))
        return EXIT_USAGE;

    // A RUN'S NAMES END WITH THE RUN, which is why this is a local and not a
    // global: words_runtime.hpp makes the point that M22 runs many programs in
    // one process and each needs its own numbering.
    words::Words words;
    const Parse parsed = parse(source, words);

    // THE ANSWER GOES TO STDOUT AND THE COMPLAINTS TO STDERR. Both are printed
    // for a partly-parsed file, because what was understood is an answer even
    // when the whole file was not.
    //
    // AND THE COMPLAINTS ARE THE REPORTER'S NOW, which is what M5 changed here:
    // this arm used to compose `satl: %s:%u: %s` itself, which was the fourth
    // place in the tree that knew what an error looks like.
    report(path, source, parsed.errors);
    fputs(unparse(parsed.ast).c_str(), stdout);
    return parsed.ok() ? EXIT_FINE : EXIT_MALFORMED;
}

} // namespace satellite
