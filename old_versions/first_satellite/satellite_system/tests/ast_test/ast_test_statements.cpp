// Statements: return with and without a value, assignment through a plain name
// and through a library path, and the four spaces a block adds per level.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. The
// failure counter and the tree builders these checks use are declared in
// ast_test.hpp and defined in ast_test.cpp.

#include "ast_test.hpp"

void ast_test_statements()
{
    // ---- statements -------------------------------------------------------
    check_text(unparse(*make_stmt(Return{sat()}, {})),
               "satellite.return(satellite)", "return the runtime singleton");
    check_text(unparse(*make_stmt(Return{nullptr}, {})), "satellite.return()",
               "a bare return");
    check_text(unparse(*make_stmt(Assign{name("x"), num(5)}, {})), "x = 5",
               "an assignment");
    check_text(
        unparse(*make_stmt(
            Assign{path({"library", "main", "x"}), num(5)}, {})),
        "satellite.library.main.x = 5", "an assignment through a library path");
}

void ast_test_blocks()
{
    // ---- blocks indent ----------------------------------------------------
    {
        StmtPtr inner = make_stmt(
            Block{{make_stmt(Return{num(1)}, {})}}, {});
        StmtPtr outer = make_stmt(Block{{inner}}, {});
        check_text(unparse(*outer),
                   "{\n"
                   "    {\n"
                   "        satellite.return(1)\n"
                   "    }\n"
                   "}",
                   "nested blocks indent four spaces per level");
    }
}
