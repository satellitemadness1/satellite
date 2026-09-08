#pragma once

// The node table -- part of words.hpp, which includes this file. See there for
// why a consumer of words.def is the point.
//
// A NODE IS A ROW OF words.def AND A PathId IS ITS INDEX. That is the whole of
// the representation, and it is what DESIGN §4.5 means by "the terminal node it
// lands on has an interned id": the trie walk happens once, when the source is
// read, and the uint32_t it returns is what everything downstream holds.
//
// THE ENUM AND THE ARRAY ARE THE SAME LIST TWICE, so their indices must agree
// or every lookup in this module reads the wrong row. NodeId::NONE is 0 and the
// enumerators run on from 1 in file order; kNodes carries a row 0 that is not a
// node so that kNodes[id] needs no arithmetic. NodeId::COUNT_ is a trailing
// sentinel rather than the name of the last word, because naming it would put a
// word's spelling in an assert that a later append has to remember to edit --
// and an assert edited to keep a build quiet is worse than no assert.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace satellite::words {

// One uint32_t for a whole path (DESIGN §4.5). 0 is no path.
//
// NOT THE NUMBERS PACKED INTO BIT-FIELDS, and WORD_NUMBERS §1.4 rules that out
// on purpose: the widest child list here is satellite.container.list with 25
// children, six segments of five bits would fit in 32 with two to spare, and
// then §3's user-allocated names -- a thousand capsules need ten bits -- make
// fixed-width packing impossible. This is an index into a table, so depth costs
// it nothing and satellite.library.main.arguments.machine.cores interns to the
// same four bytes as satellite.main.
using PathId = uint32_t;

inline constexpr PathId kNoPath = 0;

// Every node, in the order words.def declares them.
enum class NodeId : PathId {
    NONE = 0,
#define SAT_NODE(parent, ident, text, kind) ident,
#include "satellite_words/words.def"
    COUNT_
};

// The two kinds, read back from words.def so the values live there and only
// there. kBare is position 0 -- the parent called with no arguments, which
// WORD_NUMBERS §1.3 defines as a real number and not a piece of notation.
inline constexpr unsigned kNumbered = SAT_NUMBERED;
inline constexpr unsigned kBare = SAT_BARE;

// A row. `text` is §2.2's path column with the parent's path removed; see
// words.def for the join rule and for why a text may begin with `(`.
struct Node {
    NodeId parent;
    const char *text;
    unsigned kind;
};

inline constexpr Node kNodes[] = {
    {NodeId::NONE, "", kNumbered},   // index 0: not a node, so kNodes[id] works
#define SAT_NODE(parent, ident, text, kind) {NodeId::parent, text, kind},
#include "satellite_words/words.def"
};

inline constexpr PathId kNodeCount = sizeof(kNodes) / sizeof(kNodes[0]) - 1;

// The identifier as written in words.def, for `satl --words` and for a
// diagnostic that has to name a row rather than describe it. Metadata: nothing
// a program means depends on it.
inline constexpr const char *kIdents[] = {
    "NONE",
#define SAT_NODE(parent, ident, text, kind) #ident,
#include "satellite_words/words.def"
};

// A second spelling of a node, written relative to that node's PARENT and free
// to carry a dot (WORD_NUMBERS §2.3). An alias is not a node and takes no
// number, which is what makes PLAN M2's properties 2 and 4 hold by
// construction -- words.def carries that argument in full.
struct Alias {
    NodeId of;
    const char *text;
};

inline constexpr Alias kAliases[] = {
#define SAT_ALIAS(ident, text) {NodeId::ident, text},
#include "satellite_words/words.def"
};

inline constexpr size_t kAliasCount = sizeof(kAliases) / sizeof(kAliases[0]);

