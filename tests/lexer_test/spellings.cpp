// The words half of the lexer: which spelling of the language a bare word is,
// the six aliases that are lexical, and DESIGN §2's reserved word.
//
// THIS SECTION EXISTS BECAUSE THE TYPE SYSTEM CANNOT HOLD THE DISTINCTION.
// words_spellings.hpp says `using SpellingId = PathId`, so handing the parser a
// spelling where it expects a path compiles clean and dispatches on a number
// that means something else entirely. An earlier draft of this lexer did
// exactly that. What catches it is a word the language spells TWICE.

#include "lexer_test.hpp"

#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <vector>

namespace lexer_test {

namespace {

using satellite::Token;
using satellite::TokenKind;

satellite::words::SpellingId spelling_of_source(const std::string &source)
{
    const std::vector<Token> tokens = satellite::lex(source);
    return tokens.empty() ? satellite::words::kNoSpelling : tokens[0].spelling;
}

} // namespace

void section_spellings()
{
    namespace words = satellite::words;

    // -- Known words carry identity, user names carry text (DESIGN §5.6) ----

    check(spelling_of_source("satellite") != words::kNoSpelling,
          "§5.6: a word the language owns comes out with a spelling");
    check(spelling_of_source("display") != words::kNoSpelling,
          "§5.6: display is one of the language's spellings");
    check(spelling_of_source("my_variable") == words::kNoSpelling,
          "§5.6: a user's name has no spelling id");
    check(satellite::lex("my_variable")[0].text == "my_variable",
          "§5.6: a user-owned bare word carries its TEXT");

    // -- A SPELLING IS NOT A PATH, and `console` is the proof -------------
    //
    // The language spells `console` twice: satellite.console and
    // satellite.window.console are two different nodes with two different
    // numbers. DESIGN §4.4 is explicit that the interner is "deduplication, not
    // identity", and §4.5 is explicit that a PathId comes from a WALK. So one
    // spelling id must answer for both paths, and the two paths must not be it.
    {
        const words::Walk bare = words::walk("satellite.console");
        const words::Walk nested = words::walk("satellite.window.console");
        check(bare.error == words::WalkError::NONE, "satellite.console walks");
        check(nested.error == words::WalkError::NONE, "satellite.window.console walks");
        check(bare.id != nested.id, "the two consoles are two different nodes");

        const words::SpellingId spelling = spelling_of_source("console");
        check(spelling != words::kNoSpelling, "console is a spelling the language uses");
        check(spelling == words::spelling_id(static_cast<words::NodeId>(bare.id)),
              "the lexer's id is the spelling both nodes share");
        check(spelling == words::spelling_id(static_cast<words::NodeId>(nested.id)),
              "and it is the same id from the other node");
        check(spelling != nested.id,
              "A SPELLING ID IS NOT A PathId -- the lexer must not hand out a path");
    }

    // -- The lexer's half of the spelling table (PLAN M3) ------------------
    //
    // words::intern() knows the nodes and not the aliases. Resolving the six
    // spellings of `arguments` to ONE id is what makes DESIGN §7.7's "one node,
    // six spellings" true of the token stream rather than only of words.def.
    {
        const words::SpellingId canonical = spelling_of_source("arguments");
        check(canonical != words::kNoSpelling, "arguments is a spelling");
        const char *const spellings[] = {"arg", "args", "argz", "argument", "argumentz"};
        for (const char *spelling : spellings)
            check(spelling_of_source(spelling) == canonical,
                  std::string("§7.7: ") + spelling + " is a spelling of arguments");
    }

    // WORD_NUMBERS §2.3's other lexical alias.
    check(spelling_of_source("hexadecimal") == spelling_of_source("hex")
              && spelling_of_source("hex") != words::kNoSpelling,
          "§2.3: hexadecimal is a second spelling of hex");

    // THE THREE DOTTED ALIASES ARE NOT LEXICAL AND MUST NOT LEAK IN HERE.
    // `fast.range(min, max)` rewrites a two-segment PATH, so no amount of
    // looking at one bare word can decide it -- and `range` is not a node
    // spelling either, so if this ever answers, the dot filter has gone.
    check(spelling_of_source("range") == words::kNoSpelling,
          "§2.3: `range` is a path rewrite (M16), not a lexical spelling");

    // intern_word() is the function, and it agrees with what lex() produced.
    check(satellite::intern_word("argz") == spelling_of_source("argz"),
          "intern_word is what the lexer calls");
    check(satellite::intern_word("") == words::kNoSpelling,
          "the empty spelling is nobody's word");
    check(satellite::intern_word("no_such_word_anywhere") == words::kNoSpelling,
          "a name the language does not use has no spelling");

    // -- DESIGN §2's reserved word, as one integer compare ------------------

    check(is_reserved_word(satellite::lex("satellite")[0]),
          "§4.4: `satellite` is answered by one interner comparison");
    check(!is_reserved_word(satellite::lex("satellites")[0]),
          "and a word that merely starts with it is not");
    check(!is_reserved_word(satellite::lex("display")[0]),
          "nor is another word of the language");
    check(satellite::lex("satellite")[0].spelling == words::kSatelliteSpelling,
          "the reserved word's id is the one words.hpp names");

    // §2's THREE TOKENS: `satellite . variable` against `satellite . library`
    // separates a type path from a value path, and the parser makes that test
    // by INDEX rather than by comparing strings on every path in the language.
    {
        const std::vector<Token> type_path = satellite::lex("satellite.variable.string");
        const std::vector<Token> value_path = satellite::lex("satellite.library.main");
        check(is_reserved_word(type_path[0]) && is_reserved_word(value_path[0]),
              "§2: both paths are rooted at the reserved word");
        check(type_path[1].text == "." && value_path[1].text == ".",
              "§2: token 1 is the dot in both");
        check(type_path[2].spelling != value_path[2].spelling,
              "§2: token 2 is what separates a type path from a value path");
        check(type_path[2].spelling == satellite::intern_word("variable")
                  && value_path[2].spelling == satellite::intern_word("library"),
              "§2: and it is decidable without a string compare");
    }

    // A Punct and a Number never carry a spelling, whatever they are spelled.
    check(satellite::lex(".")[0].spelling == words::kNoSpelling, "a Punct has no spelling");
    check(satellite::lex("3")[0].spelling == words::kNoSpelling, "a Number has no spelling");
    check(satellite::lex("\"display\"")[0].spelling == words::kNoSpelling,
          "a String's BODY is not looked up, however it is spelled");
}

} // namespace lexer_test
