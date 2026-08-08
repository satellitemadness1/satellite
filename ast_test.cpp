// AST tests. There is no parser yet, so trees are built by hand and checked
// through unparse — which is exactly the check the parser will need later,
// when unparse(parse(src)) == src becomes the round-trip test.

#include "ast.hpp"
#include "value.hpp"   // only to assert the two variants stayed independent

#include <cstdio>
#include <string>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

static void check_text(const std::string &got, const std::string &want,
                       const std::string &what)
{
    if (got != want) {
        printf("FAIL: %s\n  want: %s\n   got: %s\n", what.c_str(),
               want.c_str(), got.c_str());
        failures++;
    }
}

// --- small builders, so the trees below stay readable ----------------------

static ExprPtr num(long long v, const std::string &text)
{
    return make_expr(NumberLit{v, text}, {});
}
static ExprPtr num(int v) { return num(v, std::to_string(v)); }

static ExprPtr str(const std::string &s)
{
    return make_expr(StringLit{encode(s), s}, {});
}
static ExprPtr sat() { return make_expr(SatelliteLit{}, {}); }
static ExprPtr name(const std::string &n) { return make_expr(Name{n}, {}); }

static ExprPtr member(ExprPtr target, const std::string &n)
{
    return make_expr(Member{std::move(target), n}, {});
}
static ExprPtr call(ExprPtr target, std::vector<ExprPtr> args = {})
{
    return make_expr(Call{std::move(target), std::move(args)}, {});
}
static ExprPtr binary(const std::string &op, ExprPtr l, ExprPtr r)
{
    return make_expr(Binary{op, std::move(l), std::move(r)}, {});
}

// satellite.time.now  ->  Member(Member(SatelliteLit, "time"), "now")
static ExprPtr path(std::vector<std::string> segments)
{
    ExprPtr node = sat();
    for (const std::string &s : segments)
        node = member(node, s);
    return node;
}

static Type var_type(const std::string &n) { return Type{"variable", n, {}, {}}; }

int main()
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

    // Control flow sits in its own language namespace, so every keyword the
    // language owns is still satellite-rooted.
    check(std::string(KW_IF).rfind("satellite.statement.", 0) == 0 &&
              std::string(KW_ELSE).rfind("satellite.statement.", 0) == 0 &&
              std::string(KW_WHILE).rfind("satellite.statement.", 0) == 0 &&
              std::string(KW_FOR).rfind("satellite.statement.", 0) == 0,
          "control flow lives under satellite.statement");

    // ---- spans ------------------------------------------------------------
    {
        std::vector<Token> t = lex("satellite.variable.time my_time");
        Span s = span_of(t[5]);
        check(s.start == 24 && s.end == 31 && s.line == 1,
              "a span carries byte offsets straight from the token");
        Span joined = span_join(span_of(t[0]), span_of(t[5]));
        check(joined.start == 0 && joined.end == 31,
              "joining spans covers the whole range");
    }

    // ---- Expr and Value stay separate ------------------------------------
    // This is what the split buys, and it is the reason the expression kinds
    // did not join Value's variant. A syntax node is much larger than a
    // runtime value, so folding the two together would have grown every
    // number and every list element in the language to match the largest
    // syntax node — turning a list of a million numbers from 38 MB into
    // 91 MB to carry fields no value ever reads.
    check(sizeof(Value) < sizeof(Expr),
          "folding Expr into Value would grow every runtime value");

    if (failures) {
        printf("%d ast check(s) failed\n", failures);
        return 1;
    }
    printf("PASS: ast (types, generics, chains, slices, capsules, "
           "precedence-aware unparse; Value=%zu Expr=%zu Stmt=%zu bytes)\n",
           sizeof(Value), sizeof(Expr), sizeof(Stmt));
    return 0;
}
