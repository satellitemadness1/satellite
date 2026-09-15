#pragma once

// The walk -- what `satellite.help` actually prints. PLAN M18.
//
// ONE WALK AT THREE DEPTHS, WHICH IS PLAN'S OWN DESCRIPTION AND IS LITERAL
// HERE. `satellite.help` and `satellite.help()` are `answer_for(satellite)`;
// `satellite.help(x)` is `answer_for(x's node)`. There is no separate listing
// routine and no root special case: the topics a bare ask prints are the built
// CHILDREN of `satellite`, which is the same clause that prints display and
// input under `satellite.console`. The 13 topics fall out of the walk instead
// of being a second thing that has to agree with it.
//
// A NODE IS ANSWERED FOR TOGETHER WITH THE SHAPES BESIDE IT. Asking about
// `satellite.console.input` brings up `input()`, `input(prompt)` and
// `input(prompt, target)` at once, because those three are one word written
// three ways -- and PLAN says the same of `satellite.main`, which answers for
// `1 3` and `1 3 0`. The group is help_text.hpp's `head`, authored in
// help_lines/ as the `>` line each entry was written under, so the document and
// the language group the language the same way by construction.
//
// AND WHAT IS NOT BUILT IS NOT OFFERED. A child that nothing implements is
// absent from the listing -- never named, never counted -- which is the whole
// of the done-when's second check: `satellite.help(satellite.network)` refuses
// in plain words rather than printing seven shapes nobody has written. Inside a
// group the rule is one notch weaker and deliberately so: `satellite.include`
// answers, and its `(spaceship)` shape is shown carrying `not built yet`,
// because a person asking about include is better served by knowing the shape
// exists and is M25's than by a silence they cannot tell from a typo.

#include "satellite_help/built.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::help {

// Everything help says about one node: its group's entries, then a line for
// each built word underneath. `built` is walked once by the caller and handed
// in, so one ask reads the handler table once.
std::string answer_for(const BuiltSet &built, words::PathId id);

// The path as a person types it into `satellite.help(...)` -- the group's head
// with the argument list taken off, so `1 5 3` answers `satellite.console.input`
// and `1 3 0` answers `satellite.main`.
std::string query_text(words::PathId id);

} // namespace satellite::help
