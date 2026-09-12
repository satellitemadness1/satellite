// What a name is -- a slot, a capsule, a spacesuit, or nothing at all. DESIGN
// §7.2 is the storage and §7.7 is the one library global the language provides.
//
// THE ORDER OF THE LOOKUPS IS THE LANGUAGE'S SHADOWING RULE and is the only
// place it is written down. In scope first, then the capsules this file
// declares, then the spacesuits -- so a local called `fact` shadows a capsule
// called `fact` for the rest of its scope, which is what somebody who wrote
// both meant. It is the opposite order from words_runtime.hpp's find(), and
// deliberately: there the LANGUAGE's children are searched first, because a
// user's name may never answer in place of a word the language owns. Names the
// user chose shadow each other; nothing shadows the language.

#include "name_resolver/resolve_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "error_reporter/suggest.hpp"
#include "satellite_cache/paths.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite::resolve {

namespace {

// Every spelling of the arguments object, as a sentence -- READ OUT OF THE
// REGISTRY AND NOT WRITTEN HERE. S0531's `{1}`.
//
// THIS FUNCTION EXISTS BECAUSE THE LIST WAS A STRING LITERAL AND IT WENT STALE
// THE HOUR `argv` WAS ADDED. On 2026-09-09 the author added the seventh
// spelling; words.def, WORD_NUMBERS §2.3, DESIGN §7.7, the lexer's spelling
// table and four test files all moved, the alias resolved end to end -- and
// S0531 went on listing six, because this one call site held the sentence as
// text. THE ERROR WHOSE ENTIRE JOB IS TO NAME THE ACCEPTED SPELLINGS WAS THE
// LAST THING IN THE TREE THAT DID NOT KNOW ONE HAD BEEN ACCEPTED, and it was
// found by running the program rather than by any of the fourteen suites --
// none of them reads the sentence, they read the code.
//
// So the list is built from `words::kAliases` plus the node's own text. An
// alias row added to words.def shows up here with no edit at all, which is
// what the comment at the call site had been claiming before it was true.
std::string accepted_spellings()
{
    std::vector<std::string_view> spellings;
    spellings.push_back(words::text_of(words::NodeId::LIBRARY_MAIN_ARGUMENTS));
    for (size_t i = 0; i < words::kAliasCount; i++)
        if (words::kAliases[i].of == words::NodeId::LIBRARY_MAIN_ARGUMENTS)
            spellings.push_back(words::kAliases[i].text);

    // "a, b or c" -- `or` and not `and`, because exactly one of them is written.
    std::string sentence;
    for (size_t i = 0; i < spellings.size(); i++) {
        if (i != 0)
            sentence += (i + 1 == spellings.size()) ? " or " : ", ";
        sentence += spellings[i];
    }
    return sentence;
}

} // namespace

