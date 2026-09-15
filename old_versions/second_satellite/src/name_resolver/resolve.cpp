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
#include <memory>
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
        out_.declared.push_back({ast_[item].a, Declares::Capsule, item});

        // A LAUNCH ANSWERS NOBODY -- 2026-09-13, S0525's argument one
        // declaration over. The caret goes on the type, which is the part to
        // delete.
        if (ast_.is_launch(item) && ast_[item].c != kNoNode)
            problem<errors::Code::RESOLVE_LAUNCH_RETURNS>(ast_[item].c, spelling);
    }
}

// EVERY GLOBAL THE FILE DECLARES, BY NUMBER -- M25. Nothing here resolves one;
// pass 3 does that. What another file needs before any body is walked is to
// know that `satellite.library.ship.total` is a global and not a capsule.
void Resolver::note_globals()
{
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind == NodeKind::Global)
            out_.declared.push_back({ast_[item].a, Declares::Global, item});
    }
}

// EVERY CAPSULE'S DECLARED VARIABLES, NUMBERED UNDER THE CAPSULE -- M26's
// nested globals, and most of this was already built. `satellite.library
// .memory_vars` is ALREADY `1 14 4`: WORD_NUMBERS §3 gives every user capsule
// the next free number under `satellite.library` when the parser first meets
// it, so the first segment of the author's line has had a number all along.
// What was missing is the SECOND -- a capsule's locals were never interned as
// its children, so the walk stopped at the capsule and S0521 said `memory_vars`
// "is not a word the language has", which was true of the language and false of
// the program. That is exactly the sentence M9 met one level up, for globals.
//
// ONLY THE BODY'S TOP LEVEL, AND ONLY A DECLARATION WITH AN INITIALISER. A name
// inside an `if` is reachable only when that branch runs, and one with no `=`
// has no value written down anywhere -- neither has an answer to give a reader
// standing outside the capsule. Both are left alone rather than given a number
// that means nothing, which is WORD_NUMBERS §1.2's "never renumber, never
// reuse" read forward: a number handed out here is one the language must be
// able to answer forever.
//
// A METHOD'S BODY IS NOT WALKED. A spacesuit's capsules are numbered under the
// SUIT, and their locals are a suit's business -- DESIGN §12 defers bare field
// access precisely so that a spacesuit's insides are reached through its own
// methods. `satellite.library` is the road to the file's capsules.
void Resolver::note_capsule_constants()
{
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind != NodeKind::Capsule)
            continue;

        // `satellite.main` IS NOT UNDER `satellite.library` AND IS SKIPPED.
        // The author's spelling is `satellite.library.capsule_name
        // .variable_name`, and main's path is `1 3` -- a LANGUAGE word, whose
        // children are the language's own rows. Interning a program's locals
        // there would put user names in a namespace the language is still
        // appending to, which is the one thing WORD_NUMBERS §3 keeps the user's
        // numbers out of.
        const words::PathId owner = ast_[item].a;
        if (words::is_language_word(owner))
            continue;

        const NodeIndex body = ast_[item].d;
        if (body == kNoNode || ast_[body].kind != NodeKind::Block)
            continue;

        for (uint32_t k = 0; k < ast_.list_size(ast_[body].a); k++) {
            const NodeIndex line = ast_.list_at(ast_[body].a, k);
            if (ast_[line].kind != NodeKind::VarDecl || ast_[line].b == kNoNode)
                continue;

            // AND THE INITIALISER HAS TO BE A CONSTANT, WHICH IS THE WHOLE OF
            // WHAT MAKES THIS SOUND. The value is run ONCE, at startup, in the
            // top-level block -- where there is no frame at all -- so an
            // initialiser naming a parameter or another local compiles to
            // `op_local` against nothing. That is not a refusal, it is a
            // SEGFAULT, and it is how this was found: infinity_data_main.satl
            // resolved clean, compiled clean, and died in op_local under
            // run_top_level, because `satellite.main`'s line 1381 is
            // `infinity_data infinity_pointer = local_infinity_data.pointer()`
            // and `local_infinity_data` is a local.
            //
            // A LITERAL IS THE ONLY THING A DECLARATION WRITES DOWN WITH NO
            // REGARD TO WHO IS CALLING, which is exactly the property the
            // outside view needs -- and it is what the author's own case is:
            // `satellite.variable.number target_gb = 50`. Anything else is a
            // value that depends on the call, and a capsule's call is the thing
            // a reader standing outside it does not have.
            if (!constant_initialiser(ast_[line].b))
                continue;

            // `intern` AND NOT `define`, so a name written twice in one body
            // answers one number rather than allocating a second nobody can
            // reach. The duplicate itself is S0501's business in pass 4, where
            // the slot is, and it is not this pass's to report twice.
            const words::PathId path =
                words_.intern(owner, ast_.text_of(line));
            if (path == words::kNoPath)
                continue;
            // QUIETLY, because pass 4 walks this declaration again and says
            // whatever is wrong with its type there. Asked aloud here too, a
            // bad type in a capsule's first lines was reported twice.
            quiet_ = true;
            const words::PathId type = type_of(ast_[line].a);
            quiet_ = false;
            out_.capsule_constants.push_back({path, type, line, ast_[line].b});
        }
    }
}

