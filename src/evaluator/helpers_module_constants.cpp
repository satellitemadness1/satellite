// The values a satellite-rooted path answers with when it is resolved rather
// than called.
//
// Moved verbatim out of helpers.cpp, which was 779 lines.
//
// Part of src/evaluator/. See eval_internal.hpp for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// The whole language, on one screen.
//
// That is only possible because the language IS one screen: two dozen methods,
// seven module functions, four statement forms. A reference that fits is worth
// more than a reference that is complete, and here they are the same thing --
// so this is checked against the code below rather than written once and left
// to rot.

ValuePtr module_constant(const std::vector<std::string> &path)
{
    // Deliberately reachable WITHOUT parentheses. Someone who needs help is by
    // definition someone who may not remember the calling syntax, and making
    // them get it right first is the one place a language can least afford to.
    if (path.size() == 2 && path[0] == "satellite" && path[1] == "help")
        return make_value(encode_raw(help_overview()));

    // A module does NOT answer here, and the reason is worth keeping: making
    // `satellite.directory` a value broke `satellite.directory.list()`, which
    // the evaluator reads as that value with `.list()` called on it -- so the
    // whole module disappeared behind "satellite.variable.string has no method
    // list". satellite.help can be bare because nothing is nested under it;
    // every module has its commands nested under it, so the parenthesised
    // satellite.directory() in modules.cpp is the only spelling that can work.

    if (path.size() == 3 && path[0] == "satellite" && path[1] == "bool") {
        if (path[2] == "true")
            return make_value(true);
        if (path[2] == "false")
            return make_value(false);
    }
    return nullptr;
}

} // namespace satellite
