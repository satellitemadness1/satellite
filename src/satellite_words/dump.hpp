#pragma once

// What `satl --words` prints.
//
// THE REGISTRY GETS A CONSUMER IN THE MILESTONE THAT WRITES IT. PLAN M2 asks
// for this by name and says why: the first satellite shipped three commits
// where its word registry had none, and four defects accumulated behind a
// guarantee nothing was checking. A dump is the cheapest possible consumer and
// it is not a toy -- it is the only way to read the numbering the machine
// actually has, as opposed to the one WORD_NUMBERS.md says it should have.
//
// SEPARATE FROM THE HEADERS BECAUSE IT IS THE ONLY PART THAT PRINTS. Everything
// under satellite_words/ except this file is constexpr data and pure functions
// over it, which is what lets a future disassembler, a `.satc` reader or the
// bootstrap read the numbering without dragging <string> and stdio in behind
// it. That was true of the first satellite's registry and worth keeping.

#include <string>

namespace satellite::words {

// Every node, one line each, as `<path>  <number>`, in the order words.def
// declares them -- which is numbering order, so the output can be read straight
// down against WORD_NUMBERS §2.2.
std::string dump_text();

// One path, walked. The number when it resolves; when it does not, which
// segment failed and what was under it, which is DESIGN §4.6's answer and what
// M5's reporter will be built from.
//
// `resolved` IS AN OUT-PARAMETER RATHER THAN SOMETHING THE CALLER READS OUT OF
// THE TEXT. The first draft had main() decide by searching the returned string
// for a word it happened to contain, which is the same mistake as inferring a
// row's kind from its spelling: it makes a formatting change into a behaviour
// change, silently, in a caller that has no reason to be looking.
std::string walk_text(const std::string &path, bool &resolved);

} // namespace satellite::words