// WHETHER A DECLARATION WRITES DOWN A VALUE THAT DOES NOT DEPEND ON THE CALL.
//
// THE FOUR LITERAL KINDS AND NOTHING ELSE, AND THE NARROWNESS IS DELIBERATE
// RATHER THAN LAZY. A wider rule -- "anything that reads no frame slot" --
// would admit `satellite.library.other.setting + 1`, which is a real and useful
// shape; what it needs first is a way to ASK whether an expression touches a
// frame, and this resolver's walk is a work queue rather than a recursion, so
// there is no point in it that brackets "inside this initialiser". Widening
// this is a later milestone's, and it is a widening: every program that works
// under this rule works under that one.
bool Resolver::constant_initialiser(NodeIndex node) const
{
    if (node == kNoNode)
        return false;
    switch (ast_[node].kind) {
    case NodeKind::Number:
    case NodeKind::String:
    case NodeKind::Bits:
        return true;
    default:
        return false;
    }
}

// THE REST OF A PATH THAT LEFT THE LANGUAGE'S WORDS -- M26. `language_path`
// walks the FROZEN trie and cannot do this: it is handed an `Ast` and a node
// and has no runtime `Words` at all, which is right for a cache that must read
// a file written by another run. So the walk is finished here, where the
// program's own names live.
//
// IT PICKS UP EXACTLY WHERE THE FROZEN WALK STOPPED. `found.under` is the last
// language node and `found.at` is the Member that failed under it, so the
// segments still to place are `found.at` and every Member between it and
// `node`. Each one is a `find` under the last, and EVERY step must land on a
// user word -- a language word appearing mid-chain would mean the frozen walk
// should have taken it and did not, which is a bug rather than a path.
//
// THIS GENERALISES THE ONE-SEGMENT ARM M9 WROTE and does not replace its
// reason. M9 answered `satellite.library.total` by asking the runtime trie for
// one name under one language node; a capsule's variable is the same question
// asked twice, and asking it n times is the only difference.
words::PathId Resolver::user_path_of(const cache::PathMatch &found,
                                     NodeIndex node) const
{
    if (found.under == words::kNoPath || found.at == kNoNode)
        return words::kNoPath;

    std::vector<NodeIndex> chain;
    for (NodeIndex at = node; at != kNoNode; at = ast_[at].a) {
        if (ast_[at].kind != NodeKind::Member)
            return words::kNoPath;
        chain.push_back(at);
        if (at == found.at)
            break;
    }
    if (chain.empty() || chain.back() != found.at)
        return words::kNoPath;

    // THE SEGMENTS FROM `from` DOWN, skipping the first `skip` of them.
    const auto walk = [&](words::PathId from, size_t skip) {
        words::PathId at = from;
        for (size_t i = chain.size() - skip; i > 0; i--) {
            at = words_.find(at, ast_.text_of(chain[i - 1]));
            if (at == words::kNoPath || words::is_language_word(at))
                return words::kNoPath;
        }
        return at;
    };
    if (found.under != static_cast<words::PathId>(words::NodeId::LIBRARY))
        return walk(found.under, 0);

    // `satellite.library.ship.rest` -- A SPACESHIP THIS FILE INCLUDES, M25, and
    // the rest is looked up among ITS names, from its node. For the file satl
    // was given, reached as a spaceship, that node is `satellite.library`
    // itself, which is why this is not simply the walk below.
    if (const Spaceship *ship = spaceship_named(ast_.text_of(chain.back()))) {
        if (ship->node == words::kNoPath || chain.size() < 2)
            return words::kNoPath;
        return walk(ship->node, 1);
    }

    // A SPACESHIP'S OWN `satellite.library` IS ITS NODE, AND NOTHING ELSE IS.
    // Inside `ship.satl`, `satellite.library.total` is ship's own global; the
    // program's globals and every other file's names are not reachable from
    // it, because the file never included them -- found by review, when a
    // spaceship read and overwrote its includer's global through its own
    // bare `satellite.library.secret`.
    if (library_ != found.under)
        return walk(library_, 0);

    // AND THE FILE satl WAS GIVEN REACHES ITS OWN NAMES, but not a spaceship
    // it did not include -- whose node hangs under the same `satellite.library`.
    const words::PathId first = words_.find(found.under, ast_.text_of(chain.back()));
    if (a_spaceship_node(first))
        return words::kNoPath;
    return walk(found.under, 0);
}

