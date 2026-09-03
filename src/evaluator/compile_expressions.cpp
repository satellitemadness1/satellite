// Compiling DESIGN §6's expressions. See evaluator/evaluator_internal.hpp for
// the walk, and compile_statements.cpp for the other half.
//
// EVERY CASE ANSWERS EXACTLY ONE OP, and that op leaves exactly one value when
// it runs -- which is the two halves of one contract, written down in two files
// so that neither can be changed alone.
//
// A CASE NAMES ITS CHILDREN IN SOURCE ORDER, which is MILESTONES/M8.5.md §3.3's
// rule for the printers arriving here for the same reason. A stack pops
// backwards, so the naive translation of `left op right` visits the right side
// first and reads wrong; visit_reversed() and take_many() are the two places
// the inversion lives, and every case above them reads forwards.
//
// AND WHAT IS NOT BUILT REFUSES IN WORDS WITH A MILESTONE NUMBER IN IT. DESIGN
// §6's grammar is wider than this evaluator and will be until M28 -- a list
// literal, a spacesuit, an include and a subscript all parse today. errors.def's
// S0720 block note is the argument for saying so with a code and a caret rather
// than with a crash or a wrong answer.

#include "evaluator/evaluator_internal.hpp"

#include "satellite_number/bignum.hpp"

#include <string>
#include <utility>

