#pragma once

// `satl <file>` and `satl --run <file>` -- PLAN M10, and the arm the whole
// binary has been pointing at since M1.
//
// THIS IS THE MILESTONE AT WHICH SATELLITE EXECUTES ANYTHING AT ALL. Every
// other arm in programs/ answers a question about a program -- what its tokens
// are, what it parses to, what its names bind to, what it compiles to. This one
// RUNS it, and everything from M11 on depends on it for the same reason every
// milestone after M1 depends on there being a binary: without it there is
// nothing to print through and nothing can be demonstrated.
//
// THREE THINGS IT NEEDS THAT NO EARLIER MILESTONE BUILT, and they are M10's
// whole scope: a console with a printer thread behind
// `satellite.console.display` `1 5 1`; `satellite.main` `1 3` as the place a
// program starts; and `satellite.return` `1 15` in all three shapes, of which
// the middle one -- `return(satellite)` `1 15 1` -- needed `satellite` to be a
// value, which DESIGN §8's table has always had a row for.
//
// ITS DONE-WHEN IS NOT DESIGN §3, WHICH IS WORTH SAYING OUT LOUD. §3's hello
// world declares `satellite.container.list<satellite.variable.string>
// arguments`, and an empty list is still a list -- so the parameter is M16's
// and hello world is M17's. What runs here is the BARE `satellite.main()` form,
// which is equally legal and always was: §6's grammar reads `"(" [ param_list ]
// ")"`. ~~A `main` that declares the parameter is met with a caret under it and
// the milestone that will bind it, rather than with an argument-count refusal
// that names no milestone at all.~~
//
// AND SINCE M16 IT IS MET WITH THE LIST. The caret sentence above was true from
// M10 until 2026-09-05 and is kept because it is what the split between these
// two milestones was for; the arm that raised it is gone, and the .cpp's note
// over the binding says what replaced it. M17 is where DESIGN §3 runs end to
// end, and this file is still M10's -- the milestone that RUNS a program did
// not change when the program it could run did.
//
// AND WHAT A PROGRAM ANSWERS IS NOT AN EXIT STATUS. See the note on
// status_of() in the .cpp: the four statuses in programs/opening.hpp are
// satl's, they are assigned by what a failure IS, and a program returning 2
// from `satellite.main` would mean "the command line did not name something
// satl can do".

#include <string>
#include <vector>

namespace satellite {

// Run a file. `file_at` is where the path sits in `args` -- 1 for the bare form
// and 2 for `--run` -- and everything after it is the program's own arguments.
int run_command(const std::vector<std::string> &args, size_t file_at);

} // namespace satellite
