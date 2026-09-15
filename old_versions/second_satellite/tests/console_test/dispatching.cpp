// `satellite.console.display` `1 5 1` through a compiled program. See
// tests/console_test/console_test.hpp.
//
// THE FIRST REAL ROW handlers[path_id] HAS EVER HELD, and this is its reader.
// evaluator/dispatch.hpp says of itself that it is "EMPTY IN `satl` AT M9 ...
// the first rows are M10's console", and PLAN M2's rule is that a registry gets
// a consumer in the milestone that writes it. tests/eval_test installs handlers
// of its own to prove the MECHANISM; what is proved here is that the real row
// is installed, is found by its number, and prints through the real queue.
//
// THE PROGRAM IS COMPILED AND RUN RATHER THAN THE HANDLER CALLED, which is the
// point of the fixture. Calling `display()` directly would check the console
// and skip everything DESIGN §4's numbering is for -- the trie walk that turns
// `satellite.console.display` into `1 5 1`, the array index that finds the row,
// the arity check, and the inline cache that makes the second call at a site do
// no lookup at all.

#include "console_test.hpp"

#include "evaluator/dispatch.hpp"
#include "evaluator/evaluate.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_console/console.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <vector>

namespace console_test {

namespace {

// Compile a source and run the capsule called `main`, the way `satl` does.
//
// EVERYTHING LIVES IN ONE SCOPE AND IT HAS TO: a user's PathId is valid inside
// one run, an op's operands index the arena beside it, and a Machine holds
// references to both.
void run(const std::string &source)
{
    using namespace satellite;

    words::Words words;
    Parse parsed = parse(source, words);
    if (!parsed.ok()) {
        check(false, "the fixture did not parse");
        return;
    }
    resolve::Resolved resolved = resolve::resolve(parsed.ast, words);
    if (!resolved.ok()) {
        check(false, "the fixture did not resolve");
        return;
    }
    eval::Program program = eval::compile(parsed.ast, resolved, words);
    if (!program.ok()) {
        check(false, "the fixture did not compile");
        return;
    }

    const int which =
        program.find(static_cast<words::PathId>(words::NodeId::MAIN));
    if (which < 0) {
        check(false, "the fixture declares no satellite.main");
        return;
    }

    eval::Policy policy;
    policy.max_depth = 64ull * 1024 * 1024;
    eval::Machine machine(program.closures, parsed.ast, policy);
    machine.run_top_level();
    if (machine.ok())
        machine.call(static_cast<uint32_t>(which), {});
    check(machine.ok(), "and it ran without a complaint");
}

} // namespace

void section_dispatching()
{
    using namespace satellite;

    console::install_handlers();

    const words::PathId display =
        static_cast<words::PathId>(words::NodeId::CONSOLE_DISPLAY);
    const eval::Handler *row = eval::Handlers::table().find(display);
    check(row != nullptr,
          "`satellite.console.display` `1 5 1` has a row in handlers[path_id] "
          "-- the first the table has ever held");
    if (row != nullptr) {
        check(row->arity == 1, "it takes one argument");
        check(row->binds_receiver == false,
              "and binds no receiver: DESIGN §6.4 q2's tag is about a method "
              "called ON a value, and `console` is a namespace");
    }

    // --- a program prints ----------------------------------------------------
    {
        const std::string out = capture([] {
            run("satellite.capsule satellite.main()\n"
                "{\n"
                "    satellite.console.display(\"Hello, World!\")\n"
                "    satellite.return(satellite)\n"
                "}\n");
        });
        check(out == "Hello, World!\n",
              "a compiled program reaches the console through its path number "
              "and one array index -- PLAN §1.1's seven-arm string chain, gone");
    }

    // --- and it prints every kind of value -----------------------------------
    //
    // `display` TAKES A VALUE AND NOT A STRING, which is DESIGN §7.7 read from
    // the other end: "Displaying it bare prints all of it" is said there about
    // `arguments`, which is a list. satellite_value/render.hpp is the one place
    // a value becomes characters, and its own comment has said since M9 that
    // "M10's `satellite.console.display` will print through it".
    //
    // AND RENDERING IS NOT CONVERSION. The language still has none: `"x" + 1`
    // is as much an error after this milestone as before it, which
    // tests/eval_test/arithmetic.cpp is where that is asserted.
    {
        const std::string out = capture([] {
            run("satellite.capsule satellite.main()\n"
                "{\n"
                "    satellite.console.display(6 * 7)\n"
                "    satellite.console.display(\"text\")\n"
                "    satellite.console.display(satellite)\n"
                "    satellite.return(satellite)\n"
                "}\n");
        });
        check(out == "42\ntext\nsatellite\n",
              "a number, a string and the runtime singleton each print as the "
              "thing a person reads");
    }

    // --- the wrong number of arguments is refused before the handler runs -----
    //
    // AND NOTHING IS PRINTED, which is the half worth checking. PLAN §2.3's
    // "how many arguments, already checked" is about the cost; what it buys
    // here is that a call satl is going to refuse does not first put a line on
    // the screen.
    {
        const std::string out = capture([] {
            using namespace satellite;
            words::Words words;
            Parse parsed = parse("satellite.capsule satellite.main()\n"
                                 "{\n"
                                 "    satellite.console.display(\"a\", \"b\")\n"
                                 "}\n",
                                 words);
            resolve::Resolved resolved = resolve::resolve(parsed.ast, words);
            eval::Program program = eval::compile(parsed.ast, resolved, words);
            eval::Policy policy;
            policy.max_depth = 64ull * 1024 * 1024;
            eval::Machine machine(program.closures, parsed.ast, policy);
            machine.call(
                static_cast<uint32_t>(
                    program.find(static_cast<words::PathId>(words::NodeId::MAIN))),
                {});
            check(!machine.ok(), "a two-argument `display` is refused");
            check(!machine.problems().empty() &&
                      machine.problems().front().code ==
                          errors::Code::EVAL_ARGUMENT_COUNT,
                  "S0722, and the sentence reads \"takes 1 argument\" rather "
                  "than \"1 arguments\" -- eval::arity_text");
        });
        check(out.empty(), "and nothing was printed on the way to refusing it");
    }

    // --- two options, each under its own name --------------------------------
    //
    // M30. The compiler once pushed named values in written order onto a task
    // stack that runs the last push first, so `foreground=` was handed end='s
    // "!" and refused. One option cannot show that; two in both orders can.
    // capture() is a pipe, so the style is checked and the text comes out plain.
    {
        const std::string out = capture([] {
            run("satellite.capsule satellite.main()\n"
                "{\n"
                "    satellite.console.display(\"a\", foreground=xFF8800, end=\"!\")\n"
                "    satellite.console.display(\"b\", end=\"?\", background=x000000)\n"
                "    satellite.console.display(\"\")\n"
                "    satellite.return(satellite)\n"
                "}\n");
        });
        check(out == "a!b?\n",
              "each option reaches the handler under its own name, in either "
              "order, and a pipe gets the text without escapes");
    }

    // --- a colour that is not xRRGGBB is refused, even into a pipe ----------
    {
        const std::string out = capture([] {
            using namespace satellite;
            words::Words words;
            Parse parsed = parse("satellite.capsule satellite.main()\n"
                                 "{\n"
                                 "    satellite.console.display(\"a\", foreground=xF80)\n"
                                 "}\n",
                                 words);
            resolve::Resolved resolved = resolve::resolve(parsed.ast, words);
            eval::Program program = eval::compile(parsed.ast, resolved, words);
            eval::Policy policy;
            policy.max_depth = 64ull * 1024 * 1024;
            eval::Machine machine(program.closures, parsed.ast, policy);
            machine.call(
                static_cast<uint32_t>(
                    program.find(static_cast<words::PathId>(words::NodeId::MAIN))),
                {});
            check(!machine.problems().empty() &&
                      machine.problems().front().code ==
                          errors::Code::CONSOLE_NOT_A_COLOUR,
                  "S1004: xF80 is three digits, and a colour is six");
        });
        check(out.empty(), "and the text was not printed without its colour");
    }
}

} // namespace console_test
