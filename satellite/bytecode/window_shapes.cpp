// satellite/bytecode/window_shapes.cpp -- WHAT EACH WINDOW METHOD TAKES, asked
// by the checker before a program runs and by the reader as it runs: how many
// arguments, whether a second count is also right, which methods name a
// capsule, and the one sentence that lists them all. window_readers.hpp says
// how the six window files were cut on 2026-09-22.
//
// NOTHING HERE DRAWS, so nothing here is under the #if: a satl built without a
// window still checks a program's window lines before refusing to run them.

#include "window_calls.hpp"

#include "method_periods.hpp"
#include "program_walk.hpp"

#include <string>

namespace satellite004 {

using token::Code;

// THE SENTENCE A PERSON GETS WHEN THEY ASK A WINDOW FOR SOMETHING IT HAS NO
// METHOD FOR, and it lives here rather than in program_check.cpp because this is
// the file that knows. The checker used to carry its own copy of a list like
// this and it went stale the same afternoon it was written.
std::string window_methods_are()
{
    // EVERY ONE WRITTEN `.satellite.` (NM-8, 2026-10-11): a window's words run their numbers, 1 6 18 n.
    return "a window has .satellite.append(piece, across, down), .satellite.open(), .satellite.close(), "
           ".satellite.focus(), .satellite.title(\"text\"), .satellite.resize(wide, tall), .satellite.fullscreen and "
           ".satellite.ok; every piece has .satellite.width, .satellite.height, .satellite.colour(\"#00ff88\"), "
           ".satellite.background(...) and .satellite.font(\"a face\", 12); "
           "a row, a column or a grid has .satellite.append too -- .satellite.append(piece) for a row, "
           ".satellite.append(piece, across, down) for a grid's cell; "
           "a piece in one has .satellite.text; a checkbox or a switch has .satellite.on; a slider, a number box or "
           "a progress bar has .satellite.value; a choice or a one-of has .satellite.chosen -- all read bare and "
           "written with brackets; a button has .satellite.pressed(a_capsule) and .satellite.press(); anything a "
           "person can change has .satellite.changed(a_capsule); and a window has .satellite.closed(a_capsule) and "
           ".satellite.every(a_capsule, 1000) and .satellite.key(a_capsule); anything that is not a button has "
           ".satellite.clicked(a_capsule); and a window has .satellite.message(\"saying\"), "
           ".satellite.ask(a_capsule, \"a question?\"), .satellite.choose_a_file(a_capsule) and .satellite.answer; "
           "a menu has .satellite.item(a_capsule, \"Open\"), .satellite.separator() and .satellite.menu(a_menu) for a "
           "menu inside it, and a window has .satellite.menu(a_menu); "
           "a canvas has .satellite.line(from_across, from_down, to_across, to_down), .satellite.box(across, down, "
           "wide, tall), .satellite.circle(across, down, radius), .satellite.arc(across, down, radius, from_degrees, "
           "to_degrees), .satellite.write(across, down, \"words\"), .satellite.clear() and "
           ".satellite.save(\"picture.png\"), and its pen has .satellite.thickness(3) and .satellite.outline(1); "
           "anything that is not a button or a menu has .satellite.across and .satellite.down, where the last click "
           "on it landed; a piece going into a set of tabs is named by its .satellite.title(\"a name\"), and the "
           "tabs' .satellite.chosen is the one in front; "
           "a console has .satellite.display(\"words\"), .satellite.typed(a_capsule) -- and .satellite.typed read "
           "bare is the last line a person finished -- .satellite.clear(), .satellite.home(), .satellite.columns and "
           ".satellite.rows; a window or a console has .satellite.scroll.vertical(1) and "
           ".satellite.scroll.horizontal(1), each read bare for the answer; "
           "a window and a widget make widgets with .satellite.widgets.display.file(\"a_name\") and "
           ".satellite.widgets.display.input(\"a_name\"), reached after as my_window.a_name -- and in a section with "
           "a size, .satellite.widgets.display.file(\"a_name\", 100%, 100%); a window and a section make sections "
           "with .satellite.section(\"a_name\", 100%, 50%), each a share of what holds it or pixels, laid out left "
           "to right; a file display has .satellite.directory(\"/home/you/\"), read bare for the one it shows; and a "
           "text input has .satellite.listen() and .satellite.listen(a_capsule) "
           "(GTK_AND_NO_DEPENDENCIES.md Part 2G lists every piece and what it does)";
}

int window_method_arity(Code method)
{
    switch (method) {
    case token::append_token:  return 3;     // the piece, and where its centre goes
    case token::open_token:    return 0;     // on the screen, and drawn, before the next line
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
    case token::choose_a_file_token: return 1;  // the capsule's NAME; the path arrives in .answer (GTK-11)
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
    case token::clear_token:      return 0;  // nothing drawn any more; on a console, nothing on the screen
    case token::save_token:       return 1;  // the file to write the picture to
    case token::display_token:    return 1;  // a line into a console (GTK-17)
    case token::typed_token:      return 1;  // the capsule's name; read bare for the last line finished
    case token::home_token:       return 0;  // the cursor to the top-left corner
    case token::columns_token:    return 0;  // a question: how many characters fit across
    case token::rows_token:       return 0;  // a question: how many lines fit down
    // WORDS OF PARTS, EACH ONE WORD (the author's final syntax, 2026-10-10 ~21:15): .satellite.scroll.vertical(on)
    // and .satellite.scroll.horizontal(on), written and read bare for the answer (2026-10-07); and his widgets
    // (2026-10-09), .satellite.widgets.display.file("a_name") and .satellite.widgets.display.input("a_name"),
    // which make one and take its name. Their parts -- scroll, widgets, displays, file, input, vertical,
    // horizontal -- are no window's words on their own, and are refused naming the whole word.
    case token::scroll_vertical_token:       return 1;  // written; read bare for the answer
    case token::scroll_horizontal_token:     return 1;
    case token::widgets_display_file_token:  return 1;  // the widget's name, in quotes
    case token::widgets_display_input_token: return 1;
    case token::directory_token:   return 1;  // written; read bare for the one it shows
    case token::listen_token:      return 1;  // the capsule's name -- or nothing, window_method_also_takes
    // SECTIONS (the author, 2026-10-10): `.section("a_name", 100%, 50%)` -- its name, how wide and how
    // tall, each a share of what holds it or pixels. `.file` and `.input` take a size the same way.
    case token::section_token:     return 3;
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
    //
    // `.listen` IS THE SECOND (the author, 2026-10-09: "it runs a capsule if you give it a capsule,
    // and ... it keeps the string text up to date, so we don't have to add some syntax that is
    // unnecessary"): `.listen()` and `.listen(a_capsule)`.
    if (method == token::listen_token) return 0;
    // `.file` AND `.input` TAKE A SIZE TOO (2026-10-10): `.file("a_name", 100%, 100%)` in a section.
    if (method == token::widgets_display_file_token || method == token::widgets_display_input_token) return 3;
    return method == token::append_token ? 1 : -1;
}

std::string window_method_takes(Code method)
{
    const int one = window_method_arity(method), other = window_method_also_takes(method);
    if (other < 0)
        return std::to_string(one) + (one == 1 ? " argument" : " arguments");
    return std::to_string(one < other ? one : other) + " or " + std::to_string(one < other ? other : one) +
           " arguments";
}

// THE AUTHOR'S WORDS OF PARTS (his final window syntax, 2026-10-10 ~21:15): widgets.display.file,
// widgets.display.input, scroll.vertical and scroll.horizontal are each ONE word -- `.satellite.` once, straight
// before it, and plain periods inside it: main_window.files_display.satellite.widgets.display.file(...). The lexer
// reads each as one code after `.satellite.` (bytecode_registry.cpp), so what reaches the checker or the walker as
// its PARTS is the word written some other way: bare, with `.satellite.` again inside it, with the family's old
// name displays (his D of 21:15: refused, naming the new), or begun and not finished. A part is one of the seven
// codes below -- or `.display`, the console's own word display("words"), where .file or .input follows it, or
// after widgets.
namespace {

bool a_part(Code code)
{
    return code == token::widgets_token || code == token::displays_token || code == token::file_piece_token ||
           code == token::input_piece_token || code == token::scroll_token || code == token::vertical_token ||
           code == token::horizontal_token;
}

bool a_maker_part(Code code)
{
    return code == token::file_piece_token || code == token::input_piece_token;
}

} // namespace

bool a_word_in_parts(const std::vector<std::bitset<16>> &row, std::size_t dot)
{
    const Code first = code_at(row, dot + 1);
    return a_part(first) || (first == token::display_token && a_method_period(code_at(row, dot + 2)) &&
                             a_maker_part(code_at(row, dot + 3)));
}

std::string word_in_parts_written_wrong(const std::vector<std::bitset<16>> &row, std::size_t dot,
                                        const std::string &receiver)
{
    // THE PARTS AS WRITTEN, a period and a part at a time, to the part that says which word it is.
    std::string written = receiver;
    bool widgets = false, family = false, old_name = false, scroll = false, under = false;
    Code last = 0;
    std::size_t parts = 0;
    for (std::size_t k = dot; a_method_period(code_at(row, k)) && last == 0; k += 2) {
        const Code part = code_at(row, k + 1);
        const bool the_family = part == token::displays_token ||
                                (part == token::display_token &&
                                 (widgets || (a_method_period(code_at(row, k + 2)) && a_maker_part(code_at(row, k + 3)))));
        if (!a_part(part) && !the_family)
            break;
        under = under || code_at(row, k) == token::satellite_method_token;
        written += std::string(code_at(row, k) == token::satellite_method_token ? ".satellite." : ".") +
                   token::method_name_of(part);
        ++parts;
        widgets = widgets || part == token::widgets_token;
        family = family || the_family;
        old_name = old_name || part == token::displays_token;
        scroll = scroll || part == token::scroll_token;
        if (a_maker_part(part) || part == token::vertical_token || part == token::horizontal_token)
            last = part;
    }
    const std::string head = receiver + ".satellite.";
    const char *word = last == token::file_piece_token    ? "widgets.display.file"
                       : last == token::input_piece_token ? "widgets.display.input"
                       : last == token::vertical_token    ? "scroll.vertical"
                       : last == token::horizontal_token  ? "scroll.horizontal"
                                                          : nullptr;
    // BEGUN AND NOT FINISHED: the words it begins.
    if (word == nullptr)
        return written + " -- " + (widgets || family ? "widgets.display" : "scroll") + " is the start of a word: " +
               (widgets || family ? head + "widgets.display.file(\"a_name\") or " + head + "widgets.display.input(\"a_name\")"
                                  : head + "scroll.vertical(on) or " + head + "scroll.horizontal(on)");
    // EVERY PART RIGHT AND IN ITS PLACE, ALL BARE: the old spelling of a numbered word, said as every one is.
    const bool whole = last == token::vertical_token || last == token::horizontal_token
                           ? parts == 2 && scroll
                           : parts == 3 && widgets && family && !old_name;
    if (whole && !under)
        return period_written_wrong(receiver, word, satelliteObject::window, false);
    return written + " is written " + head + word + " -- " +
           (old_name ? "the widgets' family is display now (the author, 2026-10-10), and the word is written whole, "
                       ".satellite. once, straight before it"
                     : "it is one word, written with .satellite. once, straight before it, and plain periods inside it");
}

// AND WHICH OF THEM MAKES ONE, for the checker's pass over every name a program gives a widget.
bool makes_a_widget(Code method)
{
    return method == token::widgets_display_file_token || method == token::widgets_display_input_token;
}

// A NAME A PROGRAM CAN WRITE AFTER A DOT, and nothing else: `my_window.file display` is two words
// and `my_window.2nd` is not a name. A METHOD'S OWN WORD IS REFUSED TOO -- the lexer makes
// `my_window.text` the method, so a widget called "text" could never be reached by its name.
bool a_widget_name_fits(const std::string &name, std::string &why)
{
    bool fits = !name.empty() && !(name[0] >= '0' && name[0] <= '9');
    for (const char c : name)
        fits = fits && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
    if (!fits) {
        why = "\"" + name + "\" is not a name a widget can have -- a widget is reached by its name after a dot, "
              "so it is letters, digits and _, and not first a digit: \"file_display\"";
        return false;
    }
    if (token::method_code_of(name) != 0) {
        why = "\"" + name + "\" is a method's own word, so my_window." + name + " would be the method and never "
              "the widget -- give it a name of its own: \"" + name + "_display\"";
        return false;
    }
    return true;
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
           method == token::ask_token || method == token::item_token || method == token::typed_token ||
           method == token::listen_token || method == token::choose_a_file_token;
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
