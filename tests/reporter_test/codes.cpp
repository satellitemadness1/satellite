// The registry: the numbers, the blocks they sit in, and the holes in every
// sentence. See tests/reporter_test/reporter_test.hpp.
//
// MOST OF THE REGISTRY IS CHECKED AT COMPILE TIME AND THAT IS THE POINT.
// codes.hpp holds five static_asserts and every consumer inherits them, which
// is the arrangement FORMAT/CXX.md §8 calls the single most important decision
// in the first satellite's registry. What is left for run time is what a
// `constexpr` cannot see: that the blocks errors.def declares in a comment are
// the blocks the rows are actually in, that a code survives being written out
// and read back, and that `satl --errors` can reach every row.

#include "reporter_test.hpp"

#include "error_reporter/codes.hpp"
#include "error_reporter/dump.hpp"

#include <string>

namespace reporter_test {

namespace {

using satellite::errors::Code;
using satellite::errors::Severity;

// Which block a code is in, as errors.def's comment writes them.
unsigned block_of(Code code)
{
    return static_cast<unsigned>(code) / 100;
}

} // namespace

void section_codes()
{
    using namespace satellite::errors;

    // THE COUNT FIRST, FOR THE REASON words_test GIVES ABOUT ITS OWN: a parser
    // bug that silently read half a table reports PASS over the half it read.
    // This one is a compile-time array so it cannot be half-read, but the
    // number is what says a row was DELETED -- which the ascending assert
    // cannot see, because a table with a row removed still ascends.
    check(kCodeCount == 47,
          "errors.def has 47 rows -- if that changed on purpose, change it here "
          "and say so in MILESTONES; a row DELETED is invisible to every "
          "static_assert in codes.hpp");

    // The blocks errors.def's header declares, checked against where the rows
    // actually are. A code in the wrong block is a code whose first two digits
    // lie about who raised it, which is the whole reason it has four.
    check(block_of(Code::LEX_UNTERMINATED_STRING) == 1, "the lexer is S01xx");
    check(block_of(Code::PARSE_EXPECTED_PUNCT) == 2, "the parser is S02xx");
    check(block_of(Code::SATC_NOT_A_SATC) == 3, "the cache is S03xx");
    check(block_of(Code::FILE_UNREADABLE) == 4, "the file satl was given is S04xx");
    check(block_of(Code::CONFIG_NOT_A_SETTING) == 8, "the machine limits are S08xx");

    // THE RESERVED BLOCKS ARE EMPTY, and this is the check that makes reserving
    // them worth anything. errors.def keeps S05xx for M7's resolve, S06xx for
    // M8's numbers and S07xx for the evaluator; a milestone that takes the
    // next free number instead of its own block would put a resolve error in
    // the parser's range and nothing else would notice.
    //
    // THE TEST THAT PROVED THIS WORKS IS M6 ARRIVING. It landed BEFORE all
    // three of those milestones and took S08xx rather than S05xx, so the block
    // numbers no longer run in build order -- which is a reservation being
    // honoured and is exactly what this loop asks for. The check therefore
    // names the three empty blocks instead of counting up to the highest one
    // taken: `<= 4` was the same claim only while nothing had jumped the gap,
    // and it would have failed on a correct edit.
    for (const Code code : kCodes)
        check(block_of(code) < 5 || block_of(code) > 7,
              "no code is in a block reserved for a later milestone -- S05xx is "
              "M7's, S06xx is M8's, S07xx is the evaluator's; take your own "
              "block, do not append");

    // A code out and back again. `satl --errors S0231` is the only reason
    // code_text and code_of both exist, and a round trip is the whole contract
    // between them.
    for (const Code code : kCodes)
        check(code_of(code_text(code).view()) == code,
              "S" + std::string(code_text(code).view()).substr(1) +
                  " reads back as the code it was written from");

    check(std::string(code_text(Code::PARSE_NO_SUCH_STATEMENT).view()) == "S0212",
          "a code is `S` and four digits, zero-padded, so a column of them "
          "lines up");
    check(code_of("S9999") == Code::NONE, "a code satl does not have is NONE");
    check(code_of("S0212x") == Code::NONE, "a code is exactly five characters");
    check(code_of("0212") == Code::NONE, "a code without its S is not a code");

    // THE ARITY IS COUNTED FROM THE SENTENCE and the call sites are checked
    // against it by static_assert; what is checked here is the counter itself,
    // because everything else trusts it.
    check(arity_of(Code::PARSE_EXPECTED_PUNCT) == 3,
          "`expected {1} {2}, and found {3}` has three holes");
    check(arity_of(Code::PARSE_GENERIC_CLOSE_GE) == 0,
          "a sentence with no holes has arity 0");
    check(arity_of(Code::NONE) == 0, "NONE is not a message and has no holes");

    check(severity_of(Code::NOTE_OPENED_HERE) == Severity::NOTE,
          "a note is declared SAT_NOTE");
    check(severity_of(Code::PARSE_EXPECTED_PUNCT) == Severity::ERROR,
          "an error is declared SAT_ERROR");
    check(severity_of(static_cast<Code>(9999)) == Severity::ERROR,
          "a code the table does not have is not quietly a note");

    // `satl --errors` REACHES EVERY ROW, which is the rule PLAN M2 made for the
    // word registry -- a registry with no reader is a promise nothing checks.
    // A code that the dump cannot print is a code nobody can look up, which is
    // the same as not having given it a number.
    const std::string dump = dump_text();
    for (const Code code : kCodes)
        check(holds(dump, std::string(code_text(code).view())),
              "`satl --errors` lists " + std::string(ident_of(code)));

    bool known = false;
    const std::string one = explain_text("S0212", known);
    check(known, "`satl --errors S0212` finds it");
    check(holds(one, "PARSE_NO_SUCH_STATEMENT"),
          "explaining a code names the row in errors.def, so the sentence can "
          "be found in the file it lives in");
    check(holds(one, "no statement is spelled"), "and prints its sentence");

    explain_text("S0999", known);
    check(!known, "a code satl does not have is a usage failure, not an answer");
}

} // namespace reporter_test
