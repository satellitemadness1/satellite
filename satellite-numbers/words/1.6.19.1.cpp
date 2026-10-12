// satellite-numbers/words/1.6.19.1.cpp -- satellite.variable.color.transparency, built into satl
//
// satellite.variable.color.transparency   1 6 19 1
// How see-through a colour is, a whole number from 0 (solid) to 99 (almost clear) -- the author, 2026-09-22:
// "my_color.transparency(0 - 99)". ONE ROW, TWO SHAPES, as the token is one: c.satellite.transparency reads it,
// a number, and c.satellite.transparency(50) sets it and answers the colour.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we still need to get the word satellite onto everything ...
// everything builtin needs the word satellite still"): `c.satellite.transparency(50)` reaches the function
// below through the function table -- bytecode/scalar_calls.cpp's run_scalar_number -- and the code in it is
// what bytecode/color_values.cpp's color_method ran for a colour's .transparency until that day, moved here
// whole.
//
// NUMBERED THE SAME DAY: a colour had no rows under satellite.variable.color, and took the next free ones,
// 1 6 19 1 to 5, for what it answered.

#include "../number_row.hpp"
#include "../../satellite/bytecode/scalar_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"
#include "../../satellite/bytecode/color_values.hpp"
#include "../../satellite/satellite_object/object_color.hpp"

namespace {

using namespace satellite004;

// `.transparency(50)` CHANGES THE VARIABLE, through `home`, and answers the colour it made, as a window's
// methods answer the window. On a colour that is NOT a name's own -- an item of a list, a value in brackets --
// there is nothing to change but a copy, and a statement that changed a copy would do nothing without a word
// said; so that is refused, naming the way that works.
Value method(const Value &receiver, Value *home, const TypeShape *, const std::vector<Value> &arguments, bool,
             const std::string &name, ExpressionContext &context)
{
    const satellite_color *color = receiver.as_color();
    const std::string spelled = name + ".satellite.transparency";
    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood, spelled + " takes one number, 0 to 99, and was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }
    if (arguments.empty())
        return Value::of_number(satellite_number(static_cast<unsigned long long int>(color->transparency)));
    unsigned int transparency = satellite_color::kSolid;
    std::string why;
    if (!a_transparency(arguments.front(), transparency, why)) {
        context.refuse(types_do_not_meet, spelled + "(...) -- " + why);
        return Value();
    }
    satellite_color *held = home == nullptr ? nullptr : home->as_color();
    if (held == nullptr) {
        const std::string written = ".satellite.transparency(" + std::to_string(transparency) + ")";
        context.refuse(not_built_yet, written + " on a color reached through " + name +
                                          " would change a copy and leave " + name +
                                          " as it was -- write it on a color's own name: "
                                          "satellite.variable.color c = ..., and then c" + written);
        return Value();
    }
    held->transparency = transparency;
    return *home;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_6_19_1(LibraryRow *row)
{
    row->name = "satellite.variable.color.transparency";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 19;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
