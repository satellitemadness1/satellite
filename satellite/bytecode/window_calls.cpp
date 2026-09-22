// satellite/bytecode/window_calls.cpp -- satellite.window's words and a window's
// own methods. The header says why they are the object model's and not
// libraries, and why this is the one file that knows whether satl has a window.

#include "window_calls.hpp"

#include "word_codes.hpp"
#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_variable_window/window_desk.hpp"

#include <string>
#include <utility>

// SET BY make_support/047-window.mk. 0 means pkg-config found no gtk4 when this
// satl was built, and the window sources were not compiled at all -- so every
// word here still LEXES, still CHECKS, and refuses at the moment it would draw.
#ifndef SATELLITE_HAS_WINDOW
#define SATELLITE_HAS_WINDOW 0
#endif

namespace satellite004 {
namespace {

using token::Code;
namespace fast = number_fast_path;

Code window_word() { return word::code_of(1, 27); }

// ---------------------------------------------------------------------------
// EVERY WORD UNDER satellite.window, IN ONE TABLE (GTK-0's recipe).
// ---------------------------------------------------------------------------
//
// A WIDGET IS A ROW HERE AND A `Piece` NEXT DOOR, and nothing else in satl knows
// the numbers. is_window_word, the arity, the sentence a wrong argument count
// gets and `.append`'s own refusal are every one of them written FROM this
// table, so a widget added here cannot be left out of any of them -- which is
// how the hand-written list in program_check.cpp went stale in an afternoon and
// what one table is the fix for.
//
// `satellite.window` ITSELF IS NOT IN HERE. It is the family name, it answers
// nothing, and it is refused by name -- a row with no arity and no Piece would
// be a word that exists, which it is not.
struct AWord {
    unsigned int number;                  // its number under 1 27
    const char *spelling;                 // satellite.window.<this>
    std::size_t arity;
    satellite_window::Piece makes;        // what a program gets back
    // WHAT ITS ARGUMENTS ARE. `arity` says how many and this says of what, and
    // together they are every shape a window word has: nothing, one line of
    // words, two numbers, one list, one file, a size. A widget added with a
    // seventh shape is one more value here and one more branch in
    // call_window_word -- not a seventh field and not a second table.
    //
    // `a_size` IS NOT `numbers` (GTK-15): a slider's two numbers are a range and
    // the least must be the smaller, while a canvas's two are a width and a
    // height and 400 by 300 is the ordinary case. One shape with a flag would
    // have been the range rule refusing every landscape canvas.
    enum Takes { words, numbers, items, a_file, a_size } takes_what;
    const char *takes;                    // and what to say when the count is wrong
};

constexpr AWord kWords[] = {
    {1, "new", 3, satellite_window::window, AWord::words,
     "satellite.window.new takes a title, a width and a height: "
     "satellite.window.new(\"my window\", 800, 600)"},
    {2, "button", 1, satellite_window::button, AWord::words,
     "satellite.window.button takes the text on it: satellite.window.button(\"press me\")"},
    {3, "label", 1, satellite_window::label, AWord::words,
     "satellite.window.label takes the text it shows: satellite.window.label(\"a line of text\")"},
    {4, "text_box", 1, satellite_window::text_box, AWord::words,
     "satellite.window.text_box takes the text already in it, and \"\" for an empty one: "
     "satellite.window.text_box(\"\")"},
    {5, "text_area", 1, satellite_window::text_area, AWord::words,
     "satellite.window.text_area takes the text already in it, and \"\" for an empty one: "
     "satellite.window.text_area(\"\")"},
    {6, "checkbox", 1, satellite_window::checkbox, AWord::words,
     "satellite.window.checkbox takes the text beside it: satellite.window.checkbox(\"I agree\")"},
    {7, "switch", 0, satellite_window::a_switch, AWord::words,
     "satellite.window.switch takes nothing -- a switch says nothing, it is only on or off: "
     "satellite.window.switch()"},
    {8, "slider", 2, satellite_window::slider, AWord::numbers,
     "satellite.window.slider takes the least and the most it runs between: "
     "satellite.window.slider(0, 100)"},
    {9, "number_box", 2, satellite_window::number_box, AWord::numbers,
     "satellite.window.number_box takes the least and the most it runs between: "
     "satellite.window.number_box(1, 12)"},
    {10, "progress", 0, satellite_window::progress, AWord::numbers,
     "satellite.window.progress takes nothing -- how far along it is, is .value: "
     "satellite.window.progress()"},
    {11, "choice", 1, satellite_window::choice, AWord::items,
     "satellite.window.choice takes a list of what a person may pick: "
     "satellite.window.choice({\"red\", \"green\"})"},
    {12, "row", 0, satellite_window::row, AWord::words,
     "satellite.window.row takes nothing -- what goes in it is .append'ed: satellite.window.row()"},
    {13, "column", 0, satellite_window::column, AWord::words,
     "satellite.window.column takes nothing -- what goes in it is .append'ed: "
     "satellite.window.column()"},
    {14, "grid", 0, satellite_window::grid, AWord::words,
     "satellite.window.grid takes nothing -- what goes in it is .append'ed at a cell: "
     "satellite.window.grid()"},
    {15, "picture", 1, satellite_window::picture, AWord::a_file,
     "satellite.window.picture takes the name of a file to show: "
     "satellite.window.picture(\"logo.png\")"},
    {16, "scroll", 0, satellite_window::scroll, AWord::words,
     "satellite.window.scroll takes nothing -- the one piece it shows is .append'ed: "
     "satellite.window.scroll()"},
    {17, "frame", 1, satellite_window::frame, AWord::words,
     "satellite.window.frame takes the words on its edge, and \"\" for none: "
     "satellite.window.frame(\"a title\")"},
    {18, "split", 0, satellite_window::split, AWord::words,
     "satellite.window.split takes nothing -- the two pieces either side are .append'ed: "
     "satellite.window.split()"},
    // A MENU TAKES ITS HEADING, and that is GTK's ruling: a menu bar drops a
    // top-level item that has no submenu, silently, so a menu with no word on
    // the bar would be a menu that is nowhere (window_menu.cpp).
    {19, "menu", 1, satellite_window::menu, AWord::words,
     "satellite.window.menu takes the word that goes on the bar: satellite.window.menu(\"File\")"},
    {20, "canvas", 2, satellite_window::canvas, AWord::a_size,
     "satellite.window.canvas takes how wide and how tall it is: satellite.window.canvas(400, 300)"},
    // A SET OF TABS TAKES NOTHING: each tab is named by its PIECE's own .title
    // (GTK-16), so there is nothing here for the adding to carry.
    {21, "tabs", 0, satellite_window::tabs, AWord::words,
     "satellite.window.tabs takes nothing -- each piece .append'ed is a tab, named by that "
     "piece's .title: satellite.window.tabs()"},
    // THE RADIO (GTK-3, built 2026-09-22 as the recommendation): one word that
    // draws many buttons, made from a list as a choice is, and asked `.chosen`
    // as a choice is. The other spelling -- `.group(other_checkbox)` on a
    // checkbox -- stays the author's to ask for; nothing here forecloses it.
    {22, "one_of", 1, satellite_window::one_of, AWord::items,
     "satellite.window.one_of takes a list of what a person may pick one of: "
     "satellite.window.one_of({\"small\", \"large\"})"},
};

// A LINEAR SCAN, AND IT STAYS ONE. This is asked once a window word in a
// program, not once a line, and seven rows -- eighteen one day -- is nothing
// beside the code_of() call it is comparing against.
//
// A WORD THAT TAKES NOTHING IS **TWO ROWS** IN words.tsv, and both answer the
// one row here. That is satellite.infinity's own shape -- `1 26` is the name and
// `1 26 0` is the call -- and it exists for the refusal rather than for the
// call: with only `satellite.window.switch()` registered, writing
// `satellite.window.switch("on")` matches no word at all and is refused as **"no
// capsule named switch"**, which tells a person nothing. Measured 2026-09-21;
// `satellite.window.nosuchword("on")` says exactly the same thing, which is what
// proved it was the unregistered NAME and not the switch.
const AWord *word_at(Code code)
{
    for (const AWord &row : kWords) {
        if (code == word::code_of(1, 27, row.number))
            return &row;
        if (row.arity == 0 && code == word::code_of(1, 27, row.number, 0))
            return &row;
    }
    return nullptr;
}

// THE WORDS THAT MAKE SOMETHING TO PUT IN A WINDOW, written out of the table so
// that `.append`'s refusal names the widget added this morning without anybody
// having remembered to come back here. The window is skipped: a window does not
// go inside a window.
//
// THE FIRST ONE IS SPELLED OUT AND THE REST ARE NOT -- `satellite.window.button,
// .label or .text_box` -- because this goes inside a sentence a person is
// reading at the worst moment, and eighteen full calls would be a paragraph
// where a list was wanted. One of them spelled in full is enough to show the
// shape.
std::string the_words_that_make_a_piece()
{
    std::string out;
    std::size_t left = 0;
    for (const AWord &row : kWords)
        if (row.makes != satellite_window::window)
            ++left;
    for (const AWord &row : kWords) {
        if (row.makes == satellite_window::window)
            continue;
        if (out.empty())
            out = "satellite.window." + std::string(row.spelling) + "(\"text\")";
        else
            out += (left == 1 ? " or ." : ", .") + std::string(row.spelling);
        --left;
    }
    return out;
}

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
const satellite_number &a_millionth_of_the_whole()
{
    static const satellite_number value =
        satellite_number(10000000000000000ull) * satellite_number(1000000000000ull);   // 10^16 * 10^12
    return value;
}

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
              ExpressionContext &context, const char *units = "a number of pixels")
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

} // namespace

