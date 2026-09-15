// Numbers, strings, escapes and DESIGN §8.5's Bits literals -- everything the
// lexer decides the VALUE of rather than merely the extent of.

#include "lexer_test.hpp"

#include "lexical_analyzer/lexer.hpp"
#include "satellite_string/satellite_string.hpp"

#include <string>
#include <vector>

namespace lexer_test {

namespace {

using satellite::Token;
using satellite::TokenKind;

std::vector<Token> body(const std::string &source)
{
    std::vector<Token> tokens = satellite::lex(source);
    if (!tokens.empty() && tokens.back().kind == TokenKind::End)
        tokens.pop_back();
    return tokens;
}

Token one(const std::string &source)
{
    const std::vector<Token> tokens = body(source);
    return tokens.size() == 1 ? tokens[0] : Token{};
}

} // namespace

void section_literals()
{
    // -- Numbers, and the two rules §5.6 states about them ------------------

    check(one("3").kind == TokenKind::Number && one("3").text == "3", "3 is a Number");
    check(one("3.14").kind == TokenKind::Number && one("3.14").text == "3.14",
          "§5.6: 3.14 is ONE Number");

    // A '.' joins a Number only when the token started with a digit AND a digit
    // follows, so a method call on a name is three tokens.
    {
        const std::vector<Token> tokens = body("main.x");
        check(tokens.size() == 3 && tokens[0].kind == TokenKind::Word
                  && tokens[1].kind == TokenKind::Punct
                  && tokens[2].kind == TokenKind::Word,
              "§5.6: main.x is Word Punct Word");
    }
    {
        const std::vector<Token> tokens = body("3.x");
        check(tokens.size() == 3 && tokens[0].text == "3" && tokens[1].text == ".",
              "§5.6: a '.' with no digit after it does not join the number");
    }

    // NEVER FOLD A SIGN INTO A NUMBER. Folding would turn a-1 into Word(a)
    // Number(-1) and break subtraction, so unary minus is an expression rule.
    {
        const std::vector<Token> tokens = body("-1");
        check(tokens.size() == 2 && tokens[0].text == "-" && tokens[1].text == "1",
              "§5.6: -1 is Punct(-) Number(1)");
    }
    {
        const std::vector<Token> tokens = body("a-1");
        check(tokens.size() == 3 && tokens[1].text == "-"
                  && tokens[2].kind == TokenKind::Number,
              "§5.6: a-1 keeps the subtraction");
    }

    // -- Bits literals, DESIGN §8.5 ----------------------------------------

    check(one("x00FF").kind == TokenKind::Bits && one("x00FF").radix == 16,
          "§8.5: x00FF is a Bits literal, radix 16");
    check(one("b1010").kind == TokenKind::Bits && one("b1010").radix == 2,
          "§8.5: b1010 is a Bits literal, radix 2");
    check(one("x00ff").kind == TokenKind::Bits, "§8.5: hex accepts lower case");

    // THE WIDTH IS PART OF THE VALUE, which is the whole reason these are not
    // number literals in another base. x0009 is not x9, and the lexer is what
    // has to keep them apart -- it does it by keeping the literal's text
    // verbatim, so nothing downstream can normalise the width away.
    check(one("x0009").text == "x0009" && one("x9").text == "x9",
          "§8.5: a Bits literal keeps its text verbatim");
    check(one("x0009").text != one("x9").text, "§8.5: x0009 is NOT x9");

    // What must NOT become a literal, or the shape would eat identifiers.
    check(one("x2_y").kind == TokenKind::Word, "§8.5: x2_y is a Word -- '_' is no hex digit");
    check(one("bad").kind == TokenKind::Word, "§8.5: bad is a Word -- 'a' is no binary digit");
    check(one("b").kind == TokenKind::Word, "§8.5: a bare b is a Word");
    check(one("x").kind == TokenKind::Word, "§8.5: a bare x is a Word");
    check(one("xyz").kind == TokenKind::Word, "§8.5: xyz is a Word");
    check(one("b1010").radix == 2 && one("hello").radix == 0,
          "radix is 0 on everything that is not a Bits literal");

    // `#` IS `x` -- the author's decision of 2026-09-13. The token is the same
    // Bits literal, its text spelled with `x` so a `.satc` never holds a `#`
    // colour for unnumber.cpp's `#1.14` pass to misread.
    check(one("#D4AF37").kind == TokenKind::Bits && one("#D4AF37").radix == 16 &&
              one("#D4AF37").text == "xD4AF37",
          "#D4AF37 is the Bits literal xD4AF37");
    check(one("#fff").text == "xfff", "#fff keeps its width, as xfff does");
    {
        const std::vector<Token> tokens = body("#12G456");
        check(tokens.size() >= 2 && tokens[0].kind == TokenKind::Punct,
              "#12G456 is not all hex digits, so # stays punctuation");
    }
    {
        const std::vector<Token> tokens = body("#1.14");
        check(tokens.size() >= 2 && tokens[0].kind == TokenKind::Punct,
              "#1.14 -- a .satc path -- is never claimed as a colour");
    }
    {
        const std::vector<Token> tokens = body("#000000 // black");
        check(tokens.size() == 1 && tokens[0].text == "x000000",
              "a // comment after a # colour is still a comment");
    }

    // -- Strings, §5.3 and §5.4 --------------------------------------------

    check(one("\"hello\"").kind == TokenKind::String, "a quoted body is a String");
    check(satellite::decode(one("\"hello\"").str) == "hello", "the body decodes back");
    check(one("\"\"").kind == TokenKind::String, "an empty string is still a String");

    // ALL FIVE ESCAPES ROUND-TRIP, by two different routes: \n \t \r have no
    // code-table entry and land in the raw area, while \\ and \" are real
    // punctuation codes. Checked through decode, which is the round trip.
    check(satellite::decode(one("\"a\\nb\"").str) == "a\nb", "§5.4: \\n round-trips");
    check(satellite::decode(one("\"a\\tb\"").str) == "a\tb", "§5.4: \\t round-trips");
    check(satellite::decode(one("\"a\\rb\"").str) == "a\rb", "§5.4: \\r round-trips");
    check(satellite::decode(one("\"a\\\\b\"").str) == "a\\b", "§5.4: \\\\ round-trips");
    check(satellite::decode(one("\"a\\\"b\"").str) == "a\"b", "§5.4: \\\" round-trips");

    // THE TEXT KEEPS THE ESCAPE AND THE VALUE DOES NOT, which is why a Token
    // carries both. An unparser with only `str` would print a real newline back
    // into the source and change the program.
    check(one("\"a\\nb\"").text == "a\\nb", "the as-written text keeps the backslash");
    check(one("\"a\\nb\"").text != satellite::decode(one("\"a\\nb\"").str),
          "text and str are different facts and neither is derivable");

    // §5.4 -- MATCH ESCAPE NAMES LONGEST-FIRST, so a short name cannot shadow a
    // longer one that starts with it. \threads is ONE character (a live value),
    // not a tab followed by "hreads". This is the check that fails if anybody
    // alphabetises the escape table.
    check(one("\"\\threads\"").str.size() == 1, "§5.4: \\threads is one code, not \\t + hreads");
    check(satellite::decode(one("\"\\threads\"").str) == "<threads>",
          "the live codes are stubbed until M9/M6 and say so");

    // §5.4's other half: an escape the table does not know passes through with
    // the backslash still attached. That is a decision, recorded in DESIGN §13,
    // and \' is the one spelling that was made redundant rather than left to
    // print a stray backslash 501 times.
    check(satellite::decode(one("\"DOESN\\'T\"").str) == "DOESN'T",
          "§5.4: \\' is a redundant spelling of ' and loses the backslash");
    check(satellite::decode(one("\"a\\qb\"").str) == "a\\qb",
          "§5.4: an unknown escape keeps its backslash");

    // §5.3 -- LEX RAW. encode() everywhere would rewrite the program before the
    // lexer saw it: a backslash in ordinary code is a Punct, and \home in a
    // comment never becomes anybody's home directory.
    {
        // `home` IS a word of the language, so what this checks is not that the
        // three tokens are unremarkable -- it is that \home stayed three tokens
        // instead of becoming one expanded path. Under encode() it would have
        // been the user's home directory before the lexer ever ran.
        const std::vector<Token> tokens = body("a\\home");
        check(tokens.size() == 3 && tokens[0].text == "a" && tokens[1].text == "\\"
                  && tokens[2].text == "home",
              "§5.3: a backslash outside a string is punctuation, not an expansion");
    }
    check(body("// C:\\home\n").size() == 1, "§5.3: \\home in a comment is discarded whole");

    // -- The lexer never throws (§5.6) -------------------------------------

    {
        const std::vector<Token> tokens = satellite::lex("\"unterminated");
        check(tokens.size() >= 2 && tokens[0].kind == TokenKind::Error,
              "§5.6: an unterminated string is an Error token, not an exception");
        check(tokens.back().kind == TokenKind::End,
              "§5.6: the stream still ends with End after an Error");
    }
    {
        // A string may not span lines, and a trailing backslash must not carry
        // it onto the next one -- see the span section for why that matters
        // beyond this token.
        const std::vector<Token> tokens = satellite::lex("\"open\\\nstill open");
        check(tokens[0].kind == TokenKind::Error,
              "§5.4: a backslash does not escape the newline out of a string");
    }
}

} // namespace lexer_test
