// A ring of spacesuits that holds itself alive. See program_diagnostics/diagnose.hpp.
//
// THE ONE LEAK THIS LANGUAGE CAN HAVE, WHICH IS WHY IT IS THE FIRST CHECK. M26
// made a spacesuit a reference type held by a count: an object dies when the
// last handle to it goes. That frees everything shaped like a tree and nothing
// shaped like a ring -- if `a` holds `b` and `b` holds `a`, each is holding the
// other's count above zero, both are unreachable from the program, and neither
// is ever freed. There is no cycle collector; DESIGN defers one. So a cycle is
// not a RISK of a leak in satellite, it IS one, and it is decidable by reading
// the field types.
//
// A WARNING AND NOT AN ERROR, deliberately. A ring of objects is a legitimate
// thing to want -- a doubly-linked list, a parent a child can look back at, a
// graph -- and the answer is `.pointer()`, not refusing the program. DESIGN
// §1.1 is that satellite never does anything behind the user's back; it is not
// that satellite second-guesses them. So this says what will happen and lets
// the run happen.
//
// WHEN `.pointer()` BECOMES A DECLARED FIELD KIND, AN EDGE THROUGH ONE STOPS
// COUNTING. A weak handle does not raise the count, so a ring with one weak
// link in it frees normally, and flagging it would train somebody to ignore
// this code. Today a field's declared type is all there is, so every field of
// suit type is an owning edge; the moment `resolve::Field` can say otherwise,
// the `continue` goes in the loop below and this comment is the reason.
//
// OWN FIELDS ONLY, WHICH IS NOT AN OPTIMISATION. resolve.hpp's `Suit` keeps a
// child's inherited fields at the FRONT of `fields`, holding the indices they
// had in the parent -- so walking them here would find every inherited edge
// once per descendant and report one ring as many rings. The parent is a node
// in this graph in its own right and its edges are walked when it is visited.

#include "program_diagnostics/diagnostics_internal.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace satellite::diagnostics {

namespace {

// One owning field, as an edge. `field` indexes the FROM suit's `fields`.
struct Edge {
    uint32_t to = 0;
    uint32_t field = 0;
};

// Which suit this path is, or -1. Linear because a file's suits are counted in
// tens -- resolve.hpp's own `suit_at()` is the same loop for the same reason,
// and a map here would be a hash built to be asked eleven questions.
int suit_index(const std::vector<resolve::Suit> &suits, words::PathId path)
{
    for (size_t i = 0; i < suits.size(); i++)
        if (suits[i].path == path)
            return static_cast<int>(i);
    return -1;
}

// `a -> b -> a`, which is the sentence's second hole. The chain is what makes
// the diagnostic actionable: a person told only that `a` leaks has to find the
// ring themselves, and it is their program, but it is three suits long.
std::string chain_text(const std::vector<resolve::Suit> &suits,
                       const std::vector<uint32_t> &ring, uint32_t back_to)
{
    std::string out;
    for (const uint32_t at : ring) {
        out += std::string(suits[at].name);
        out += " -> ";
    }
    out += std::string(suits[back_to].name);
    return out;
}

} // namespace

void find_suit_cycles(const Subject &subject, std::vector<errors::Diagnostic> &into)
{
    const std::vector<resolve::Suit> &suits = subject.resolved.suits;
    const size_t count = suits.size();
    if (count == 0)
        return;

    std::vector<std::vector<Edge>> edges(count);
    for (size_t i = 0; i < count; i++) {
        const resolve::Suit &suit = suits[i];
        for (size_t f = suit.inherited; f < suit.fields.size(); f++) {
            const resolve::Field &field = suit.fields[f];
            if (field.type == words::kNoPath)
                continue;
            const int to = suit_index(suits, field.type);
            if (to < 0)
                continue;
            edges[i].push_back({static_cast<uint32_t>(to), static_cast<uint32_t>(f)});
        }
    }

    // THE WALKER KEEPS ITS OWN STACK, which is this tree's rule wherever a walk
    // is as deep as something the user wrote. A file may declare as many suits
    // as it likes and they may chain as deep as they like; a recursive DFS here
    // would put the language's limit back in the C++ stack, which is the one
    // place satellite refuses to keep it.
    enum Colour : uint8_t { White = 0, OnTheStack = 1, Done = 2 };
    std::vector<uint8_t> colour(count, White);

    // The suit being explored and how far through its edges we are.
    struct Frame {
        uint32_t at = 0;
        uint32_t next = 0;
    };

    for (size_t root = 0; root < count; root++) {
        if (colour[root] != White)
            continue;

        std::vector<Frame> work;
        std::vector<uint32_t> path;
        work.push_back({static_cast<uint32_t>(root), 0});
        colour[root] = OnTheStack;
        path.push_back(static_cast<uint32_t>(root));

        while (!work.empty()) {
            Frame &top = work.back();
            if (top.next >= edges[top.at].size()) {
                colour[top.at] = Done;
                work.pop_back();
                path.pop_back();
                continue;
            }

            const Edge edge = edges[top.at][top.next++];

            // A BACK EDGE IS THE RING. Everything from where `to` sits in the
            // path to the top of it is holding the next one up, and this edge
            // closes it. Reported from the FIELD that closes it, because that
            // is the line somebody can change -- the other links are innocent.
            if (colour[edge.to] == OnTheStack) {
                size_t from = 0;
                while (from < path.size() && path[from] != edge.to)
                    from++;
                const std::vector<uint32_t> ring(path.begin() + static_cast<long>(from),
                                                 path.end());
                const resolve::Field &closing = suits[top.at].fields[edge.field];
                into.push_back(errors::make<errors::Code::ANALYSIS_SUIT_CYCLE>(
                    subject.span_of(closing.at), std::string(closing.name),
                    chain_text(suits, ring, edge.to)));
                continue;
            }

            if (colour[edge.to] == White) {
                colour[edge.to] = OnTheStack;
                path.push_back(edge.to);
                work.push_back({edge.to, 0});
            }
        }
    }
}

} // namespace satellite::diagnostics
