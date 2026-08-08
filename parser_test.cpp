// Parser tests. The main check is the round trip: for canonically formatted
// source, unparse(parse(src)) must give back exactly src. That single property
// exercises the whole front end at once, because a parse that drops, reorders
// or misgroups anything shows up as a text difference.

#include "ast.hpp"
#include "parser.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// The core property. `src` must already be in canonical form.
static void round_trip(const std::string &src)
{
    ParseResult r = parse(src);
    if (!r.ok()) {
        printf("FAIL: did not parse: %s\n", src.c_str());
        for (const ParseError &e : r.errors)
            printf("%s\n", format_error(e, src).c_str());
        failures++;
        return;
    }
    const std::string got = unparse(r.program);
    if (got != src) {
        printf("FAIL: round trip\n  want: %s\n   got: %s\n", src.c_str(),
               got.c_str());
        failures++;
    }
}

// Parse a lone expression by wrapping it in a statement position.
static ExprPtr expr_of(const std::string &src)
{
    ParseResult r = parse(src);
    if (!r.ok() || r.program.items.size() != 1)
        return nullptr;
    const StmtPtr *s = std::get_if<StmtPtr>(&r.program.items[0]);
    if (!s)
        return nullptr;
    const ExprStmt *e = std::get_if<ExprStmt>(s->get());
    return e ? e->expr : nullptr;
}

static void check_expr(const std::string &src, const std::string &want)
{
    ExprPtr e = expr_of(src);
    if (!e) {
        printf("FAIL: did not parse as an expression: %s\n", src.c_str());
        failures++;
        return;
    }
    if (unparse(*e) != want) {
        printf("FAIL: %s\n  want: %s\n   got: %s\n", src.c_str(), want.c_str(),
               unparse(*e).c_str());
        failures++;
    }
}

static void check_fails(const std::string &src, const std::string &what)
{
    ParseResult r = parse(src);
    if (r.ok()) {
        printf("FAIL: expected a parse error (%s): %s\n", what.c_str(),
               src.c_str());
        failures++;
    }
}

