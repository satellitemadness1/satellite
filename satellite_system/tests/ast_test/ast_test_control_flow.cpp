// Control flow: if, if/else, while, the three-part for and the infinite loop
// it degrades to, how a nested loop indents, and the check that every keyword
// the language owns is still rooted at satellite.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. The
// failure counter and the tree builders these checks use are declared in
// ast_test.hpp and defined in ast_test.cpp.

#include "ast_test.hpp"

void ast_test_control_flow()
{
    // ---- control flow -----------------------------------------------------
    {
        StmtPtr body = make_stmt(Block{{make_stmt(Return{num(1)}, {})}}, {});

        check_text(unparse(*make_stmt(
                       If{binary("<=", name("n"), num(1)), body, nullptr}, {})),
                   "satellite.statement.if(n <= 1)\n"
                   "{\n"
                   "    satellite.return(1)\n"
                   "}",
                   "an if statement");

        check_text(unparse(*make_stmt(
                       If{binary("<=", name("n"), num(1)), body, body}, {})),
                   "satellite.statement.if(n <= 1)\n"
                   "{\n"
                   "    satellite.return(1)\n"
                   "}\n"
                   "satellite.statement.else\n"
                   "{\n"
                   "    satellite.return(1)\n"
                   "}",
                   "an if with an else branch");

        check_text(unparse(*make_stmt(
                       While{binary(">", name("n"), num(0)), body}, {})),
                   "satellite.statement.while(n > 0)\n"
                   "{\n"
                   "    satellite.return(1)\n"
                   "}",
                   "a while statement");

        // The three-part for reuses VarDecl for init and Assign for step, so
        // it introduces no statement form the language did not already have.
        StmtPtr init = make_stmt(VarDecl{var_type("number"), "i", num(0)}, {});
        StmtPtr step = make_stmt(
            Assign{name("i"), binary("+", name("i"), num(1))}, {});
        StmtPtr display = make_stmt(
            Block{{make_stmt(
                ExprStmt{call(path({"console", "display"}),
                              {make_expr(Index{name("argz"), name("i")}, {})})},
                {})}},
            {});

        check_text(unparse(*make_stmt(
                       For{init, binary("<", name("i"), num(10)), step,
                           display},
                       {})),
                   "satellite.statement.for(satellite.variable.number i = 0; "
                   "i < 10; i = i + 1)\n"
                   "{\n"
                   "    satellite.console.display(argz[i])\n"
                   "}",
                   "a three-part for loop");

        // Every part is optional, so for(;;) is the infinite loop.
        check_text(unparse(*make_stmt(
                       For{nullptr, nullptr, nullptr, body}, {})),
                   "satellite.statement.for(; ; )\n"
                   "{\n"
                   "    satellite.return(1)\n"
                   "}",
                   "a for loop with no header parts");

        // Nesting keeps the header at level 0 while the body indents.
        StmtPtr outer = make_stmt(
            Block{{make_stmt(While{binary(">", name("n"), num(0)), body}, {})}},
            {});
        check_text(unparse(*outer),
                   "{\n"
                   "    satellite.statement.while(n > 0)\n"
                   "    {\n"
                   "        satellite.return(1)\n"
                   "    }\n"
                   "}",
                   "a nested loop indents its header and body together");
    }
}

void ast_test_control_flow_keywords()
{
    // Control flow sits in its own language namespace, so every keyword the
    // language owns is still satellite-rooted.
    check(std::string(KW_IF).rfind("satellite.statement.", 0) == 0 &&
              std::string(KW_ELSE).rfind("satellite.statement.", 0) == 0 &&
              std::string(KW_WHILE).rfind("satellite.statement.", 0) == 0 &&
              std::string(KW_FOR).rfind("satellite.statement.", 0) == 0,
          "control flow lives under satellite.statement");
}
