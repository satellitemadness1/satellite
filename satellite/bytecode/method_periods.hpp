#pragma once
// satellite/bytecode/method_periods.hpp -- WHICH PERIOD A METHOD IS WRITTEN WITH: `.` or `.satellite.`
// (NEW_MILESTONES.md NM-8).
//
// The author, 2026-10-10: "we need to place everything that isn't under the name "satellite", like
// everything built in, under the name "satellite"" -- `.find("str")` becomes `.satellite.find()`. Then,
// on when and how: each type takes it as soon as its methods have numbers ("E. yes we do this before we
// do anything further"); the old spelling is refused, naming the new one ("D. yes"); every call of a
// chain takes it ("B yes": `s.satellite.upper().satellite.find("X")`); and his own spacesuit's capsules
// stay as he writes them ("C yes": only the language's own words take satellite).
//
// SO A METHOD IS WRITTEN `.satellite.` EXACTLY WHEN IT RUNS A NUMBER -- today a string's 24
// (string_number_of) and a list's 24 (list_number_of). Every other method keeps its `.` until its type
// is numbered: a map's, a file's, a number's, a window's. An object's capsules keep it for good.
//
// ONE RULE, ASKED IN TWO PLACES: by the walker (expression.cpp's call_method), which knows what the
// receiver holds, and by the checker (program_check.cpp's method_on_a_name), which knows what a name was
// declared as and says it before anything runs. The lexer keeps which period was written
// (satellite_method_token, bytecode_registry.cpp) and judges nothing: it does not know the receiver.

#include "container_calls.hpp"
#include "scalar_calls.hpp"
#include "number_calls.hpp"
#include "string_calls.hpp"
#include "token_codes.hpp"
#include "file_calls.hpp"
#include "thread_calls.hpp"
#include "program_calls.hpp"
#include "pointer_calls.hpp"
#include "window_calls.hpp"
#include "../satellite_object/satellite_object.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// A METHOD'S PERIOD: `.`, or `.satellite.` before a word under satellite. The lexer makes the second only
// straight before a method's own code, so a reader that asks this and then is_method_code(next) has found
// a method call written either way; which way its receiver takes is the rule below.
inline bool a_method_period(token::Code code)
{
    return code == token::method_token || code == token::satellite_method_token;
}

// The number a method runs on this kind of receiver, or 0 when it runs none -- and so is written `.`.
// ONE CASE A TYPE, added the day its words are numbered (the author, 2026-10-11: "we still need to get the
// word satellite onto everything ... everything builtin needs the word satellite still"): its switch, in its
// own file, says which method runs which number, and this is the only place that knows every type has one.
inline token::Code number_a_method_runs(satelliteObject::Kind kind, token::Code method)
{
    switch (kind) {
    case satelliteObject::string: return string_number_of(method);
    case satelliteObject::list: return list_number_of(method);
    case satelliteObject::file: return file_number_of(method);
    case satelliteObject::thread: return thread_number_of(method);
    // A PROGRAM AND A BASH LINE ARE ONE KIND, AND HAVE ONE SET OF WORDS: which of the two numbers a word runs,
    // 1 6 23 n or 1 6 24 n, is the value's own to say when it runs (program_calls.hpp) -- whether it has one
    // at all, which is all a period asks, is the same for both.
    case satelliteObject::program: return program_number_of(method);
    // AN OBJECT'S FIVE WORDS, and a pointer's, which are its object's (pointer_calls.hpp): his own
    // capsules run no number and keep their `.`, and call_member asks his spacesuit which a name is.
    case satelliteObject::user_defined: case satelliteObject::pointer: return object_number_of(method);
    case satelliteObject::window: return window_number_of(method);   // and every piece in one (2026-10-11)
    // THE OTHER SCALARS (scalar_calls.hpp), 2026-10-11.
    case satelliteObject::floating: return float_number_of(method);
    case satelliteObject::hexadecimal: return hexadecimal_number_of(method);
    case satelliteObject::binary: return binary_number_of(method);
    case satelliteObject::color: return color_number_of(method);
    case satelliteObject::fraction: return fraction_number_of(method);
    case satelliteObject::percentage: return percentage_number_of(method);
    case satelliteObject::boolean: return bool_number_of(method);
    case satelliteObject::time: return time_number_of(method);
    case satelliteObject::number: return number_number_of(method);
    // AN INFINITY'S NAME MAY HOLD A PLAIN NUMBER (SATELLITE_INFINITY.md Q24) -- the loss tests' `x.add(...)`,
    // `.reverse()` and `.string` run on one that does -- and the checker judges a name by its declaration where
    // the walker judges the value. So an infinity's period is its own words' AND the number's (number_calls.hpp),
    // or `x.satellite.add(5)` on an infinity name holding 5 would be refused by one and asked for by the other.
    // Its own are asked first: its .string is its display, and its .power_of is not the number's.
    case satelliteObject::infinity: {
        const token::Code own = infinity_number_of(method);
        return own != 0 ? own : number_number_of(method);
    }
    // A MAP'S, 2026-10-11 -- and so the index's, the arguments variable's and an info item's: one kind.
    case satelliteObject::index: return map_number_of(method);
    default: return 0;
    }
}