int main()
{
    // ---- hello world ------------------------------------------------------
    const std::string hello =
        "satellite.include(satellite)\n"
        "\n"
        "satellite.capsule satellite.main("
        "satellite.container.list<satellite.variable.string> argz)\n"
        "{\n"
        "    satellite.console.display(\"hello, world!\")\n"
        "    satellite.return(satellite)\n"
        "}\n";
    round_trip(hello);

    // Comments and blank lines are not in the tree, so they are the one thing
    // the round trip cannot reproduce — but they must not change the parse.
    {
        const std::string commented =
            "satellite.include(satellite) // all programs include satellite\n"
            "\n"
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.string> argz)\n"
            "{\n"
            "    // \"capsules\" are just functions in satellite\n"
            "    satellite.console.display(\"hello, world!\")\n"
            "    satellite.return(satellite) // return(0); in satellite\n"
            "}\n";
        ParseResult r = parse(commented);
        check(r.ok(), "the commented hello world parses");
        check(unparse(r.program) == hello,
              "comments do not change the parse");
    }

    // ---- declarations -----------------------------------------------------
    round_trip("satellite.variable.time my_time = satellite.time.now()\n");
    round_trip("satellite.variable.file my_file = satellite.file.new()\n");
    round_trip("satellite.variable.file my_file\n");
    round_trip("satellite.variable.number x = 1\n");
    round_trip("satellite.variable.string s = \"hi\"\n");
    round_trip("satellite.container.list<satellite.variable.string> names\n");
    round_trip("satellite.container.list<satellite.container.list"
               "<satellite.variable.string>> grid\n");

    // The declaration is found by segment 1, not by shape. Dispatching on
    // shape would read this as declaring a variable named `if`.
    {
        ParseResult r = parse(
            "satellite.statement.if(x)\n{\n    satellite.return(1)\n}\n");
        check(r.ok(), "an if statement parses");
        check(r.program.items.size() == 1, "an if is one item");
        const StmtPtr *s = std::get_if<StmtPtr>(&r.program.items[0]);
        check(s && std::holds_alternative<If>(**s),
              "satellite.statement.if is an If, not a declaration named 'if'");
    }

    // ---- method calls and chaining ---------------------------------------
    round_trip("my_time.some_function()\n");
    round_trip("my_file.some_function()\n");
    round_trip("satellite.time.now().some_function()\n");
    round_trip("my_list.append(1)\n");
    round_trip("my_list.append(other.value(), 2)\n");
    round_trip("a.b().c().d\n");

    // A language call and a user method call parse to the same shape; nothing
    // in the tree records which is which.
    check_expr("satellite.time.now()", "satellite.time.now()");
    check_expr("my_time.some_function()", "my_time.some_function()");

    // ---- indexing and slicing --------------------------------------------
    round_trip("list_name[3]\n");
    round_trip("list_name[some_number]\n");
    round_trip("l[2:5]\n");
    round_trip("l[:5]\n");
    round_trip("l[2:]\n");
    round_trip("l[:]\n");
    round_trip("grid[0][1]\n");
    round_trip("l[0].f()[1:2]\n");
    round_trip("satellite.library.main.x = 5\n");

    // ---- operators and precedence ----------------------------------------
    check_expr("a + b * c", "a + b * c");
    check_expr("a * b + c", "a * b + c");
    check_expr("(a + b) * c", "(a + b) * c");
    check_expr("a + b + c", "a + b + c");
    check_expr("a + (b + c)", "a + (b + c)");
    check_expr("a == b + c", "a == b + c");
    check_expr("-x", "-x");
    check_expr("-x + y", "-x + y");
    check_expr("-(x + y)", "-(x + y)");
    check_expr("a <= b", "a <= b");
    check_expr("a >= b", "a >= b");
    check_expr("a != b", "a != b");
    // Redundant parentheses are not in the tree, so they are dropped.
    check_expr("((a))", "a");

    // '<' inside a type is a generic opener; '<' in an expression is
    // less-than. The reservation rule is what keeps these apart.
    check_expr("a < b", "a < b");
    round_trip("satellite.container.list<satellite.variable.number> ns\n");

    // ---- control flow -----------------------------------------------------
    round_trip("satellite.statement.if(n <= 1)\n"
               "{\n"
               "    satellite.return(1)\n"
               "}\n");
    round_trip("satellite.statement.if(n <= 1)\n"
               "{\n"
               "    satellite.return(1)\n"
               "}\n"
               "satellite.statement.else\n"
               "{\n"
               "    satellite.return(2)\n"
               "}\n");
    round_trip("satellite.statement.while(n > 0)\n"
               "{\n"
               "    n = n - 1\n"
               "}\n");
    round_trip("satellite.statement.for(satellite.variable.number i = 0; "
               "i < 10; i = i + 1)\n"
               "{\n"
               "    satellite.console.display(argz[i])\n"
               "}\n");
    round_trip("satellite.statement.for(; ; )\n"
               "{\n"
               "    satellite.return(satellite)\n"
               "}\n");

    // else-if chains nest rather than needing a separate form.
    {
        ParseResult r = parse("satellite.statement.if(a)\n{\n}\n"
                              "satellite.statement.else\n"
                              "satellite.statement.if(b)\n{\n}\n");
        check(r.ok(), "an else-if chain parses");
        const StmtPtr *s = std::get_if<StmtPtr>(&r.program.items[0]);
        const If *outer = s ? std::get_if<If>(s->get()) : nullptr;
        check(outer && outer->else_branch &&
                  std::holds_alternative<If>(*outer->else_branch),
              "the else branch of an else-if is another If");
    }

    // ---- capsules ---------------------------------------------------------
    round_trip("satellite.capsule fact(satellite.variable.number n) "
               "satellite.returns(satellite.variable.number)\n"
               "{\n"
               "    satellite.return(n)\n"
               "}\n");
    round_trip("satellite.capsule noop()\n{\n}\n");
    round_trip("satellite.capsule add(satellite.variable.number a, "
               "satellite.variable.number b)\n"
               "{\n"
               "    satellite.return(a + b)\n"
               "}\n");

    // A capsule the user writes is bare; only the entry point is prefixed.
    {
        ParseResult r = parse("satellite.capsule fact()\n{\n}\n");
        const Capsule *c = std::get_if<Capsule>(&r.program.items[0]);
        check(c && !c->reserved && c->name == "fact",
              "a user capsule is bare");

        ParseResult m = parse("satellite.capsule satellite.main()\n{\n}\n");
        const Capsule *mc = std::get_if<Capsule>(&m.program.items[0]);
        check(mc && mc->reserved && mc->name == "main",
              "satellite.main is the reserved entry point");
    }

    // ---- the same-line rule ----------------------------------------------
    // Without it, a line beginning '(' or '[' is silently absorbed by the line
    // above — the defect that forced JavaScript's semicolon-insertion rules.
    // The '(' case is the one observable today; '[' carries the same guard and
    // becomes observable when list literals exist.
    {
        ParseResult r = parse("display(x)\n(y)\n");
        check(r.ok(), "two statements on two lines parse");
        check(r.program.items.size() == 2,
              "a '(' on the next line does not call the line above");

        ParseResult joined = parse("display(x)(y)\n");
        check(joined.ok() && joined.program.items.size() == 1,
              "a '(' on the same line still calls");
    }

    // Statements are newline-separated, so a second statement crammed onto one
    // line is an error rather than a silent double parse.
    check_fails("display(x) display(y)\n", "two statements on one line");

    // ---- nesting ----------------------------------------------------------
    round_trip("satellite.capsule satellite.main("
               "satellite.container.list<satellite.variable.string> argz)\n"
               "{\n"
               "    satellite.statement.for(satellite.variable.number i = 0; "
               "i < 10; i = i + 1)\n"
               "    {\n"
               "        satellite.statement.if(i == 5)\n"
               "        {\n"
               "            satellite.console.display(argz[i])\n"
               "        }\n"
               "    }\n"
               "    satellite.return(satellite)\n"
               "}\n");

    // ---- multiple top-level items keep their order -----------------------
    round_trip("satellite.include(satellite)\n"
               "\n"
               "satellite.capsule a()\n{\n}\n"
               "\n"
               "satellite.capsule b()\n{\n}\n");

    // ---- errors do not throw and do point somewhere ----------------------
    check_fails("satellite.variable.time\n", "a type with no name");
    check_fails("satellite.capsule\n", "a capsule with no name");
    check_fails("satellite.capsule f(\n{\n}\n", "an unclosed parameter list");
    check_fails("satellite.return(\n", "an unclosed return");
    check_fails("satellite.statement.if(a)\n", "an if with no block");
    check_fails("satellite.container.list<satellite.variable.string ns\n",
                "an unclosed generic");
    check_fails("\"unterminated\n", "a lexer error surfaces as a parse error");
    check_fails("satellite.nonsense.thing x = 1\n", "a bad type namespace");
    check_fails("satellite.variable.number satellite = 1\n",
                "'satellite' cannot name a variable");
    check_fails("x = \n", "an assignment with no value");

    {
        // An error must carry a usable position and render with a caret.
        const std::string src = "satellite.variable.number x = 1\n"
                                "satellite.return(\n";
        ParseResult r = parse(src);
        check(!r.ok(), "the bad line is reported");
        if (!r.errors.empty()) {
            check(r.errors[0].span.line == 2, "the error points at line 2");
            const std::string rendered = format_error(r.errors[0], src);
            check(rendered.find("line 2") != std::string::npos,
                  "the rendered error names the line");
            check(rendered.find("^") != std::string::npos,
                  "the rendered error draws a caret");
            check(rendered.find("satellite.return(") != std::string::npos,
                  "the rendered error shows the source line");
        }
    }

    {
        // Recovery: a bad statement must not swallow the ones after it.
        ParseResult r = parse("satellite.capsule a()\n{\n}\n"
                              "satellite.variable.time\n"
                              "satellite.capsule b()\n{\n}\n");
        check(!r.ok(), "the malformed declaration is reported");
        size_t capsules = 0;
        for (const TopLevel &item : r.program.items)
            if (std::holds_alternative<Capsule>(item))
                capsules++;
        check(capsules == 2, "both good capsules survive the bad line between");
    }

    // ---- unparse output always reparses ----------------------------------
    // Idempotence holds even where exact round tripping cannot: reparsing the
    // canonical form must give the canonical form back.
    for (const std::string &src :
         {std::string("a + (b + c) * -d\n"), std::string("((x))\n"),
          std::string("l[0].f()[1:2]\n"), hello}) {
        ParseResult first = parse(src);
        check(first.ok(), "sample parses: " + src);
        const std::string once = unparse(first.program);
        ParseResult second = parse(once);
        check(second.ok(), "canonical form reparses: " + once);
        check(unparse(second.program) == once,
              "unparse is idempotent: " + once);
    }

    if (failures) {
        printf("%d parser check(s) failed\n", failures);
        return 1;
    }
    printf("PASS: parser (round trip, segment-1 dispatch, generics vs "
           "less-than, same-line postfix, error recovery)\n");
    return 0;
}
