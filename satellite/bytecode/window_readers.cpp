// satellite/bytecode/window_readers.cpp -- A VALUE GOING IN, as the C++ the
// desk wants: text, on or off, a size, a place, and a progress bar's
// percentage. window_readers.hpp says how the six window files were cut.
//
// EVERY REFUSAL HERE NAMES WHAT WAS ASKED FOR AND WHAT WAS GIVEN, because these
// are the sentences a person reads at the worst moment.

#include "window_readers.hpp"

#include "../satellite_variable_number/number_conversions.hpp"

#include <string>
#include <utility>

namespace satellite004 {

namespace fast = number_fast_path;

// Text going in. A NUMBER WHERE TEXT IS EXPECTED IS ITS DIGITS, which is
// file_calls.cpp's rule and the author's: `satellite.window.new(5, 80, 24)` is
// a window titled "5" rather than a refusal about kinds.
bool text_of(const Value &value, std::string &out, const std::string &what, ExpressionContext &context)
{
    if (value.is_string()) { out = value.text_utf8(); return true; }
    if (const satellite_number *number = value.as_number()) { out = fast::to_text(*number); return true; }
    context.refuse(types_do_not_meet, what + " takes text, and was given " + value.kind_name());
    return false;
}

// ---------------------------------------------------------------------------
// A PROGRESS BAR'S PERCENTAGE, BOTH WAYS (GTK-4).
// ---------------------------------------------------------------------------
//
// A PERCENTAGE IS HELD AS ITSELF TIMES 10^32 (satellite_percentage.hpp), so the
// whole of it -- 100% -- is 10^34. The desk speaks MILLIONTHS, so one millionth
// of the whole is exactly 10^34 / 10^6 = **10^28**, and both conversions are a
// multiply or a divide by that one number. NOTHING ROUNDS ON THIS SIDE: the only
// place precision is lost is GTK's own double, which is what the millionths are
// there to fence off.
//
// WHY A PERCENTAGE AT ALL: because satellite has one, the author added it
// himself on 2026-09-17, and a progress bar is the thing it was made to say.
// `p.value(50%)` is what a person means; `p.value(0.5)` is a binary fraction
// this language does not have.
namespace {

const satellite_number &a_millionth_of_the_whole()
{
    static const satellite_number value =
        satellite_number(10000000000000000ull) * satellite_number(1000000000000ull);   // 10^16 * 10^12
    return value;
}

} // namespace

Value a_percentage_of(long long int millionths)
{
    satellite_percentage out;
    const unsigned long long int magnitude =
        millionths < 0 ? static_cast<unsigned long long int>(-millionths)
                       : static_cast<unsigned long long int>(millionths);
    out.scaled = satellite_number(magnitude, millionths < 0) * a_millionth_of_the_whole();
    return Value::of_percentage(std::move(out));
}

// AND BACK. A percentage finer than a millionth is TRUNCATED and that is said
// out loud rather than discovered: 0.00000012% and 0.00000019% set the same
// pixel, because a progress bar is drawn from a double and no screen has a
// million pixels of width. A program that wants the number it wrote back
// unchanged should keep it; this is what the BAR is at.
bool millionths_of(const satellite_percentage &from, long long int &out)
{
    satellite_number quotient, remainder;
    if (satellite_number::divide(from.scaled, a_millionth_of_the_whole(), quotient, remainder) != success)
        return false;
    if (!fast::fits_a_count(quotient))
        return false;
    const unsigned long long int got = fast::as_count(quotient);
    if (got > 9223372036854775807ull)
        return false;
    out = from.negative() ? -static_cast<long long int>(got) : static_cast<long long int>(got);
    return true;
}

// ON OR OFF, GOING IN. A NUMBER WHERE A BOOL IS EXPECTED IS 0 FOR OFF AND
// ANYTHING ELSE FOR ON, which is text_of's rule pointing the other way -- and it
// is not a convenience, it is the only way to write one today. SATELLITE HAS NO
// `true` AND NO `false` TO TYPE: a bool comes out of a comparison or out of
// `.ok`, and `c.on(1 < 2)` is not a sentence anybody should have to write. That
// gap is the LANGUAGE's and is written up as the author's in
// GTK_AND_NO_DEPENDENCIES.md GTK-3; accepting a number here is what makes the
// checkbox usable until he rules.
bool on_of(const Value &value, bool &out, const std::string &what, ExpressionContext &context)
{
    if (value.is_bool()) { out = value.as_bool(); return true; }
    if (const satellite_number *number = value.as_number()) { out = !number->is_zero(); return true; }
    context.refuse(types_do_not_meet, what + " takes 1 to turn it on and 0 to turn it off, and was "
                                             "given " + value.kind_name());
    return false;
}

// A SIZE. Negative is refused where it is written, rather than reaching GTK as
// an enormous count -- file_calls.cpp learned that one the hard way, where
// `f.truncate(-1)` quietly did nothing.
bool size_of(const Value &value, unsigned long long int &out, const std::string &what,
             ExpressionContext &context)
{
    const satellite_number *number = value.as_number();
    if (number == nullptr) {
        context.refuse(types_do_not_meet, what + " takes a number of pixels, and was given " + value.kind_name());
        return false;
    }
    if (number->negative()) {
        context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                           ", and a window has no negative size");
        return false;
    }
    out = fast::fits_a_count(*number) ? fast::as_count(*number) : ~0ull;
    return true;
}

// A PLACE, OR ANY OTHER WHOLE NUMBER THAT MAY BE NEGATIVE. Unlike a size, a
// NEGATIVE one is meaningful -- a centre off the left of the window, a slider
// that runs from -50.
//
// `units` IS WHAT THE NUMBER IS OF, and it is a parameter because this reader is
// borrowed. It was written for `.append`'s across and down and says "a number of
// pixels"; a slider's value is not pixels, and `s.value takes a number of
// pixels` is a sentence that is wrong in the one place a person is reading
// carefully.
bool place_of(const Value &value, long long int &out, const std::string &what,
              ExpressionContext &context, const char *units)
{
    const satellite_number *number = value.as_number();
    if (number == nullptr) {
        context.refuse(types_do_not_meet, what + " takes " + units + ", and was given " + value.kind_name());
        return false;
    }
    const unsigned long long int size = fast::fits_a_count(*number) ? fast::as_count(*number) : ~0ull;
    if (size > 2147483647ull) {
        context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                           ", which is further than any screen reaches");
        return false;
    }
    out = number->negative() ? -static_cast<long long int>(size) : static_cast<long long int>(size);
    return true;
}

#if SATELLITE_HAS_WINDOW == 0
// THE ONE SENTENCE THIS satl SAYS WHEN IT WAS BUILT WITHOUT A WINDOW. It names
// what to do about it, because "not built" with no way forward is the refusal a
// person can do nothing with.
Value no_window_here(const std::string &what, ExpressionContext &context)
{
    context.refuse(not_built_yet,
                   what + ": this satl was built without a window -- pkg-config found no gtk4 when it "
                          "was made. Install gtk4-devel (AlmaLinux/RHEL: dnf install gtk4-devel; "
                          "Debian/Ubuntu: apt install libgtk-4-dev) and build again");
    return Value();
}
#endif

} // namespace satellite004
