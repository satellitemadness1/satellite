// Evaluator lifetime: construction, failure, the top-level walk.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

Evaluator::Evaluator(const ResolveResult &resolved, std::string ns, bool echo)
    : resolved_(resolved), ns_(std::move(ns)), echo_(echo),
      max_depth_(read_max_depth()), division_digits_(read_division_digits())
{
}

void Evaluator::fail(Span span, std::string message)
{
    errors_.push_back(EvalError{std::move(message), span});
}

void Evaluator::run(const Program &program)
{
    // No pre-pass over capsule names any more: resolve() already collected
    // every one of them, before any body was walked. That is what makes a call
    // to a capsule defined further down the file — and mutual recursion —
    // resolvable at all (§6).

    for (const TopLevel &item : program.items) {
        if (failed())
            return;

        // satellite.include is parsed and validated, then ignored: there is no
        // module system in v1 and saying so is better than implying one.
        if (std::holds_alternative<Include>(item))
            continue;
        if (std::holds_alternative<Capsule>(item))
            continue;
        // A spacesuit is a declaration, not a statement: resolve() already
        // built its layout and its method table, and nothing runs until an
        // instance is constructed.
        if (std::holds_alternative<Spacesuit>(item))
            continue;

        const StmtPtr &stmt = std::get<StmtPtr>(item);
        if (!stmt)
            continue;

        // A bare expression at the top level echoes its value, which is what
        // makes `x` print 1 in the REPL. A statement that produced nil stays
        // silent, so satellite.console.display does not print twice.
        if (echo_) {
            if (const ExprStmt *e = std::get_if<ExprStmt>(stmt.get())) {
                if (!e->expr)
                    continue;
                ValuePtr v = eval(*e->expr);
                if (failed())
                    return;
                if (v && !std::holds_alternative<std::monostate>(*v))
                    output_ += to_string(*v) + "\n";
                continue;
            }
        }

        if (exec(*stmt) == Flow::Return)
            return;
    }
}

void Evaluator::run_entry(const Program &program, const List &args)
{
    run(program);
    if (failed())
        return;
    // A satellite.return at the top level already stopped the program; running
    // main afterwards would ignore it.
    if (returned_)
        return;

    const CapsuleInfo *info = resolved_.find("satellite.main");
    const Capsule *entry = info ? info->capsule : nullptr;

    // A program that is only top-level statements is legal — the REPL produces
    // nothing else — so a missing satellite.main is not an error.
    if (!entry)
        return;

    if (entry->params.size() > 1) {
        fail(entry->span, "satellite.main takes at most one parameter, got " +
                          std::to_string(entry->params.size()));
        return;
    }

    std::vector<ValuePtr> argv;

    if (entry->params.size() == 1) {
        const Param &param = entry->params[0];

        // The signature is fixed by §2, so check the declared shape rather than
        // only matches(): an empty argument list matches list<anything>, and
        // accepting satellite.main(list<number> argz) because nothing was
        // passed today would break the first time something was.
        const Type &type = param.type;
        bool shaped = type.space == "container" && type.name == "list" &&
                      type.args.size() == 1 &&
                      type.args[0].space == "variable" &&
                      type.args[0].name == "string";
        if (!shaped) {
            fail(param.span,
                 "satellite.main's parameter must be "
                 "satellite.container.list<satellite.variable.string>, not " +
                 unparse(type));
            return;
        }

        argv.push_back(make_value(make_list(args)));
    }

    // From here it is an ordinary call: argz lands in slot 0 of main's frame
    // like any other parameter, which is what §11 said M3 would replace the
    // namespace binding with.
    ValuePtr result = call_capsule(*info, "satellite.main", argv, entry->span);
    if (failed())
        return;

    // The entry point's return value is the program's, and call_capsule has
    // already restored the (empty) top-level one.
    returned_ = result;
}

} // namespace satellite
