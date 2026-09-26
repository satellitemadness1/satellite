#pragma once

// The value model, split across sibling headers. THIS FILE IS THE UMBRELLA:
// dozens of translation units include "satellite_value/value.hpp" and nothing
// else, so it keeps its path and its meaning — everything it declared before
// the split it still provides, in an order that compiles.
//
// The parts, in dependency order:
//
//   value_types.hpp      the handles and the struct definitions a Value holds:
//                        ValuePtr/List/Str/ListRef, MapEntry/MapBody, Time,
//                        Object, Bits, FileHandle, and suit_name
//   value_bits.hpp       §21's free functions over a Bits
//   value_arguments.hpp  ArgumentEntry/Arguments — what satellite.main is handed
//   value_result.hpp     ResultBody — what Satellite Orbit hands back
//   value_variant.hpp    ValueBase and Value. APPEND ONLY — read the comment
//                        block there before touching the alternative list
//   value_helpers.hpp    make_* constructors and as_* readers
//   value_layout.hpp     §8.7's memory model, the sizeof() static_asserts,
//                        and value_bytes
//   value_printer.hpp    ValuePrinter, to_string, and the inline printers

#include "satellite_value/value_types.hpp"
#include "satellite_value/value_bits.hpp"
#include "satellite_value/value_arguments.hpp"
#include "satellite_value/value_result.hpp"
#include "satellite_value/value_variant.hpp"
#include "satellite_value/value_helpers.hpp"
#include "satellite_value/value_layout.hpp"
#include "satellite_value/value_printer.hpp"
