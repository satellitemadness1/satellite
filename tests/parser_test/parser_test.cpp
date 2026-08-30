// parser_test -- the proof that src/parser/ and src/abstract_syntax_tree/ do
// what DESIGN §6 says. See tests/parser_test/parser_test.hpp for the harness.
//
// THE THREE INHERITED RULES ARE WHAT THIS TEST IS FOR. DESIGN §6.1, §6.2 and
// §6.3 each record a verified defect -- a structural declaration rule that
// silently declares a variable of type `satellite.control.return`, a postfix
// sketch that cannot parse `foo().bar()`, and a parser taught the standard
// library's shape -- and a rule inherited without a check is a rule that gets
// rediscovered. Every one of the three has a check here that names it.
//
// AND FOR THE ONE THING ONLY A ROUND-TRIP CAN SEE. A tree is not observable
// until M8 runs one, so every check below that asserts a shape is asserting
// what the parser was TOLD to build. section_roundtrip() asserts something
// else: that what came out means the same program that went in, over the four
// files LAYOUT.md calls the acceptance programs.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "error_reporter/report.hpp"
#include "parser/parser.hpp"

#include <cstdio>
#include <string>

namespace parser_test {

int failures = 0;
std::string example_directory = "example";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

std::string Program::first_error() const
{
    if (parse.errors.empty())
        return std::string();
    // THE SENTENCE AND NOT THE BLOCK, which is what a check that says "and the
    // reason names the construct" wants. M5 turned a ParseError's `reason` into
    // an errors::Diagnostic; errors::sentence is the same words with the holes
    // filled and nothing around them.
    return satellite::errors::sentence(parse.errors.front());
}

Program run(const std::string &source)
{
    Program program;
    program.parse = satellite::parse(source, program.words);
    return program;
}

Program in_capsule(const std::string &statements)
{
    return run("satellite.capsule fixture()\n{\n" + statements + "\n}\n");
}

satellite::NodeIndex first_of(const satellite::Ast &ast, satellite::NodeKind kind)
{
    for (satellite::NodeIndex i = 1; i < ast.size(); i++)
        if (ast[i].kind == kind)
            return i;
    return satellite::kNoNode;
}

size_t count_of(const satellite::Ast &ast, satellite::NodeKind kind)
{
    size_t found = 0;
    for (satellite::NodeIndex i = 1; i < ast.size(); i++)
        if (ast[i].kind == kind)
            found++;
    return found;
}

std::string print(const satellite::Ast &ast, satellite::NodeIndex node)
{
    return satellite::unparse(ast, node);
}

} // namespace parser_test

int main(int argc, char **argv)
{
    if (argc > 1)
        parser_test::example_directory = argv[1];

    parser_test::section_arena();
    parser_test::section_expressions();
    parser_test::section_statements();
    parser_test::section_declarations();
    parser_test::section_roundtrip();

    if (parser_test::failures) {
        printf("parser_test: %d failure(s)\n", parser_test::failures);
        return 1;
    }
    printf("parser_test: ok\n");
    return 0;
}
