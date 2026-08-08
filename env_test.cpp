// Resolver tests — milestone M3, part one (DESIGN.md §6).
//
// The resolver is tested entirely on its own, before any frame code exists in
// the evaluator. That ordering is deliberate: once frames run, a resolver bug
// and an evaluator bug produce the same symptom — a wrong value out of a
// capsule — and telling them apart costs far more than testing this pass alone
// costs now.
//
// Nothing here evaluates anything. Every assertion is about slot numbers,
// capsule metadata, or an error reported BEFORE execution.

#include "env.hpp"
#include "parser.hpp"

#include <cstdio>
#include <string>
#include <utility>
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

// --- walking the resolved tree ---------------------------------------------

using Slots = std::vector<std::pair<std::string, int>>;

static void walk(const Expr &expr, Slots &out);

static void walk(const Stmt &stmt, Slots &out)
{
    if (const VarDecl *decl = std::get_if<VarDecl>(&stmt)) {
        if (decl->init)
            walk(*decl->init, out);
        out.emplace_back(decl->name, decl->slot);
        return;
    }
    if (const Assign *node = std::get_if<Assign>(&stmt)) {
        if (node->value)
            walk(*node->value, out);
        if (node->target)
            walk(*node->target, out);
        return;
    }
    if (const ExprStmt *node = std::get_if<ExprStmt>(&stmt)) {
        if (node->expr)
            walk(*node->expr, out);
        return;
    }
    if (const Return *node = std::get_if<Return>(&stmt)) {
        if (node->value)
            walk(*node->value, out);
        return;
    }
    if (const Block *node = std::get_if<Block>(&stmt)) {
        for (const StmtPtr &inner : node->statements)
            if (inner)
                walk(*inner, out);
        return;
    }
    if (const If *node = std::get_if<If>(&stmt)) {
        if (node->condition)
            walk(*node->condition, out);
        if (node->then_branch)
            walk(*node->then_branch, out);
        if (node->else_branch)
            walk(*node->else_branch, out);
        return;
    }
    if (const While *node = std::get_if<While>(&stmt)) {
        if (node->condition)
            walk(*node->condition, out);
        if (node->body)
            walk(*node->body, out);
        return;
    }
    if (const For *node = std::get_if<For>(&stmt)) {
        if (node->init)
            walk(*node->init, out);
        if (node->condition)
            walk(*node->condition, out);
        if (node->step)
            walk(*node->step, out);
        if (node->body)
            walk(*node->body, out);
        return;
    }
}

static void walk(const Expr &expr, Slots &out)
{
    if (const Name *name = std::get_if<Name>(&expr)) {
        out.emplace_back(name->text, name->slot);
        return;
    }
    if (const Call *node = std::get_if<Call>(&expr)) {
        if (node->target)
            walk(*node->target, out);
        for (const ExprPtr &arg : node->args)
            if (arg)
                walk(*arg, out);
        return;
    }
    if (const Member *node = std::get_if<Member>(&expr)) {
        if (node->target)
            walk(*node->target, out);
        return;
    }
    if (const Index *node = std::get_if<Index>(&expr)) {
        if (node->target)
            walk(*node->target, out);
        if (node->subscript)
            walk(*node->subscript, out);
        return;
    }
    if (const Slice *node = std::get_if<Slice>(&expr)) {
        if (node->target)
            walk(*node->target, out);
        if (node->lo)
            walk(*node->lo, out);
        if (node->hi)
            walk(*node->hi, out);
        return;
    }
    if (const Unary *node = std::get_if<Unary>(&expr)) {
        if (node->operand)
            walk(*node->operand, out);
        return;
    }
    if (const Binary *node = std::get_if<Binary>(&expr)) {
        if (node->left)
            walk(*node->left, out);
        if (node->right)
            walk(*node->right, out);
        return;
    }
}

static Slots slots_of(const CapsuleInfo &info)
{
    Slots out;
    if (info.capsule && info.capsule->body)
        walk(*info.capsule->body, out);
    return out;
}

// First slot recorded for `name`, or -99 if the name never appeared.
static int slot_for(const Slots &slots, const std::string &name)
{
    for (const auto &entry : slots)
        if (entry.first == name)
            return entry.second;
    return -99;
}

// --- error helpers ----------------------------------------------------------

static bool has_error(const ResolveResult &result, const std::string &fragment)
{
    for (const ResolveError &error : result.errors)
        if (error.message.find(fragment) != std::string::npos)
            return true;
    return false;
}

static void check_rejects(const std::string &source, const std::string &fragment,
                          const std::string &what)
{
    ParseResult parsed = parse(source);
    if (!parsed.ok()) {
        printf("FAIL: %s (source did not even parse)\n", what.c_str());
        failures++;
        return;
    }
    ResolveResult result = resolve(parsed.program);
    if (!has_error(result, fragment)) {
        printf("FAIL: %s\n  want an error containing: %s\n  got: %s\n",
               what.c_str(), fragment.c_str(),
               result.errors.empty()
                   ? "no errors at all"
                   : result.errors.front().message.c_str());
        failures++;
    }
}

