// The built set. See satellite_help/built.hpp for what the four kinds are and
// why the fourth is derived rather than listed.

#include "satellite_help/built.hpp"

#include "evaluator/dispatch.hpp"

namespace satellite::help {

BuiltSet BuiltSet::now()
{
    BuiltSet out;

    // The three that are stated. A row in either table, or a row in words.def's
    // front-end list.
    for (words::PathId i = 1; i <= words::kNodeCount; i++)
        out.rows_[i] = eval::Handlers::table().find(i) != nullptr ||
                       eval::Assigners::table().find(i) != nullptr ||
                       words::is_front_end(i);

    // AND THE ONE THAT IS DERIVED, IN ONE BACKWARD PASS. A parent is declared
    // before its children -- words_invariants.hpp asserts it, and it is the
    // same property as "no node is its own ancestor" seen from the other end --
    // so walking DOWN from the last row reaches every child before its parent,
    // and one pass carries a built word all the way to the root. The forward
    // pass words_numbers.hpp does for the numbering is this one reversed, and
    // for the same reason: the order words.def is written in is what makes both
    // a single pass instead of a fixed point.
    for (words::PathId i = words::kNodeCount; i >= 1; i--)
        if (out.rows_[i]) {
            const words::PathId parent =
                static_cast<words::PathId>(words::parent_of(static_cast<words::NodeId>(i)));
            if (parent != words::kNoPath)
                out.rows_[parent] = true;
        }

    return out;
}

size_t BuiltSet::count() const
{
    size_t n = 0;
    for (words::PathId i = 1; i <= words::kNodeCount; i++)
        if (rows_[i])
            n++;
    return n;
}

} // namespace satellite::help