// ---------------------------------------------------------------------------
// Reading a text
// ---------------------------------------------------------------------------
//
// A row's text is one string and it holds two things: the word, and the call
// shape's argument list. They are split here rather than stored apart because
// the text is a transcription of §2.2 and splitting it in the file would make
// the two halves able to disagree with the authority independently.

// The word a text names, which is everything before `(`.
//
// EMPTY IS A REAL ANSWER and it is what tells a walk that a row cannot be
// reached by name. `satellite.include(satellite)` is stored as the text
// "(satellite)": it is include's child 1 because `satellite` is word 1 and
// extends the path, and it has no spelling of its own because an argument is
// not a word. Nothing may look such a row up by spelling; a call reaches it by
// handing over the argument.
constexpr std::string_view spelling_of(std::string_view text)
{
    const size_t open = text.find('(');
    return open == std::string_view::npos ? text : text.substr(0, open);
}

// The argument list, parentheses included, or empty when the row has none.
//
// "()" AND "" ARE DIFFERENT ANSWERS. "" is a word written with no call at all --
// `satellite.console.display`, whose number is the whole of it. "()" is a call
// written with no arguments, which §1.3 gives its own number because
// `include()` and `include(satellite)` are two things a program can ask for.
constexpr std::string_view arguments_of(std::string_view text)
{
    const size_t open = text.find('(');
    return open == std::string_view::npos ? std::string_view() : text.substr(open);
}

constexpr std::string_view text_of(NodeId id)
{
    return kNodes[static_cast<PathId>(id)].text;
}

constexpr std::string_view spelling_of(NodeId id)
{
    return spelling_of(text_of(id));
}

constexpr std::string_view arguments_of(NodeId id)
{
    return arguments_of(text_of(id));
}

constexpr NodeId parent_of(NodeId id)
{
    return kNodes[static_cast<PathId>(id)].parent;
}

constexpr bool is_bare(NodeId id)
{
    return kNodes[static_cast<PathId>(id)].kind == kBare;
}

// Whether a PathId names a word of the language rather than a user's name.
//
// THE FROZEN HALF ENDS HERE, and everything above it is allocated at parse time
// under PLAN §8.1. A caller that is about to write a PathId down anywhere that
// outlives the run has to ask this first -- WORD_NUMBERS §3 and SATC.md §3 both
// require a user name to be recorded as a name, because its number is not the
// same in the next run.
constexpr bool is_language_word(PathId id)
{
    return id != kNoPath && id <= kNodeCount;
}

// ---------------------------------------------------------------------------
// The place parameters
// ---------------------------------------------------------------------------

// No argument of this word is a place -- the answer for every row but one.
inline constexpr uint32_t kNoPlaceParameter = 0xFFFFFFFFu;

// Which written argument (0-based) receives the word's answer, or
// kNoPlaceParameter. words.def's third list is the declaration and carries
// the policy -- one row, deliberately not the start of a general facility --
// and the evaluator's compiler is the consumer: it compiles a place as a
// slot, never as an expression, and refuses before any prompt could print.
constexpr uint32_t place_parameter_of(NodeId id)
{
    uint32_t out = kNoPlaceParameter;
#define SAT_PLACE(ident, index)                                                \
    if (id == NodeId::ident)                                                   \
        out = index;
#include "satellite_words/words.def"
    return out;
}

// ---------------------------------------------------------------------------
// The unevaluated arguments
// ---------------------------------------------------------------------------

// No argument of this word is a topic -- the answer for every row but one.
inline constexpr uint32_t kNoTopicParameter = 0xFFFFFFFFu;

// Which written argument (0-based) is read as a PATH rather than compiled as an
// expression, or kNoTopicParameter. words.def's fifth list is the declaration
// and carries the argument for it; the evaluator's compiler is the consumer,
// and it never visits that argument at all -- which is the difference from a
// place, where the argument IS compiled and the op is left unreferenced.
constexpr uint32_t topic_parameter_of(NodeId id)
{
    uint32_t out = kNoTopicParameter;
#define SAT_TOPIC(ident, index)                                                \
    if (id == NodeId::ident)                                                   \
        out = index;
#include "satellite_words/words.def"
    return out;
}