void Resolver::name(NodeIndex node)
{
    const std::string_view spelling = ast_.text_of(node);

    if (const Binding *bound = lookup(spelling)) {
        info(node).slot = bound->slot;
        info(node).type = bound->type;
        if (bound->arguments) {
            info(node).arguments = true;
            info(node).origin = Origin::Bound;
            info(node).path =
                static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS);
            // AND ITS TYPE IS THE OBJECT'S OWN, WHICH IS TRUER THAN WHAT WAS
            // WRITTEN -- M20, and the three shaped selectors need it. A
            // program declares the parameter `satellite.container.list<
            // satellite.variable.string>`, which is v1's compatibility
            // spelling and is what DESIGN §7.7 keeps so hello world stays five
            // lines; what the slot HOLDS is a
            // `satellite.container.arguments` `1 4 3`. call_target_done()
            // looks a shaped selector up under the receiver's `type`, so
            // `argz.has("machine.threads")` finds `has(k)` `1 4 3 6` here and
            // would have looked for it on a list otherwise.
            //
            // THE FACTS ARE NOT AFFECTED AND THAT IS BY ORDER RATHER THAN BY
            // LUCK: member_done() branches on `arguments` BEFORE it reaches
            // the type fold, so `argz.machine` is still the object's road.
            info(node).type =
                static_cast<words::PathId>(words::NodeId::CONTAINER_ARGUMENTS);
        }
        return;
    }

    // A FIELD OF THE SPACESUIT WHOSE METHOD WE ARE INSIDE -- M26, and it sits
    // BETWEEN the locals above and the capsules below for a reason worth
    // stating. A local shadows a field, because a parameter named `n` in a suit
    // that also has a field `n` means the parameter -- what every language with
    // both does, and what a reader expects. A field shadows a CAPSULE, because
    // storage is nearer than a name in another table.
    //
    // AND IT IS REACHED ONLY FROM INSIDE. `inside_` is set for the length of
    // one method body and is null everywhere else, so `n` at the top of a file
    // or in an ordinary capsule finds nothing here -- which is DESIGN §12's
    // "a spacesuit field is reachable from inside the spacesuit and nowhere
    // else", enforced by the scope rather than by a check somebody has to
    // remember to write.
    if (inside_ != nullptr) {
        if (const Field *found = inside_->field_named(spelling)) {
            info(node).slot = kSlotField;
            info(node).member =
                static_cast<uint32_t>(found - inside_->fields.data());
            info(node).type = found->type;
            info(node).origin = Origin::Bound;
            return;
        }

        // A METHOD OF THE SAME SUIT, CALLED WITHOUT A RECEIVER. `bump()` inside
        // `call_bump()` is how every one of the author's own suits is written,
        // and it is not a capsule of the file -- it is a sibling. It resolves
        // to the method's PATH, and the compiler turns that into a call on the
        // receiver already sitting at slot 0.
        if (const Method *found = inside_->method_named(spelling)) {
            info(node).slot = kSlotCapsule;
            info(node).path = found->path;
            info(node).origin = Origin::Bound;
            return;
        }
    }

    // DESIGN §7.6: capsules are not in the registry and live in their own
    // table. A bare capsule name cannot even form a legal registry key -- v1
    // verified it -- so this is a lookup and never a fallback.
    if (const Capsule *found = capsule_named(spelling)) {
        info(node).slot = kSlotCapsule;
        info(node).path = found->path;
        info(node).origin = Origin::Bound;
        return;
    }

    if (const Capsule *found = suit_named(spelling)) {
        info(node).slot = kSlotSpacesuit;
        info(node).path = found->path;
        info(node).origin = Origin::Bound;
        return;
    }

    // NO "IT MIGHT BE A GLOBAL" ARM, AND THE M6 DRAFT HAS ONE. Its resolver
    // answers SLOT_GLOBAL for any unknown name met outside a capsule, which is
    // right for the grammar it was written against and wrong for this one:
    // DESIGN §6's `top_level` is include, capsule, spacesuit and global, so the
    // only expressions outside a body are a global's initialiser and an
    // include's argument -- and an unknown name in either of those is unknown.
    // The parser's S0204 says the same thing in the words a user reads.
    // AND INSIDE AN UNEVALUATED ARGUMENT, NOTHING IS SAID HERE. A bare word in
    // `satellite.help(random)` is a TOPIC somebody left the `satellite.` off,
    // not a variable being read, and S0511's sentence answers the wrong
    // question about it. The node is left unresolved on purpose: the
    // evaluator's compiler finds no path and no slot on it and raises S1102,
    // which is the sentence the author settled for this exact mistake.
    // resolve_internal.hpp's visit_topic() carries the argument in full.
    if (in_topic_)
        return;

    problem<errors::Code::RESOLVE_NO_SUCH_NAME>(node, spelling);
    if (const std::string_view near = nearest_in_scope(spelling); !near.empty())
        suggest(near);
}

