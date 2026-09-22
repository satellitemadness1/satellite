// satellite/bytecode/window_shapes.cpp -- WHAT EACH WINDOW METHOD TAKES, asked
// by the checker before a program runs and by the reader as it runs: how many
// arguments, whether a second count is also right, which methods name a
// capsule, and the one sentence that lists them all. window_readers.hpp says
// how the six window files were cut on 2026-09-22.
//
// NOTHING HERE DRAWS, so nothing here is under the #if: a satl built without a
// window still checks a program's window lines before refusing to run them.

#include "window_calls.hpp"

#include <string>

namespace satellite004 {

using token::Code;

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
           "and its pen has .thickness(3) and .outline(1); anything that is not a button or a menu "
           "has .across and .down, where the last click on it landed; a piece going into a set of "
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


} // namespace satellite004