// THE BARE SPELLING OF A CONVERSION, OR NO PATH -- 2026-09-12. Four words, and
// the table is written out rather than derived because these four are the only
// bare spellings the language answers: a fifth would be a decision, not a
// pattern to be matched by accident.
words::PathId Resolver::conversion_named(std::string_view spelling) const
{
    using words::NodeId;
    if (spelling == "string")
        return static_cast<words::PathId>(NodeId::VARIABLE_STRING_OF);
    if (spelling == "number")
        return static_cast<words::PathId>(NodeId::VARIABLE_NUMBER_OF);
    if (spelling == "binary")
        return static_cast<words::PathId>(NodeId::VARIABLE_BINARY_OF);
    if (spelling == "hex")
        return static_cast<words::PathId>(NodeId::VARIABLE_HEX_OF);
    return words::kNoPath;
}

// --- pass 2 -- every spacesuit name, and it is M26's ------------------------

// PASS 2, AND IT WAS A NAMED HOLE FROM M7 UNTIL M26. What stood here kept its
// place in DESIGN §7.3's order, resolved nothing, and said so -- because "a
// pass that silently resolved nothing is indistinguishable from one that
// worked" -- and its note recorded exactly why it was empty:
//
//     "What the M6 draft does here is the other 40% of its source. It links
//      superclasses, breaks inheritance cycles, flattens field layouts, builds
//      method tables and checks access -- and every line of that is a decision
//      about what a spacesuit IS, which is M26's to take. Copying it forward
//      would have been this milestone deciding another one's design in
//      passing."
//
// SO WHAT M26 BUILT IS THE TWO OF THOSE FIVE THAT ARE THIS LANGUAGE'S, AND NOT
// THE OTHER THREE. It flattens the field layout and builds the method table.
// It does NOT link superclasses or break inheritance cycles -- the parser keeps
// a `super` name (Spacesuit::c) and DESIGN §13 defers even `super(...)`, so
// inheritance is parsed, carried, and not yet given meaning. And access is
// checked where it can be reported against the thing being reached, which is
// names.cpp and not here.
//
// A FIELD IS A SLOT AND A METHOD IS A PATH -- resolve.hpp's `Suit` carries the
// argument. Both halves are mechanisms the language already had: DESIGN §7.2's
// storage decided before anything runs, and §7.6's capsules in a table of their
// own. Neither needed inventing, which is why the largest feature in PLAN §8 is
// this short.
// THE NAMES FIRST, ALL OF THEM, AND THEN THE MEMBERS -- M26's inheritance,
// and the two halves cannot be one loop. A suit may extend one declared
// FURTHER DOWN THE FILE, which is the same forward reference DESIGN §7.3
// grants every capsule; and a child's field layout is its parent's
// layout followed by its own, so no child's members can be gathered until
// its parent's have been. One loop in file order would have answered "no
// such spacesuit" to a program that is correct.
//
// AND THE TWO HALVES ARE TWO FUNCTIONS SINCE M25, because the forward reference
// now crosses files: a field of `ship.box` in this file is gathered after every
// file's suits have names, which resolve_run() arranges by calling this half
// for every file before note_spacesuits() for any.
void Resolver::name_spacesuits()
{
    const Node &program = ast_[ast_.root()];
    for (uint32_t i = 0; i < ast_.list_size(program.a); i++) {
        const NodeIndex item = ast_.list_at(program.a, i);
        if (ast_[item].kind != NodeKind::Spacesuit)
            continue;
        info(item).slot = kSlotSpacesuit;
        info(item).path = ast_[item].a;

        Suit suit;
        suit.path = ast_[item].a;
        suit.name = ast_.text_of(item);
        suit.node = item;
        suits_.push_back({suit.path, suit.name, item});
        out_.suits.push_back(std::move(suit));
        out_.declared.push_back({ast_[item].a, Declares::Spacesuit, item});
    }
}