void Resolver::member(NodeIndex node)
{
    const Node &n = ast_[node];

    // THE CHAIN FIRST, AND IT IS ASKED FROM THE OUTSIDE IN. That is the
    // direction satellite_cache/paths.cpp already walks and it is what makes
    // `satellite.time.now().some_function()` fall out with no rule of its own:
    // the outer chain does not match, so the receiver is resolved and the
    // member is a selector on whatever it came back as.
    const cache::PathMatch found = path_of(node, false);
    if (found.found()) {
        info(node).path = found.id;
        info(node).origin = took_ ? Origin::Cached : Origin::Walked;
        info(node).type = found.id;
        return;
    }

    // ROOTED AT `satellite` AND STOPPED PARTWAY, which is the error
    // satellite_cache/paths.cpp names and declines to raise: "a misspelled word
    // under `satellite` is a program M7 refuses with M5's did-you-mean over the
    // node the segment failed under; what a cache owes it is the program
    // UNCHANGED." This is that refusal, and it does NOT go on to resolve the
    // receiver -- every segment inside a broken path would fail the same way
    // and one bad word would print four carets.
    if (found.under != words::kNoPath) {
        // A NAME THIS PROGRAM DECLARED UNDER A LANGUAGE NODE, WHICH M9 FOUND
        // WAS UNREACHABLE. DESIGN §7.2 reserves `satellite.library` for shared
        // and global state and the parser numbers a global when it meets the
        // declaration -- parser_declarations.cpp hands `NodeId::LIBRARY` in as
        // the owner. Nothing could READ one back: `language_path` walks the
        // FROZEN table, so `satellite.library.total` stopped at `total` and got
        // S0521, "not a word the language has under satellite.library". True of
        // the language and false of the program.
        //
        // IT WAS INVISIBLE UNTIL SOMETHING COULD HOLD A VALUE. M7 numbered the
        // declaration and `satl --resolve` printed the number; a global that
        // can be declared and not read is a global nothing can tell apart from
        // a working one until there is an evaluator, and M9 is that. The op
        // that reads one is evaluator/operations.cpp's op_global.
        //
        // `found.at == node` IS WHAT KEEPS THIS NARROW. The walk reports which
        // Member node named the segment it failed on, so this only answers when
        // the failing segment is THIS node's own name -- `satellite.library
        // .total.foo` still stops at `total` and is still refused, because the
        // outer node is not where the walk gave up.
        if (found.at == node) {
            const words::PathId declared = words_.find(
                static_cast<words::NodeId>(found.under), ast_.text_of(node));
            if (declared != words::kNoPath && !words::is_language_word(declared)) {
                info(node).path = declared;
                info(node).origin = Origin::Bound;
                return;
            }
        }
        // AND A TOPIC NAMES A WORD RATHER THAN A SHAPE -- M18, words.def's
        // fifth list. `satellite.help(satellite.variable.string.find)` stops
        // here because `find` is only ever written `find(x)`, and the walk
        // this pass uses is strict about that on purpose: `find` with no
        // arguments is not an expression. It IS a topic, and it is the topic
        // every listing help prints names it by. words_walk.hpp's word_named()
        // is the looser lookup and this is its only caller.
        //
        // `found.at == node` KEEPS IT AS NARROW AS THE ARM ABOVE. Only the
        // segment this node itself named may be answered for, so
        // `satellite.help(satellite.consle.display)` still stops at `consle`
        // and still gets "did you mean `console`?".
        if (in_topic_ && found.at == node) {
            const words::PathId shape = words::word_named(
                static_cast<words::NodeId>(found.under), ast_.text_of(node));
            if (shape != words::kNoPath) {
                info(node).path = shape;
                info(node).origin = Origin::Walked;
                info(node).type = shape;
                return;
            }
        }

        no_such_word(found);
        return;
    }

    // THE RECEIVER, AND THEN THE REST OF THIS FUNCTION. What follows reads the
    // receiver's `Info` back, so it cannot run until the whole subtree under it
    // has -- which recursion expressed by sitting after the call and an
    // explicit stack expresses as an action pushed under it.
    work_.push_back({Act::MemberDone, node, words::kNoPath});
    visit_expression(n.a);
}

