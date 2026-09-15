#pragma once

// The scalars, behind the table -- PLAN M11. The join between the value
// types and the evaluator, on satellite_console/handlers.hpp's model exactly:
// the evaluator must not know what `upper` does and satellite_string must not
// know what a Machine is, so the rows that connect them live in a module
// whose whole job is being the join.
//
// WHAT INSTALLS HERE: `satellite.bool.true` `1 17 2` and `.false` `1 17 1` --
// DESIGN §6.1's module constants, each a path that evaluates without a call,
// each costing "a handlers[path_id] entry and nothing else" (PLAN §8's M11
// entry, in those words) -- the sixteen `satellite.variable.string` methods
// at `1 6 1 1`+, the fifteen `satellite.variable.number` methods at
// `1 6 4 1`+, and since M12 the four `satellite.variable.variant` methods at
// `1 6 14 1`+. The number rows are M8's in PLAN §8's ledger, because M8 built
// everything they answer WITH; what M11 adds is the row itself, which could
// not exist before M9's table did. The variant rows are DESIGN §8.7's asking
// vocabulary, and variant_methods.cpp says why they are the only rows with no
// receiver check.
//
// ONE ROW REFUSES ON PURPOSE, NAMING A MILESTONE. `split` `1 6 1 10`
// answers a `satellite.container.list` and there is no list until M16. A row
// that says so beats an empty row's S0721, because the empty row can only
// say "a later milestone" and this one knows exactly which. Three more --
// `power` `1 6 4 10`, `truncate` `1 6 4 13`, `sqrt` `1 6 4 14` -- refused
// this way until M15 chose the rounding rule, and answer since it landed;
// number_methods.cpp's own note carries what they answer with.

namespace satellite::scalars {

// Install every scalar row into eval::Handlers::table(). Idempotent the way
// table().install is: installing twice writes the same rows twice.
void install_handlers();

} // namespace satellite::scalars
