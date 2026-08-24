// Capsules and whole programs: the runtime's entry point, which is the only
// capsule that carries the satellite prefix, a user capsule with a declared
// return type, and a Program that has to unparse back into the hello world
// source it was built from.
//
// The whole-program check ends by lexing what unparse emitted. That is the
// half of the round trip that exists today: until there is a parser, the most
// that can be shown is that the text unparse produces is text the lexer
// accepts without an error token.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. The
// failure counter and the tree builders these checks use are declared in
// ast_test.hpp and defined in ast_test.cpp.

#include "ast_test.hpp"

void ast_test_capsules()
{
    // ---- capsules ---------------------------------------------------------
    {
        Capsule c;
        c.reserved = true;
        c.name = "main";
        c.params.push_back(
            Param{Type{"container", "list", {var_type("string")}, {}}, "argz", {}});
        c.body = make_stmt(
            Block{{make_stmt(ExprStmt{call(path({"console", "display"}),
                                           {str("hello, world!")})},
                             {}),
                   make_stmt(Return{sat()}, {})}},
            {});

        check_text(unparse(c),
                   "satellite.capsule satellite.main("
                   "satellite.container.list<satellite.variable.string> argz)\n"
                   "{\n"
                   "    satellite.console.display(\"hello, world!\")\n"
                   "    satellite.return(satellite)\n"
                   "}",
                   "the hello world capsule");
    }
    {
        // A capsule the user writes is bare; only the runtime's entry point
        // is prefixed.
        Capsule c;
        c.name = "fact";
        c.params.push_back(Param{var_type("number"), "n", {}});
        c.returns = var_type("number");
        c.body = make_stmt(Block{{make_stmt(Return{name("n")}, {})}}, {});
        check_text(unparse(c),
                   "satellite.capsule fact(satellite.variable.number n) "
                   "satellite.returns(satellite.variable.number)\n"
                   "{\n"
                   "    satellite.return(n)\n"
                   "}",
                   "a user capsule with a declared return type");
    }
}

void ast_test_whole_program()
{
    // ---- a whole program --------------------------------------------------
    {
        Program p;
        p.items.push_back(Include{sat(), {}});

        Capsule c;
        c.reserved = true;
        c.name = "main";
        c.params.push_back(
            Param{Type{"container", "list", {var_type("string")}, {}}, "argz", {}});
        c.body = make_stmt(
            Block{{make_stmt(ExprStmt{call(path({"console", "display"}),
                                           {str("hello, world!")})},
                             {}),
                   make_stmt(Return{sat()}, {})}},
            {});
        p.items.push_back(std::move(c));

        const std::string want =
            "satellite.include(satellite)\n"
            "\n"
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.string> argz)\n"
            "{\n"
            "    satellite.console.display(\"hello, world!\")\n"
            "    satellite.return(satellite)\n"
            "}\n";
        check_text(unparse(p), want, "hello world unparses to itself");

        // Whatever unparse emits must lex cleanly, or the round-trip check the
        // parser milestone depends on could never hold.
        for (const Token &t : lex(unparse(p)))
            check(t.kind != TokenKind::Error, "unparsed output lexes cleanly");
    }
}
