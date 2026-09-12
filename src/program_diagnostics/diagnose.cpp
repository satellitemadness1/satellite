// The fifth pass, and the list of what it asks. See program_diagnostics/diagnose.hpp.
//
// THE FILE IS SHORT BECAUSE A CATALOGUE'S INDEX SHOULD BE. Every check is a
// file beside this one and a line in the function below; what belongs HERE is
// the ORDER they are asked in and nothing else. When this grows past a screen
// it is because the analyser grew, which is the one kind of growth this module
// is supposed to have.
//
// FINDINGS ARE NOT SORTED AND THAT IS ON PURPOSE FOR NOW. The renderer prints
// them in the order they arrive, and the order they arrive is the order of this
// list -- so a person reading `--check` output sees all of one kind together.
// When a second check starts reporting on the same lines as the first, sorting
// by span is the fix, and it belongs here rather than inside a check.

#include "program_diagnostics/diagnose.hpp"

#include "program_diagnostics/diagnostics_internal.hpp"

namespace satellite::diagnostics {

std::vector<errors::Diagnostic> diagnose(const Ast &ast,
                                         const resolve::Resolved &resolved,
                                         const eval::Program &program)
{
    const Subject subject{ast, resolved, program};
    std::vector<errors::Diagnostic> found;

    // --- memory: what this program allocates and never gives back ---
    find_suit_cycles(subject, found);

    return found;
}

} // namespace satellite::diagnostics
