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

#include "satellite_bits/bits.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

    case NodeKind::Bits: {
        // THE LITERAL IS BUILT ONCE, AT COMPILE TIME -- the Number arm's rule
        // directly above, for the Number arm's reason: a loop over a million
        // iterations reads the constant a million times and walks its digits
        // none.
        //
        // THE RADIX IS THE TOKEN'S AND NOT THIS FILE'S. lexer.cpp's
        // bits_radix() already decided `b` is 2 and `x` is 16 and already
        // proved every character is a digit of that radix, so what is left
        // here is which TYPE the literal makes -- and since 2026-09-09 both
        // have an arm to make, which is what closed M19.5.
        //
        // THE LEADING CHARACTER IS THE TYPE AND THE REST IS THE VALUE, in both
        // arms. The token cannot be shorter than two characters --
        // bits_radix() requires a radix letter and at least one digit -- so
        // `substr(1)` is never empty and every literal has at least one bit.
        const Token &token = ast_.token_of(node);
        const std::string_view written = ast_.text_of(node);
        const std::string_view digits = written.substr(1);

        // THE PARSE CANNOT FAIL AND IS CHECKED ANYWAY, both arms, for the
        // reason bits.cpp gives at each constructor: the lexer has already
        // proved every character is a digit of this radix, and a constructor
        // that trusts its caller is a crash waiting on the second caller.
        if (token.radix == 16) {
            bits::HexRun run;
            if (!bits::parse_hex(digits, run)) {
                problems_.push_back(
                    errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
                        span_of(node), std::string(written)));
                finish(kNoOp);
                return true;
            }
            finish(emit(op_constant, node,
                        out_.add_constant(Value::hex(std::move(run)))));
            return true;
        }

        bits::BitRun run;
        if (!bits::parse_binary(digits, run)) {
            problems_.push_back(errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
                span_of(node), std::string(written)));
            finish(kNoOp);
            return true;
        }
        finish(emit(op_constant, node,
                    out_.add_constant(Value::binary(std::move(run)))));
        return true;
    }

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
        // NO ARM FOR `arguments` HERE SINCE M20, AND THE DELETION IS THE
        // WHOLE CHANGE. This case used to refuse every member of the
        // arguments object with S0720 naming M20 as the milestone that would
        // build it. What M20 found is that the object needs no arm at all:
        // resolve has walked `argz.machine.threads` to `1 14 1 1 1 3` since
        // M7 -- names.cpp's arguments_member() -- and a language path read
        // without being called is already a module constant, which is the
        // dispatch two screens down. So the object arrives by having its rows
        // installed, in satellite_arguments/handlers.cpp, and this file
        // learns nothing about it.
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
            // AN UNEVALUATED ARGUMENT IS ONE THIS LINE DOES NOT VISIT.
            //
            // PLAN M18 CALLS THIS THE LOAD-BEARING LINE AND MUTATION SAYS IT
            // IS NOT. What makes the argument a PATH is call()'s topic arm,
            // which folds the path itself and never looks at the results
            // stack -- so a compiler that visited the argument anyway still
            // answers correctly: the argument's op is emitted, referenced by
            // nothing, and never runs, exactly as M14's misused place
            // arguments are. `visit_reversed` was put back in a copy of the
            // tree and every clause of help_test passed.
            //
            // SO WHAT THIS LINE BUYS IS HYGIENE, AND SAYING SO IS BETTER THAN
            // CLAIMING MORE. It keeps an op that can never run out of the
            // arena, and it keeps the results stack balanced -- the visited
            // argument's result is pushed and never taken, because the arm
            // below takes `count - 1`. That leftover sits BELOW the answer
            // and nothing reads it, which is why the mutation is invisible;
            // the guard that would see it is a floor check on `results_` in
            // compile_tree(), which is that file's to add. MILESTONES/M18.md
            // §3 records both.
            //
            // THE OTHER ARGUMENTS ARE STILL VISITED, and there are none today
            // -- `1 19 1` is the list's one row and its one argument is the
            // topic. The loop is written for the general case anyway because
            // the alternative is a line that is correct only while a count
            // stays at one, and words.def's own note about the place list is
            // that a policy is kept by saying it, not by being unable to
            // express anything else.
            if (const uint32_t skip = topic_parameter(node);
                skip == words::kNoTopicParameter) {
                visit_reversed(n.b);
            } else {
                for (uint32_t i = ast_.list_size(n.b); i > 0; i--)
                    if (i - 1 != skip)
                        visit(ast_.list_at(n.b, i - 1));
            }
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
        if (step_number == 0) {
            again(1);
            // TARGET FIRST, WHICH MEANS IT IS PUSHED LAST -- op_binary's
            // inversion, and this case reads forwards for the same reason:
            // `l[i]` is written target-then-subscript, so that is the order
            // its two diagnostics have to come out in.
            visit(n.b);
            visit(n.a);
            return true;
        }
        {
            const OpIndex subscript = take();
            const OpIndex target = take();
            finish(emit(op_index, node, target, subscript));
        }
        return true;

    case NodeKind::Slice:
        if (step_number == 0) {
            again(1);
            // Pushed backwards so they compile forwards -- target, low,
            // high. AN ABSENT BOUND IS NOT VISITED, so `l[:]` compiles two
            // fewer ops than `l[0:n]` rather than two constants somebody has
            // to recognise later as a default.
            if (n.c != kNoNode)
                visit(n.c);
            if (n.b != kNoNode)
                visit(n.b);
            visit(n.a);
            return true;
        }
        {
            const OpIndex high = n.c == kNoNode ? kNoOp : take();
            const OpIndex low = n.b == kNoNode ? kNoOp : take();
            const OpIndex target = take();
            // `d` IS HOW MANY BOUNDS ARE ON THE VALUE STACK, decided here
            // because the arm would otherwise count the same thing again --
            // and the two counts are exactly what has to agree for the arm to
            // find its target under them.
            const uint32_t bounds = (n.b != kNoNode ? 1u : 0u) +
                                    (n.c != kNoNode ? 1u : 0u);
            finish(emit(op_slice, node, target, low, high, bounds));
        }
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

    // A FACT UNDER `arguments` IS NOT A METHOD ON IT -- M20, and it is the one
    // case where a receiver in a frame slot is the WRONG reading. The object's
    // facts are facts about the process: `arguments.machine.threads` is the
    // machine's thread count, not something the object computes about itself,
    // and satellite_arguments/handlers.cpp installs every one of them with no
    // receiver and arity 0.
    //
    // WITHOUT THIS LINE THE TWO DEPTHS DISAGREED, which is how it was found.
    // `arguments.machine.threads()` compiled through the module road -- its
    // receiver `arguments.machine` is a member and names no slot -- and
    // answered 24, while `arguments.count()` one level up read as a method,
    // arrived with the object as argument 0, and was refused with S0722 "takes
    // 0 arguments and was given 1". One of those two had to be wrong and it
    // was not the one that worked.
    //
    // **THE TEN SELECTORS ARE THE OPPOSITE CASE AND THIS LINE MUST NOT CATCH
    // THEM.** `arguments.length()` and `arguments.get(k)` really are methods
    // on the object -- they fold through `satellite.container.arguments`
    // `1 4 3`, the receiver's TYPE, and they need it as argument 0. The
    // milestone that builds them has to leave `arguments` false on a selector
    // in names.cpp's arguments_member(), or they arrive here with no receiver
    // and answer about nothing.
    if (selector.arguments)
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

