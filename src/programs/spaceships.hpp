#pragma once

// Loading, resolving and compiling a program of more than one file -- PLAN M25,
// `satellite.include(spaceship)`, built 2026-09-13.
//
// THE ORDER IS built_program.hpp's, ACROSS FILES. Every file is parsed before
// any is resolved, every file is resolved before any is compiled, and a pass
// that fails in one file stops the program before the next pass starts in
// any -- because a name from `ship.satl` that never bound makes every call to
// it in `host.satl` a refusal, and the carets would bury the one real mistake.
//
// WHERE A SPACESHIP IS FOUND IS BESIDE THE FILE THAT INCLUDES IT, and nowhere
// else. `satellite.include(ship)` in `example/host.satl` is
// `example/ship.satl`, whatever directory satl was started in; a typed line at
// the prompt, which is in no directory, looks in the current one.
//
// A FILE IS LOADED ONCE PER RUN, however many includes name it and from however
// many files -- the author: "spaceships included twice, what would be the point
// of loading them twice?" So two includes of one file share its capsules and
// its globals, and each runs its launches.

#include "programs/built_program.hpp"

#include <string>

namespace satellite {

// Called by build_source() once file 0 has parsed. Loads every spaceship file 0
// reaches, then resolves and compiles all of them with it. `report` is
// build_source's. Answers whether there is a program; out.ok is set either way.
bool build_with_spaceships(const std::string &name, Built &out, bool report);

// Whether file 0 names any spaceship at all -- the question build_source asks
// before taking the road above, so a program of one file is untouched.
bool includes_a_spaceship(const Ast &ast);

// How many spaceship includes sit at the top of a file -- in the order the
// compiler meets them, which is what Built::already_included counts against.
uint32_t top_level_spaceships(const Ast &ast);

} // namespace satellite
