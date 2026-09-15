// DESIGN §5.1, §5.2, §5.5 and the comment rule -- the character-level half of
// the lexer, and four of the five rules §5 says were inherited rather than
// rediscovered.

#include "lexer_test.hpp"

#include "lexical_analyzer/lexer.hpp"

#include <string>
#include <vector>

namespace lexer_test {

namespace {

using satellite::Token;
using satellite::TokenKind;

// The tokens of `source` with the trailing End dropped, which is what almost
// every check below wants to count.
std::vector<Token> body(const std::string &source)
{
    std::vector<Token> tokens = satellite::lex(source);
    if (!tokens.empty() && tokens.back().kind == TokenKind::End)
        tokens.pop_back();
    return tokens;
}

// KIND AND TEXT ONLY, DELIBERATELY NOT describe(). describe() prints a known
// word's spelling id, which would make every check in this section depend on
// the numbering -- so a word gaining a sibling in words.def would fail a test
// about underscores. The ids are section_spellings()' subject; this section is
// about where one token ends and the next begins.
std::string shape(const std::string &source)
{
    std::string out;
    for (const Token &token : body(source)) {
        if (!out.empty())
            out += " ";
        out += satellite::kind_name(token.kind);
        // A Newline's text is a real newline, which would put a line break in
        // the middle of every expected string below.
        if (token.kind != TokenKind::Newline)
            out += "(" + token.text + ")";
    }
    return out;
}

} // namespace

void section_characters()
{
    // §5.1 -- THE HEADLINE. Underscore is punctuation in the code table and an
    // identifier character to the lexer. Under the naive rule this is three
    // tokens and every example in DESIGN breaks.
    check(shape("my_time") == "Word(my_time)", "§5.1: my_time is ONE Word");
    check(shape("_leading") == "Word(_leading)", "§5.1: a word may start with _");
    check(shape("x2_y") == "Word(x2_y)", "§5.1: digits and _ continue a word");

    // §5.1's own worked example, which DESIGN gives with both numbers: 6 tokens
    // with the rule and 16 without. Checked as a COUNT because that is the form
    // the design states it in.
    check(body("list_name[some_number_start:some_number_end]").size() == 6,
          "§5.1: the slicing example is 6 tokens, not 16");

    // §5.2 -- whitespace lives in the raw area, so a lexer that reached for
    // isspace() would emit one Error per space rather than skipping any.
    for (const Token &token : body("a b\tc\r\nd"))
        check(token.kind != TokenKind::Error, "§5.2: whitespace is skipped, not an Error");
    check(shape("  a  ") == "Word(a)", "§5.2: leading and trailing space vanish");

    // §5.5 -- THERE IS NO `<<` AND NO `>>`, EVER, which is what makes nested
    // generics need no special handling. Two independent '>' tokens is the
    // whole of the C++98 maximal-munch fix.
    check(shape("list<list<string>>")
              == "Word(list) Punct(<) Word(list) Punct(<) Word(string)"
                 " Punct(>) Punct(>)",
          "§5.5: list<list<string>> closes as two independent '>'");
    check(shape("a << b") == "Word(a) Punct(<) Punct(<) Word(b)",
          "§5.5: `<<` is two tokens and never one");

    // The four greedy operators, which are the ONLY places < or > is not a
    // single-character token.
    check(shape("a == b") == "Word(a) Punct(==) Word(b)", "greedy: ==");
    check(shape("a <= b") == "Word(a) Punct(<=) Word(b)", "greedy: <=");
    check(shape("a >= b") == "Word(a) Punct(>=) Word(b)", "greedy: >=");
    check(shape("a != b") == "Word(a) Punct(!=) Word(b)", "greedy: !=");

    // §5.5's guard: `>=` is the one two-character operator that could collide
    // with a generic close, so a parser that wants '>' must be able to take it.
    {
        std::vector<Token> tokens = satellite::lex("a>=b");
        check(satellite::split_punct(tokens, 1), "split_punct splits >=");
        check(tokens[1].kind == TokenKind::Punct && tokens[1].text == ">",
              "split_punct: the first half is '>'");
        check(tokens[2].kind == TokenKind::Punct && tokens[2].text == "=",
              "split_punct: the second half is '='");
        check(tokens[1].end == tokens[1].start + 1 && tokens[2].start == tokens[1].end,
              "split_punct: the two spans meet and do not overlap");
        check(!satellite::split_punct(tokens, 1), "split_punct refuses a one-char Punct");
        check(!satellite::split_punct(tokens, 99), "split_punct refuses an index past the end");
    }

    // §5.6 -- `//` runs to the end of the line and NEVER REACHES THE PARSER, so
    // it costs §6's grammar nothing. The newline it stops at is kept, because
    // it is a statement terminator.
    check(shape("a // comment\nb") == "Word(a) Newline Word(b)",
          "§5.6: a line comment is discarded and its newline is not");
    check(shape("// whole line").empty(), "§5.6: a file that is only a comment is only End");

    // THERE IS NO BLOCK COMMENT, on purpose -- a form that can be left unclosed
    // is a form that can swallow a file -- so `/*` is two ordinary Puncts.
    check(shape("/*") == "Punct(/) Punct(*)", "§5.6: `/*` is two Punct tokens");

    // A single '/' is division and must not be mistaken for the start of one.
    check(shape("a / b") == "Word(a) Punct(/) Word(b)", "a lone / is division");
}

} // namespace lexer_test