void Resolver::note_spacesuits()
{
    link_supers();

    // GATHERED PARENT-FIRST, AND THE ORDER IS COMPUTED RATHER THAN ASSUMED.
    // `ready` counts a suit as done when its parent is done, which walks the
    // forest from its roots however the file happened to be written. A suit
    // still unfinished when no progress is left is one whose parent chain never
    // reaches a root -- a cycle -- and link_supers has already refused it and
    // cut the link, so this loop's second pass finishes it as a root.
    std::vector<bool> done(out_.suits.size(), false);
    bool moved = true;
    while (moved) {
        moved = false;
        for (size_t i = 0; i < out_.suits.size(); i++) {
            if (done[i])
                continue;
            const words::PathId parent = out_.suits[i].parent;
            size_t from = out_.suits.size();
            if (parent != words::kNoPath) {
                for (size_t j = 0; j < out_.suits.size(); j++)
                    if (out_.suits[j].path == parent)
                        from = j;
                if (from < out_.suits.size() && !done[from])
                    continue;
            }

            // THE PARENT'S MEMBERS ARE COPIED IN BEFORE A LINE OF THE CHILD'S
            // IS READ, which is what makes the child's own field indices come
            // out right the FIRST time. `gather_members` numbers a field by
            // `into.fields.size()` as it appends, so starting the vector at the
            // parent's length is the whole of the offset arithmetic -- there is
            // no fix-up pass, and no second place that knows the layout.
            if (from < out_.suits.size()) {
                out_.suits[i].fields = out_.suits[from].fields;
                out_.suits[i].methods = out_.suits[from].methods;
                out_.suits[i].inherited =
                    static_cast<uint32_t>(out_.suits[from].fields.size());
                out_.suits[i].inherited_methods =
                    static_cast<uint32_t>(out_.suits[from].methods.size());
            }
            gather_members(out_.suits[i].node, out_.suits[i], false);
            done[i] = true;
            moved = true;
        }
    }
}

