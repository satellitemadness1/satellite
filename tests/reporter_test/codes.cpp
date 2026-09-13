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
    // AND BY ONE AT M12: S0723 EVAL_NO_SUCH_QUESTION, for a method a declared
    // type does not have -- a case that used to fall into S0720's "name the
    // receiver first", advice that cannot help a receiver that IS a name.
    // MILESTONES/M12.md §3 is the record.
    // AND BY ELEVEN AT M13, all in a NEW block: S09xx, the clock and the
    // dice. S0901-S0910 are `satellite.random`'s -- seven of them v1's
    // failure texts become rows, S0901/S0909/S0910 the refusals whose shapes
    // were specified 2026-09-04 -- and S0920 is `satellite.time.sleep`'s "not
    // a length of time". MILESTONES/M13.md §3 is the record.
    // AND BY THREE AT M14, in S10xx, the console's input: S1001 is v1's "end
    // of input" sentence become a row, and S1002/S1003 are the place
    // parameter's two misuses, raised at compile before any prompt could
    // print. The interrupted case is deliberately NOT a fourth -- it answers
    // nothing and the boundary's S0730 is the report. MILESTONES/M14.md §3.
    // AND BY THREE AT M15: S0602/S0603 in the numbers' block -- sqrt of a
    // negative, and a negative base under a fractional exponent, the two
    // facts the class-3 operations can hit -- and S0724, the retune's
    // write-side twin of S0721, for an assignment to a language path with no
    // write row. MILESTONES/M15.md §3 is the record.
    // AND BY FIVE AT M16: S0725 through S0729, the containers' block -- a
    // position outside a list, a key a map does not hold, a value that
    // cannot be a key at all, a value not in a list, and a question asked of
    // an empty one. Each names the question that answers instead of
    // stopping, which is S0715 and S0716's shape retold for the two
    // containers. MILESTONES/M16.md is the record.
    // AND BY FOUR AT M18: S1101 through S1104, `satellite.help`'s block --
    // a topic that is numbered and not built, a bare word that is neither a
    // variable nor a path, and everything else somebody can write between the
    // parentheses. S1101 is the milestone's own done-when: before M18,
    // `satellite.help(satellite.network)` refused with S0721 because the
    // ARGUMENT dispatched and died, which gave the same answer for a module
    // that IS built -- so a check that passed through the wrong mechanism now
    // has a code that is about the right thing. S1104 is the fourth and arrived
    // last: a global is a variable with no declared type, and S1103 was telling
    // somebody `n` was not the name of a variable while they looked at the line
    // declaring it. MILESTONES/M18.md §3.
    // AND BY EIGHT AT M19: S1201 through S1208, persistence's block -- and the
    // block is SMALL BECAUSE OF WHAT DESIGN §9 DOES WITH A FILE. A failed open
    // is a VALUE, so the commonest thing that goes wrong with a file raises
    // nothing at all; what is left is the split v1 drew and this block keeps,
    // that a PROGRAM mistake is an error and a RUNTIME failure is a value. Not
    // one of the eight is about a syscall failing.
    //
    // S1208 ARRIVED LAST AND FOR S1104's REASON ONE MILESTONE ON: a handle
    // whose open FAILED was being told it "is closed", which is a sentence
    // about something that never happened, while the errno explaining it sat
    // unread in `error`. Found by running a help example, not by review.
    // MILESTONES/M19.md §2.6.
    // AND BY ONE AT M19.6: S0307, the option token's refusal. `0#down` is only
    // meaningful directly after the `(` of the call it is an option to, because
    // unnumber() walks BACK over the selector to find where the Mark belongs --
    // so an option token anywhere else names no call and is a `.satc` this
    // writer did not produce. Refused rather than guessed at, which is the same
    // call S0305 and S0306 make about the two ways a `#` can be wrong.
    // MILESTONES/M19.6.md §1.
    // AND BY TWO AT M20, which is the largest single move this count has had.
    // S0731 is the arguments object's missing name -- S0726 for the object
    // rather than S0726 itself, because that row's sentence says "this map"
    // and sending somebody who asked `arguments.get("machine.thread")` to a
    // map's method table is what `satellite.container.arguments` `1 4 3`
    // exists to prevent. S1302 is the one word `min_free_mb` accepts beside a
    // number: an enumeration over a list of ONE, which is S1201's decision
    // about the four file modes reached again, and it names `0` in its own
    // sentence because the mistake it catches is somebody reaching for the off
    // switch. MILESTONES/M20.md §2.5, §3.1.
    // AND BY THREE AT M21, which takes the record M20 held for one day. The
    // seeded tier's three: S0911, a draw from a stream nothing has seeded --
    // refused rather than seeded silently, because a stream seeded by
    // accident is the determinism bug that tier exists to remove; S0912, a
    // seed that is not a whole number of zero or more, checked apart from the
    // bounds because it names a POSITION rather than a quantity; and S0913,
    // the bare `seeded()`. **S0913 is the one that could not be shared** --
    // S0901 offers "a digit_count, min and max, or min max and step" and the
    // seeded tier has no digit_count, so reusing it would have told a program
    // to write the one call this tier does not have. MILESTONES/M21.md §2.3.
    //
    // AND THE SENTENCE BELOW SAID 116 WHILE THE CHECK SAID 117, from M20
    // until M21 found it here. **A count in a message has nothing checking
    // it, INSIDE the check written to catch exactly that** -- M20 moved the
    // number by two and updated the comparison and not its own text, which is
    // the same shape as `words.def`'s stale header tallies one directory over
    // (M21.md §3.8) and is why both numbers are now written once each and
    // read together.
    // AND 133 SINCE M26.5, which added exactly one row: S1501, the static
    // analyser's reference-cycle warning and the first SAT_WARNING in the file.
    // BOTH NUMBERS BELOW MOVED TOGETHER, which is what the paragraph above is
    // about -- the comparison and the sentence it prints are two copies of one
    // fact, and M20 moved one of them and not the other.
    // AND 134 SINCE M26 CLOSED, which added one more: S0525, the constructor
    // that declares `satellite.returns` DESIGN §13 forbids.
    // AND 137 SINCE 2026-09-12, which added three: S0244 and S0245, where the
    // author's `satellite.constructor(args) { }` section may and may not go,
    // and S0526, arguments on a declaration that is not a spacesuit's.
    // AND 141 SINCE M30's NAMED ARGUMENTS, which added four: S0232 and S0233
    // in the parser, S0527 in resolve and S0732 at the handler.
    // AND 142 WITH S0921, sleep's unit word, AND 143 WITH S1004, display's
    // colour.
    check(kCodeCount == 143,
          "errors.def has 143 rows -- if that changed on purpose, change it here "
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
    check(block_of(Code::RANDOM_NEEDS_A_SHAPE) == 9, "the clock and the dice are S09xx");
    check(block_of(Code::CONSOLE_END_OF_INPUT) == 10, "the console's input is S10xx");
    check(block_of(Code::HELP_NOT_BUILT) == 11, "satellite.help is S11xx");
    check(block_of(Code::FILE_BAD_MODE) == 12, "persistence is S12xx");
    check(block_of(Code::SYSTEM_BAD_UNIT) == 13, "the machine's facts are S13xx");
    check(block_of(Code::THREAD_NOT_A_CAPSULE_CALL) == 14, "threads are S14xx");

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
