// Expressions: method calls on a receiver, indexing and slicing, and the
// operator precedence that decides where unparse has to put a parenthesis
// back. These are the forms that have to survive a round trip through text,
// so most of what they assert is the exact string.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. The
// failure counter and the tree builders these checks use are declared in
// ast_test.hpp and defined in ast_test.cpp.

#include "ast_test.hpp"

void ast_test_receiver_calls()
{
    // ---- receiver method calls -------------------------------------------
    // A method call on a user variable and a call into the language are the
    // same shape; nothing in the tree resolves which is which.
    check_text(unparse(*call(member(name("my_time"), "some_function"))),
               "my_time.some_function()", "a receiver method call");
    check_text(unparse(*call(path({"time", "now"}))), "satellite.time.now()",
               "a language call");
    check_text(unparse(*call(member(name("my_list"), "append"), {num(1)})),
               "my_list.append(1)", "a method call with an argument");

    // Chaining is uniform because Member, Call and Index are peers.
    check_text(
        unparse(*call(member(call(path({"time", "now"})), "some_function"))),
        "satellite.time.now().some_function()", "a chained call");
}

void ast_test_indexing_and_slicing()
{
    // ---- indexing and slicing --------------------------------------------
    check_text(unparse(*make_expr(Index{name("list_name"), num(3)}, {})),
               "list_name[3]", "an index");
    check_text(unparse(*make_expr(Slice{name("l"), num(2), num(5)}, {})),
               "l[2:5]", "a slice with both bounds");
    check_text(unparse(*make_expr(Slice{name("l"), nullptr, num(5)}, {})),
               "l[:5]", "a slice with no low bound");
    check_text(unparse(*make_expr(Slice{name("l"), num(2), nullptr}, {})),
               "l[2:]", "a slice with no high bound");
    check_text(unparse(*make_expr(Slice{name("l"), nullptr, nullptr}, {})),
               "l[:]", "a slice with neither bound");
}

void ast_test_operators()
{
    // ---- operators and parenthesisation ----------------------------------
    check(precedence("*") > precedence("+"), "* binds tighter than +");
    check(precedence("+") > precedence("=="), "+ binds tighter than ==");
    check(precedence("satellite") == 0, "a word is not a binary operator");
    check(is_unary_op("-") && !is_unary_op("*"), "unary operators");

    // Parentheses appear only where dropping them would reassociate.
    check_text(unparse(*binary("+", binary("+", name("a"), name("b")), name("c"))),
               "a + b + c", "left nesting needs no parentheses");
    check_text(unparse(*binary("+", name("a"), binary("+", name("b"), name("c")))),
               "a + (b + c)", "right nesting keeps its parentheses");
    check_text(unparse(*binary("*", binary("+", name("a"), name("b")), name("c"))),
               "(a + b) * c", "a looser child is wrapped");
    check_text(unparse(*binary("+", binary("*", name("a"), name("b")), name("c"))),
               "a * b + c", "a tighter child is not wrapped");
    check_text(unparse(*make_expr(Unary{"-", name("x")}, {})), "-x",
               "unary minus");
    check_text(unparse(*make_expr(Unary{"-", binary("+", name("a"), name("b"))},
                                  {})),
               "-(a + b)", "unary wraps a binary operand");
}