// ---------------------------------------------------------------------------
// The front-end words
// ---------------------------------------------------------------------------

// Whether the parser or the resolver recognises this word and nothing ever
// dispatches it -- words.def's fourth list, read back.
//
// THIS IS ONE THIRD OF AN ANSWER AND NOT THE ANSWER. PLAN M18's `built()` is
// this, plus a handler row, plus an assigner row, plus anything with a built
// word underneath it -- and the other three are tables the evaluator owns, so
// the predicate itself cannot live in this module. satellite_help/built.hpp is
// where the four are put together, and it is the only caller of this function.
constexpr bool is_front_end(PathId id)
{
    bool out = false;
#define SAT_BUILT(ident, listing)                                              \
    if (id == static_cast<PathId>(NodeId::ident))                              \
        out = true;
#include "satellite_words/words.def"
    return out;
}

// Whether a word that IS built is nevertheless never offered in a listing.
//
// FALSE FOR EVERYTHING BUT ONE ROW, AND THE DEFAULT IS THE SAFE DIRECTION. A
// word nobody has thought about shows up in the listing rather than vanishing
// from it, so the failure this can have is a word being advertised that should
// not be -- visible, and someone will say so -- rather than a word quietly
// missing, which is the failure a help text cannot recover from. The one row is
// `satellite.returns`: it answers when asked about and is not offered, because
// it is optional and is not how a capsule is written here.
constexpr bool is_unlisted(PathId id)
{
    bool out = false;
#define SAT_BUILT(ident, listing)                                              \
    if (id == static_cast<PathId>(NodeId::ident) && (listing) == 0)            \
        out = true;
#include "satellite_words/words.def"
    return out;
}

// ---------------------------------------------------------------------------
// The words that take a literal option
// ---------------------------------------------------------------------------

// Whether `word` under `parent` is a word WORD_NUMBERS §1.5's fold applies to
// -- words.def's sixth list, read back.
//
// ASKED AS (PARENT, WORD) AND NOT AS A NODE, because that is the question the
// resolver has at the moment it has to answer it: it holds the receiver's
// declared type and the selector's text, and the node is what it is trying to
// find. The list stores the node whose SPELLING is the word, which carries both
// halves in one identifier -- so `CONTAINER_LIST_SORT_0` says "under a list,
// `sort` takes options" and no second table of parents is needed.
//
// A GUESS IS WHAT THIS REPLACES, and words.def's own note carries the two words
// the guess was wrong about. The short version is that "a sibling is spelled
// `<word>_<something>`" is a fact about spelling and the fold needs a fact
// about meaning: `sort_down` IS `sort` with an option and `remove_first` is not
// `remove` with one, and nothing in the characters can tell them apart.
constexpr bool takes_options(PathId parent, std::string_view word)
{
    // THE INTEGER COMPARE GATES EVERYTHING, AND THAT IS THE WHOLE COST FOR
    // ALMOST EVERY CALL. `parent` is the receiver's declared type as a PathId,
    // so a selector on a string, a number, a map or a file fails the first
    // `&&` and never looks at a character. Only a call on a LIST reaches the
    // spelling compare, and only a call spelled `sort` gets past it.
    //
    // AND IT RETURNS THE MOMENT IT KNOWS, rather than setting a flag and
    // reading the rest of the list. With one row that is the same machine code;
    // with twenty it is the difference between a lookup and a sweep, and the
    // day somebody appends the twentieth is not the day to notice.
#define SAT_OPTIONS(ident)                                                     \
    if (static_cast<PathId>(parent_of(NodeId::ident)) == parent &&             \
        spelling_of(NodeId::ident) == word)                                    \
        return true;
#include "satellite_words/words.def"
    return false;
}

} // namespace satellite::words