// WHETHER A KIND'S WORDS ARE UNDER SATELLITE AT ALL -- whether any method runs a number on it. Asked only to
// say a refusal's sentence, so its walk over the method codes (they share one high byte, token_codes.hpp)
// costs a program that is right nothing.
inline bool words_under_satellite(satelliteObject::Kind kind)
{
    for (token::Code code = 0x0B00; code <= 0x0BFF; ++code)
        if (token::is_method_code(code) && number_a_method_runs(kind, code) != 0)
            return true;
    return false;
}

// "a string's and a list's" -- every kind whose words are under satellite, for the sentence that tells a
// type whose words are not yet where to look. `count` is how many it named.
inline std::string the_kinds_under_satellite(std::size_t &count)
{
    std::vector<std::string> names;
    for (std::size_t k = 0; k < satelliteObject::how_many_kinds; ++k) {
        const auto kind = static_cast<satelliteObject::Kind>(k);
        if (kind != satelliteObject::user_defined && kind != satelliteObject::pointer && words_under_satellite(kind))
            names.push_back(std::string(satelliteObject::kind_name_of(kind)) + "'s");
    }
    count = names.size();
    std::string said;
    for (std::size_t at = 0; at < names.size(); ++at)
        said += (at == 0 ? "" : at + 1 == names.size() ? " and " : ", ") + names[at];
    return said;
}

// WHAT A NUMBER-LIKE VALUE'S CONVERSION ANSWERS: its text for .string, .binary and .hex, and nothing known
// for anything else (the walker judges it when it runs).
inline satelliteObject::Kind conversion_answers(token::Code method)
{
    return method == token::to_string_token || method == token::to_binary_token ||
                   method == token::to_hexadecimal_token
               ? satelliteObject::string
               : satelliteObject::how_many_kinds;
}

