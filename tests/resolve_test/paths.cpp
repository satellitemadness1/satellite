// DESIGN §6.3's walk, and the numbers it arrives at -- WORD_NUMBERS §2.2 is the
// authority and every number below is quoted from it.
//
// THE NUMBERS ARE WRITTEN OUT RATHER THAN DERIVED, which is the same choice
// tests/words_test makes and for the same reason: a test that computed the
// number it expected would compute it the way the code does and agree with a
// wrong answer. "1 5 1" is in WORD_NUMBERS §2.2 and in DESIGN §4, and it is
// here as a string.

#include "resolve_test.hpp"

#include "name_resolver/resolve.hpp"

#include <string>

namespace resolve_test {

void section_paths()
{
    Run run;
    resolve_source(R"(
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("hello")
    satellite.console.input(">>>", arguments)
    satellite.return(satellite)
}
)",
                   run);
    check(run.parsed_clean() && run.resolved.ok(), "the path fixture resolves");

    check(resolved_to(run, "1 5 1"),
          "satellite.console.display is 1 5 1 -- DESIGN §4's own example");
    check(resolved_to(run, "1 5 4"),
          "and input(prompt, target) is 1 5 4, slotted by ARITY: the same word "
          "at two arguments is a different row from input(prompt) at 1 5 3");
    check(resolved_to(run, "1 4 2"), "satellite.container.list is 1 4 2");
    check(resolved_to(run, "1 6 1"), "satellite.variable.string is 1 6 1");
    check(resolved_to(run, "1 3"), "satellite.main is 1 3");
    check(resolved_to(run, "1 1 1"),
          "satellite.include(satellite) is 1 1 1 -- the argument is absorbed, "
          "because the NUMBER is what names it (SATC §5.1 step 3)");
    check(resolved_to(run, "1 15 1"), "satellite.return(satellite) is 1 15 1");

    // THE STATEMENT FORMS THE PARSER DOES NOT BUILD AS A CHAIN, and there are
    // exactly two. `satellite.return(x)` is a Return node and
    // `satellite.include(satellite)` is an Include node, so neither reaches the
    // chain matcher -- satellite_cache/paths.hpp says the same about the same
    // pair one milestone earlier. Their SHAPES still have to be found by arity.
    Run returns;
    resolve_source(R"(
satellite.capsule three()
{
    satellite.return(1)
}
)",
                   returns);
    check(resolved_to(returns, "1 15 2"),
          "satellite.return(value) is 1 15 2 and not 1 15 1 -- the reserved "
          "word decides between two rows of equal arity, and getting it "
          "backwards would DROP the value");

    Run bare;
    resolve_source(R"(
satellite.capsule none()
{
    satellite.return()
}
)",
                   bare);
    check(resolved_to(bare, "1 15 0"),
          "and satellite.return() is 1 15 0 -- \"()\" and \"\" are different "
          "answers, which is the defect satellite_cache/paths.cpp records");

    // S0521 -- THE REFUSAL satellite_cache/paths.cpp NAMES AND DECLINES. Its
    // own comment: "a misspelled word under `satellite` is a program M7 refuses
    // with M5's did-you-mean over the node the segment failed under".
    Run misspelled;
    resolve_source(R"(
satellite.capsule reads()
{
    satellite.consle.display("x")
    satellite.return()
}
)",
                   misspelled);
    check(only_problem(misspelled, satellite::errors::Code::RESOLVE_NO_SUCH_WORD),
          "a misspelled segment is S0521, and it is ONE error -- a path that "
          "failed does not go on to report every segment inside it");
    check(!misspelled.resolved.problems.empty() &&
              misspelled.resolved.problems.front().suggestion == "console",
          "and the suggestion is over the node the segment failed UNDER, which "
          "is DESIGN §4.6's whole argument for the numbering");

    // AND THE SAME CHAIN WITH NO CALL ON IT, which is a SECOND site and not the
    // same one. DESIGN §6.2 makes Member and Call peers, so a path is asked
    // about at whichever node ends it -- `display("x")` is refused from the
    // Call arm and a bare `satellite.consle.width` from the Member arm. Both
    // reach the same refusal and only one of them was covered until a mutation
    // asked: deleting the Member arm's report left every assertion above green,
    // because every fixture in this file happened to end in a call.
    Run bare_chain;
    resolve_source(R"(
satellite.capsule reads()
{
    satellite.variable.number w = satellite.consle.width
    satellite.return(w)
}
)",
                   bare_chain);
    check(only_problem(bare_chain, satellite::errors::Code::RESOLVE_NO_SUCH_WORD),
          "a misspelled segment in a chain with NO call on it is S0521 too");
    check(!bare_chain.resolved.problems.empty() &&
              bare_chain.resolved.problems.front().suggestion == "console",
          "and it is answered the same way, from the Member arm rather than the "
          "Call arm");

    // S0522 -- the same, for a type.
    Run bad_type;
    resolve_source(R"(
satellite.capsule reads()
{
    satellite.variable.strig s = "x"
    satellite.return()
}
)",
                   bad_type);
    check(raised(bad_type, satellite::errors::Code::RESOLVE_NO_SUCH_TYPE),
          "a type the numbering does not have is S0522");
    check(!bad_type.resolved.problems.empty() &&
              bad_type.resolved.problems.front().suggestion == "string",
          "with `string` offered, from satellite.variable's children");

    // WORD_NUMBERS §1.5's FOLD, WHICH IS THE CLAUSE PLAN'S M7 BULLET CLAIMS BY
    // NAME. "sort_down at 1 4 2 5 is that fold; sort(direction) at 1 4 2 4 is
    // the general form kept for when the option is a variable."
    Run folded;
    resolve_source(R"(
satellite.capsule sorts()
{
    satellite.container.list<satellite.variable.number> xs
    xs.sort("down")
    satellite.return()
}
)",
                   folded);
    check(folded.resolved.ok(), "a literal option resolves clean");
    check(resolved_to(folded, "1 4 2 5"),
          "sort(\"down\") folds to sort_down() 1 4 2 5 at resolve");
    check(!resolved_to(folded, "1 4 2 4"),
          "and NOT to sort(direction) 1 4 2 4, which is the general form kept "
          "for when the option is a variable");

    Run general;
    resolve_source(R"(
satellite.capsule sorts(satellite.variable.string how)
{
    satellite.container.list<satellite.variable.number> xs
    xs.sort(how)
    satellite.return()
}
)",
                   general);
    check(resolved_to(general, "1 4 2 4"),
          "and a variable option IS sort(direction) 1 4 2 4 -- the fold is a "
          "literal's and §2.4's inline cache takes the other case");

    // S0524 -- AN OPTION THE NUMBERING DOES NOT HAVE, refused rather than left
    // to the runtime. The legal options are not a table in the source: they are
    // the sibling rows spelled `<word>_<option>`.
    Run wrong_option;
    resolve_source(R"(
satellite.capsule sorts()
{
    satellite.container.list<satellite.variable.number> xs
    xs.sort("sideways")
    satellite.return()
}
)",
                   wrong_option);
    check(only_problem(wrong_option, satellite::errors::Code::RESOLVE_NO_SUCH_OPTION),
          "an option that names no row is S0524");
    check(!wrong_option.resolved.problems.empty() &&
              holds(satellite::errors::sentence(wrong_option.resolved.problems.front()),
                    "down"),
          "and the message lists the options the NUMBERING has, so a row added "
          "to words.def is offered here with no edit to any source file");

    // S0523 -- AND THE NUMBERING'S OWN ASYMMETRY, FOUND BY BUILDING THIS.
    // WORD_NUMBERS §2.2 has sort() 1 4 2 3 as "ascending, no key" and
    // sort_up(key) 1 4 2 7, and NO sort_up() -- because sort() already is it.
    // So the option `up` exists and the shape does not, which is a different
    // sentence and a different fix.
    Run wrong_shape;
    resolve_source(R"(
satellite.capsule sorts()
{
    satellite.container.list<satellite.variable.number> xs
    xs.sort("up")
    satellite.return()
}
)",
                   wrong_shape);
    check(only_problem(wrong_shape, satellite::errors::Code::RESOLVE_NO_SUCH_SHAPE),
          "sort(\"up\") with no key is S0523 and names sort_up(key) -- there is "
          "no sort_up(), because WORD_NUMBERS §2.2 makes sort() the ascending "
          "form. MILESTONES/M7.md §6 carries what that leaves open");

    // AND A WORD WITH NO `<word>_` SIBLINGS TAKES NO OPTIONS AT ALL, so a
    // string argument to it is an ordinary string.
    Run not_an_option;
    resolve_source(R"(
satellite.capsule looks()
{
    satellite.container.list<satellite.variable.string> xs
    xs.contains("ok")
    satellite.return()
}
)",
                   not_an_option);
    check(not_an_option.resolved.ok(),
          "contains(\"ok\") is not a fold -- `contains` has no sibling spelled "
          "contains_<something>, so the literal is just a literal");

    // THE ONE HOP, AND ITS BOUNDARY. WORD_NUMBERS §1.5: a selector's number is
    // reachable only THROUGH THE RECEIVER'S TYPE. This milestone knows one
    // receiver's type -- a name it just read the declaration of.
    Run selector;
    resolve_source(R"(
satellite.capsule uses()
{
    satellite.variable.thread t = satellite.thread.new(uses())
    t.start()
    satellite.return()
}
)",
                   selector);
    check(resolved_to(selector, "1 6 13 1"),
          "t.start() is 1 6 13 1, reached through the declared type of `t` -- "
          "which is the number a `.satc` can never carry (WORD_NUMBERS §1.5)");
}

} // namespace resolve_test
