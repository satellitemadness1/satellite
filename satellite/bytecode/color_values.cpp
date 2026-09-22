// satellite/bytecode/color_values.cpp -- what the walker asks about a color
// (color_values.hpp says why this is its own file; color_check.cpp is the
// checker's half, and says which spellings of a colour are read and why).
//
// (the author, 2026-09-22) "satellite.variable.color my_color = x000000 or just
// 000000 without the x, but a variable.color is always width 6 hexadecimal number,
// and then we'll add transparency in two ways, my_color.transparency(0 - 99) and
// my_color = x000000, 0-99 for transparency".
//
// BOTH OF HIS WAYS ARE HERE: color_reads_its_value reads the comma, and
// color_method reads `.transparency(50)`. SATURATION IS NOT BUILT. He asked "aren't
// we going to need saturation as well" and answered himself the same breath: "we
// may just have to leave saturation out for our color variable, it makes the .satl
// code simply too complicated". Leaning is not ruling, so it is carried as a
// question, and nothing here stands in its way.

#include "color_values.hpp"
#include "color_reading.hpp"

#include "file_calls.hpp"
#include "word_codes.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

using namespace color_reading;

bool a_color_name(token::Code holds) { return holds == word::code_of(1, 6, 19); }

// A TRANSPARENCY IS A WHOLE NUMBER FROM 0 TO 99, the author's "0 - 99", both ends
// in. Said the same way from his two ways of writing one, so a person meets one
// sentence for one rule.
bool a_transparency(const Value &given, unsigned int &out, std::string &why)
{
    const satellite_number *number = given.as_number();
    const bool in_range = number != nullptr && !number->negative() && number->fits_one_limb() &&
                          number->limb(0) <= satellite_color::kMostSeeThrough;
    if (!in_range) {
        why = std::string(kWhatATransparencyIs) + ", and it was given " +
              (number != nullptr ? number->to_text() : std::string(given.kind_name()));
        return false;
    }
    out = static_cast<unsigned int>(number->limb(0));
    return true;
}

Value text_value(const std::string &written, const std::string &spelled, ExpressionContext &context)
{
    satellite_string text;
    std::size_t bad_offset = 0;
    const signed long long int made = satellite_string::from_utf8(written, text, bad_offset);
    if (made != success) {
        context.refuse(made, spelled + " could not be written as text");
        return Value();
    }
    return Value::of_string(std::move(text));
}

} // namespace

// SIX DIGITS WITHOUT THE x, AND THEN THE COMMA. On the line that declares a colour
// the lexer has already made the digits a hex, so this only ever reads them itself
// on a later `c = ...`, by the rule color_check.cpp's table gives: a number token
// that is six digits, or a name no variable has that is six hex digits, standing
// alone. A VARIABLE ALWAYS WINS: if a name is declared, it is the variable.
bool color_reads_its_value(token::Code holds, const std::vector<std::bitset<16>> &row, std::size_t &at, Value &value,
                           ExpressionContext &context)
{
    if (!a_color_name(holds))
        return false;

    const token::Code first = code_at(row, at);
    bool read = false;
    if (first == token::number_token || first == token::name_token) {
        std::size_t k = at;
        const std::string text = text_at(row, k);
        const bool digits_not_a_name =
            first == token::number_token ? all_digits(text)
                                         : all_hex_digits(text) && context.variables.find(text) == context.variables.end();
        satellite_color color;
        if (digits_not_a_name && the_value_ends(code_at(row, k)) && satellite_color::from_digits(text, color)) {
            value = Value::of_color(color);
            at = k;
            read = true;
        }
    }
    if (!read) {
        value = evaluate_expression(row, at, context);
        if (context.code != success)
            return true;
    }
    if (code_at(row, at) != token::comma_token)
        return true;

    // "my_color = x000000, 0-99 for transparency". The colour is made first, so the
    // transparency has something to belong to; a value that cannot be a colour is
    // left as it is, and the walker's store refuses it in the words it refuses
    // every value in -- the transparency is not the mistake there.
    const std::size_t transparency_at = at + 1;
    ++at;
    const Value given = evaluate_expression(row, at, context);
    if (context.code != success)
        return true;
    std::string why;
    if (color_on_store(holds, value, why) != success)
        return true;
    unsigned int transparency = satellite_color::kSolid;
    if (!a_transparency(given, transparency, why)) {
        context.refuse(types_do_not_meet, why, transparency_at);
        return true;
    }
    value.as_color()->transparency = transparency;
    return true;
}

