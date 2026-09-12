// Arena AST to closure tree -- the passes, and the walk they share. See
// evaluator/evaluator_internal.hpp for the shape and why it is the machine's.
//
// THREE PASSES AND THE ORDER IS RESOLVE'S. Every capsule gets a compiled entry
// and a slot count before any body is compiled, then the globals, then the
// bodies. DESIGN §7.3's argument is the reason and it is unchanged here: "a
// capsule may call one defined further down the file, and mutual recursion is
// unresolvable in single-pass recursive descent." A compiler that emitted a
// call as it met one would have nothing to point the call AT half the time.
//
// AND IT IS NOT FOUR PASSES, WHICH IS THE ONE PLACE THIS WALK IS SMALLER THAN
// resolve's. Pass 2 there is spacesuits and it is a named hole for M26; there is
// nothing for this pass to do with a hole, so a `satellite.spacesuit` compiles
// to one op that refuses in words with the milestone number in it.

#include "evaluator/evaluator_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"

#include <utility>

namespace satellite {
namespace eval {

OpIndex Compiled::add(OpFn fn, NodeIndex node, uint32_t a, uint32_t b, uint32_t c,
                      uint32_t d)
{
    // OP ZERO IS THE EMPTY STATEMENT AND IS SET HERE rather than in a
    // constructor, because op_no_op lives in operations.cpp and a header that
    // named it would put the machine's arms in every file that holds a program.
    if (ops_[kNoOp].fn == nullptr)
        ops_[kNoOp].fn = op_no_op;

    ops_.push_back({fn, a, b, c, d});
    nodes_.push_back(node);
    return static_cast<OpIndex>(ops_.size() - 1);
}

OpListId Compiled::add_list(const std::vector<OpIndex> &items)
{
    if (items.empty())
        return kNoOpList;
    const OpListId at = static_cast<OpListId>(lists_.size());
    lists_.push_back(static_cast<OpIndex>(items.size()));
    lists_.insert(lists_.end(), items.begin(), items.end());
    return at;
}

uint32_t Compiled::add_constant(Value value)
{
    constants_.push_back(std::move(value));
    return static_cast<uint32_t>(constants_.size() - 1);
}

uint32_t Compiled::add_text(std::string text)
{
    texts_.push_back(std::move(text));
    return static_cast<uint32_t>(texts_.size() - 1);
}

Compiler::Compiler(const Ast &ast, const resolve::Resolved &resolved, words::Words &words)
    : ast_(ast), resolved_(resolved), words_(words)
{
}

OpIndex Compiler::emit(OpFn fn, NodeIndex node, uint32_t a, uint32_t b, uint32_t c,
                       uint32_t d)
{
    return out_.add(fn, node, a, b, c, d);
}

errors::Span Compiler::span_of(NodeIndex node) const
{
    const Token &at = ast_.token_of(node);
    return errors::Span{at.start, at.end, at.line};
}

OpIndex Compiler::not_built(NodeIndex node, const std::string &what,
                            const char *milestone)
{
    return emit(op_refuse, node, out_.add_text(what), out_.add_text(milestone));
}

std::vector<OpIndex> Compiler::take_many(uint32_t count)
{
    std::vector<OpIndex> out(count);
    for (uint32_t i = count; i > 0; i--)
        out[i - 1] = take();
    return out;
}

void Compiler::visit_reversed(ListId list)
{
    for (uint32_t i = ast_.list_size(list); i > 0; i--)
        visit(ast_.list_at(list, i - 1));
}

OpIndex Compiler::compile_tree(NodeIndex root)
{
    // THE WHOLE WALK, AND IT IS THE MACHINE'S LOOP WITH THE NOUNS CHANGED.
    // A task is read rather than popped -- the case decides whether it stays --
    // which is what makes `step` a return address instead of a state machine.
    const size_t floor = tasks_.size();
    tasks_.push_back({root, 0});
    while (tasks_.size() > floor) {
        const Task task = tasks_.back();
        step(task.node, task.step);
    }
    return take();
}

void Compiler::step(NodeIndex node, uint32_t step_number)
{
    if (step_expression(node, step_number))
        return;
    if (step_statement(node, step_number))
        return;

    // A KIND NEITHER HALF CLAIMED. Every one of them is a DECLARATION -- the
    // two passes below meet those, so reaching here means a declaration turned
    // up inside a body, which the parser does not produce. Refusing in words
    // rather than asserting is DESIGN §9.1's rule: satl does not throw, and a
    // sentence a user can read beats a crash even when nothing can send it.
    finish(not_built(node, std::string(kind_name(ast_[node].kind)) +
                               " inside a capsule body",
                     "no milestone -- the parser does not produce this"));
}

void Compiler::capsule(NodeIndex node)
{
    const words::PathId path = ast_[node].a;
    const auto found = capsules_.find(path);
    if (found == capsules_.end())
        return;

    Capsule &target = out_.capsules()[found->second];
    const NodeIndex body = ast_[node].d;
    target.body = body == kNoNode ? kNoOp : compile_tree(body);
    target.entry = emit(op_enter, node, found->second);
}

void Compiler::global(NodeIndex node)
{
    const words::PathId path = ast_[node].a;
    const uint32_t slot = globals_[path];
    const NodeIndex init = ast_[node].b;
    const OpIndex value = init == kNoNode ? kNoOp : compile_tree(init);
    results_.push_back(emit(op_store_global, node, slot, value));
}

Compiled Compiler::compile()
{
    const Node &program = ast_[ast_.root()];

    // PASS 1 -- every capsule's frame, from resolve's own answer.
    //
    // THE SLOT COUNT IS RESOLVE'S AND IS NOT RECOUNTED HERE, which is PLAN
    // §2.2's side table doing its job: resolve decided how big every frame is
    // and this pass copies the number. A compiler that counted declarations for
    // itself would be a second answer to "how many slots", and DESIGN §7.4's
    // rule that a slot is never reused across scopes is exactly the sort of
    // thing two counters disagree about.
    for (const resolve::Frame &frame : resolved_.frames) {
        capsules_[frame.capsule] = static_cast<uint32_t>(out_.capsules().size());
        out_.capsules().push_back({frame.capsule, kNoOp, kNoOp,
                                   static_cast<uint32_t>(frame.size()),
                                   frame.parameters, frame.node});
    }

    // PASS 1b -- EVERY SPACESUIT'S LAYOUT, and it is here for pass 1's reason
    // exactly: a field initialiser may construct another suit declared further
    // down the file, so every layout has to have an index before any of them is
    // compiled. That is DESIGN §7.3's forward-reference argument, applied to a
    // second kind of declaration.
    //
    // THE NAMES ARE COPIED AND THE `string_view`s ARE NOT KEPT. resolve's
    // `Field::name` points into the source text, which outlives the compile and
    // does NOT outlive the run in M22's prompt -- where each typed line has its
    // own text and the objects built by line 4 are still alive at line 5. One
    // copy per field per program, once.
    for (const resolve::Suit &suit : resolved_.suits) {
        suit::Layout layout;
        layout.name = std::string(suit.name);
        for (const resolve::Field &field : suit.fields) {
            layout.field_names.push_back(std::string(field.name));
            layout.field_is_public.push_back(field.is_public);
        }
        suits_[suit.path] = out_.add_suit(std::move(layout));
    }

    // PASS 2 -- every global's slot, before any initialiser is compiled, so
    // that one global's initialiser may read another declared below it.
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind == NodeKind::Global)
            globals_[ast_[item].a] = out_.add_global();
    }

