#pragma once
// satellite/satellite_variable_float/float_precision.hpp -- HOW MANY DIGITS A FLOAT
// KEEPS, ON EACH SIDE OF ITS POINT, AND HOW MANY IT SHOWS.
//
// (the author, 2026-09-22) "a precision that it gets from arguments ... should we
// divide the precision to arguments.float.whole(4096) and
// arguments.float.decimal(4096) so users can set different precisions? I dunno, I
// think we should have the different values thing". So there are two rows, and
// this is where the running interpreter holds them.
//
// ONE HOLDER, SET ONCE, AND NOT A PRECISION CARRIED BY EVERY VALUE. A float is
// made in three places that have no ExpressionContext to read a row through --
// the literal (float_values.cpp's float_literal), the object layer's arithmetic
// (object_float.cpp) and the display -- and all three must round the same way, so
// the rows are copied here once, where satl gives the walker its arguments
// (structured-library.cpp, beside `state.arguments = &arguments`), before any
// program line or prompt line runs. Nothing writes it after that, so every thread
// reads the same three numbers and none of them needs a lock.
//
// Carrying the precisions on each value was the other way, and it is what changes
// if the author ever wants two floats in one program kept at two precisions: the
// struct grows two fields, and float_from_scaled reads them from its operands
// rather than from here. Today every float in a run is kept the one way the rows
// say, which is what one row per side means.
//
// A CALLER WITH NO ARGUMENTS AT ALL -- a test binary that never runs satl's start
// -- gets the defaults below, exactly as infinity_calls.cpp falls back to the
// author's 128 when there is no config behind the state.

namespace satellite004 {

struct float_precisions {
    // THE WHOLE SIDE: the most significant digits kept left of the point. Past
    // them the low digits round to zeros -- the value keeps its size and is never
    // refused. 4096 is the author's own number for it.
    unsigned long long int whole = 4096;

    // THE DECIMAL SIDE: the most places kept right of the point. A division that
    // does not end is cut here, the last place rounded half away from zero
    // (SATELLITE_INFINITY.md Q17b). 128 is arguments.infinity's width, so a float
    // always fits inside an infinity's count with no second rounding when the two
    // are mixed (the recommendation to the author, reversible in
    // satellite_config.hpp).
    unsigned long long int decimal = 128;

    // THE PLACES DISPLAY SHOWS: arguments.infinity_display, 32, as
    // SATELLITE_INFINITY.md Q43 says for "a float on its own". Shown rounded, held
    // in full. If the author rules that a float shows every place it holds, this
    // becomes `decimal` and the row is not read.
    unsigned long long int shown = 32;
};

// THE ONE IN USE. Written by float_precision_from (float_values.cpp) at start-up
// and read everywhere else.
inline float_precisions float_precision_in_use{};

} // namespace satellite004