// WHAT A METHOD ANSWERS, by what its receiver is -- for the checker, which follows a chain past its first
// link only while this knows: `s.satellite.upper().find("X")` is told before anything runs that its second
// link takes `.satellite.` too (the author's B of 2026-10-10: every call of a chain). how_many_kinds is
// "not known here" -- an item of a list, a capsule's answer -- and the walker judges the rest when it runs.
// `given` is how many arguments the link was written with, asked by the one word whose answer it decides: a
// colour's .transparency reads a number, and .transparency(50) answers the colour it changed (2026-10-11).
inline satelliteObject::Kind kind_a_method_answers(satelliteObject::Kind receiver, token::Code method,
                                                   std::size_t given = 0)
{
    using O = satelliteObject;
    switch (receiver) {
    case O::string:
        switch (method) {
        case token::size_token: case token::find_token: case token::to_number_token: return O::number;
        case token::empty_token: case token::contains_token: case token::starts_with_token:
        case token::ends_with_token: return O::boolean;
        case token::split_token: return O::list;
        default: return string_number_of(method) != 0 ? O::string : O::how_many_kinds;
        }
    case O::list:
        switch (method) {
        case token::size_token: case token::index_of_token: case token::search_token: return O::number;
        case token::contains_token: case token::empty_token: return O::boolean;
        case token::join_token: return O::string;
        case token::sort_token: case token::by_name_token: case token::by_value_token: case token::reverse_token:
        case token::append_token: case token::clear_token: case token::insert_token: case token::truncate_token:
        case token::reserve_token: case token::remove_token: case token::remove_at_token:
        case token::remove_first_token: case token::remove_last_token: return O::list;   // each answers the list
        default: return O::how_many_kinds;     // an item -- .first, .last, .max -- is what the run put there
        }
    case O::index:
        switch (method) {
        case token::keys_token: case token::values_token: return O::list;
        // .reverse() ANSWERS THE KEYS BACK TO FRONT, AS A LIST (1 4 1 14) -- what the walker always answered,
        // where this table said a map, so no spelling of m.reverse().size could run (the inventory of 2026-10-11).
        case token::reverse_token: return O::list;
        case token::size_token: return O::number;
        case token::contains_token: case token::empty_token: return O::boolean;
        case token::sort_token: return O::index;   // a map in order is still a map
        case token::clear_token: case token::remove_token: case token::remove_first_token:
        case token::remove_last_token: return O::index;   // each answers the map it changed
        default: return O::how_many_kinds;     // a key -- .first, .last -- or a value, .get: what the run put there
        }
    // A FILE'S (file_calls.cpp, 2026-10-11): a count or a line's number, a line or the file's own words as
    // text, and a bool from every question and every change -- a file's changes answer whether they landed,
    // never the file, so after `f.satellite.append("a")` a chain is on a bool. Its lock answers nothing.
    case O::file:
        switch (method) {
        case token::size_token: case token::index_of_token: case token::search_token: return O::number;
        case token::first_token: case token::last_token: case token::read_all_token: case token::path_token:
        case token::error_text_token: return O::string;
        case token::lock_token: case token::unlock_token: return O::how_many_kinds;
        default: return file_number_of(method) != 0 ? O::boolean : O::how_many_kinds;
        }
    // ONE CASE A TYPE, so each is changed on its own the day it is numbered.
    case O::number:
        switch (method) {
        case token::to_string_token: case token::to_binary_token: case token::to_hexadecimal_token: return O::string;
        case token::to_number_token: case token::reverse_token: case token::abs_token: case token::floor_token:
        case token::ceil_token: case token::round_token: case token::truncate_token: case token::max_token:
        case token::min_token: case token::clamp_token: case token::digits_token: case token::bytes_token:
        case token::shift_left_method_token: case token::shift_right_method_token: return O::number;
        default: return O::how_many_kinds;     // .add, .power and .modulus answer what + ^ and % do: 2 ^ -1 is a float
        }

    // THE OTHER SCALARS (scalar_calls.hpp), each as its numbers' files answer, 2026-10-11.
    case O::floating:
        // .number is the whole number a float is (3.0 is 3; 2.5 is refused as it runs); the rest are its text.
        return method == token::to_number_token ? O::number : conversion_answers(method);

    case O::hexadecimal:
        // .number and .width are numbers, and .add IS + -- which answers a NUMBER on a hex, not a hex:
        // x00FF.satellite.add(x01) is 256 (object_hexadecimal.cpp). .reverse() is a hex, its digits turned round.
        switch (method) {
        case token::to_number_token: case token::width_token: case token::add_token: return O::number;
        case token::reverse_token: return O::hexadecimal;
        default: return conversion_answers(method);
        }

    case O::binary:
        // As a hex's: .add answers a number (b1010.satellite.add(b0001) is 11), .reverse() a binary. .width and
        // .clear are numbered and not built, so nothing after them is known.
        switch (method) {
        case token::to_number_token: case token::add_token: return O::number;
        case token::reverse_token: return O::binary;
        default: return conversion_answers(method);
        }

    case O::fraction:
        // .numerator and .denominator are a number when whole and a float when not -- 1/3's numerator is 1, and
        // 1.5/2's is 1.5 -- which only the value says, so the walker judges what follows them.
        return method == token::to_string_token ? O::string : O::how_many_kinds;

    case O::color:
        // .transparency READ is a number, 0 to 99; GIVEN one, it answers the colour it changed.
        if (method == token::transparency_token)
            return given == 0 ? O::number : O::color;
        return method == token::to_number_token ? O::number : conversion_answers(method);

    case O::percentage:
        // .add IS + on a percentage: 50%.satellite.add(10%) is 60%.
        return method == token::add_token         ? O::percentage
               : method == token::to_string_token ? O::string
                                                  : O::how_many_kinds;

    case O::time:
        return method == token::to_string_token ? O::string : O::how_many_kinds;   // its text, as shown

    case O::boolean:
        // .add IS + on a bool, and + joins a bool only with text -- so it answers text (his A, 2026-10-11).
        return method == token::to_string_token || method == token::add_token ? O::string : O::how_many_kinds;

    case O::infinity:     // its display; and the number's words, which answer on the plain number it may hold
        return method == token::to_string_token ? O::string : kind_a_method_answers(O::number, method);

    // start() and stop() answer the thread, so `t.satellite.start().satellite.join()` is judged whole before
    // the thread starts; join() and wait() answer what its capsule handed back, which only the run knows.
    case O::thread:
        return method == token::start_token || method == token::stop_token ? O::thread : O::how_many_kinds;

    // A program's -- and a bash line's: start() answers the program, ok() and pass() whether, error() why,
    // and join(), end() and their second names its exit code.
    case O::program:
        switch (method) {
        case token::start_token: return O::program;
        case token::ok_token: case token::pass_token: return O::boolean;
        case token::error_text_token: return O::string;
        case token::join_token: case token::code_token: case token::return_token: case token::end_token:
        case token::exit_token: case token::quit_token: case token::shutdown_token: return O::number;
        default: return O::how_many_kinds;
        }

    // AN OBJECT'S ok() IS A BOOL. Its pointer() and reference() answer an object of the same spacesuit,
    // whose next link may be a capsule of his under a bare `.` -- which only a reader that knows the
    // spacesuit can say, so program_check.cpp's member_of_an_object follows those links itself, and this
    // says "not known here" of them. lock() and unlock() answer nothing.
    case O::user_defined:
    case O::pointer:
        return method == token::ok_token ? O::boolean : O::how_many_kinds;

    // A WINDOW'S, AND EVERY PIECE'S IN ONE (window_methods.cpp): a doing answers the window, so the chain goes
    // on with it, and a maker answers the piece it made -- a window value too. NOT KNOWN where the answer's kind
    // depends on the brackets (`.satellite.title` reads text, `.satellite.title("x")` answers the window) or on the
    // piece (`.satellite.value` is a number from a slider and a percentage from a bar): the walker judges those.
    case O::window:
        switch (method) {
        case token::width_token: case token::height_token: case token::across_token: case token::down_token:
        case token::columns_token: case token::rows_token: return O::number;
        case token::ok_token: return O::boolean;
        case token::answer_token: return O::string;
        case token::append_token: case token::open_token: case token::close_token: case token::focus_token:
        case token::press_token: case token::resize_token: case token::colour_token: case token::background_token:
        case token::font_token: case token::every_token: case token::message_token: case token::ask_token:
        case token::choose_a_file_token: case token::menu_token: case token::item_token: case token::separator_token:
        case token::line_token: case token::box_token: case token::circle_token: case token::write_token:
        case token::arc_token: case token::clear_token: case token::save_token: case token::display_token:
        case token::home_token: case token::listen_token: case token::widgets_display_file_token:
        case token::widgets_display_input_token: case token::section_token: return O::window;
        default: return O::how_many_kinds;
        }

    default:
        return O::how_many_kinds;
    }
}