void Resolver::member_done(NodeIndex node)
{
    const Node &n = ast_[node];
    const Info receiver = out_.at(n.a);
    const std::string_view word = ast_.text_of(node);

    if (receiver.arguments) {
        arguments_member(node, word);
        return;
    }

    // ONE HOP, AND THE BOUNDARY IS WORTH STATING. WORD_NUMBERS §1.5 says a
    // selector's number is reachable only THROUGH THE RECEIVER'S TYPE, and the
    // one receiver whose type this milestone knows is a name whose declaration
    // it just read. `my_list.size` is `1 4 2 2`; `f().size` is not, because
    // what `f()` returns is DESIGN §8's value model and that is M9's. A
    // milestone that inferred the second would be building M9's type rules to
    // finish M7's clause.
    if (receiver.type != words::kNoPath) {
        // A SPACESUIT'S METHOD IS FOUND IN PASS 2's TABLE AND NOT IN THE FROZEN
        // TRIE -- M26, and this arm is FIRST because `child_named` takes a
        // NodeId and a suit's path is not one. Casting a user's PathId to a
        // NodeId and walking the frozen child lists with it is exactly the
        // read past the end MILESTONES/M4.md §6 recorded, arriving from the
        // other direction.
        //
        // THE ACCESS CHECK IS HERE AND NOT AT THE CALL, because this is where
        // the receiver and the member are both in hand and where M5's caret can
        // point at the word that was reached for. DESIGN §12: "a spacesuit
        // field is reachable from inside the spacesuit and nowhere else."
        if (!words::is_language_word(receiver.type)) {
            const Suit *of = out_.suit_at(receiver.type);
            if (of == nullptr)
                return;

            if (const Method *found = of->method_named(word)) {
                if (!found->is_public && inside_ != of) {
                    problem<errors::Code::RESOLVE_SUIT_MEMBER_PROTECTED>(
                        node, word, of->name);
                    return;
                }
                info(node).path = found->path;
                info(node).origin = Origin::Bound;
                return;
            }

            // A FIELD REACHED FROM OUTSIDE IS ITS OWN SENTENCE AND NOT "no such
            // word". DESIGN §12 defers BARE FIELD ACCESS -- "accessor methods
            // only" -- so `c.n` is a form the language has decided against
            // rather than a name it cannot find, and telling somebody their
            // field does not exist would send them to add one.
            if (of->field_named(word) != nullptr) {
                problem<errors::Code::RESOLVE_NO_BARE_FIELD>(node, word,
                                                             of->name);
                return;
            }

            problem<errors::Code::RESOLVE_NO_SUCH_MEMBER>(node, word, of->name);
            return;
        }

        out_.walked++;
        const words::PathId selector =
            child_named(static_cast<words::NodeId>(receiver.type), word);
        if (selector != words::kNoPath) {
            info(node).path = selector;
            info(node).origin = Origin::Walked;
            info(node).type = selector;
        }
    }
}

void Resolver::call(NodeIndex node)
{
    const Node &n = ast_[node];

    // THREE QUESTIONS IN ONE ORDER, AND IT IS THE WRITER'S ORDER. `satl --satc`
    // asks the same three of the same tree one milestone earlier, so the number
    // in a `.satc` and the number resolve arrives at are the same number by
    // construction rather than by agreement.
    //
    // ONE: is the whole call a row of the numbering? `satellite.console.input()`
    // is 1 5 2 and the parentheses are part of what that number says.
    const cache::PathMatch whole = path_of(node, true);
    if (whole.found()) {
        info(node).path = whole.id;
        info(node).origin = took_ ? Origin::Cached : Origin::Walked;
        // AN ABSORBED ARGUMENT IS PART OF THE NUMBER AND NOT AN EXPRESSION.
        // SATC §5.1 step 3: `include(satellite)` is 1 1 1 and the reserved word
        // is what the number NAMES, so there is nothing left to resolve.
        if (!whole.absorbs_argument)
            visit_arguments(node);
        return;
    }

    if (whole.under != words::kNoPath) {
        no_such_word(whole);
        return;
    }

    // TWO: is the TARGET a row, with the call still to make?
    // `satellite.console.display("x")` is 1 5 1 and the argument is the
    // program's own -- paths.hpp's own example of the distinction, and the
    // reason the two questions are asked separately rather than once.
    //
    // The answer is read off the target's `Info`, so the rest of the call waits
    // under it the way member()'s does.
    work_.push_back({Act::CallTargetDone, node, words::kNoPath});
    visit_expression(n.a);
}

