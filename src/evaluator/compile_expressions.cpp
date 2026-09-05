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

#include "satellite_cache/paths.hpp"
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
        // `satellite.bool.true` is DESIGN §6.1's example, and M11 is what
        // turned this arm from a refusal naming itself into a dispatch: a
        // module constant is a path that evaluates without a call, so it costs
        // a handlers[path_id] entry and nothing else. A constant nobody has
        // built yet -- `satellite.console.width` until M14 -- answers S0721 at
        // RUN time, for op_refuse's reason exactly: a read in a branch that
        // never runs is a program that runs.
        //
        // THE SPELLING IS THE CANONICAL ONE, path_text and not this file's
        // text, because the sentence S0721 builds quotes the path as the
        // LANGUAGE writes it -- the file's own spelling is under the caret
        // already. BARE, with no backticks of its own: the sentences that
        // quote a callee carry their own (S0721's row is "`{1}` is a path
        // ..."), so baking a pair in here rendered every unbuilt module
        // constant as ``satellite.time.new`` -- found at M13 by asking after
        // the one reserved row this milestone left pointing at M29.
        finish(emit(op_dispatch, node, about.path, kNoOpList, out_.add_cache(),
                    out_.add_text(std::string(words::path_text(
                        static_cast<words::NodeId>(about.path))))));
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
            // THE RECEIVER COMPILES FIRST AND BECOMES ARGUMENT 0 -- M11.
            // Pushed after the written arguments, so the task stack hands it
            // to call() at the bottom of take_many's answer, which is where
            // DESIGN §6.4's written-out form puts it.
            {
                const NodeIndex receiver = method_receiver(node);
                if (receiver != kNoNode)
                    visit(receiver);
            }
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

NodeIndex Compiler::method_receiver(NodeIndex call_node) const
{
    const NodeIndex target = ast_[call_node].a;
    if (target == kNoNode || ast_[target].kind != NodeKind::Member)
        return kNoNode;

    // THE SELECTOR RESOLVED TO A LANGUAGE PATH, or this is no method. names.cpp
    // folds `s.upper` to `1 6 1 9` through the receiver's DECLARED type and
    // through nothing else, so a selector with a path and a receiver that is
    // not a language word is exactly the folded case and nothing but it.
    const resolve::Info &selector = info(target);
    if (selector.path == words::kNoPath || !words::is_language_word(selector.path))
        return kNoNode;

    // AND THE RECEIVER NAMES A STORAGE LOCATION, which is the positive test
    // and not a negative one. The first cut of this function asked "is the
    // receiver NOT a language word" -- and an intermediate segment like the
    // `satellite.console` under `display` carries no path of its own, so a
    // plain module call read as a method call and the module road was never
    // taken. What makes a method call is a receiver that IS somewhere: a
    // frame slot, or a global this program declared. Everything else is the
    // module road's.
    const NodeIndex receiver = ast_[target].a;
    if (receiver == kNoNode)
        return kNoNode;
    const resolve::Info &holder = info(receiver);
    if (resolve::in_a_frame(holder.slot))
        return receiver;
    if (holder.path != words::kNoPath && !words::is_language_word(holder.path) &&
        globals_.find(holder.path) != globals_.end())
        return receiver;
    return kNoNode;
}

