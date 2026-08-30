#pragma once

// What `satl --errors` prints.
//
// THE REGISTRY GETS A CONSUMER IN THE MILESTONE THAT WRITES IT. PLAN M2 made
// that a rule and said why -- the first satellite shipped three commits where
// its word registry had none, and four defects accumulated behind a guarantee
// nothing was checking -- and `satl --words` is what came of it. This is the
// same rule one registry later, and it is the stronger case of the two: a code
// exists so that somebody can LOOK IT UP, so a code registry with no way to
// look a code up is not a smaller version of the feature, it is none of it.
//
// SEPARATE FROM THE HEADERS BECAUSE IT IS THE PART THAT PRINTS, which is the
// arrangement satellite_words/ and lexical_analyzer/ both have. codes.hpp is
// constexpr data and pure functions over it and stays that way.

#include <string>

namespace satellite::errors {

// Every code, one line each, as `<code>  <severity>  <sentence>`, in the order
// errors.def declares them -- which is code order, so the output reads straight
// down against that file.
std::string dump_text();

// One code, looked up. `found` says whether it is a code satl has; a code it
// does not have is a command line naming something satl cannot do, which is the
// split `satl --words <path>` already makes.
std::string explain_text(const std::string &code, bool &found);

} // namespace satellite::errors