void Resolver::call_target_done(NodeIndex node)
{
    const Node &n = ast_[node];

    if (out_.at(n.a).path != words::kNoPath) {
        visit_arguments(node);
        return;
    }

    // THREE: a SELECTOR call, which is the one WORD_NUMBERS §1.5 says a `.satc`
    // can never carry -- "reaching its number needs the receiver's TYPE, which
    // is not known until resolve runs". It is known here for one receiver and
    // one only; names.cpp's member() states that boundary and why it is not
    // widened.
    if (n.a != kNoNode && ast_[n.a].kind == NodeKind::Member) {
        const words::PathId under = out_.at(ast_[n.a].a).type;
        if (under != words::kNoPath && !fold_option(node, n.a, under)) {
            out_.walked++;
            const int written = static_cast<int>(ast_.list_size(n.b));
            cache::PathMatch shape = cache::shape_path(
                static_cast<words::NodeId>(under), ast_.text_of(n.a),
                written, false);
            // TWO COUNTS, BECAUSE THE ROWS COUNT TWO WAYS -- found at M11,
            // the first milestone to fold both families. A string row names
            // its WRITTEN arguments -- `find(x)` is `s.find(needle)` -- and a
            // number row names DESIGN §6.4's written-out form, receiver
            // included: `abs(a)` is `n.abs()` with `a` the receiver, which is
            // exactly what "my_list.append(x) === satellite.container.list
            // .append(my_list, x)" makes it. So a selector call matches its
            // written count first -- the surface-true reading -- and the
            // written-out count second. No type names one word at both
            // counts today, and the day one does, the first reading wins and
            // this comment is where to look.
            if (!shape.found())
                shape = cache::shape_path(static_cast<words::NodeId>(under),
                                          ast_.text_of(n.a), written + 1, false);
            if (shape.found()) {
                info(n.a).path = shape.id;
                info(n.a).origin = Origin::Walked;
            }
        }
    }

    visit_arguments(node);
}

// The refusal satellite_cache/paths.cpp names and declines to raise, written
// once because member() and call() both reach it: "a misspelled word under
// `satellite` is a program M7 refuses with M5's did-you-mean over the node the
// segment failed under; what a cache owes it is the program UNCHANGED."
//
// IT DOES NOT GO ON TO RESOLVE THE RECEIVER. Every segment inside a broken path
// would fail the same way, so one wrong word would print four carets.
void Resolver::no_such_word(const cache::PathMatch &stopped)
{
    problem<errors::Code::RESOLVE_NO_SUCH_WORD>(
        stopped.at, ast_.text_of(stopped.at),
        words::path_text(static_cast<words::NodeId>(stopped.under)));
    if (const std::string_view near =
            errors::suggest(stopped.under, ast_.text_of(stopped.at));
        !near.empty())
        suggest(near);
}

