// §2's three lines. See tests/satc_test/satc_test.hpp.
//
// EACH ANSWERS A QUESTION THAT HAS EXACTLY ONE WRONG ANSWER, which is how §2
// introduces them, and the `words` line is the load-bearing one: "a `.satc` is
// meaningless except against the numbering that produced it, and DESIGN §4.3's
// whole warning is that a changed numbering SILENTLY CHANGES WHAT A PROGRAM
// MEANS."
//
// THE DIGEST IS CHECKED AGAINST words::digest_text() AND NOT AGAINST A LITERAL.
// A literal would be a second place the numbering's identity lives, and it
// would have to be edited every time a word was appended -- which is to say it
// would be edited to keep the build quiet, which FORMAT/CXX.md §1 names as
// worse than no assert at all. What is worth pinning is the SHAPE: sixteen
// lower-case hex digits, fixed width, because words_digest.hpp says the header
// is compared as text by a reader that has not parsed it yet and "a digest that
// is sometimes fifteen characters is a header line whose fields move".

#include "satc_test.hpp"

#include "parser/parser.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satc_test {

namespace {

std::string field(const std::string &line, size_t index)
{
    size_t at = 0;
    for (size_t i = 0; i < index; i++) {
        at = line.find(' ', at);
        if (at == std::string::npos)
            return std::string();
        at++;
    }
    const size_t end = line.find(' ', at);
    return line.substr(at, end == std::string::npos ? end : end - at);
}

} // namespace

void section_header()
{
    satellite::cache::Source source;
    source.name = "hello_world.satl";
    source.mtime = 1756304412;
    source.size = 142;

    const std::string header = satellite::cache::header_text(source);
    const std::string satc = line_with(header, "satc ");
    const std::string words = line_with(header, "words ");
    const std::string line = line_with(header, "source ");

    check(satc == "satc 1", "the format line: " + satc);

    check(field(words, 0) == "words", "the numbering line is named `words`");
    check(field(words, 2) == satellite::words::digest_text(),
          "the digest is the numbering's own: " + field(words, 2));
    check(field(words, 2).size() == 16,
          "the digest is sixteen digits wide: " + field(words, 2));
    for (const char c : field(words, 2))
        check((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'),
              "the digest is lower-case hex: " + field(words, 2));

    // THE SOURCE LINE IS THREE FIELDS AND THE NAME IS ONE OF THEM. It is what
    // makes a shared cache directory safe: two programs called
    // hello_world.satl in two directories hash to two files, and a reader that
    // opened the wrong one is told so by this line rather than by running the
    // wrong program. satellite_cache/file.cpp is where that argument lives.
    check(field(line, 1) == "hello_world.satl", "the source's name: " + line);
    check(field(line, 2) == "1756304412", "the source's mtime: " + line);
    check(field(line, 3) == "142", "the source's size: " + line);

    // A HEADER AND A BODY ARE SEPARATED BY A BLANK LINE, which is not
    // decoration: §2 is three lines and a reader stops there, so the blank is
    // what says the header ended rather than a fourth line being lost.
    satellite::words::Words numbering;
    const satellite::Parse parsed =
        satellite::parse("satellite.include(satellite)\n", numbering);
    const std::string whole =
        satellite::cache::satc_text(parsed.ast, numbering, source);
    check(whole.find(header + "\n") == 0,
          "the header comes first and a blank line follows it");
}

} // namespace satc_test
