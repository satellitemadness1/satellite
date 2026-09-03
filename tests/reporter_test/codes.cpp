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

#include <array>
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

// The blocks errors.def's header has written down and no row has taken yet.
//
// EMPTY AS OF M9, and the loop below says why it is kept that way rather than
// deleted. S05xx, S06xx and S07xx were each in here until the milestone they
// were held for arrived.
// A std::array AND NOT A C ARRAY, because `unsigned kReservedBlocks[0]` is
// ill-formed -- there is no zero-length array in C++ and the empty case is
// exactly the one this list is in.
constexpr std::array<unsigned, 0> kReservedBlocks = {};

} // namespace

void section_codes()
{
    using namespace satellite::errors;

    // THE COUNT FIRST, FOR THE REASON words_test GIVES ABOUT ITS OWN: a parser
    // bug that silently read half a table reports PASS over the half it read.
    // This one is a compile-time array so it cannot be half-read, but the
    // number is what says a row was DELETED -- which the ascending assert
    // cannot see, because a table with a row removed still ascends.
    // THIS NUMBER CAME DOWN TWICE ON 2026-08-31 AND THAT IS WHY THE ASSERTION
    // EXISTS. It was 63; S0501 RESOLVE_TOO_DEEP went when DESIGN §7.5 said the
    // language has no depth limit, and S0612 NUMBER_OPERATOR_NOT_YET went when
    // DESIGN §8.6 turned out to specify modulus in full -- both rows deleted for
    // disagreeing with a document written before them.
    //
    // NOTHING ELSE IN THE BUILD COULD HAVE NOTICED EITHER OF THEM. The table
    // still ascends, the switch still compiles, and every static_assert in
    // codes.hpp passes over a shorter list. MILESTONES/M8.md §3.8 and §6 are
    // where the second one is said out loud, which is what this message asks
    // for.
    // AND IT WENT UP BY EIGHT AT M9, which is the other direction and is the
    // easy one: an added row cannot hide, because the site that raises it has
    // to name the code. The number is here so that a row REMOVED still cannot.
    // AND BY ONE AT M10: S0402 FILE_NO_MAIN, for a file handed to `satl` to run
    // that declares no `satellite.main`. MILESTONES/M10.md §3 says why it is in
    // S04xx -- the file satl was given -- rather than in the evaluator's block,
    // and why it carries no span.
    // AND BY EIGHT AT M11, all in the evaluator's block: S0713-S0719, the
    // methods' rows -- wrong type, holding nothing, past the end, not found,
    // nowhere to write back, needs a receiver, backwards substring -- and
    // S0730 EVAL_INTERRUPTED, which opens the block's fourth decade because a
    // Ctrl-C is raised at the run from OUTSIDE it. MILESTONES/M11.md §3 is
    // the record.
    check(kCodeCount == 78,
          "errors.def has 78 rows -- if that changed on purpose, change it here "
          "and say so in MILESTONES; a row DELETED is invisible to every "
          "static_assert in codes.hpp");

    // The blocks errors.def's header declares, checked against where the rows
    // actually are. A code in the wrong block is a code whose first two digits
    // lie about who raised it, which is the whole reason it has four.
    check(block_of(Code::LEX_UNTERMINATED_STRING) == 1, "the lexer is S01xx");
    check(block_of(Code::PARSE_EXPECTED_PUNCT) == 2, "the parser is S02xx");
    check(block_of(Code::SATC_NOT_A_SATC) == 3, "the cache is S03xx");
    check(block_of(Code::FILE_UNREADABLE) == 4, "the file satl was given is S04xx");
    check(block_of(Code::RESOLVE_NO_SUCH_NAME) == 5, "resolve is S05xx");
    check(block_of(Code::NUMBER_DIVIDE_BY_ZERO) == 6, "numbers are S06xx");
    check(block_of(Code::EVAL_TOO_DEEP) == 7, "the evaluator is S07xx");
    check(block_of(Code::CONFIG_NOT_A_SETTING) == 8, "the machine limits are S08xx");

    // THE RESERVED BLOCK IS EMPTY, and this is the check that makes reserving
    // it worth anything. errors.def keeps S07xx for the evaluator; a milestone
    // that took the next free number instead of its own block would put an
    // evaluator error in the parser's range and nothing else would notice.
    //
    // THE TEST THAT PROVED THIS WORKS IS M6 ARRIVING. It landed BEFORE all
    // three of the reserved blocks and took S08xx rather than S05xx, so the
    // block numbers no longer run in build order -- which is a reservation
    // being honoured and is exactly what this loop asks for. The check
    // therefore names the EMPTY blocks instead of counting up to the highest
    // one taken: `<= 4` was the same claim only while nothing had jumped the
    // gap, and it would have failed on a correct edit.
    //
    // S05xx CAME OUT OF THIS LIST ON 2026-08-31, WHICH IS THE MECHANISM
    // WORKING END TO END. M7 took the block that had been held for it since M5
    // reserved it, and nothing moved to make room: no row was renumbered, no
    // block was split, and the edit to this loop is the deletion of one number
    // from it. That is the whole of what a reservation buys, and it is the
    // first time this suite has been able to record it happening rather than
    // being promised.
    //
    // S06xx CAME OUT THE SAME WAY AT M8, WHICH MADE IT TWICE, AND S07xx CAME
    // OUT AT M9, WHICH EMPTIES THIS LOOP. Three blocks were claimed by the
    // milestone each was reserved for, out of build order every time, and this
    // check shrank by one number on each -- so the mechanism is recorded three
    // times over and there is nothing left for it to hold.
    //
    // THE LOOP IS KEPT WITH AN EMPTY LIST RATHER THAN DELETED, and that is not
    // sentiment. errors.def's header says a milestone "takes a block instead of
    // taking the next free number and interleaving itself with everybody else",
    // and the next milestone to add codes -- M10's console, M11's scalars --
    // reserves S09xx or S10xx in that header and adds it here in one edit. A
    // deleted check is one somebody has to think of writing again; an empty one
    // is a line with a hole in it.
    for (const Code code : kCodes)
        for (const unsigned reserved : kReservedBlocks)
            check(block_of(code) != reserved,
                  "no code is in a block reserved for a later milestone -- "
                  "take your own block, do not append");

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
