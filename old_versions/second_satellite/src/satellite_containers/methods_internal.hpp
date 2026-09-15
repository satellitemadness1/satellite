#pragma once

// What satellite_containers' handler files share -- the install halves, and
// the checks every method makes before it answers. Not a door onto this
// module; satellite_containers/handlers.hpp is. The same split
// satellite_scalars keeps, for the same reason.
//
// A METHOD'S FIRST QUESTION IS WHAT IT WAS ASKED OF, scalars' rule verbatim:
// the compiler proved the receiver's DECLARATION was the right type -- that is
// how the selector got its number -- but a declaration is not a value. The
// slot may hold nothing (S0714), and nothing stops a program assigning a
// number into a name declared as a list, because satellite checks VALUES and
// not annotations. So every handler re-asks at run time, through the helpers
// below, and each sentence is written once.

#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_value/value.hpp"

#include <cstdint>
#include <string>

namespace satellite::containers {

// What the refusal sentences quote: the selector under the caret.
std::string asked(eval::Machine &m);

// The receiver or an argument as the list it must be, or a refusal in the
// machine and false. `who` is which of the handler's values to check -- 0 is
// the receiver.
bool list_at(eval::Machine &m, const Value *arguments, uint32_t who,
             const List **out);

// The same, for a map.
bool map_at(eval::Machine &m, const Value *arguments, uint32_t who,
            const MapBody **out);

// An argument as a position -- a whole number no less than zero that a
// machine word can hold -- or a refusal and false. Positions count from 0,
// scalars' rule: `l[0]` is the first element.
bool position_at(eval::Machine &m, const Value *arguments, uint32_t who,
                 unsigned long long *out);

// An argument as a map key, canonicalised -- or S0727 and false. The refusal
// is raised here so the key rule is stated the same way wherever a program
// breaks it (v1's key_type_error, one tree over).
bool key_at(eval::Machine &m, const Value *arguments, uint32_t who,
            std::string *canonical);

// The install halves handlers.cpp sums. Each row is a words.def path, so the
// table stays a property of the build -- dispatch.hpp's warning about user
// PathIds is why nothing here may ever take one.
void install_list_methods();
void install_list_sorting();
void install_map_methods();

} // namespace satellite::containers