namespace satellite {
namespace eval {

bool Compiler::step_expression(NodeIndex node, uint32_t step_number)
{
    const Node &n = ast_[node];

    switch (n.kind) {
    case NodeKind::Number: {
        // THE LITERAL IS PARSED ONCE, AT COMPILE TIME, which is PLAN §2.3's
        // "every decision that CAN be made before execution IS" at its
        // smallest. A loop counting to a million reads a constant a million
        // times and parses its digits none.
        Number value;
        if (!Number::parse(std::string(ast_.text_of(node)), value)) {
            problems_.push_back(errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
                span_of(node), std::string(ast_.text_of(node))));
            finish(kNoOp);
            return true;
        }
        finish(emit(op_constant, node, out_.add_constant(Value::number(std::move(value)))));
        return true;
    }

    case NodeKind::String:
        // `str` AND NOT `text`, and lexer.hpp is emphatic about the difference:
        // "`str` is the value the program means; `text` is what the file says."
        // The escapes are already expanded and DESIGN §5's live codes are still
        // in there as codes -- satellite_value/render.cpp is what answers them,
        // when the string is USED and not when it was read.
        finish(emit(op_constant, node,
                    out_.add_constant(Value::string(ast_.token_of(node).str))));
        return true;

    case NodeKind::Bits:
        // A GAP IN PLAN §8 RATHER THAN IN THIS MILESTONE, and saying so is the
        // work. M3 lexes `x00FF` and `b1010`, DESIGN §8.5 specifies them as
        // real types where "the width is part of the value", and PLAN §8 gives
        // `satellite.variable.binary` `1 6 5` and `.hex` `1 6 11` to the
        // LEXER's milestone and to no evaluator's. M11 is scalars and names
        // bool, number and string; M16 is containers. Neither claims these two.
        finish(not_built(node, "a binary or hexadecimal literal",
                         "no milestone in PLAN.md §8 owns the value -- M3 lexes "
                         "it and DESIGN §8.5 specifies it"));
        return true;

    case NodeKind::Satellite:
        // THE RUNTIME SINGLETON, AND IT IS A CONSTANT LIKE ANY OTHER -- M10.
        // DESIGN §8's table gives `satellite` a row and §3 says what it means:
        // "the singleton runtime object, not a zero sentinel ... include the
        // runtime, return the runtime (that is, success)."
        //
        // WHICH IS WHY `satellite.return(satellite)` NEEDS NO CASE OF ITS OWN.
        // The three return shapes are resolve's numbering -- `return()` `1 15 0`,
        // `return(satellite)` `1 15 1`, `return(value)` `1 15 2` -- and what
        // separates them is what the argument IS, which is decided here. A
        // compiler that special-cased the middle shape would be deciding a
        // path's meaning in two places, and WORD_NUMBERS §1.2 is what that
        // costs.
        finish(emit(op_constant, node, out_.add_constant(Value::runtime())));
        return true;

    case NodeKind::Name: {
        const resolve::Info &about = info(node);
        if (resolve::in_a_frame(about.slot)) {
            finish(emit(op_local, node, static_cast<uint32_t>(about.slot)));
            return true;
        }
        if (about.slot == resolve::kSlotCapsule) {
            // DESIGN §12 DEFERS "a bare name can be a value", and QUAD.md §3
            // closed the one thing that wanted it -- sorting needs a primitive
            // rather than a comparator. So a capsule's name outside a call is
            // not an oversight here, it is a language that does not have it.
            finish(not_built(node, "a capsule's name used as a value",
                             "DESIGN.md §12 defers it, and nothing in the "
                             "language needs it yet"));
            return true;
        }
        finish(not_built(node, "this name", "resolve gave it no slot"));
        return true;
    }

    case NodeKind::Member: {
        const resolve::Info &about = info(node);
        if (about.arguments) {
            finish(not_built(node, "`arguments`",
                             "PLAN.md §8 builds it at M20, and DESIGN §7.7 "
                             "specifies it"));
            return true;
        }
        const auto found = globals_.find(about.path);
        if (found != globals_.end()) {
            finish(emit(op_global, node, found->second));
            return true;
        }
        // A USER'S NAME UNDER A LANGUAGE NODE THAT IS NOT A GLOBAL is a
        // capsule read as a value, which DESIGN §12 defers. It gets its own
        // sentence rather than the module-constant one below, because sending
        // somebody to M11 for a thing M11 will not build is worse than saying
        // nothing.
        if (about.path != words::kNoPath && !words::is_language_word(about.path)) {
            finish(not_built(node, "a capsule's name used as a value",
                             "DESIGN.md §12 defers it, and nothing in the "
                             "language needs it yet"));
            return true;
        }

        // A LANGUAGE PATH READ WITHOUT BEING CALLED is a module constant --
        // `satellite.bool.true` is DESIGN §6.1's example and it is M11's.
        finish(emit(op_refuse, node,
                    out_.add_text("`" + std::string(ast_.text_of(node)) + "`"),
                    out_.add_text("PLAN.md §8 builds the module constants at M11")));
        return true;
    }

    case NodeKind::Unary:
        if (step_number == 0) {
            again(1);
            visit(n.a);
            return true;
        }
        finish(emit(op_unary, node, take(),
                    static_cast<uint32_t>(unary_op_of(ast_.text_of(node)))));
        return true;

    case NodeKind::Binary:
        if (step_number == 0) {
            again(1);
            // LEFT FIRST, which means it is pushed LAST. The one inversion,
            // and it is written here rather than felt everywhere.
            visit(n.b);
            visit(n.a);
            return true;
        }
        {
            const OpIndex right = take();
            const OpIndex left = take();
            finish(emit(op_binary, node, left, right,
                        static_cast<uint32_t>(binary_op_of(ast_.text_of(node)))));
        }
        return true;

    case NodeKind::Call:
        if (step_number == 0) {
            again(1);
            visit_reversed(n.b);
            return true;
        }
        finish(call(node));
        return true;

    case NodeKind::Index:
    case NodeKind::Slice:
        finish(not_built(node, "a subscript",
                         "PLAN.md §8 builds the containers at M16"));
        return true;

    default:
        return false;
    }
}

OpIndex Compiler::call(NodeIndex node)
{
    const Node &n = ast_[node];
    const uint32_t count = ast_.list_size(n.b);
    const OpListId arguments = out_.add_list(take_many(count));

    const NodeIndex target = n.a;
    const resolve::Info &about = info(target);

    // A CAPSULE THIS PROGRAM DECLARED. The index was decided in pass 1, before
    // any body was compiled, which is what makes a call to a capsule further
    // down the file -- and mutual recursion -- work at all.
    if (about.slot == resolve::kSlotCapsule) {
        const auto found = capsules_.find(about.path);
        if (found != capsules_.end())
            return emit(op_call, node, found->second, arguments,
                        out_.add_text(std::string(ast_.text_of(target))));
    }

    // A LANGUAGE PATH -- `handlers[path_id]`, PLAN §1.1's whole argument. The
    // cache cell is allocated here and not on first execution, so the op is
    // immutable and the mutable half is a side table: DESIGN §10.5's threads
    // walk one arena and hold one cache each.
    if (about.path != words::kNoPath && words::is_language_word(about.path))
        return emit(op_dispatch, node, about.path, arguments, out_.add_cache(),
                    out_.add_text(std::string(ast_.text_of(target))));

    return not_built(node, "this call", "resolve found nothing to call");
}

} // namespace eval
} // namespace satellite
