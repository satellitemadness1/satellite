// The four passes and their order -- DESIGN §7.3. See name_resolver/resolve.hpp
// for what the pass is for and name_resolver/resolve_internal.hpp for the split.
//
// THE ORDER IS THE WHOLE REASON RESOLVE IS NOT IN THE PARSER, and §7.3 says it
// in one sentence: "a capsule may call one defined further down the file, and
// mutual recursion is unresolvable in single-pass recursive descent." So every
// capsule NAME is collected before any capsule BODY is read, and a forward
// reference is a lookup in a table that is already complete rather than a
// promise to come back later.
//
// TWO PROPERTIES FALL OUT OF THAT AND §7.3 NAMES BOTH: the parser's purity
// survives -- DESIGN §6.3, and parser.hpp repeats it -- and a resolver bug says
// so BEFORE anything executes rather than producing a wrong value somewhere
// inside a walk.

#include "name_resolver/resolve_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_words/words.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace satellite::resolve {

Info &Resolver::info(NodeIndex node)
{
    if (node >= out_.nodes.size())
        out_.nodes.resize(node + 1);
    return out_.nodes[node];
}

// --- pass 1 -- every capsule name -------------------------------------------

void Resolver::collect_capsules()
{
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind != NodeKind::Capsule)
            continue;

        const std::string_view spelling = ast_.text_of(item);

        // THE PARSER ALREADY REFUSES A DUPLICATE AND ONE FORM GETS PAST IT.
        // `define_name()` reports S0242 for a capsule of the user's own, so a
        // second `fact` never reaches here -- but a capsule named
        // `satellite.main` is LOOKED UP rather than defined (capsule_decl's
        // reserved arm), and the numbering has nothing to say about a name it
        // did not allocate. So a file with two `satellite.main`s parses clean,
        // and this is the only pass that can see it.
        if (const Capsule *first = capsule_named(spelling)) {
            problem<errors::Code::RESOLVE_CAPSULE_TWICE>(item, spelling);
            attach(errors::note<errors::Code::NOTE_DECLARED_FIRST_HERE>(
                span_of(first->node), spelling));
            continue;
        }

        info(item).slot = kSlotCapsule;
        info(item).path = ast_[item].a;
        info(item).origin = Origin::Parsed;
        capsules_.push_back({ast_[item].a, spelling, item});
    }
}

// --- pass 2 -- every spacesuit name, and it is M26's ------------------------

void Resolver::note_spacesuits()
{
    // A NAMED HOLE AND NOT AN OMISSION. DESIGN §7.3 puts spacesuits second in
    // the order and PLAN §8 puts them at M26, so the honest shape is a pass
    // that EXISTS, keeps its place in the order, and resolves nothing -- and
    // `satl --resolve` prints how many it skipped, because a pass that silently
    // resolved nothing is indistinguishable from one that worked.
    //
    // WHAT THE M6 DRAFT DOES HERE IS THE OTHER 40% OF ITS SOURCE. It links
    // superclasses, breaks inheritance cycles, flattens field layouts, builds
    // method tables and checks access -- and every line of that is a decision
    // about what a spacesuit IS, which is M26's to take. Copying it forward
    // would have been this milestone deciding another one's design in passing.
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind != NodeKind::Spacesuit)
            continue;
        info(item).slot = kSlotSpacesuit;
        info(item).path = ast_[item].a;
        out_.spacesuits++;
    }
}

// --- pass 3 -- the top level ------------------------------------------------

void Resolver::globals()
{
    // §7.3 CALLS THIS PASS "top-level statements" AND THIS GRAMMAR HAS NONE,
    // which is worth saying rather than quietly renaming. DESIGN §6's
    // `top_level` is include, capsule, spacesuit and global -- and the parser's
    // S0204 says so in the words a user reads: "a statement at the top of a
    // file is not an unfinished feature, it is a program with no capsule to
    // run". So what this pass actually resolves is the two forms that CAN carry
    // an expression outside a body: a global's initialiser and an include's
    // argument. The pass keeps its place because §7.3's ORDER is what matters
    // -- a global read by a capsule body has to be numbered before pass 4.
    frame_ = nullptr;
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        const Node &node = ast_[item];
        if (node.kind == NodeKind::Global) {
            info(item).slot = kSlotGlobal;
            info(item).path = node.a;
            info(item).origin = Origin::Parsed;
            if (node.b != kNoNode)
                expression(node.b);
        } else if (node.kind == NodeKind::Include) {
            // THE ONE PLACE THAT PUSHES WITHOUT DRAINING, so it drains here.
            // statement_form() is written to be reached from inside the walk --
            // `satellite.return(x)` is the other caller and the stack is
            // already turning when it arrives -- and an include is met before
            // any walk has started.
            statement_form(item, words::NodeId::SATELLITE, "include");
            run_work();
        }
    }
}

// --- pass 4 -- every body ---------------------------------------------------

void Resolver::bodies()
{
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind != NodeKind::Capsule)
            continue;
        // The duplicate pass 1 refused has no frame, and giving it one here
        // would put a second `satellite.main` in the dump as though the file
        // had two.
        const Capsule *owner = capsule_named(ast_.text_of(item));
        if (owner == nullptr || owner->node != item)
            continue;

        out_.frames.push_back(Frame{});
        Frame &frame = out_.frames.back();
        frame.capsule = ast_[item].a;
        frame.node = item;
        body_of(item, frame);
    }
}

void Resolver::run()
{
    collect_capsules();
    note_spacesuits();
    globals();
    bodies();

    // SORTED BY WHERE THEY ARE IN THE FILE, WHICH FOUR PASSES DO NOT PRODUCE.
    // Pass 1 sees a capsule declared twice on line 14 and pass 4 sees a
    // parameter declared twice on line 3, so the order they were FOUND in is
    // the order of the passes and not of the program. Nobody reads a file by
    // pass. The lexer and the parser get this for free by walking forward
    // once; this pass has to ask for it, and it is one stable_sort at the end
    // rather than a rule every site has to keep.
    //
    // STABLE, so that two problems on one line stay in the order they were
    // found -- which is the order somebody would fix them in.
    std::stable_sort(out_.problems.begin(), out_.problems.end(),
                     [](const errors::Diagnostic &a, const errors::Diagnostic &b) {
                         return a.at.start < b.at.start;
                     });
}

Resolved resolve(const Ast &ast, words::Words &words, const cache::Marks &marks,
                 const cache::Folded &folded)
{
    Resolved out;
    // ONE ENTRY PER NODE, RESERVED RATHER THAN GROWN. The arena's size is known
    // and every pass indexes into this by node, so a resize inside the walk
    // would be the one allocation the walk could not account for.
    out.nodes.resize(ast.size());
    if (ast.root() == kNoNode)
        return out;
    Resolver(ast, words, marks, folded, out).run();
    return out;
}

cache::Folds folds_of(const Resolved &resolved)
{
    cache::Folds out;
    out.selector.resize(resolved.nodes.size(), false);
    for (size_t node = 0; node < resolved.nodes.size(); node++)
        out.selector[node] = resolved.nodes[node].folded_option;
    return out;
}

} // namespace satellite::resolve
