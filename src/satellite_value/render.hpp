#pragma once

// A value, as characters -- PLAN M9. The other half of satellite_value/.
//
// SEPARATE FROM value.hpp BECAUSE IT READS THE MACHINE. Turning a number into
// digits is arithmetic; turning a STRING into text resolves DESIGN §5's six
// live codes, which means asking system_facts/ who is running this and how much
// memory the machine has. value.hpp is a type and three predicates over it and
// depends on nothing; this file is the one place in the module that opens
// /proc. satellite_number/render.cpp is the same seam one module over.
//
// THE READ IS FRESH EVERY TIME AND THAT IS DELIBERATE. facts.hpp's rule for the
// whole module is that nothing is cached, "because the question is what this
// program is using NOW"; a satellite program that prints "\memused" in a loop
// must see the number move, and one that changes directory and prints "\cwd"
// twice must see two answers. What it costs is measured in MILESTONES/M9.md --
// it is a /proc read per live code per render, and a string with no live code
// in it pays nothing at all, because live_values() is not called until one is
// found.

#include "satellite_value/value.hpp"

#include <string>

namespace satellite {

// One value as the text a person reads. DESIGN §9's sentences quote values
// through this, and M10's `satellite.console.display` will print through it.
std::string text_of(const Value &value);

// A string's codes as text, with DESIGN §5's live codes 95-100 answered from
// this machine. `satellite_string::decode()` is the same walk with the six left
// as placeholders, and the lexer uses that one -- satellite_string.hpp says
// why the two entry points exist and which is which.
std::string live_text(const SatString &text);

} // namespace satellite