// WHICH SUIT EACH `(name)` MEANS, AND WHETHER THE CHAIN ENDS -- M26. This is
// two of the five things pass 2's note listed as "a decision about what a
// spacesuit IS": linking superclasses, and refusing the cycles the M6 draft
// silently broke.
//
// A CYCLE IS REFUSED AND THEN CUT, IN THAT ORDER. The refusal is the answer a
// person gets; the cut is so that every pass after this one can assume the
// parent chain terminates, which is what lets `note_spacesuits` above walk it
// with a counter instead of a visited set, and what stops a field layout from
// being defined in terms of itself.
void Resolver::link_supers()
{
    for (Suit &suit : out_.suits) {
        const NodeIndex super = ast_[suit.node].c;
        if (super == kNoNode)
            continue;

        const std::string_view spelling = ast_.text_of(super);
        const Capsule *found = suit_named(spelling);
        if (found == nullptr) {
            problem<errors::Code::RESOLVE_SUIT_NO_SUCH_SUPER>(super, spelling,
                                                              suit.name);
            continue;
        }
        if (found->path == suit.path) {
            problem<errors::Code::RESOLVE_SUIT_INHERITANCE_CYCLE>(super,
                                                                  suit.name);
            continue;
        }
        suit.parent = found->path;
        info(super).slot = kSlotSpacesuit;
        info(super).path = found->path;
        info(super).type = found->path;
        info(super).origin = Origin::Bound;
    }

    // AND NOW THE CHAINS, WHICH ARE A SEPARATE WALK BECAUSE A CYCLE IS NOT A
    // PROPERTY OF ONE ROW. Every link above is individually fine in `a(b)`,
    // `b(c)`, `c(a)`; what is wrong is the loop, and it can only be seen by
    // following one. The bound is the number of suits -- a chain longer than
    // that has repeated a suit by the pigeonhole and nothing else -- so this
    // costs one walk per suit and needs no marking.
    for (Suit &suit : out_.suits) {
        words::PathId at = suit.parent;
        for (size_t steps = 0; at != words::kNoPath && steps <= out_.suits.size();
             steps++) {
            if (at == suit.path) {
                problem<errors::Code::RESOLVE_SUIT_INHERITANCE_CYCLE>(
                    ast_[suit.node].c, suit.name);
                suit.parent = words::kNoPath;
                break;
            }
            const Suit *up = out_.suit_at(at);
            at = up == nullptr ? words::kNoPath : up->parent;
        }
    }
}