// A SIX-DIGIT HEX GIVEN TO A COLOUR NAME BECOMES A COLOUR -- the author's "just a
// hexadecimal number of a mandatory width, 6 digits". It is also how the literal
// x000000 arrives, since an x literal is a hex (hexadecimal_values.cpp). Any other
// width is refused rather than widened, which was 003's rule for a colour too
// (CONSOLE_NOT_A_COLOUR: "xF80 is refused rather than widened").
//
// A NUMBER IS REFUSED, and told the six digits it would be: a number has no width,
// and a colour's digits are the whole of it. A colour given to any other name is
// that name's business, and type_shape.cpp says what it holds.
signed long long int color_on_store(token::Code holds, Value &value, std::string &why)
{
    // A COLOUR GIVEN TO A NUMBER NAME IS NOT QUIETLY ITS WORTH, as a hex's is: a
    // colour is not a number that happens to be written in hex. The sentence says
    // where the worth is. Every other name judges a colour for itself.
    if (holds == word::code_of(1, 6, 4) && value.is_color()) {
        why = "it was given the color " + value.as_color()->written() +
              " -- a color is not a number, and .number on it is what its six digits are worth";
        return types_do_not_meet;
    }
    if (!a_color_name(holds) || value.is_color())
        return success;
    if (const satellite_hexadecimal_number *hex = value.as_hexadecimal()) {
        if (hex->negative()) {
            why = "it was given " + hex->written() + " -- a color has no sign: x000000 is black, and there is no "
                  "color below it";
            return types_do_not_meet;
        }
        satellite_color color;
        if (hex->width != 6 || !satellite_color::from_digits(hex->digits(), color)) {
            why = "it was given " + hex->written() + ", which is " + how_many_digits(hex->width) + " -- " + kSixDigits;
            return types_do_not_meet;
        }
        value = Value::of_color(color);
        return success;
    }
    if (const satellite_number *number = value.as_number()) {
        why = "it was given the number " + number->to_text() + " -- " + kSixDigits;
        if (!number->negative() && number->fits_one_limb() && number->limb(0) <= 0xFFFFFFull) {
            const satellite_color as_digits{static_cast<unsigned int>(number->limb(0)), satellite_color::kSolid};
            why += ", and " + number->to_text() + " written as one is x" + as_digits.digits();
        }
        return types_do_not_meet;
    }
    why = std::string("it holds ") + value.kind_name() + " -- " + kSixDigits;
    return types_do_not_meet;
}

// A COLOUR'S METHODS. `.transparency` reads how see-through it is and
// `.transparency(50)` sets it; the four conversions are answered here and never
// by the ones every type shares, which reach a number first and would lose the
// six-digit width (`.hex` on x00FF00 is "00FF00", not "FF00").
//
// `.transparency(50)` CHANGES THE VARIABLE, through `slot`, and answers the colour
// it made, as a window's methods answer the window. On a colour that is NOT a
// name's own -- an item of a list, a value in brackets -- there is nothing to
// change but a copy, and a statement that changed a copy would do nothing without
// a word said; so that is refused, naming the way that works.
Value color_method(token::Code method, const Value &receiver, Value *slot, const std::vector<Value> &arguments,
                   bool, const std::string &name, ExpressionContext &context, bool &answered)
{
    answered = false;
    const satellite_color *color = receiver.as_color();
    if (color == nullptr)
        return Value();
    const std::string spelled = name + "." + method_spelling(method);

    if (method == token::transparency_token) {
        if (arguments.size() > 1) {
            context.refuse(satl_line_not_understood, spelled + " takes one number, 0 to 99, and was given " +
                                                         std::to_string(arguments.size()));
            return Value();
        }
        if (arguments.empty()) {
            answered = true;
            return Value::of_number(satellite_number(static_cast<unsigned long long int>(color->transparency)));
        }
        unsigned int transparency = satellite_color::kSolid;
        std::string why;
        if (!a_transparency(arguments.front(), transparency, why)) {
            context.refuse(types_do_not_meet, spelled + "(...) -- " + why);
            return Value();
        }
        satellite_color *held = slot == nullptr ? nullptr : slot->as_color();
        if (held == nullptr) {
            const std::string written = ".transparency(" + std::to_string(transparency) + ")";
            context.refuse(not_built_yet, written + " on a color reached through " + name +
                                              " would change a copy and leave " + name +
                                              " as it was -- write it on a color's own name: "
                                              "satellite.variable.color c = ..., and then c" + written);
            return Value();
        }
        held->transparency = transparency;
        answered = true;
        return *slot;
    }

    const bool a_conversion = method == token::to_string_token || method == token::to_number_token ||
                              method == token::to_binary_token || method == token::to_hexadecimal_token;
    if (!a_conversion) {
        context.refuse(types_do_not_meet, spelled + " -- " + kWhatAColorHas);
        return Value();
    }
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood,
                       spelled + " takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }
    answered = true;
    if (method == token::to_number_token)
        return Value::of_number(satellite_number(static_cast<unsigned long long int>(color->rgb)));
    const std::string written = method == token::to_string_token ? color->written()
                                : method == token::to_binary_token ? color->bits()
                                                                    : color->digits();
    Value text = text_value(written, spelled, context);
    answered = context.code == success;
    return text;
}

} // namespace satellite004