void Resolver::statement_form(NodeIndex node, words::NodeId under,
                              const char *word)
{
    // THE TWO FORMS THE PARSER DOES NOT BUILD AS A CHAIN, and there are exactly
    // two: `satellite.return(x)` is a Return node and
    // `satellite.include(satellite)` is an Include node. satellite_cache/
    // paths.hpp says the same thing about the same pair one milestone earlier
    // and for the same reason -- neither ever reaches the chain matcher, and
    // both still have to find their row.
    const Node &n = ast_[node];
    const bool has_argument = n.a != kNoNode;
    const int argc = has_argument ? 1 : 0;
    const bool reserved = has_argument && ast_[n.a].kind == NodeKind::Satellite;

    out_.walked++;
    const cache::PathMatch found = cache::shape_path(under, word, argc, reserved);
    if (found.found()) {
        info(node).path = found.id;
        info(node).origin = Origin::Walked;
    }

    // A SPACESHIP'S NAME IS THE SPACESHIP'S AND NOT A VARIABLE ANYBODY FORGOT
    // TO DECLARE. `satellite.include(cargo)` is `1 1 2`, which WORD_NUMBERS
    // §2.2 spells "a user-named spaceship" -- so walking into it as an
    // expression met S0511, "nothing called `cargo` is in scope here -- it is
    // not a parameter, not a local declared above this line, and not a capsule
    // this file declares." Every clause of that sentence is true and the
    // sentence is wrong: `cargo` was never going to be any of those three, and
    // no edit to the program could make it one.
    //
    // THE OTHER TWO SPELLINGS ALREADY ANSWERED CORRECTLY, WHICH IS HOW IT WAS
    // FOUND. `include("cargo")` and `include(satellite.console)` reach the
    // compiler and get S0720 naming M25; only the bare identifier -- the one
    // spelling both DESIGN §3 and WORD_NUMBERS use -- was diverted into scope
    // lookup on the way. Leaving the name alone puts all three on the same
    // sentence, and it stays right after M25 lands: a spaceship is looked up
    // among spaceships, never among locals.
    const bool names_a_spaceship =
        found.found() &&
        found.id == static_cast<words::PathId>(words::NodeId::INCLUDE_SPACESHIP) &&
        has_argument && ast_[n.a].kind == NodeKind::Name;

    if (!found.absorbs_argument && !names_a_spaceship)
        visit_expression(n.a);
}

void Resolver::main_parameter(NodeIndex decl, std::string_view spelling,
                              Slot slot)
{
    // ONLY `satellite.main`, WHICH IS A DECISION AND NOT A NARROWING. DESIGN
    // §7.7 puts the object at `satellite.library.main.arguments` and describes
    // it as "the language handing the PROGRAM everything it knows about the
    // machine it woke up on" -- `satellite.main`'s parameter, and no other. A
    // capsule of the user's own with a parameter called `args` is holding
    // whatever its caller passed, and the M6 draft's version -- which
    // recognises the seven spellings in every capsule -- would give that
    // parameter the machine's answers instead. That is DESIGN §1.1's "behind
    // their back" with the wrong value in the variable.
    if (frame_ == nullptr ||
        frame_->capsule != static_cast<words::PathId>(words::NodeId::MAIN))
        return;

    out_.walked++;
    if (child_named(words::NodeId::LIBRARY_MAIN, spelling) ==
        static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS)) {
        frame_->arguments = slot;
        if (!bindings_.empty())
            bindings_.back().arguments = true;
        info(decl).arguments = true;
        info(decl).origin = Origin::Walked;
        info(decl).path =
            static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS);
        return;
    }

    // DESIGN §7.7's OPEN QUESTION, ANSWERED. "Declaring a parameter named X
    // gets a plain list with no properties, silently. Under §9 that silence is
    // wrong -- the language should say so." It is said here and ONLY where
    // somebody plainly meant the object: the suggester has to come back with
    // one of the spellings first, so `argvs` is refused and `input_lines` is an
    // ordinary list and is left alone.
    //
    // `argv` WAS THIS ERROR'S WORKED EXAMPLE UNTIL 2026-09-09 AND IS NOW A
    // SPELLING, added to words.def by the author as §7.7 said only the author
    // could. The rule did not change; which side of it `argv` sits on did.
    const std::string_view near =
        errors::suggest(static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN),
                        spelling);
    if (near.empty() || near == "()")
        return;

    problem<errors::Code::RESOLVE_ALMOST_ARGUMENTS>(decl, accepted_spellings(),
                                                    spelling);
    suggest(near);
}