bool is_window_word(Code code) { return code == window_word() || word_at(code) != nullptr; }

std::size_t window_word_arity(Code code)
{
    const AWord *row = word_at(code);
    return row == nullptr ? 0 : row->arity;
}

std::string window_word_takes(Code code)
{
    const AWord *row = word_at(code);
    if (row != nullptr)
        return row->takes;
    return "satellite.window is the family name and is not a call -- "
           "satellite.window.new(...) makes a window";
}

// THE SENTENCE A PERSON GETS WHEN THEY ASK A WINDOW FOR SOMETHING IT HAS NO
// METHOD FOR, and it lives here rather than in program_check.cpp because this is
// the file that knows. The checker used to carry its own copy of a list like
// this and it went stale the same afternoon it was written.
std::string window_methods_are()
{
    return "a window has .append(piece, across, down), .close(), .focus(), .title(\"text\"), "
           ".resize(wide, tall), .fullscreen and .ok; every piece has .width, .height, "
           ".colour(\"#00ff88\"), .background(...) and .font(\"a face\", 12); "
           "a row, a column or a grid has .append too -- .append(piece) for a row, "
           ".append(piece, across, down) for a grid's cell; "
           "a piece in one has .text; a checkbox or a switch has .on; a slider, a number box or "
           "a progress bar has .value; a choice or a one-of has .chosen -- all read bare and written with "
           "brackets; a button has .pressed(a_capsule) and .press(); anything a person can change "
           "has .changed(a_capsule); and a window has .closed(a_capsule) and "
           ".every(a_capsule, 1000) and .key(a_capsule); anything that is not a button has "
           ".clicked(a_capsule); and a window has .message(\"saying\"), "
           ".ask(a_capsule, \"a question?\") and .answer; a menu has .item(a_capsule, \"Open\"), "
           ".separator() and .menu(a_menu) for a menu inside it, and a window has .menu(a_menu); "
           "a canvas has .line(from_across, from_down, to_across, to_down), .box(across, down, "
           "wide, tall), .circle(across, down, radius), .arc(across, down, radius, from_degrees, "
           "to_degrees), .write(across, down, \"words\"), .clear() and .save(\"picture.png\"), "
           "and its pen has .thickness(3) and .outline(1); anything that is not a button has "
           ".across and .down, where the last click on it landed; a piece going into a set of "
           "tabs is named by its .title(\"a name\"), and the tabs' .chosen is the one in front "
           "(GTK_AND_NO_DEPENDENCIES.md Part 2G lists every piece and what it does)";
}

