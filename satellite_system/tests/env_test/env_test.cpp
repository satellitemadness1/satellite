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
//
// This file holds what every section needs and nothing that is a section: the
// failure counter, the check helpers, the walk that reads slot numbers back
// off the resolved tree, and a main() that calls the sections in the order
// they ran in when they were consecutive blocks of one 330-line function. The
// sections themselves are in env_test_<topic>.cpp beside this file, and are
// declared — along with everything below — in env_test.hpp.
//
// The helpers here were `static` while this was one translation unit. Losing
// that keyword is the whole of what the split did to them; not one line of a
// check's body or a message it prints was touched.

#include "env_test.hpp"

#include <cstdio>

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// --- walking the resolved tree ---------------------------------------------

void walk(const Stmt &stmt, Slots &out)
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

void walk(const Expr &expr, Slots &out)
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

Slots slots_of(const CapsuleInfo &info)
{
    Slots out;
    if (info.capsule && info.capsule->body)
        walk(*info.capsule->body, out);
    return out;
}

// First slot recorded for `name`, or -99 if the name never appeared.
int slot_for(const Slots &slots, const std::string &name)
{
    for (const auto &entry : slots)
        if (entry.first == name)
            return entry.second;
    return -99;
}

// --- error helpers ----------------------------------------------------------

bool has_error(const ResolveResult &result, const std::string &fragment)
{
    for (const ResolveError &error : result.errors)
        if (error.message.find(fragment) != std::string::npos)
            return true;
    return false;
}

void check_rejects(const std::string &source, const std::string &fragment,
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

void check_accepts(const std::string &source, const std::string &what)
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
    env_test_slot_allocation();
    env_test_forward_calls();
    env_test_lexical_closure();
    env_test_scoping();
    env_test_top_level();
    env_test_reserved_word();
    env_test_duplicates();
    env_test_entry_point();
    env_test_recursion_bound();

    if (failures)
        return 1;
    printf("PASS: env (slot allocation, forward calls, mutual recursion, "
           "lexical closure, block/for scoping, shadowing, static arity, "
           "reserved word, satellite.main)\n");
    return 0;
}