static void check_accepts(const std::string &source, const std::string &what)
{
    ParseResult parsed = parse(source);
    if (!parsed.ok()) {
        printf("FAIL: %s (source did not even parse)\n", what.c_str());
        failures++;
        return;
    }
    ResolveResult result = resolve(parsed.program);
    if (!result.ok()) {
        printf("FAIL: %s\n  unexpected error: %s\n", what.c_str(),
               result.errors.front().message.c_str());
        failures++;
    }
}

int main()
{
    // --- slot allocation ---------------------------------------------------
    {
        ParseResult parsed = parse(
            "satellite.capsule f(satellite.variable.number a, "
            "satellite.variable.string b)\n"
            "{\n"
            "    satellite.variable.number c = a\n"
            "    satellite.return(c)\n"
            "}\n");
        check(parsed.ok(), "slot program parses");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "slot program resolves clean");

        const CapsuleInfo *info = result.find("f");
        check(info != nullptr, "capsule f is in the table");
        if (info) {
            check(info->param_count == 2, "f has two parameters");
            check(info->slot_count == 3, "two parameters plus one local = 3 slots");
            check(info->slot_names == std::vector<std::string>({"a", "b", "c"}),
                  "slots are named in declaration order");
            // Declared types are static, and parallel to the slots.
            check(info->slot_types.size() == 3, "one declared type per slot");
            check(info->slot_types[1].name == "string",
                  "slot 1 keeps parameter b's declared type");

            Slots slots = slots_of(*info);
            check(slot_for(slots, "a") == 0, "parameter a is slot 0");
            check(slot_for(slots, "c") == 2, "local c is slot 2");
        }
    }

    // Locals declared in the body follow the parameters, never overlap them.
    {
        ParseResult parsed = parse(
            "satellite.capsule f(satellite.variable.number p)\n"
            "{\n"
            "    satellite.variable.number x = 1\n"
            "    satellite.variable.number y = 2\n"
            "    satellite.return(p)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        const CapsuleInfo *info = result.find("f");
        check(info && info->slot_count == 3, "one parameter and two locals");
        if (info) {
            Slots slots = slots_of(*info);
            check(slot_for(slots, "x") == 1 && slot_for(slots, "y") == 2,
                  "locals take slots after the parameters");
        }
    }

    // --- what only a separate pass can do ----------------------------------
    // A forward call and mutual recursion are the whole reason this is not in
    // the parser: neither is resolvable in single-pass recursive descent.
    check_accepts("satellite.capsule first(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(second(n))\n"
                  "}\n"
                  "satellite.capsule second(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(n)\n"
                  "}\n",
                  "a capsule may call one defined further down");

    check_accepts("satellite.capsule is_even(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(is_odd(n))\n"
                  "}\n"
                  "satellite.capsule is_odd(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(is_even(n))\n"
                  "}\n",
                  "mutual recursion resolves");

    check_accepts("satellite.capsule fact(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(fact(n - 1))\n"
                  "}\n",
                  "self recursion resolves");

    // A capsule name in call position is marked as such, not read as a
    // variable.
    {
        ParseResult parsed = parse(
            "satellite.capsule helper()\n"
            "{\n"
            "    satellite.return(satellite)\n"
            "}\n"
            "satellite.capsule caller()\n"
            "{\n"
            "    satellite.return(helper())\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "call program resolves clean");
        const CapsuleInfo *info = result.find("caller");
        if (info)
            check(slot_for(slots_of(*info), "helper") == SLOT_CAPSULE,
                  "a called capsule is SLOT_CAPSULE, not a variable");
    }

    // Arity is knowable before anything runs.
    check_rejects("satellite.capsule f(satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(n)\n"
                  "}\n"
                  "satellite.capsule g()\n"
                  "{\n"
                  "    satellite.return(f(1, 2))\n"
                  "}\n",
                  "takes 1 argument, got 2", "arity is checked statically");

    // --- capsules are lexically closed -------------------------------------
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(nope)\n"
                  "}\n",
                  "unknown variable in capsule: nope",
                  "an unknown name inside a capsule is a resolve-time error");

    // ...but a top-level name is not, because the REPL declares on one line
    // and reads on the next — two Programs one resolve() cannot see at once.
    check_accepts("nope\n", "an unknown name at the top level is left to run time");
    check_accepts("satellite.variable.number x = 1\nx = x + 1\n",
                  "top-level declaration and use");

    // A global is still reachable from a capsule, by the four-segment path.
    check_accepts("satellite.variable.number g = 1\n"
                  "satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(satellite.library.main.g)\n"
                  "}\n",
                  "a capsule reaches a global through satellite.library");

    // The initialiser is resolved before the name is declared.
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.variable.number x = x\n"
                  "    satellite.return(x)\n"
                  "}\n",
                  "unknown variable in capsule: x",
                  "a declaration cannot read the slot it is writing");

    // --- scoping -----------------------------------------------------------
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    {\n"
                  "        satellite.variable.number inner = 1\n"
                  "    }\n"
                  "    satellite.return(inner)\n"
                  "}\n",
                  "unknown variable in capsule: inner",
                  "a block-scoped local does not leak out of its block");

    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.statement.for (satellite.variable.number i = 0; "
                  "i < 3; i = i + 1) {\n"
                  "        satellite.console.display(i)\n"
                  "    }\n"
                  "    satellite.return(i)\n"
                  "}\n",
                  "unknown variable in capsule: i",
                  "a for-loop variable is scoped to the loop");

    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.variable.number x = 1\n"
                  "    satellite.variable.number x = 2\n"
                  "    satellite.return(x)\n"
                  "}\n",
                  "already declared in this scope", "no redeclaration in one scope");

    // A parameter shadows a top-level global of the same name: inside f, x is
    // slot 0, not satellite.library.main.x.
    {
        ParseResult parsed = parse(
            "satellite.variable.number x = 1\n"
            "satellite.capsule f(satellite.variable.number x)\n"
            "{\n"
            "    satellite.return(x)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "shadowing resolves clean");
        const CapsuleInfo *info = result.find("f");
        if (info)
            check(slot_for(slots_of(*info), "x") == 0,
                  "a parameter shadows a global of the same name");
    }

    // --- top level stays in satellite.library ------------------------------
    {
        ParseResult parsed = parse("satellite.variable.number top = 1\ntop\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "top-level program resolves clean");
        Slots slots;
        for (const TopLevel &item : parsed.program.items)
            if (const StmtPtr *stmt = std::get_if<StmtPtr>(&item))
                if (*stmt)
                    walk(**stmt, slots);
        check(slot_for(slots, "top") == SLOT_GLOBAL,
              "a top-level declaration stays a satellite.library global");
    }

    // --- the reserved word, at every binding site --------------------------
    check_rejects("satellite.capsule f(satellite.variable.number satellite)\n"
                  "{\n"
                  "    satellite.return(1)\n"
                  "}\n",
                  "cannot name a parameter",
                  "satellite is reserved as a parameter name too");

    // --- duplicates ---------------------------------------------------------
    check_rejects("satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(1)\n"
                  "}\n"
                  "satellite.capsule f()\n"
                  "{\n"
                  "    satellite.return(2)\n"
                  "}\n",
                  "already defined", "a duplicate capsule is rejected");

    check_rejects("satellite.capsule f(satellite.variable.number n, "
                  "satellite.variable.number n)\n"
                  "{\n"
                  "    satellite.return(n)\n"
                  "}\n",
                  "duplicate parameter n", "a duplicate parameter is rejected");

    // A failed parameter still occupies its slot, so later parameters keep
    // lining up with the arguments that will fill them.
    {
        ParseResult parsed = parse(
            "satellite.capsule f(satellite.variable.number n, "
            "satellite.variable.number n, satellite.variable.number z)\n"
            "{\n"
            "    satellite.return(z)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        const CapsuleInfo *info = result.find("f");
        check(info && info->param_count == 3, "arity counts every parameter");
        if (info)
            check(slot_for(slots_of(*info), "z") == 2,
                  "a duplicate parameter does not shift the ones after it");
    }

    // --- satellite.main -----------------------------------------------------
    {
        ParseResult parsed = parse(
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.string> argz)\n"
            "{\n"
            "    satellite.return(argz)\n"
            "}\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok(), "satellite.main resolves clean");
        const CapsuleInfo *info = result.find("satellite.main");
        check(info != nullptr, "the entry point is keyed satellite.main");
        check(result.find("main") == nullptr, "and not bare main");
        if (info) {
            check(info->param_count == 1 && info->slot_count == 1,
                  "satellite.main has one parameter and one slot");
            check(slot_for(slots_of(*info), "argz") == 0, "argz is slot 0");
        }
    }

    // A program with no capsules resolves to an empty table, not an error.
    {
        ParseResult parsed = parse("1 + 1\n");
        ResolveResult result = resolve(parsed.program);
        check(result.ok() && result.capsules.empty(),
              "a program with no capsules is legal");
    }

    // --- the resolver bounds its own recursion ------------------------------
    // Without this the resolver segfaults on a deep tree exactly the way the
    // evaluator does, and it would do so before any evaluation guard could
    // possibly help.
    {
        std::string deep = "satellite.capsule f()\n{\n    satellite.return(";
        deep += std::string(3000, '-');
        deep += "1)\n}\n";
        ParseResult parsed = parse(deep);
        if (parsed.ok()) {
            ResolveResult result = resolve(parsed.program);
            check(!result.ok() && has_error(result, "nests deeper"),
                  "a deeply nested expression is an error, not a crash");
        }
    }

    if (failures)
        return 1;
    printf("PASS: env (slot allocation, forward calls, mutual recursion, "
           "lexical closure, block/for scoping, shadowing, static arity, "
           "reserved word, satellite.main)\n");
    return 0;
}
