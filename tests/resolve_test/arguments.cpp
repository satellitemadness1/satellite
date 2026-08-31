// DESIGN §7.7 -- the one library global the language provides, its six
// spellings, and the seventh that §7.7 left open and this milestone answered.

#include "resolve_test.hpp"

#include "name_resolver/resolve.hpp"

#include <string>

namespace resolve_test {

namespace {

std::string main_with(const std::string &parameter)
{
    return "\nsatellite.capsule satellite.main("
           "satellite.container.list<satellite.variable.string> " +
           parameter + ")\n{\n    satellite.return(satellite)\n}\n";
}

} // namespace

void section_arguments()
{
    using namespace satellite::resolve;

    // ALL SIX, AND THE COUNT IS THE CHECK. WORD_NUMBERS §2.3's third alias row
    // is one node with six spellings, and §7.7 says why the other five exist:
    // "people type what they type, and a language whose tie-breaker is do
    // absolutely everything for the user does not make somebody lose an
    // afternoon to a plural."
    const char *const six[] = {"arg",      "args",      "argz",
                               "argument", "arguments", "argumentz"};
    for (const char *const spelling : six) {
        Run run;
        resolve_source(main_with(spelling), run);
        const Frame *frame = frame_of(run, "main");
        check(run.resolved.ok(),
              std::string("satellite.main(") + spelling + ") resolves clean");
        check(frame != nullptr && frame->arguments == 0,
              std::string("`") + spelling +
                  "` is the arguments object, and it is slot 0 -- a PARAMETER "
                  "with a real slot, which is DESIGN §7.1's `parameters are "
                  "locals too` and the reason the registry could not be kept");
    }

    // AND A SEVENTH IS REFUSED. DESIGN §7.7's own open question: "Declaring a
    // parameter named `argv` gets a plain list with no properties, silently.
    // Under §9 that silence is wrong -- the language should say so."
    Run seventh;
    resolve_source(main_with("argv"), seventh);
    check(only_problem(seventh, satellite::errors::Code::RESOLVE_ALMOST_ARGUMENTS),
          "`argv` is S0531 rather than a silent plain list");
    check(!seventh.resolved.problems.empty() &&
              !seventh.resolved.problems.front().suggestion.empty(),
          "and it is answered with one of the six, because the suggester having "
          "an answer is the CONDITION for refusing at all");

    // AND A NAME THAT WAS NEVER TRYING TO BE ONE IS LEFT ALONE, which is the
    // half that makes the rule a rule rather than a blocklist. `input_lines` is
    // nowhere near any of the six, so it is an ordinary list and says nothing.
    Run plain;
    resolve_source(main_with("input_lines"), plain);
    check(plain.resolved.ok(),
          "a parameter that is not near any of the six is an ordinary list");
    const Frame *plain_main = frame_of(plain, "main");
    check(plain_main != nullptr && plain_main->arguments == -1,
          "and it is NOT the arguments object -- §7.7 recognises the name the "
          "user chose, it does not introduce one");

    // ONLY satellite.main HAS ONE, WHICH IS THIS MILESTONE'S DECISION. §7.7
    // puts the object at `satellite.library.main.arguments` and calls it "the
    // language handing the PROGRAM everything it knows about the machine it
    // woke up on". A capsule of the user's own with a parameter called `args`
    // is holding whatever its caller passed; the M6 draft recognises the six
    // spellings in EVERY capsule, which would put the machine's answers in that
    // variable instead.
    Run other;
    resolve_source(R"(
satellite.capsule helper(satellite.container.list<satellite.variable.string> args)
{
    satellite.return(args)
}
)",
                   other);
    check(other.resolved.ok(), "a capsule of the user's own may take `args`");
    const Frame *helper = frame_of(other, "helper");
    check(helper != nullptr && helper->arguments == -1,
          "and it is NOT the arguments object -- it holds what its caller "
          "passed, and giving it the machine's answers would be DESIGN §1.1's "
          "`behind their back` with the wrong value in the variable");

    Run also_argv;
    resolve_source(R"(
satellite.capsule helper(satellite.container.list<satellite.variable.string> argv)
{
    satellite.return(argv)
}
)",
                   also_argv);
    check(also_argv.resolved.ok(),
          "and `argv` on a capsule of the user's own is not refused either -- "
          "S0531 is about satellite.main's parameter and nothing else");

    // WHAT IT HOLDS, AND THE FIELDS COME OUT OF THE NUMBERING. §7.7 writes
    // seven names and words.def has all seven as nodes, so nothing in the
    // resolver keeps a list of them. `arguments.machine.threads` is 1 14 1 1 1
    // 3 -- six numbers deep, which WORD_NUMBERS §4 calls the clearest argument
    // in the language for §1.3 refusing a segment limit.
    Run fields;
    resolve_source(R"(
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> args)
{
    satellite.console.display(args.username)
    satellite.console.display(args.memory.total)
    satellite.console.display(args.machine.cores)
    satellite.console.display(args.machine.threads)
    satellite.return(satellite)
}
)",
                   fields);
    check(fields.resolved.ok(), "every field DESIGN §7.7 lists resolves");
    check(resolved_to(fields, "1 14 1 1 1 3"),
          "arguments.machine.threads is 1 14 1 1 1 3 -- six numbers deep, and "
          "reached through a spelling the language does not itself use");
    check(resolved_to(fields, "1 14 1 1 2 1"), "arguments.memory.total is 1 14 1 1 2 1");
    check(resolved_to(fields, "1 14 1 1 3"), "arguments.username is 1 14 1 1 3");

    // S0532 -- A FIELD IT DOES NOT HAVE.
    Run missing;
    resolve_source(R"(
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> args)
{
    satellite.console.display(args.machne)
    satellite.return(satellite)
}
)",
                   missing);
    check(only_problem(missing, satellite::errors::Code::RESOLVE_NO_SUCH_ARGUMENT_FIELD),
          "a field the arguments object does not have is S0532");
    check(!missing.resolved.problems.empty() &&
              missing.resolved.problems.front().suggestion == "machine",
          "with the suggestion coming out of the numbering rather than a list "
          "in the resolver -- which is what keeps the two from drifting when "
          "§7.7's open question about v1's other 33 entries is settled");

    // AND DISPLAYING IT BARE IS LEGAL, which §7.7 makes a point of: "displaying
    // it bare prints all of it ... the language hands over all of it rather
    // than making somebody ask for it one field at a time."
    Run bare;
    resolve_source(R"(
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(arguments)
    satellite.return(satellite)
}
)",
                   bare);
    check(bare.resolved.ok(), "displaying the object bare resolves");
}

} // namespace resolve_test