void Resolver::arguments_member(NodeIndex node, std::string_view word)
{
    // THE FIELDS COME OUT OF THE NUMBERING AND ARE NOT A LIST IN THIS FILE.
    // DESIGN §7.7 writes seven of them and words.def has all seven as nodes --
    // `arguments.machine.threads` is `1 14 1 1 1 3`, six numbers deep, which
    // WORD_NUMBERS §4 calls the clearest argument in the language for §1.3
    // refusing a segment limit. The M6 draft keeps a `kValidProps[]` array of
    // the same seven strings, which is a second place they live and the one
    // that goes stale when §7.7's open question about v1's other 33 is settled.
    const words::PathId parent = out_.at(ast_[node].a).path;
    if (parent == words::kNoPath)
        return;

    // PAST A LEAF FACT, SAY NOTHING AND LET THE ONE HOP SAY IT -- M20, and
    // the empty list is what found it. `argz.machine.threads.to_string()` runs
    // this function with `threads` `1 14 1 1 1 3` as the parent: a leaf with no
    // children, so the sentence below came out as "the arguments object has no
    // `to_string` on it ... and  is what this build has", with nothing between
    // "and" and "is".
    //
    // AND THE ADVICE IT OWED WAS A DIFFERENT SENTENCE ENTIRELY. `threads` is a
    // NUMBER, `to_string` is a number's method `1 6 4 6`, and what stops the
    // fold is WORD_NUMBERS §1.5's one hop -- a selector folds through a
    // DECLARED name and a fact two members deep is not one. Leaving the node
    // unresolved is what lets compile_expressions.cpp say exactly that, on
    // both the bare reading and the call.
    //
    // THE FLAG STILL GOES ON THE LEAF, WHICH IS WHY THIS IS A GUARD AND NOT A
    // NARROWING OF THE MARK. `arguments.count()` needs `count` marked: without
    // it method_receiver() reads the call as DESIGN §6.4's method sugar, hands
    // the object over as argument 0 and refuses with S0722 -- which is the bug
    // this milestone's third commit fixed.
    if (words::first_child(static_cast<words::NodeId>(parent)) == words::kNoPath)
        return;

    const words::PathId field =
        child_named(static_cast<words::NodeId>(parent), word);
    out_.walked++;
    if (field != words::kNoPath) {
        info(node).arguments = true;
        info(node).origin = Origin::Walked;
        info(node).path = field;
        return;
    }

    // AND THEN THE OBJECT'S OWN METHODS, WHICH ARE THE OPPOSITE CASE AND MUST
    // NOT BE MARKED `arguments` -- PLAN M20 asks for this line by name.
    // `argz.machine.threads` is a FACT about the process: it takes no
    // receiver, compiles as a module constant, and the flag below is what
    // tells compile_expressions.cpp's method_receiver() to leave it alone.
    // `argz.length()` is a METHOD on the object, folded through
    // `satellite.container.arguments` `1 4 3` -- the receiver's TYPE, which is
    // §1.5's one hop -- and it needs the object as argument 0. So the flag
    // stays false here and `type` is set instead, which is exactly what the
    // ordinary selector fold forty lines up does.
    //
    // THE FACTS ARE ASKED FOR FIRST, AND NOTHING IS SPELLED BOTH WAYS -- the
    // order is a tie-break for a tie that no longer exists, and it is written
    // down because the day it does exist again it will be silent. `count` was
    // the one collision: `1 14 1 1 9` was spelled `count` for eight hours on
    // 2026-09-11, beside `satellite.container.arguments.count` `1 4 3 2`, and
    // off `satellite.main`'s parameter this loop answered the fact -- 3, where
    // the help line the same milestone wrote promised 37.
    //
    // THE AUTHOR SPLIT THE WORDS RATHER THAN LET THE ORDER DECIDE IT, and
    // words.def's note at `1 14 1 1 9` carries the reasoning: `length` is how
    // long the command line is, `count` is how many entries there are, each
    // meaning one thing everywhere (DESIGN §1). So a word appended under
    // `arguments` that is also one of the ten would be shadowed HERE, quietly,
    // and whoever appends it should read this instead of finding out.
    //
    // AND ONLY OFF THE OBJECT ITSELF, WHICH IS THE NARROWING THIS CLAUSE
    // NEEDS. This function runs at every depth -- `machine` is marked and so
    // `threads` comes through it too -- so an unguarded lookup would make
    // `argz.machine.length()` resolve to `1 4 3 1` and then fail one layer
    // later with a sentence about receivers. A selector is on the OBJECT, the
    // way `size` is on a list and not on what a list holds.
    const bool on_the_object =
        parent == static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS);
    const words::PathId selector =
        on_the_object ? child_named(words::NodeId::CONTAINER_ARGUMENTS, word)
                      : words::kNoPath;
    if (selector != words::kNoPath) {
        info(node).origin = Origin::Walked;
        info(node).path = selector;
        info(node).type = selector;
        return;
    }

    // A SHAPED SELECTOR IS NOT REFUSED HERE, IT IS LEFT FOR THE ARITY ROAD.
    // `child_named` answers only rows with an EMPTY argument list, which is
    // numbers.cpp's rule and the reason `1 4 3 1`'s `length` was found above
    // and `1 4 3 6`'s `has(k)` was not. The shapes are matched by ARITY, in
    // call_target_done(), against the receiver's type -- so this function's
    // job for one is to say nothing and let the call carry on. Refusing would
    // report "the arguments object has no `has` on it" about a row three lines
    // of this same file just listed.
    if (on_the_object)
        for (words::PathId c = words::first_child(words::NodeId::CONTAINER_ARGUMENTS);
             c != words::kNoPath; c = words::next_sibling(c)) {
            const words::NodeId child = static_cast<words::NodeId>(c);
            if (!words::arguments_of(child).empty() &&
                words::spelling_of(child) == word)
                return;
        }

    std::string has;
    for (words::PathId c = words::first_child(static_cast<words::NodeId>(parent));
         c != words::kNoPath; c = words::next_sibling(c)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<words::NodeId>(c));
        if (spelling.empty())
            continue;
        if (!has.empty())
            has += ", ";
        has += spelling;
    }
    // THE TEN ARE PART OF THE ANSWER WHERE THEY ARE REACHABLE. A sentence
    // listing what the object holds and leaving out half of what can be asked
    // of it is the kind of advice S0723's note calls worse than none -- so the
    // selectors are appended off the object and left out one level down, which
    // is exactly where each is true.
    if (on_the_object)
        for (words::PathId c = words::first_child(words::NodeId::CONTAINER_ARGUMENTS);
             c != words::kNoPath; c = words::next_sibling(c)) {
            // THE WORD AND NOT THE SHAPE, so `has(k)` reads `has` beside
            // `machine` -- the listing is what a reader may WRITE after the
            // dot, and the arguments come from their own line of the source.
            const std::string_view spelling =
                words::spelling_of(static_cast<words::NodeId>(c));
            // AND NEVER TWICE. `count` is spelled both ways on purpose --
            // `1 14 1 1 9` counts the command line, `1 4 3 2` counts every
            // entry -- and a sentence that listed it twice would read as a
            // transcription mistake rather than as the one thing a reader can
            // type.
            if (spelling.empty() || has.find(std::string(spelling)) != std::string::npos)
                continue;
            has += ", ";
            has += spelling;
        }
    problem<errors::Code::RESOLVE_NO_SUCH_ARGUMENT_FIELD>(node, word, has);
    if (const std::string_view near = errors::suggest(parent, word); !near.empty())
        suggest(near);
}

} // namespace satellite::resolve