uint32_t Compiler::topic_parameter(NodeIndex call_node) const
{
    // ASKED OF THE CALL NODE AND NOT OF ITS TARGET, which is the same thing
    // `satl --resolve` settled for the numbering arm below: resolve's question
    // one walks the whole shape, parentheses included, onto the CALL node --
    // `satellite.help(satellite.console)` is `1 19 1` there -- and leaves the
    // target member with no path at all. Reading the target would find kNoPath
    // and this function would answer "no topic" for the one row that has one.
    const resolve::Info &self = info(call_node);
    if (self.path == words::kNoPath || !words::is_language_word(self.path))
        return words::kNoTopicParameter;
    return words::topic_parameter_of(static_cast<words::NodeId>(self.path));
}

OpIndex Compiler::topic(NodeIndex node, words::PathId path, NodeIndex written)
{
    // WHAT THE HANDLER IS HANDED IS THE CANONICAL PATH TEXT, folded to a
    // constant right here. Three reasons it is the path's TEXT and not its
    // number: the refusals help raises quote the path, so the text is needed
    // whatever else is; `satl --compile` prints a legible constant instead of
    // an integer nobody can read back; and a `Value` holding a PathId would be
    // a number that is not a number, which satellite_value/value.hpp's rule
    // about arms with no producer exists to keep out. Walking the text back to
    // a node costs help a dozen character compares once per ask, and help is
    // asked by a person.
    //
    // AND IT IS THE LANGUAGE'S SPELLING, not the file's. `satellite.help(s)`
    // on a string variable folds `satellite.variable.string` -- the node the
    // declared type ends at, which resolve already knows and nothing has to
    // run to find out. That is the whole of "help answers about a variable",
    // and it is why it still answers after the program that declared it has
    // finished: no value was ever consulted.
    const resolve::Info &about = info(written);

    words::PathId topic_path = words::kNoPath;
    if (ast_[written].kind == NodeKind::Satellite) {
        // `satellite.help(satellite)` -- THE ROOT, ASKED ABOUT BY NAME. The
        // bare word is its own node kind and carries no path: DESIGN §3 makes
        // it "the singleton runtime object, not a zero sentinel", so resolve
        // has nothing to walk and every other reader of this node treats it as
        // a VALUE. As a topic it is the one thing it cannot be as a value --
        // the node `1`, which is where the walk starts anyway. That is why the
        // three shapes are one walk: this line is what makes
        // `satellite.help(satellite)` and bare `satellite.help` the same
        // program, and help_test asserts they print the same bytes.
        topic_path = static_cast<words::PathId>(words::NodeId::SATELLITE);
    } else if (about.path != words::kNoPath && words::is_language_word(about.path)) {
        topic_path = about.path;
    } else if ((resolve::in_a_frame(about.slot) ||
                globals_.find(about.path) != globals_.end()) &&
               about.type != words::kNoPath) {
        topic_path = about.type;
    }

    if (topic_path == words::kNoPath &&
        globals_.find(about.path) != globals_.end()) {
        // A GLOBAL, WHICH IS A VARIABLE WITH NO DECLARED TYPE. It reaches here
        // and not the arm above because there is no typed form of a global in
        // this language -- `satellite.library.n = 0` is the whole of it and
        // S0204 refuses anything else -- so resolve has nothing to hand over
        // and help would have to run the program to answer. The sentence below
        // used to be S1103's, which told somebody that `n` was not the name of
        // a variable while they were looking at the line declaring it.
        return emit(op_misuse, node,
                    static_cast<uint32_t>(errors::Code::HELP_GLOBAL_HAS_NO_TYPE),
                    out_.add_text(std::string(ast_.text_of(written))));
    }

    if (topic_path == words::kNoPath) {
        // A BARE WORD NOBODY DECLARED gets the sentence the author settled on
        // 2026-09-07, and it covers the two things such a word can be in one
        // line: a variable this program forgot to declare, or a word of the
        // language somebody left the `satellite.` off the front of.
        //
        // BOTH REFUSALS ARE op_misuse AND THEREFORE RUN-TIME, which is M14's
        // place arguments and S0720's argument: a mistake in a branch that
        // never runs is a program that runs. Everything about the ask was
        // still DECIDED here, before anything ran.
        const std::string text = std::string(ast_.text_of(written));
        if (ast_[written].kind == NodeKind::Name)
            return emit(op_misuse, node,
                        static_cast<uint32_t>(errors::Code::HELP_NO_SUCH_NAME),
                        out_.add_text(text));
        return emit(op_misuse, node,
                    static_cast<uint32_t>(errors::Code::HELP_NOT_A_TOPIC),
                    out_.add_text("`" + text + "`"));
    }

    const OpIndex folded =
        emit(op_constant, written,
             out_.add_constant(Value::string(encode_raw(std::string(
                 words::path_text(static_cast<words::NodeId>(topic_path)))))));
    return emit(op_dispatch, node, path, out_.add_list({folded}),
                out_.add_cache(),
                out_.add_text(std::string(ast_.text_of(ast_[node].a))));
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
        const resolve::Info &about = info(n.a);
        std::vector<OpIndex> given = take_many(count + 1);
        // A FOLDED LITERAL IS PART OF THE NUMBER AND NOT AN ARGUMENT -- M16,
        // and resolve.hpp's `folded_option` says why the flag exists rather
        // than being derived. `my_list.sort("down")` IS `sort_down()`
        // `1 4 2 5`, whose whole argument list is the receiver, so the
        // compiled op must hand the handler one value and not two. The
        // string is dropped HERE and not at run time, which is what keeps
        // the fold a resolve-time decision the walk never pays for; its
        // compiled op is simply left unreferenced in the arena, exactly as
        // M14's misused place arguments are.
        //
        // WHICH ONE IS DROPPED IS THE FIRST WRITTEN ARGUMENT, at index 1 --
        // index 0 is the receiver, which DESIGN §6.4's written-out form puts
        // there and which no fold ever touches.
        if (about.folded_option && given.size() > 1)
            given.erase(given.begin() + 1);
        const OpListId with_receiver = out_.add_list(given);
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

    // ONLY WHAT WAS VISITED IS ON THE STACK. A topic argument was never
    // compiled, so taking `count` results here would take somebody else's --
    // the value under this call's own, which is a silent miscompile rather
    // than an error. The Call case above and this line are one decision read
    // from two ends, which is why they ask the same function.
    //
    // AND THE TEST IS THE SAME ONE THE ARM BELOW MAKES, index included. A row
    // that declared a topic at an index the call did not write would leave the
    // arm to fall through to an ordinary dispatch, and an `unevaluated` of 1
    // here would then take one result too few -- or underflow `count` at zero
    // and ask for four billion. Neither is reachable today, because resolve
    // gives a call `1 19 1` only when one argument was written; both are
    // ruled out by asking the identical question rather than a similar one.
    const uint32_t written_topic = topic_parameter(node);
    const uint32_t unevaluated =
        written_topic != words::kNoTopicParameter && written_topic < count ? 1u
                                                                          : 0u;
    const OpListId arguments = out_.add_list(take_many(count - unevaluated));

    const NodeIndex target = n.a;
    const resolve::Info &about = info(target);

    // A CAPSULE THIS PROGRAM DECLARED AT A PATH THE LANGUAGE ALSO NUMBERS, AND
    // IT HAS TO BE ASKED BEFORE THE TWO ARMS BELOW OR IT IS UNREACHABLE.
    // `satellite.main` is the only such name today: parser_declarations.cpp's
    // capsule_decl has two arms and the reserved one LOOKS THE NAME UP instead
    // of defining it, so the capsule's path is `1 3` -- a language word --
    // while every capsule of the user's own is interned under
    // `satellite.library` and gets an id past kNodeCount that
    // `is_language_word` answers false for. That difference is why the plain
    // capsule arm below, which fires on `kSlotCapsule`, catches every OTHER
    // capsule and never this one.
    //
    // WHAT IT LOOKED LIKE BEFORE: `satellite.main()` resolved its call node to
    // `1 3 0`, took the numbering arm, and dispatched a path with no handler --
    // S0721, "a path satellite has a number for and nothing behind yet". A
    // program calling a capsule IT HAD ITSELF DECLARED was told the language
    // had not built it, which is the wrong sentence about the wrong thing.
    //
    // AND IT IS ASKED ON THE CALL NODE'S PARENT, WHICH `satl --resolve` IS
    // WHAT SETTLED. `satellite.main()` does not resolve as a member with a
    // path and a call around it: resolve's question one walks the whole shape,
    // parentheses included, onto the CALL node as `1 3 0`, and the target
    // member is left with no path at all. So the capsule to look for is the
    // parent of that row -- `1 3` -- and reading `info(target).path` finds
    // `kNoPath` and matches nothing. Measured before this was written, on a
    // file whose main calls itself.
    //
    // The guard is `capsules_` and not a test for `NodeId::MAIN`, so the day
    // the reserved arm admits a second name this needs no edit.
    words::PathId declared = about.path;
    if (const resolve::Info &self = info(node);
        declared == words::kNoPath && self.path != words::kNoPath &&
        words::is_language_word(self.path))
        declared = static_cast<words::PathId>(
            words::parent_of(static_cast<words::NodeId>(self.path)));

    if (const auto mine = capsules_.find(declared);
        mine != capsules_.end() && words::is_language_word(declared))
        return emit(op_call, node, mine->second, arguments,
                    out_.add_text(std::string(ast_.text_of(target))));

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
        // A ROW THAT DECLARES AN UNEVALUATED TOPIC -- words.def's fifth list,
        // one row long, and it is asked BEFORE the place arm because the two
        // are the same shape pointed at opposite halves of the same problem
        // and a row can only be one of them. A place is an argument compiled
        // as a SLOT; a topic is an argument compiled as a NUMBER; and both
        // are refusals the compiler can make before anything runs.
        if (const uint32_t which = words::topic_parameter_of(
                static_cast<words::NodeId>(self.path));
            which != words::kNoTopicParameter && which < count)
            return topic(node, self.path, ast_.list_at(n.b, which));

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
                const std::string_view asked = ast_.text_of(target);
                const bool variant =
                    holder.type ==
                    static_cast<words::PathId>(words::NodeId::VARIABLE_VARIANT);
                // ONE ROW IS CHOSEN BY THE SELECTOR AND NOT ONLY BY THE TYPE,
                // and it is the only one: `as_number` on a hex run. Binary has
                // that row at `1 6 5 4` and hex deliberately does not, so the
                // person who reaches for it here has almost certainly read
                // binary's -- and the generic sentence, "its methods are the
                // words numbered under that path", answers a question they
                // did not ask. The author dropped the row on 2026-09-09 and
                // asked for this refusal in the same breath.
                //
                // THE REASON IN IT IS THE TRUE ONE. `as_number` reads the
                // characters as an ordinary decimal; binary can answer because
                // `0` and `1` ARE decimal digits and hex cannot because `A` to
                // `F` are not. It is NOT about leading zeros -- no conversion
                // to a number keeps those on either radix, `007` being `7` --
                // and a sentence saying otherwise would teach the wrong rule
                // in the one place a person is definitely reading.
                const bool hex_as_number =
                    holder.type ==
                        static_cast<words::PathId>(words::NodeId::VARIABLE_HEX) &&
                    asked == "as_number";
                const char *advice =
                    hex_as_number
                        ? "`as_number` reads the digits as an ordinary "
                          "decimal, and `A` to `F` are not decimal digits -- "
                          "ask `to_number()` for what they are worth, or name "
                          "a `to_binary()` of it and ask that"
                    : variant ? "a variant answers `holding`, `holds(x)`, "
                                "`held` and `clear`; to use what it holds, "
                                "copy it to a typed name, or take it with "
                                "`held()`"
                              : "its methods are the words numbered under "
                                "that path, and this is not one of them";
                return emit(
                    op_no_question, node,
                    out_.add_text(std::string(asked)),
                    out_.add_text(words::path_text(
                        static_cast<words::NodeId>(holder.type))),
                    out_.add_text(advice));
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