int window_method_arity(Code method)
{
    switch (method) {
    case token::append_token:  return 3;     // the piece, and where its centre goes
    case token::close_token:   return 0;
    case token::focus_token:   return 0;
    case token::title_token:   return 1;     // written; read with no brackets
    case token::pressed_token: return 1;     // the capsule's name; read with no brackets
    case token::press_token:   return 0;     // a click has nothing to say
    case token::text_token:    return 1;     // written; read with no brackets (GTK-1)
    case token::on_token:      return 1;     // written; read with no brackets (GTK-3)
    case token::value_token:   return 1;     // written; read with no brackets (GTK-4)
    case token::chosen_token:  return 1;     // written; read with no brackets (GTK-5)
    case token::path_token:    return 1;     // written; read with no brackets (GTK-6)
    case token::changed_token: return 1;     // the capsule's name; read with no brackets (GTK-9)
    case token::closed_token:  return 1;     // the capsule's name; read with no brackets (GTK-9)
    case token::resize_token:  return 2;     // a width and a height (GTK-8)
    case token::width_token:   return 0;     // a question, read bare or bracketed (GTK-8)
    case token::height_token:  return 0;     // a question, read bare or bracketed (GTK-8)
    case token::fullscreen_token: return 1;  // written; read with no brackets (GTK-8)
    case token::colour_token:     return 1;  // GTK-10; there is no reading one back
    case token::background_token: return 1;
    case token::font_token:       return 2;  // the face and the size
    case token::every_token:      return 2;  // the capsule's NAME, then how often (GTK-13)
    case token::key_token:        return 1;  // the capsule's name; read bare for the last key (GTK-14)
    case token::clicked_token:    return 1;  // the capsule's name; read with no brackets (GTK-14)
    case token::message_token:    return 1;  // what to say (GTK-11)
    case token::ask_token:        return 2;  // the question, then the capsule's NAME
    case token::answer_token:     return 0;  // a question, read bare or bracketed
    case token::menu_token:       return 1;  // the menu to put across the top, or inside this one (GTK-12)
    case token::item_token:       return 2;  // the capsule's NAME, then the words on the item
    case token::separator_token:  return 0;  // a line under the items so far (GTK-12)
    case token::line_token:       return 4;  // from a point to a point (GTK-15)
    case token::box_token:        return 4;  // a corner, a width and a height
    case token::circle_token:     return 3;  // a centre and a radius
    case token::write_token:      return 3;  // a point and the words
    case token::arc_token:        return 5;  // a centre, a radius and two angles (2026-09-22)
    case token::thickness_token:  return 1;  // the pen's width; read bare for what it is
    case token::outline_token:    return 1;  // whether the pen outlines; read bare for what it is
    case token::across_token:     return 0;  // a question: where the last click landed
    case token::down_token:       return 0;  // a question: where the last click landed
    case token::clear_token:      return 0;  // nothing drawn any more
    case token::save_token:       return 1;  // the file to write the picture to
    case token::ok_token:      return 0;
    default:                   return -1;
    }
}

int window_method_also_takes(Code method)
{
    // `.append` IS THE FIRST METHOD WITH TWO RIGHT COUNTS (GTK-7). A window and
    // a grid place what goes in them, so they take the piece and where it goes;
    // a row and a column put their pieces one after another and take just the
    // piece.
    //
    // WHICH ONE IS RIGHT IS THE RECEIVER'S AND THE CHECKER CANNOT KNOW IT. A
    // `satellite.variable.window` name may hold a window or a row, and which it
    // holds is not decided until the line that makes it RUNS. So the checker
    // lets both counts through and window_append() names the wrong one with the
    // piece it actually got -- which is the one place that can.
    return method == token::append_token ? 1 : -1;
}

// THREE METHODS NAME A CAPSULE NOW (GTK-9), and every rule WIN-11 wrote for
// `.pressed` holds for all of them without a line changing: the name is read as
// written and not as text, only one name may stand there, the capsule must
// exist, and it may declare at most the piece and its window. That is what this
// one predicate buys -- extending it extended the checker.
bool window_method_takes_a_capsule_name(Code method)
{
    return method == token::pressed_token || method == token::changed_token ||
           method == token::closed_token || method == token::every_token ||
           method == token::key_token || method == token::clicked_token ||
           method == token::ask_token || method == token::item_token;
}

// AND WHETHER ANYTHING MAY FOLLOW THAT NAME (GTK-13). `.pressed`, `.changed`
// and `.closed` take one capsule's name and nothing else; `.every` takes the
// name and then how often. The name comes FIRST in both shapes, which is not a
// style choice -- it is where the checker looks for it, and it is where
// expression.cpp reads a name instead of working out a value.
// THE NAME COMES FIRST IN EVERY ONE OF THEM, and that is a LANGUAGE rule rather
// than a convenience (GTK-11). `.ask(when_answered, "delete it?")` reads less
// like English than the other way round, and it is spelled this way because
// every capsule-naming method spells it this way: the checker looks for a name
// at the first argument and expression.cpp reads a name there instead of working
// out a value. One rule a person can hold in their head beats one line that
// reads slightly better.
bool window_method_takes_more_after_the_name(Code method)
{
    return method == token::every_token || method == token::ask_token || method == token::item_token;
}