OpIndex Compiler::call(NodeIndex node)
{
    const Node &n = ast_[node];
    const uint32_t count = ast_.list_size(n.b);

    // A METHOD ON A VALUE -- DESIGN §6.4's sugar, compiled down. The receiver
    // was visited by the Call case and sits under the written arguments, so
    // taking one extra result hands back the argument list with the receiver
    // at 0. WHICH SLOT IT WRITES BACK TO IS DECIDED HERE, not at run time: a
    // folded receiver is a declared name, a declared name is a frame slot or a
    // global, and the op carries the answer so a mutating row costs the walk
    // nothing it can feel. The last arm is unreachable while names.cpp folds
    // only declared names, and it is a refusal rather than an assumption for
    // when that boundary moves.
    if (const NodeIndex receiver = method_receiver(node); receiver != kNoNode) {
        const OpListId with_receiver = out_.add_list(take_many(count + 1));
        const resolve::Info &about = info(n.a);
        const resolve::Info &holder = info(receiver);
        if (resolve::in_a_frame(holder.slot))
            return emit(op_method, node, about.path, with_receiver,
                        out_.add_cache(), static_cast<uint32_t>(holder.slot));
        if (const auto found = globals_.find(holder.path); found != globals_.end())
            return emit(op_method_global, node, about.path, with_receiver,
                        out_.add_cache(), found->second);
        return not_built(node, "a method on this receiver",
                         "resolve folded a selector through a receiver that "
                         "names no slot, which names.cpp's one-hop boundary "
                         "is supposed to make impossible");
    }

    const OpListId arguments = out_.add_list(take_many(count));

    const NodeIndex target = n.a;
    const resolve::Info &about = info(target);

    // A WHOLE CALL THAT IS A ROW OF THE NUMBERING -- resolve's question ONE,
    // answered onto the CALL node and not its target, because the parentheses
    // are part of what the number says: `satellite.random.fast(2)` is `1 7 4`
    // and `fast` alone under `random` is no word at all. This road was
    // unreachable until M13 -- M10's `display` and M11's methods all carry
    // their number on the word, so the first shape-numbered call in the
    // language (names.cpp's `input()` example was still M14's future) is what
    // found the compiler reading `info(target)` alone and refusing its own
    // resolved program as "a method on this expression". An absorbed argument
    // is part of the number, not an expression -- resolve never visited it --
    // so an absorber row is left to the statement arms that own those forms.
    if (const resolve::Info &self = info(node);
        self.path != words::kNoPath && words::is_language_word(self.path) &&
        !cache::is_absorber(
            words::arguments_of(static_cast<words::NodeId>(self.path)))) {
        // A ROW THAT DECLARES A PLACE -- words.def's third list, one row long
        // by policy: `input(prompt, target)` `1 5 4`. The place compiles as a
        // SLOT and never as an expression, the misuses are caught HERE --
        // before any prompt could print, which is done-when clause 5 -- and
        // v1's flatten-and-strcmp route to the same number (expr_call.cpp:113,
        // PLAN §7's first bullet) has no descendant anywhere in this arm.
        if (const uint32_t place = words::place_parameter_of(
                static_cast<words::NodeId>(self.path));
            place != words::kNoPlaceParameter) {
            const NodeIndex where = ast_.list_at(n.b, place);
            const resolve::Info &held = info(where);
            const bool local = resolve::in_a_frame(held.slot);
            const auto global = globals_.find(held.path);

            // "Writes a place and yields nothing" -- anywhere its answer
            // could be read is refused, and "a non-variable second argument
            // fails before the prompt prints". Both misuses still compile to
            // an op (op_refuse's argument: a mistake in a branch that never
            // runs is a program that runs), and the compiled argument ops are
            // simply left unreferenced in the arena.
            if (node != statement_root_)
                return emit(op_misuse, node,
                            static_cast<uint32_t>(
                                errors::Code::CONSOLE_ANSWER_IS_THE_PLACE),
                            out_.add_text(std::string(ast_.text_of(target))));
            if (!local && global == globals_.end())
                return emit(op_misuse, node,
                            static_cast<uint32_t>(
                                errors::Code::CONSOLE_TARGET_NOT_A_PLACE),
                            out_.add_text(std::string(ast_.text_of(target))));

            // The handler sees every argument BUT the place -- its arity is
            // the written count less one -- and the op spends its fourth
            // operand on the slot, exactly as the method ops do.
            std::vector<OpIndex> kept;
            for (uint32_t i = 0; i < count; i++)
                if (i != place)
                    kept.push_back(out_.list_at(arguments, i));
            const OpListId given = out_.add_list(kept);
            return local ? emit(op_place, node, self.path, given,
                                out_.add_cache(),
                                static_cast<uint32_t>(held.slot))
                         : emit(op_place_global, node, self.path, given,
                                out_.add_cache(), global->second);
        }
        return emit(op_dispatch, node, self.path, arguments, out_.add_cache(),
                    out_.add_text(std::string(ast_.text_of(target))));
    }

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

    // A SELECTOR THAT NEVER FOLDED is two sentences, told apart by whether
    // the receiver IS a declared name. When it is one and carries a type, the
    // fold failed because the TYPE HAS NO SUCH WORD -- `box.upper()` on a
    // `variant`, `s.holding()` on a string -- and until M12 that case fell
    // into the other sentence's "name the receiver first", advice that cannot
    // help a receiver that already is a name. S0723's errors.def note is the
    // account; the advice text is chosen here because this is the one place
    // that knows the type, and the variant gets its own because M12 made a
    // wrong selector on one an easy mistake to make.
    if (ast_[target].kind == NodeKind::Member) {
        const NodeIndex who = ast_[target].a;
        if (who != kNoNode) {
            const resolve::Info &holder = info(who);
            const bool named = resolve::in_a_frame(holder.slot) ||
                               globals_.find(holder.path) != globals_.end();
            if (named && holder.type != words::kNoPath) {
                const bool variant =
                    holder.type ==
                    static_cast<words::PathId>(words::NodeId::VARIABLE_VARIANT);
                return emit(
                    op_no_question, node,
                    out_.add_text(std::string(ast_.text_of(target))),
                    out_.add_text(words::path_text(
                        static_cast<words::NodeId>(holder.type))),
                    out_.add_text(
                        variant ? "a variant answers `holding`, `holds(x)`, "
                                  "`held` and `clear`; to use what it holds, "
                                  "copy it to a typed name, or take it with "
                                  "`held()`"
                                : "its methods are the words numbered under "
                                  "that path, and this is not one of them"));
            }
        }
        // The person who wrote `f().trim()` did nothing wrong by the grammar
        // and needs the boundary named: names.cpp folds a method only through
        // a DECLARED name, WORD_NUMBERS §1.5's one hop, so a method on a
        // call's answer waits for the type rules that would see through it.
        // The fix a person can make today is a named variable in between.
        return not_built(node, "a method on this expression",
                         "a selector folds only through a declared name -- "
                         "WORD_NUMBERS.md §1.5's one hop -- so name the "
                         "receiver first");
    }

    return not_built(node, "this call", "resolve found nothing to call");
}

} // namespace eval
} // namespace satellite
