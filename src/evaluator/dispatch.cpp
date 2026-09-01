// The handler table. See evaluator/dispatch.hpp.
//
// THE FILE IS SHORT BECAUSE DESIGN §4 DID THE WORK. Every language-owned
// operation already has a small dense integer, so "which handler" is an array
// index and there is nothing here to be clever about. PLAN §1.1's seven-arm
// string chain is what this replaces and the difference is the numbering, not
// the code below it.

#include "evaluator/dispatch.hpp"

namespace satellite::eval {

Handlers &Handlers::table()
{
    // A FUNCTION-LOCAL STATIC AND NOT A NAMESPACE-SCOPE ONE, so the table is
    // built the first time it is asked for rather than before main(). PLAN
    // §4.3's startup floor is measured on every milestone and a global
    // constructor is exactly the thing that moves it -- `satl --version` must
    // not pay for a table it never reads.
    static Handlers one;
    return one;
}

void Handlers::install(words::PathId path, Handler handler)
{
    if (path >= rows_.size())
        rows_.resize(path + 1);
    if (rows_[path].fn == nullptr)
        installed_++;
    rows_[path] = handler;
}

void Handlers::clear()
{
    rows_.clear();
    installed_ = 0;
}

} // namespace satellite::eval