// ---------------------------------------------------------------------------
// THE WORDS AND THE METHODS. Everything below exists twice -- once for a satl
// with a window and once for one without -- and the two halves must answer the
// same shapes, which is why they sit in one file under one #if rather than in
// two files that could drift apart.
// ---------------------------------------------------------------------------
#if SATELLITE_HAS_WINDOW

Value call_window_word(Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    const AWord *row = word_at(code);
    if (row == nullptr) {
        context.refuse(satl_line_not_understood, window_word_takes(code));
        return Value();
    }
    if (arguments.size() != row->arity) {
        context.refuse(satl_line_not_understood, std::string(row->takes) + " -- it was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }

    std::string why;
    const std::string called = "satellite.window." + std::string(row->spelling);

    // EVERY PIECE MADE FROM ONE LINE OF TEXT GOES THROUGH HERE, and a widget
    // added to the table above needs no branch of its own -- the Piece in its row
    // is what window_pieces.cpp turns into a GtkWidget. `new` is the one word
    // that is not this shape, and it falls past.
    if (row->makes != satellite_window::window) {
        // A PIECE'S WORD TAKES ITS WORDS OR IT TAKES NOTHING, and the table's own
        // arity is what says which -- `satellite.window.switch()` is the first
        // piece with nothing to say. A third shape one day is a third branch;
        // two do not need one.
        WindowHandle made;
        long long int least = 0, most = 0;
        bool an_empty_list = false;
        if (row->takes_what == AWord::a_file) {
            std::string path;
            if (!text_of(arguments[0], path, called, context))
                return Value();
            made = window_piece_of_a_file(row->makes, path, why);
        } else if (row->takes_what == AWord::items) {
            // A LIST OF WORDS. Each item goes through text_of, so a list of
            // numbers is a choice of their digits -- the author's own rule for a
            // number where text is expected, applied one item at a time rather
            // than invented again here.
            const ListHandle *given = arguments[0].as_list();
            if (given == nullptr || *given == nullptr) {
                context.refuse(types_do_not_meet,
                               called + " takes a list, and was given " + arguments[0].kind_name());
                return Value();
            }
            std::vector<std::string> wanted;
            wanted.reserve((*given)->items.size());
            for (const Value &item : (*given)->items) {
                std::string one;
                if (!text_of(item, one, called + "'s items", context))
                    return Value();
                wanted.push_back(std::move(one));
            }
            an_empty_list = wanted.empty();
            made = window_piece_of_items(row->makes, wanted, why);
        } else if (row->takes_what == AWord::a_size) {
            // A WIDTH AND A HEIGHT (GTK-15). The same reader as a place, for
            // the same reason a slider's range borrows it: a whole number that
            // must fit a screen's worth of int.
            if (!place_of(arguments[0], least, called + "'s width", context) ||
                !place_of(arguments[1], most, called + "'s height", context))
                return Value();
            made = window_piece_of_a_size(row->makes, least, most, why);
        } else if (row->takes_what == AWord::numbers) {
            // TWO NUMBERS OR NONE. place_of is borrowed on purpose rather than
            // copied: a slider's least and most are the same kind of thing as a
            // position -- a whole number that may be negative and must fit a
            // screen's worth of int -- and one reader for both is one place for
            // that rule to live.
            if (row->arity == 2 &&
                (!place_of(arguments[0], least, called + "'s least", context, "a whole number") ||
                 !place_of(arguments[1], most, called + "'s most", context, "a whole number")))
                return Value();
            made = window_piece_of_numbers(row->makes, least, most, why);
        } else {
            std::string text;
            if (row->arity == 1 && !text_of(arguments[0], text, called, context))
                return Value();
            made = window_piece_of_text(row->makes, text, why);
        }
        if (made == nullptr) {
            // A BAD RANGE IS THE PROGRAM'S AND NO SCREEN IS THE MACHINE'S, told
            // apart the same way satellite.window.new tells them apart: by
            // testing the very thing the factory checks BEFORE it ever asks for
            // a display. Asking only whether the word takes numbers got this
            // wrong and got it wrong quietly -- a slider on a machine with no
            // screen exited 13 while printing "there is no display to draw on",
            // which is a code and a sentence disagreeing about what happened.
            // A FILE THAT IS NOT THERE IS THE PROGRAM'S, and so is an empty
            // list and a range of nothing. No screen is the machine's. They are
            // told apart the same way satellite.window.new tells them apart --
            // by testing the very thing the factory checks -- except for a
            // picture, which needs GDK started before it can read a file at all,
            // so `why` is the only thing that knows.
            const bool a_bad_file = row->takes_what == AWord::a_file &&
                                    why.find("no display to draw on") == std::string::npos;
            // AN EMPTY LIST IS THE PROGRAM'S; A LIST WITH NO SCREEN TO DRAW IT
            // ON IS THE MACHINE'S (found 2026-09-22, by running the one-of
            // headless). This line once read `row->takes_what == AWord::items`,
            // so a valid choice on a machine with no display printed S110
            // LINE_NOT_UNDERSTOOD over a sentence saying there was no display
            // -- the code and the sentence disagreeing, again. The test is the
            // very thing the factory checks, as it is for every shape here.
            const bool the_program = (row->takes_what == AWord::numbers && row->arity == 2 &&
                                      least >= most) ||
                                     (row->takes_what == AWord::a_size &&
                                      (least <= 0 || most <= 0 || least > 32767 || most > 32767)) ||
                                     an_empty_list || a_bad_file;
            // A FILE GETS A FILE'S CODE. satellite.variable.file already has the
            // scale -- file_not_found for nothing at that path,
            // file_unreadable for a file that is there and cannot be used -- and
            // a picture is a file like any other. satl_line_not_understood would
            // say the LINE was wrong, and the line is fine: the file is not.
            //
            // GLib's own message is what tells the two apart, because GLib is
            // what looked. It is better than anything written here would be: it
            // names the path, and it knows a missing file from one that is there
            // and is not a picture.
            const signed long long int code =
                a_bad_file ? (why.find("No such file or directory") != std::string::npos
                                  ? file_not_found
                                  : file_unreadable)
                           : (the_program ? satl_line_not_understood : no_display);
            context.refuse(code, called + " could not be made -- " + why);
            return Value();
        }
        return Value::of_window(std::move(made));
    }

    std::string title;
    unsigned long long int wide = 0, tall = 0;
    if (!text_of(arguments[0], title, "satellite.window.new", context) ||
        !size_of(arguments[1], wide, "satellite.window.new's width", context) ||
        !size_of(arguments[2], tall, "satellite.window.new's height", context))
        return Value();
    WindowHandle made = window_new(title, wide, tall, why);
    if (made == nullptr) {
        // NO SCREEN IS THE MACHINE'S ANSWER AND A BAD SIZE IS THE PROGRAM'S, and
        // they are told apart by which one window_new checked first: it refuses a
        // size before it ever asks for a display.
        const bool the_program = wide == 0 || tall == 0 || wide > 32767 || tall > 32767;
        context.refuse(the_program ? satl_line_not_understood : no_display,
                       "satellite.window.new could not open a window -- " + why);
        return Value();
    }
    return Value::of_window(std::move(made));
}

Value call_window_method(Code method, const WindowHandle &which, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string what = name + "." + std::string(token::method_name_of(method));
    const int wanted = window_method_arity(method);
    if (wanted < 0) {
        context.refuse(not_built_yet, what + " is not built for a window yet");
        return Value();
    }

    // `.title` WITH NO BRACKETS READS IT BACK, and with brackets writes it. That
    // is the one method here that is both, and it is both because the author
    // wrote `my_window.title(...)` and a person who can set a thing expects to be
    // able to ask it. Every other window method is an action and wants its
    // brackets: `.close` alone would read as a thing rather than a doing.
    // `.ok` READS WITH OR WITHOUT ITS BRACKETS, because it is a QUESTION and not a
    // doing -- `f.ok` is how a file is asked the same thing
    // (SATELLITE_FILE_OPERATIONS). `.close` and `.focus` are doings and want
    // their brackets; `.ok` written bare reads exactly as what it means.
    // `.answer` IS A QUESTION, so it reads with or without its brackets -- the
    // rule `.ok`, `.width` and `.height` already follow. Empty until a person
    // has answered one.
    if (method == token::answer_token) {
        satellite_window *piece = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece == nullptr ? std::string() : piece->last_answer, out, bad_offset);
        return out;
    }
    if (method == token::ok_token) {
        satellite_window *asked = which.get();
        return Value::of_bool(asked != nullptr && asked->on_the_screen);
    }
    // `.width` AND `.height` ARE QUESTIONS, so they read with or without their
    // brackets -- the rule `.ok` already follows. A doing wants its brackets; a
    // question written bare reads exactly as what it means.
    if (method == token::width_token || method == token::height_token) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        long long int got = 0;
        std::string why;
        if (!window_size_of(*piece, method == token::height_token, got, why)) {
            // TWO REFUSALS AND TWO CODES, as `.text` has: a closed piece is
            // window_is_closed, and a MENU -- which has no size of its own,
            // open or closed -- is a kind that does not meet (GTK-12). One code
            // for both printed S505 under a sentence that was not about closing.
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        return Value::of_number(satellite_number(static_cast<unsigned long long int>(got)));
    }
    // `.across` AND `.down` ARE QUESTIONS TOO (GTK-15's leftover, 2026-09-22):
    // where the last click on this piece landed, in its own pixels, which on a
    // canvas are the pixels `.line` draws in. 0 and 0 until a click has
    // happened, as `.key` is "" until a key has. THE HANDLE'S OWN, written by
    // THIS thread off the event as `.key` is, so no desk is asked and a closed
    // piece still answers where it was last clicked.
    if (method == token::across_token || method == token::down_token) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        const long long int got = method == token::across_token ? piece->last_across : piece->last_down;
        satellite_number answer(got < 0 ? static_cast<unsigned long long int>(-got)
                                        : static_cast<unsigned long long int>(got),
                                got < 0);
        return Value::of_number(std::move(answer));
    }
    // `.thickness` AND `.outline` WITH NO BRACKETS READ THE PEN BACK, and with
    // brackets set it -- `.on`'s shape, and it crosses to the desk as `.on`
    // does because the desk is the thread that holds the pen.
    if ((method == token::thickness_token || method == token::outline_token) && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        long long int got = 0;
        std::string why;
        if (!window_pen_of(*piece, method == token::thickness_token, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        if (method == token::outline_token)
            return Value::of_bool(got != 0);
        return Value::of_number(satellite_number(static_cast<unsigned long long int>(got)));
    }
    // `.fullscreen` WITH NO BRACKETS ASKS WHETHER IT FILLS THE SCREEN.
    if (method == token::fullscreen_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        bool got = false;
        std::string why;
        if (!window_fullscreen_of(*piece, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        return Value::of_bool(got);
    }
    if (method == token::title_token && !had_parentheses) {
        satellite_window *window = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(window == nullptr ? std::string() : window->title, out, bad_offset);
        return out;
    }
    // `.pressed` WITH NO BRACKETS READS THE CAPSULE'S NAME BACK, and with
    // brackets says what it is -- the same pair as `.title`, and for the author's
    // same reason: a person who can set a thing expects to be able to ask it.
    // Empty for a button that answers nobody, which is what a button is until
    // something says otherwise.
    //
    // READ HERE ON THE INTERPRETER'S THREAD while the desk may be reading it in
    // `clicked`, and that is safe because both are READS: the only writer is
    // window_pressed(), which is this same thread going through on_the_desk().
    // `.key` READ BARE IS THE ODD ONE: it answers the LAST KEY and not the
    // capsule's name, because which key was pressed is the thing a program
    // actually wants and a capsule it wrote itself is not. Empty until one is.
    if (method == token::key_token && !had_parentheses) {
        satellite_window *piece = which.get();
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(piece == nullptr ? std::string() : piece->last_key, out, bad_offset);
        return out;
    }
    if ((method == token::pressed_token || method == token::changed_token ||
         method == token::closed_token || method == token::clicked_token) && !had_parentheses) {
        satellite_window *piece = which.get();
        std::string named;
        if (piece != nullptr)
            named = method == token::pressed_token   ? piece->when_pressed
                    : method == token::changed_token ? piece->when_changed
                    : method == token::clicked_token ? piece->when_clicked
                                                     : piece->when_closed;
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(named, out, bad_offset);
        return out;
    }
    // `.text` WITH NO BRACKETS READS THE WORDS ON A PIECE, and since GTK-2 it is
    // a real ASK rather than a look at the handle: a person typing in a text box
    // changes the widget and tells satellite nothing, so window_text_of() goes
    // to the desk, copies what is there and answers that.
    //
    // AND IT CAN REFUSE, which no other bare read here can: a WINDOW has no words
    // -- its words are its title -- and being handed an empty string for one
    // would be an answer that is wrong and does not say so.
    if (method == token::text_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        std::string words, why;
        if (!window_text_of(*piece, words, why)) {
            // TWO REFUSALS AND TWO CODES, because they are two different
            // mistakes. A WINDOW asked for `.text` is a kind that does not meet
            // -- the program wanted `.title`. A CLOSED text box is not a mistake
            // at all until it happens: the piece was right and the moment was
            // late, and window_is_closed is the code that says so. Sending both
            // through types_do_not_meet printed "this operator has no scenario
            // for the two kinds it was given" under a sentence about a window
            // that had closed, which is an explanation that does not fit.
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(words, out, bad_offset);
        return out;
    }
    // `.on` WITH NO BRACKETS ASKS WHETHER A THING IS TURNED ON, and it is the
    // same shape as `.text`: a person clicking a checkbox changes the widget and
    // tells satellite nothing, so the answer is the widget's and not the
    // handle's. A piece that is neither on nor off is REFUSED rather than
    // answered false -- a label has no such question.
    if (method == token::on_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        bool on = false;
        std::string why;
        if (!window_on_of(*piece, on, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        return Value::of_bool(on);
    }
    // `.value` WITH NO BRACKETS ASKS WHAT NUMBER A PIECE IS AT, and what comes
    // back depends on the piece: a slider and a number box answer a NUMBER, and
    // a progress bar answers a PERCENTAGE. That is not two methods wearing one
    // name -- it is one question whose answer has the kind the piece has.
    if (method == token::value_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        long long int got = 0;
        std::string why;
        if (!window_value_of(*piece, got, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        if (piece->piece == satellite_window::progress)
            return a_percentage_of(got);
        satellite_number answer(got < 0 ? static_cast<unsigned long long int>(-got)
                                        : static_cast<unsigned long long int>(got),
                                got < 0);
        return Value::of_number(std::move(answer));
    }
    // `.path` WITH NO BRACKETS ASKS WHICH FILE A PICTURE SHOWS, and it never
    // crosses to the desk: satellite opened that file, so the path is ours and
    // stays true after the window has gone. A label's rule, not a text box's.
    if (method == token::path_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        std::string shown, why;
        if (!window_path_of(*piece, shown, why)) {
            context.refuse(types_do_not_meet, what + " -- " + why);
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(shown, out, bad_offset);
        return out;
    }
    // `.chosen` WITH NO BRACKETS ASKS WHICH ITEM IS PICKED, as TEXT. Nothing
    // picked is "" and not a refusal: a choice a person has not touched is an
    // ordinary state of a choice.
    if (method == token::chosen_token && !had_parentheses) {
        satellite_window *piece = which.get();
        if (piece == nullptr) {
            context.refuse(window_is_closed, what + ": there is no piece here");
            return Value();
        }
        std::string picked, why;
        if (!window_chosen_of(*piece, picked, why)) {
            context.refuse(piece->widget == nullptr ? window_is_closed : types_do_not_meet,
                           what + " -- " + why);
            return Value();
        }
        Value out;
        std::size_t bad_offset = 0;
        Value::of_utf8(picked, out, bad_offset);
        return out;
    }
    if (!had_parentheses && wanted == 0) {
        context.refuse(satl_line_not_understood, what + " is something a window DOES, so write it with "
                                                        "its brackets: " + what + "()");
        return Value();
    }
    if (arguments.size() != static_cast<std::size_t>(wanted) &&
        static_cast<int>(arguments.size()) != window_method_also_takes(method)) {
        context.refuse(satl_line_not_understood, what + " takes " + std::to_string(wanted) +
                                                     (wanted == 1 ? " argument, and was given " :
                                                                    " arguments, and was given ") +
                                                     std::to_string(arguments.size()));
        return Value();
    }
    satellite_window *window = which.get();
    if (window == nullptr) {
        context.refuse(window_is_closed, what + ": there is no window here");
        return Value();
    }

    std::string why;
    bool went = false;
    switch (method) {
    case token::press_token: went = window_press(*window, why); break;
    case token::close_token: went = window_close(*window, why); break;
    case token::focus_token: went = window_focus(*window, why); break;
    case token::title_token: {
        std::string title;
        if (!text_of(arguments[0], title, what, context))
            return Value();
        went = window_set_title(*window, title, why);
        break;
    }
    case token::pressed_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_pressed(*window, capsule, why);
        break;
    }
    case token::text_token: {
        std::string text;
        if (!text_of(arguments[0], text, what, context))
            return Value();
        went = window_set_text(*window, text, why);
        break;
    }
    case token::on_token: {
        bool on = false;
        if (!on_of(arguments[0], on, what, context))
            return Value();
        went = window_set_on(*window, on, why);
        break;
    }
    case token::value_token: {
        // THE KIND IT TAKES IS THE PIECE'S, and the wrong one is REFUSED rather
        // than converted. A bare 50 on a progress bar could mean 50% or it could
        // mean half of one -- and a guess between those two is an answer that is
        // wrong and does not say so, so the refusal names the spelling instead.
        long long int to = 0;
        if (window->piece == satellite_window::progress) {
            const satellite_percentage *asked = arguments[0].as_percentage();
            if (asked == nullptr) {
                context.refuse(types_do_not_meet,
                               what + " takes a percentage, written with its sign on: " + name +
                                   ".value(50%) -- and was given " + arguments[0].kind_name());
                return Value();
            }
            if (!millionths_of(*asked, to)) {
                context.refuse(not_a_position,
                               what + " was given a percentage no bar can be set to");
                return Value();
            }
        } else if (!place_of(arguments[0], to, what, context, "a whole number")) {
            return Value();
        }
        went = window_set_value(*window, to, why);
        break;
    }
    case token::chosen_token: {
        std::string pick;
        if (!text_of(arguments[0], pick, what, context))
            return Value();
        went = window_set_chosen(*window, pick, why);
        break;
    }
    case token::path_token: {
        std::string shown;
        if (!text_of(arguments[0], shown, what, context))
            return Value();
        went = window_set_path(*window, shown, why);
        break;
    }
    case token::changed_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_changed(*window, capsule, why);
        break;
    }
    case token::closed_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_closed(*window, capsule, why);
        break;
    }
    case token::resize_token: {
        long long int wide = 0, tall = 0;
        if (!place_of(arguments[0], wide, what + "'s width", context) ||
            !place_of(arguments[1], tall, what + "'s height", context))
            return Value();
        went = window_resize(*window, wide, tall, why);
        break;
    }
    case token::fullscreen_token: {
        bool on = false;
        if (!on_of(arguments[0], on, what, context))
            return Value();
        went = window_set_fullscreen(*window, on, why);
        break;
    }
    case token::colour_token:
    case token::background_token: {
        std::string colour;
        if (!text_of(arguments[0], colour, what, context))
            return Value();
        went = window_set_colour(*window, colour, method == token::background_token, why);
        break;
    }
    case token::message_token: {
        std::string saying;
        if (!text_of(arguments[0], saying, what, context))
            return Value();
        went = window_message(*window, saying, why);
        break;
    }
    case token::ask_token: {
        std::string capsule, question;
        if (!text_of(arguments[0], capsule, what, context) ||
            !text_of(arguments[1], question, what + "'s question", context))
            return Value();
        went = window_ask(*window, question, capsule, why);
        break;
    }
    case token::key_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_key(*window, capsule, why);
        break;
    }
    case token::item_token: {
        std::string capsule, label;
        if (!text_of(arguments[0], capsule, what, context) ||
            !text_of(arguments[1], label, what + "'s words", context))
            return Value();
        went = window_item(*window, capsule, label, why);
        break;
    }
    case token::menu_token: {
        // THE SAME READER `.append` USES: a handle or a refusal naming the kind.
        const WindowHandle *menu = arguments[0].window_handle();
        if (menu == nullptr) {
            context.refuse(types_do_not_meet, what + " takes a menu to put across the top, or inside "
                                                  "this one -- satellite.window.menu(\"File\") makes "
                                                  "one -- and was given " + arguments[0].kind_name());
            return Value();
        }
        went = window_menu(*window, *menu, why);
        break;
    }
    case token::separator_token: went = window_separator(*window, why); break;
    // WHAT A PROGRAM DRAWS ON A CANVAS (GTK-15). Every number is a place -- a
    // whole number that may be negative and must fit a screen -- read by the
    // one reader every place here goes through; what may NOT be negative is
    // the canvas's own business and is refused there, by name.
    case token::line_token: {
        long long int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (!place_of(arguments[0], x1, what + "'s from across", context) ||
            !place_of(arguments[1], y1, what + "'s from down", context) ||
            !place_of(arguments[2], x2, what + "'s to across", context) ||
            !place_of(arguments[3], y2, what + "'s to down", context))
            return Value();
        went = window_line(*window, x1, y1, x2, y2, why);
        break;
    }
    case token::box_token: {
        long long int x = 0, y = 0, wide = 0, tall = 0;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !place_of(arguments[2], wide, what + "'s width", context) ||
            !place_of(arguments[3], tall, what + "'s height", context))
            return Value();
        went = window_box(*window, x, y, wide, tall, why);
        break;
    }
    case token::circle_token: {
        long long int x = 0, y = 0, radius = 0;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !place_of(arguments[2], radius, what + "'s radius", context))
            return Value();
        went = window_circle(*window, x, y, radius, why);
        break;
    }
    case token::arc_token: {
        long long int x = 0, y = 0, radius = 0, from = 0, to = 0;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !place_of(arguments[2], radius, what + "'s radius", context) ||
            !place_of(arguments[3], from, what + "'s from", context, "a number of degrees") ||
            !place_of(arguments[4], to, what + "'s to", context, "a number of degrees"))
            return Value();
        went = window_arc(*window, x, y, radius, from, to, why);
        break;
    }
    case token::thickness_token: {
        long long int pixels = 0;
        if (!place_of(arguments[0], pixels, what, context))
            return Value();
        went = window_set_thickness(*window, pixels, why);
        break;
    }
    case token::outline_token: {
        bool outline = false;
        if (!on_of(arguments[0], outline, what, context))
            return Value();
        went = window_set_outline(*window, outline, why);
        break;
    }
    case token::write_token: {
        long long int x = 0, y = 0;
        std::string words;
        if (!place_of(arguments[0], x, what + "'s across", context) ||
            !place_of(arguments[1], y, what + "'s down", context) ||
            !text_of(arguments[2], words, what + "'s words", context))
            return Value();
        went = window_write(*window, x, y, words, why);
        break;
    }
    case token::clear_token: went = window_clear(*window, why); break;
    case token::save_token: {
        std::string path;
        if (!text_of(arguments[0], path, what, context))
            return Value();
        went = window_save(*window, path, why);
        // A PICTURE THAT COULD NOT BE WRITTEN GETS A FILE'S CODE (GTK-15).
        // file_unwritable is "a save could not write; the reason is said",
        // which is exactly this, and the tail below would have printed
        // types_do_not_meet under a sentence about a file -- the code and the
        // sentence disagreeing, which this module has fixed three times now.
        // Told apart by the sentence window_save wrote, the way
        // call_window_word tells "no display" apart. A closed canvas, a piece
        // that is not a canvas and an empty path still fall to the tail.
        if (!went && window->widget != nullptr && why.find("could not be written to") != std::string::npos) {
            context.refuse(file_unwritable, what + " could not be done -- " + why);
            return Value();
        }
        break;
    }
    case token::clicked_token: {
        std::string capsule;
        if (!text_of(arguments[0], capsule, what, context))
            return Value();
        went = window_clicked(*window, capsule, why);
        break;
    }
    case token::every_token: {
        std::string capsule;
        long long int how_often = 0;
        if (!text_of(arguments[0], capsule, what, context) ||
            !place_of(arguments[1], how_often, what + "'s how often", context, "a number of milliseconds"))
            return Value();
        went = window_every(*window, capsule, how_often, why);
        break;
    }
    case token::font_token: {
        std::string face;
        long long int size = 0;
        if (!text_of(arguments[0], face, what + "'s face", context) ||
            !place_of(arguments[1], size, what + "'s size", context, "a size in pixels"))
            return Value();
        went = window_set_font(*window, face, size, why);
        break;
    }
    case token::append_token: {
        const WindowHandle *piece = arguments[0].window_handle();
        if (piece == nullptr) {
            context.refuse(types_do_not_meet, what + " takes a piece to put in the window -- " +
                                                  the_words_that_make_a_piece() + " makes one -- and was "
                                                  "given " + arguments[0].kind_name());
            return Value();
        }
        // ONE ARGUMENT OR THREE, and which is right is decided by what the
        // RECEIVER turned out to be -- window_append() is what knows.
        const bool by_place = arguments.size() == 3;
        long long int x = 0, y = 0;
        if (by_place && (!place_of(arguments[1], x, what + "'s across", context) ||
                         !place_of(arguments[2], y, what + "'s down", context)))
            return Value();
        went = window_append(*window, *piece, by_place, x, y, why);
        break;
    }
    default: break;
    }
    if (!went) {
        // THE CODE FOLLOWS WHAT ACTUALLY HAPPENED, not what this tail used to
        // assume. Every doing-method could once fail for one reason -- the
        // window had gone -- and window_is_closed was the whole truth. It is not
        // any more: `a_choice.chosen("purple")` on a choice of red and green
        // fails with the window wide open, and reporting S505 under a sentence
        // saying "there is no purple to choose here" is a code and a sentence
        // disagreeing about what went wrong.
        context.refuse(window->widget == nullptr ? window_is_closed : types_do_not_meet,
                       what + " could not be done -- " + why);
        return Value();
    }
    // A WINDOW METHOD ANSWERS THE WINDOW, so they string together the way a
    // string's do: `w.title("x").focus()` is one line, and expression.cpp's
    // method loop needs no case for it.
    return Value::of_window(which);
}

void windows_hold_the_run_open(bool the_program_finished)
{
    windows_stay_open_until_closed(the_program_finished);
}

signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &run_a_capsule)
{
    AnEvent happened;
    while (the_desk_waits_for_something(happened)) {
        // WHAT IT SAID IS COPIED ONTO THE PIECE **HERE**, ON THE INTERPRETER'S
        // THREAD, and that is the whole reason it travelled on the event
        // (GTK-14). A key's name written by the desk and read by a capsule
        // would be a std::string with two threads on it; written here it has
        // one writer, and the capsule that is about to run is the only reader.
        if (happened.piece != nullptr) {
            if (happened.said_what == AnEvent::a_key)
                happened.piece->last_key = happened.said;
            else if (happened.said_what == AnEvent::an_answer)
                happened.piece->last_answer = happened.said;
            // AND WHERE A CLICK LANDED (GTK-15's leftover), the same way and
            // for the same reason: two numbers with one writer.
            else if (happened.said_what == AnEvent::a_place) {
                happened.piece->last_across = happened.across;
                happened.piece->last_down = happened.down;
            }
        }
        const signed long long int stopped = run_a_capsule(happened.capsule, happened.piece, happened.window);
        // A CAPSULE THAT STOPPED STOPS THE RUN, the same as a line of main
        // would have. The report is already printed by the time this answers,
        // and main() takes the windows down on a code that stops -- a person
        // told their program stopped must not be left pressing a button that
        // still looks alive.
        if (stops_the_program(stopped))
            return stopped;
    }
    return success;
}

#else

Value call_window_word(Code code, const std::vector<Value> &, ExpressionContext &context)
{
    return no_window_here(word::spelling_of(code), context);
}

Value call_window_method(Code method, const WindowHandle &, const std::vector<Value> &, bool,
                         const std::string &name, ExpressionContext &context)
{
    return no_window_here(name + "." + std::string(token::method_name_of(method)), context);
}

// NOTHING TO HOLD OPEN: no window word ever answered a window in this build.
void windows_hold_the_run_open(bool) {}

// AND NOTHING TO PRESS. No window word answered a window, so no button was made
// and no press can be waiting.
signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &)
{
    return success;
}

#endif

} // namespace satellite004