// ONE SUIT'S MEMBERS, FLATTENED ACROSS ITS SECTIONS -- and this is the second
// of this file's walkers to keep its own stack, for DESIGN §7.5's reason. A
// section may hold a section (parser_declarations.cpp's `suit_body` is one loop
// over `open` for exactly that), so the nesting is the user's to choose and no
// walker may spend the C++ stack on a depth a program picks.
//
// ACCESS TRAVELS DOWN. A member of a `public` section inside a `protected` one
// is protected: the outer answer is the one that decided you could get this far,
// which is the reading every language with nested access takes and is the only
// one that cannot be used to smuggle a field out.
void Resolver::gather_members(NodeIndex suit_node, Suit &into, bool)
{
    struct Level {
        ListId items = kNoList;
        uint32_t at = 0;
        bool is_public = false;
    };
    std::vector<Level> open;
    open.push_back({ast_[suit_node].b, 0, false});

    while (!open.empty()) {
        Level &here = open.back();
        if (here.at >= ast_.list_size(here.items)) {
            open.pop_back();
            continue;
        }
        const NodeIndex item = ast_.list_at(here.items, here.at++);
        const bool is_public = here.is_public;

        switch (ast_[item].kind) {
        case NodeKind::Section: {
            // WHICH SECTION IT IS, IS THE TOKEN'S SPELLING AND NOT A FIELD --
            // the parser says so where it builds the node, because both words
            // are in words.def and the token already carries an integer that
            // answers it.
            const bool opens_public =
                ast_.text_of(item) == "public" && !is_public ? true : is_public;
            open.push_back({ast_[item].a, 0, opens_public});
            break;
        }

        case NodeKind::VarDecl: {
            Field field;
            field.name = ast_.text_of(item);
            field.type = type_of(ast_[item].a);
            field.at = item;
            field.init = ast_[item].b;
            field.is_public = is_public;

            // A NAME DECLARED TWICE IN ONE SUIT IS S0501's QUESTION ONE LEVEL
            // UP, and it is refused here rather than left to collide in a
            // vector. IT ASKS ABOUT THIS SUIT'S OWN MEMBERS AND NOT ITS
            // INHERITED ONES -- M26 -- because redeclaring a PARENT's field is
            // not declaring one twice, it is the shadowing resolve.hpp's
            // backwards search exists to settle, and the author's own file
            // does it in every suit it has. DESIGN §7.4's fresh-slot rule is about a redeclaration in
            // a BODY, where rebinding is the right answer because the old slot
            // may still be referred to; a suit has one storage layout and two
            // fields of one name would be two indices nothing could tell apart.
            if (into.own_field_named(field.name) != nullptr ||
                into.own_method_named(field.name) != nullptr) {
                problem<errors::Code::RESOLVE_SUIT_MEMBER_TWICE>(
                    item, field.name, into.name);
                break;
            }
            info(item).slot = static_cast<Slot>(into.fields.size());
            info(item).type = field.type;
            into.fields.push_back(field);
            break;
        }

        case NodeKind::Capsule: {
            Method method;
            method.path = ast_[item].a;
            method.name = ast_.text_of(item);
            method.node = item;
            // THE CONSTRUCTOR IS PUBLIC WHEREVER IT IS WRITTEN -- 2026-09-12.
            // It is its own section beside `protected` and `public` rather than
            // a member of either, and `object_name.constructor(args)` is the
            // author's own spelling of calling it from outside the suit. The
            // parser refuses any other capsule of this name, so the spelling is
            // the whole test.
            method.is_public = is_public || method.name == "constructor";

            if (into.own_field_named(method.name) != nullptr ||
                into.own_method_named(method.name) != nullptr) {
                problem<errors::Code::RESOLVE_SUIT_MEMBER_TWICE>(
                    item, method.name, into.name);
                break;
            }
            // THE CONSTRUCTOR MAY NOT DECLARE A RETURN TYPE -- DESIGN §13,
            // enforced at M26. The caret goes on the `satellite.returns` type
            // rather than the name, because the type is the part to delete.
            // The member is still recorded: the program is refused either way,
            // and dropping it would add an S0518 nobody made. SINCE 2026-09-12
            // the constructor is the `satellite.constructor` section, not the
            // capsule named after its suit, and the sentence names the suit.
            if (method.name == "constructor" && ast_[item].c != kNoNode)
                problem<errors::Code::RESOLVE_CONSTRUCTOR_RETURNS>(
                    ast_[item].c, into.name);
            into.methods.push_back(method);
            break;
        }

        default:
            // The parser's `suit_member` admits a capsule or a field and
            // nothing else, so this is unreachable rather than defensive -- and
            // it is written out because a switch with no default is a warning
            // and a silent skip is how an added node kind disappears.
            break;
        }
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

// EVERY SPACESUIT FIELD'S INITIALISER -- M26, and it was a hole the size of the
// feature. `gather_members` recorded `field.init` and NOTHING EVER WALKED IT, so
// a field's initialiser reached the compiler with no path on any of its nodes:
//
//     satellite.container.list<infinity_subject> subjects = satellite.container.list()
//
// compiled to S0720, "a method on this expression parses and does not run yet",
// about `satellite.container.list()` -- a language row the compiler dispatches
// perfectly well one line further down a capsule body. The construction that
// worked at `box b` was the construction of a suit whose fields are LITERALS,
// which is what every fixture and every probe had.
//
// WITH NO FRAME AND NO `inside_`, WHICH IS NOT A SIMPLIFICATION. A field
// initialiser runs during op_construct, BEFORE the object exists -- its values
// are pushed onto the value stack and only then assembled -- so there is no
// receiver for a field to be read through and no slot 0 to read it from. A
// field therefore cannot name another field, and that is a fact about when the
// code runs rather than a rule this pass imposes. `frame_ = nullptr` is the
// same statement a global's initialiser makes one pass up.
void Resolver::suit_field_initialisers()
{
    frame_ = nullptr;
    inside_ = nullptr;
    for (const Suit &suit : out_.suits)
        for (size_t i = suit.inherited; i < suit.fields.size(); i++)
            if (suit.fields[i].init != kNoNode)
                expression(suit.fields[i].init);
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

    // AND EVERY METHOD OF EVERY SPACESUIT -- M26, and it is the SAME pass and
    // the same function. A method is a capsule with a receiver, so it gets a
    // frame like any other; what pass 2 already decided is which suit's fields
    // are in scope while its body is walked.
    //
    // AFTER THE CAPSULES AND NOT BEFORE, so that a method calling a top-level
    // capsule finds it -- DESIGN §7.3's whole reason for passes is that "a
    // capsule may call one defined further down the file". Pass 1 collected
    // every capsule NAME before any body was walked, so the order here is about
    // the frame list a dump prints and not about what resolves.
    //
    // A REFERENCE AND NOT A COPY, AND THE VECTOR IS NOT GROWN WHILE IT IS HELD.
    // `suits_` is pass 2's and is complete before this runs; `out_.frames` is
    // what grows, which is why `suit` below is re-read from `out_.suits` by
    // index rather than held across the push_back.
    for (size_t which = 0; which < out_.suits.size(); which++) {
        // FROM `inherited_methods`, SO AN INHERITED METHOD IS RESOLVED ONCE --
        // M26. It is resolved against the suit that DECLARED it, whose layout
        // its field indices belong to; resolving it a second time under a child
        // would rebind those indices to the child's copies of the same names
        // and silently move what the parent's own code reads.
        for (size_t m = out_.suits[which].inherited_methods;
             m < out_.suits[which].methods.size(); m++) {
            const NodeIndex node = out_.suits[which].methods[m].node;

            out_.frames.push_back(Frame{});
            Frame &frame = out_.frames.back();
            frame.capsule = out_.suits[which].methods[m].path;
            frame.node = node;
            frame.suit = out_.suits[which].path;
            inside_ = &out_.suits[which];
            body_of(node, frame);
            inside_ = nullptr;
        }
    }
}

// PHASE ONE: every name this file declares at its top -- the capsules, the
// globals, the spacesuits' names. Nothing is read yet.
void Resolver::declare_names()
{
    collect_capsules();
    note_globals();
    name_spacesuits();
}

// PHASE TWO: what the declarations hold.
void Resolver::declare_members()
{
    // BEFORE `globals()` AND BEFORE `bodies()`, FOR THE REASON §7.3 ORDERS
    // EVERY OTHER PASS: a name has to be numbered before the pass that reads it
    // runs. A global's initialiser or a capsule body may name
    // `satellite.library.other.setting`, and either would be resolved against a
    // trie that did not have it yet.
    note_capsule_constants();
    note_spacesuits();
}

// PHASE THREE: every expression and every body.
void Resolver::resolve_bodies()
{
    globals();

    // AFTER `globals()`, so a field initialiser may read one, and before
    // `bodies()` for §7.3's ordering reason -- a method body may construct the
    // suit whose fields these are.
    suit_field_initialisers();

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

void Resolver::run()
{
    declare_names();
    declare_members();
    resolve_bodies();
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

std::vector<Resolved> resolve_run(const std::vector<File> &files,
                                  words::Words &words)
{
    // SIZED ONCE, BEFORE ANY RESOLVER EXISTS: each holds a reference to its
    // own answer, and a vector that grew would move them all.
    std::vector<Resolved> out(files.size());
    const Run run{&files, &out};

    std::vector<std::unique_ptr<Resolver>> resolvers;
    for (uint32_t f = 0; f < files.size(); f++) {
        out[f].nodes.resize(files[f].ast->size());
        if (files[f].ast->root() != kNoNode)
            resolvers.push_back(std::make_unique<Resolver>(run, f, words, out[f]));
        else
            resolvers.push_back(nullptr);
    }

    for (auto &each : resolvers)
        if (each)
            each->declare_names();
    for (auto &each : resolvers)
        if (each)
            each->declare_members();
    for (auto &each : resolvers)
        if (each)
            each->resolve_bodies();
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
