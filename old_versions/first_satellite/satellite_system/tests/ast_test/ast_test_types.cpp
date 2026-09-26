// Types and declarations: how a type spells itself, what a generic looks like
// when it nests, what the empty space means, and what a VarDecl unparses to
// with and without an initialiser.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. The
// failure counter and the tree builders these checks use are declared in
// ast_test.hpp and defined in ast_test.cpp.

#include "ast_test.hpp"

void ast_test_types()
{
    // ---- types ------------------------------------------------------------
    check_text(unparse(var_type("time")), "satellite.variable.time",
               "a plain type");
    check_text(unparse(Type{"container", "list", {var_type("string")}, {}}),
               "satellite.container.list<satellite.variable.string>",
               "a generic type");
    check_text(unparse(Type{"container",
                            "list",
                            {Type{"container", "list", {var_type("string")}, {}}},
                            {}}),
               "satellite.container.list<satellite.container.list"
               "<satellite.variable.string>>",
               "a nested generic type");
    check_text(unparse(Type{}), "satellite", "the singleton type");
    check(Type{}.is_singleton(), "an empty space means the singleton type");
    check(!var_type("time").is_singleton(), "a named type is not the singleton");
}

void ast_test_declarations()
{
    // ---- the declaration form --------------------------------------------
    {
        StmtPtr decl = make_stmt(
            VarDecl{var_type("time"), "my_time",
                    call(path({"time", "now"}))},
            {});
        check_text(unparse(*decl),
                   "satellite.variable.time my_time = satellite.time.now()",
                   "the declaration from the design");
    }
    {
        // A declaration with no initialiser.
        StmtPtr decl =
            make_stmt(VarDecl{var_type("file"), "my_file", nullptr}, {});
        check_text(unparse(*decl), "satellite.variable.file my_file",
                   "an uninitialised declaration");
    }
}