    // AND EVERY CAPSULE'S DECLARED VARIABLE, WHICH IS A GLOBAL IN EVERY WAY
    // THAT MATTERS HERE -- M26's nested globals. resolve numbered
    // `satellite.library.memory_vars.target_gb` and this gives that number
    // somewhere to live; the arm that READS one already exists and was written
    // for M9's ordinary globals, because `globals_` is keyed by path and does
    // not care which pass put the row in.
    //
    // THE SLOT IS SEPARATE FROM THE CAPSULE'S FRAME SLOT AND HAS TO BE. The
    // same declaration is also a local -- DESIGN §7.1 is emphatic that a
    // capsule's variables are per-call, and M23's threads mean several calls
    // can hold different values at once -- so there is no frame to point at
    // from outside. What this slot holds is the DECLARATION: the initialiser,
    // run once at startup, which is the only value the name has that does not
    // depend on who is calling.
    for (const resolve::CapsuleConstant &each : resolved_.capsule_constants)
        globals_[each.path] = out_.add_global();

    // PASS 3 -- the top level, which at DESIGN §6's grammar is the globals'
    // initialisers and the includes. resolve.cpp's pass 3 says why there is
    // nothing else: "this grammar has no top-level statements at all."
    std::vector<OpIndex> top;
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        switch (ast_[item].kind) {
        case NodeKind::Global:
            global(item);
            top.push_back(take());
            break;
        case NodeKind::Include:
            // `satellite.include(satellite)` IS THE ONE INCLUDE FORM THAT DOES
            // NOTHING, AND DESIGN §3 SAYS SO IN THOSE WORDS: "that is not a
            // leftover -- it means 'include the runtime', which a running
            // program already has. Every other form names a spaceship and
            // loads it." So this compiles to no op at all, and the spaceship
            // form refuses naming M25.
            //
            // M9's FIRST VERSION REFUSED BOTH, AND EVERY PROGRAM IN example/
            // OPENS WITH THE FIRST ONE. `satl --call example/frames.satl
            // factorial 10` stopped on line 5 with a sentence about M25 --
            // which is the milestone boundary drawn in the wrong place, since
            // the thing being refused is specified as a no-op three documents
            // over. It is worth recording that the acceptance files caught it:
            // the fixtures in tests/eval_test are written and did not, because
            // a fixture opens with the capsule it is about.
            //
            // AND `satellite.include()` `1 1 0` DOES NOTHING FOR THE SAME
            // REASON AND NOT AS A COURTESY. WORD_NUMBERS §1.3: "0 means
            // nothing in that position." A program that asks for nothing to be
            // included has asked for exactly what it got, so there is no
            // spaceship here to refuse and nothing to load -- the two forms
            // this milestone owns both compile to no op, and the third refuses
            // naming M25. Testing against the two rather than against the one
            // is why this reads as a list: adding `1 1 2` to it later would be
            // the mistake, and it is the only row left.
            if (info(item).path !=
                    static_cast<words::PathId>(words::NodeId::INCLUDE_SATELLITE) &&
                info(item).path !=
                    static_cast<words::PathId>(words::NodeId::INCLUDE_0))
                top.push_back(not_built(
                    item, "`satellite.include` of a spaceship",
                    "PLAN.md §8 builds the spaceships at M25"));
            break;
        case NodeKind::Spacesuit:
            // A SPACESUIT DECLARATION RUNS NOTHING, WHICH IS WHY THIS CASE IS
            // EMPTY AND NOT ABSENT -- M26. What used to stand here was
            // `not_built(item, "a `satellite.spacesuit`", "M26")`: the whole
            // feature, refused by one op with a milestone number in it.
            //
            // The suit's LAYOUT was registered in pass 1 beside the frames, and
            // its methods are bodies compiled in pass 4 like any other. A
            // declaration itself is a statement about what a name MEANS, and
            // this grammar has nowhere for such a statement to execute --
            // DESIGN §6's top_level is include, capsule, spacesuit and global,
            // and three of those four already compile to nothing here.
            break;
        default:
            break;
        }
    }
    // AND THE CAPSULE CONSTANTS' INITIALISERS, IN THE SAME BLOCK AND AFTER THE
    // GLOBALS. They are ordinary expressions compiled against no frame, exactly
    // as a global's initialiser is, so `satellite.library.a.b = c + 1` works if
    // `c` is a global -- and a capsule's constant may read another capsule's,
    // because pass 2 above gave every one of them a slot before any of this ran.
    for (const resolve::CapsuleConstant &each : resolved_.capsule_constants) {
        const OpIndex value = each.initialiser == kNoNode
                                  ? kNoOp
                                  : compile_tree(each.initialiser);
        top.push_back(emit(op_store_global, each.declaration,
                           globals_[each.path], value));
    }

    out_.set_top(emit(op_block, ast_.root(), out_.add_list(top)));

    // PASS 4 -- every body, the suits' methods included.
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind == NodeKind::Capsule)
            capsule(item);
    }

    // A METHOD IS A CAPSULE AND IS COMPILED BY THE SAME FUNCTION -- M26. What
    // makes its body different is entirely resolve's doing: the receiver is at
    // slot 0 and a bare field name arrived carrying kSlotField, so the arms
    // that read those are in compile_expressions.cpp and there is nothing for
    // this loop to do but find them.
    //
    // `inside_` IS SET SO THE ARMS KNOW WHICH LAYOUT, and it is the compiler's
    // half of resolve's own member of the same name. Both exist because a
    // field index means nothing without the suit it indexes.
    //
    // AND IT STARTS AT `inherited_methods`, WHICH IS M26's INHERITANCE PAYING
    // FOR ITSELF EXACTLY ONCE. A child's method table begins with a copy of its
    // parent's, so walking all of it would compile the parent's bodies a second
    // time with `inside_` pointing at the CHILD -- overwriting `target.body`
    // with ops resolved against a different layout. The parent's methods are
    // already right for a child object without being recompiled, because a
    // child's fields sit AFTER its parent's and the parent's indices do not
    // move.
    for (const resolve::Suit &suit : resolved_.suits) {
        inside_ = &suit;
        for (size_t m = suit.inherited_methods; m < suit.methods.size(); m++)
            capsule(suit.methods[m].node);
        inside_ = nullptr;
    }

    return std::move(out_);
}

} // namespace eval
} // namespace satellite