// THE SENTENCE FOR A METHOD WRITTEN WITH THE OTHER PERIOD. `receiver` is the call as written up to its
// period -- `s`, `s.satellite.upper()` -- `word` the method's name, and `kind` what the receiver is (said
// as kind_name_of says it, "a string"). `under` is whether `.satellite.` was written: asked only when that is
// not what number_a_method_runs says, so under means the method runs no number on this kind, and not under
// means it does. The checker passes the name as the program wrote it (`s.str`), the walker the registry's.
inline std::string period_written_wrong(const std::string &receiver, const std::string &word,
                                        satelliteObject::Kind kind, bool under)
{
    const std::string name = satelliteObject::kind_name_of(kind);
    const std::string kinds = name + "'s";
    // ONE OF THE FIVE WORDS EVERY OBJECT HAS (pointer_calls.hpp), written with a bare `.` on an object whose
    // spacesuit declares no capsule of that name: always with its brackets, and every object's -- a
    // pointer's too, which answers them as its object's.
    if (!under && (kind == satelliteObject::user_defined || kind == satelliteObject::pointer))
        return receiver + "." + word + "() is written " + receiver + ".satellite." + word +
               "() now -- an object's own words are under satellite";
    if (!under)
        return receiver + "." + word + " is written " + receiver + ".satellite." + word + " now -- " + kinds +
               " own words are under satellite";
    const std::string bare = receiver + ".satellite." + word + " is written " + receiver + "." + word + " -- ";
    if (kind == satelliteObject::user_defined || kind == satelliteObject::pointer)
        return bare + "an object's capsules are its spacesuit's own, and only the language's own words are "
                      "under satellite";
    // A NUMBERED TYPE'S EVERY WORD IS UNDER SATELLITE, so one that is not among them is not its at all:
    // `s.satellite.first` is told a string has no first, never sent to write s.first and be refused again.
    if (words_under_satellite(kind))
        return receiver + ".satellite." + word + " -- " + name + " has no " + word + "; its own words are written "
               "under satellite, and " + word + " is not one of them";
    std::size_t count = 0;
    const std::string numbered = the_kinds_under_satellite(count);
    return bare + kinds + " words are not under satellite yet -- " + numbered + (count == 1 ? " is" : " are");
}

} // namespace satellite004
