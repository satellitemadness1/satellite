// argv -> satellite strings -> satellite.main's parameter: the conversion
// itself, and the parameter it lands in. Part of the interp_test binary; the
// harness it calls is declared in interp_test.hpp.

#include "interp_test.hpp"

#include "interpreter/interp.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/value.hpp"
#include "system_facts/system.hpp"

#include <string>
#include <vector>

using namespace satellite;

static std::string decoded(const List &list, size_t i)
{
    return (i < list.size() && list[i]) ? to_string(*list[i]) : "<missing>";
}

void interp_test_argument_conversion()
{
    // --- the conversion ----------------------------------------------------
    // An argument is data the shell has already finished escaping. Running the
    // backslash-name expansion over it again rewrites it: encode() turns
    // C:\home into C:/home/madness with no error and no warning. This is the
    // same defect §3.3 fixed for source text, and argv is where it bites next.
    {
        const std::string raw = "--path=C:\\home";
        List args = args_to_list({raw});
        check(args.size() == 1, "one argument in, one value out");
        check(decoded(args, 0) == raw,
              "argv keeps its backslashes (encode_raw, not encode)");
        check(decode(encode(raw)) != raw,
              "encode() really would have corrupted it");
        check(decoded(args, 0).find(home_dir()) == std::string::npos,
              "argv does not expand \\home");
    }

    // Every argument becomes exactly one satellite.variable.string.
    {
        List args = args_to_list({"alpha", "beta", "gamma"});
        check(args.size() == 3, "three arguments");
        check(decoded(args, 0) == "alpha" && decoded(args, 2) == "gamma",
              "arguments keep their order");
        check(args_to_list({}).empty(), "no arguments is an empty list");
    }

    // Arguments that are not identifiers survive too: spaces, quotes, digits.
    {
        List args = args_to_list({"two words", "a\"b", "42", ""});
        check(decoded(args, 0) == "two words", "argument with a space");
        check(decoded(args, 1) == "a\"b", "argument with a quote");
        check(decoded(args, 2) == "42", "a numeric argument stays a string");
        check(decoded(args, 3).empty(), "an empty argument stays empty");
    }
}

void interp_test_main_capsule()
{
    // --- satellite.main ----------------------------------------------------
    // The §2 hello world, byte for byte, finally invocable.
    check_run("satellite.include(satellite)\n"
              "\n"
              "satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.console.display(\"hello, world!\")\n"
              "\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"hello.sat"}, "hello, world!\n", "the DESIGN.md §2 hello world");

    // argz is bound, indexable, and has a length.
    check_run("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.console.display(argz.length())\n"
              "    satellite.console.display(argz[0])\n"
              "    satellite.console.display(argz[2])\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog", "one", "two"}, "3\nprog\ntwo\n", "argz is bound");

    // A backslash argument survives all the way into the program.
    check_run("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.console.display(argz[1])\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog", "C:\\home"}, "C:\\home\n",
              "a backslash argument reaches the program intact");

    // The parameter is optional, and its name is the user's to choose.
    check_run("satellite.capsule satellite.main()\n"
              "{\n"
              "    satellite.console.display(\"no args\")\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog"}, "no args\n", "satellite.main with no parameter");
    check_run("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> whatever)\n"
              "{\n"
              "    satellite.console.display(whatever.length())\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog"}, "1\n", "the parameter name is the user's");

    // Top-level statements run before main, and a program with no main is
    // still a legal program.
    check_run("satellite.console.display(\"top\")\n"
              "satellite.capsule satellite.main()\n"
              "{\n"
              "    satellite.console.display(\"main\")\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog"}, "top\nmain\n", "top-level runs before main");
    check_run("satellite.console.display(\"only top level\")\n", {"prog"},
              "only top level\n", "a program with no satellite.main is legal");

    // The signature is fixed by §2: a wrong parameter type is caught rather
    // than quietly accepted because today's argument list happens to be empty.
    {
        InterpResult r = run_program(
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.number> argz)\n"
            "{\n"
            "    satellite.return(satellite)\n"
            "}\n",
            {});
        check(!r.ok, "satellite.main with the wrong parameter type is rejected");
        check(r.output.find("must be") != std::string::npos,
              "and says what the parameter must be");
    }
}
