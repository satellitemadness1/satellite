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
           "a window and a section make sections with .satellite.window.section(\"a_name\", 100%, 50%), each a "
           "share of what holds it or pixels, laid out left to right; a section shows one display, which fills it -- "
           ".satellite.window.display.file(\"a_name\") or .satellite.window.display.text.input(\"a_name\") -- and a "
           "window with no sections, or a file display, shows them across its top; each is reached after by its name "
           "alone, a_name, or as my_window.a_name; a file display has .satellite.directory(\"/home/you/\"), read "
           "bare for the one it shows; and a text input has .satellite.listen() and .satellite.listen(a_capsule) "
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
    // and .satellite.scroll.horizontal(on), written and read bare for the answer (2026-10-07). Their parts --
    // scroll, vertical, horizontal -- are no window's words on their own, and are refused naming the whole word.
    case token::scroll_vertical_token:       return 1;  // written; read bare for the answer
    case token::scroll_horizontal_token:     return 1;
    case token::directory_token:   return 1;  // written; read bare for the one it shows
    case token::listen_token:      return 1;  // the capsule's name -- or nothing, window_method_also_takes
    case token::ok_token:      return 0;
    // A WINDOW'S SECTIONS AND DISPLAYS ARE UNDER satellite.window (SATELLITE_WINDOW_SECTIONS.md SEC-0, the author's
    // design of 2026-10-20). `.satellite.window.section("a_name", 100%, 50%)` -- its name, how wide and how tall,
    // each a share of what holds it or pixels (2026-10-10) -- and a display, which takes its name and no size: it
    // fills its section (the author, 2026-10-25). The old spellings -- .satellite.section, widgets.display.file and
    // widgets.display.input -- are no window's words now, and are refused naming these (a_word_in_parts, below).
    case token::window_section_token:            return 3;
    case token::window_display_file_token:       return 1;  // the display's name, in quotes
    case token::window_display_text_input_token: return 1;
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
    // A DISPLAY TOOK A SIZE TOO, from 2026-10-10 until SEC-0: `.file("a_name", 100%, 100%)` in a section. It fills
    // its section now, and a size is a section's (a_display_takes_no_size, below).
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

// THE AUTHOR'S WORDS OF PARTS (his final window syntax, 2026-10-10 ~21:15): scroll.vertical and scroll.horizontal
// are each ONE word -- `.satellite.` once, straight before it, and plain periods inside it -- and since SEC-0
// (SATELLITE_WINDOW_SECTIONS.md, the author's design of 2026-10-20) so are window.section, window.display.file and
// window.display.text.input: main_window.satellite.window.section("top", 100%, 50%). The lexer reads each as one
// code after `.satellite.`, longest first (bytecode_registry.cpp), so what reaches the checker or the walker as its
// PARTS is a word written some other way: bare, with `.satellite.` again inside it, or begun and not finished --
// `window` and `window.display` are parts and run nothing, so `.satellite.window.display.text(` is the part
// window.display and then .text until SEC-3 makes window.display.text a word.
//
// AND THE OLD SPELLINGS, which are refused naming the new (rule 8; the author's ruling D of 2026-10-10): the one-code
// words .satellite.section, .satellite.widgets.display.file and .satellite.widgets.display.input, and their parts
// written any way -- widgets, displays, file, input, and `.display` (the console's own word display("words")) where
// .file or .input follows it or after widgets or window. `.section` is ALSO a word an object's capsule may be
// called, which is why this is asked only of a window (a_word_in_parts' callers), and why window_number_of no longer
// gives it a number.
namespace {

bool a_part(Code code)
{
    return code == token::window_token || code == token::window_display_token || code == token::widgets_token ||
           code == token::displays_token || code == token::file_piece_token || code == token::input_piece_token ||
           code == token::scroll_token || code == token::vertical_token || code == token::horizontal_token;
}

bool a_maker_part(Code code)
{
    return code == token::file_piece_token || code == token::input_piece_token;
}

// THE OLD SPELLINGS THAT THE LEXER READS AS ONE CODE, each a word until SEC-0 (1 6 18 52, 49 1 1 and 49 1 2).
bool an_old_word(Code code)
{
    return code == token::section_token || code == token::widgets_display_file_token ||
           code == token::widgets_display_input_token;
}

// WHERE A WORD OF PARTS ENDS: on its last part, or on an old word that is one code.
bool a_last_part(Code code)
{
    return a_maker_part(code) || code == token::vertical_token || code == token::horizontal_token || an_old_word(code);
}

} // namespace

bool a_word_in_parts(const std::vector<std::bitset<16>> &row, std::size_t dot)
{
    const Code first = code_at(row, dot + 1);
    return a_part(first) || an_old_word(first) ||
           (first == token::display_token && a_method_period(code_at(row, dot + 2)) &&
            a_maker_part(code_at(row, dot + 3)));
}

std::string word_in_parts_written_wrong(const std::vector<std::bitset<16>> &row, std::size_t dot,
                                        const std::string &receiver)
{
    // THE PARTS AS WRITTEN, a period and a part at a time, for as long as they go on spelling a word of parts --
    // and what they spell with plain periods, `joined`: window.display.text, widgets.displays.file, section.
    std::string written = receiver, joined;
    bool under = false;
    for (std::size_t k = dot; a_method_period(code_at(row, k)); k += 2) {
        const Code part = code_at(row, k + 1);
        const bool first = joined.empty();
        const bool goes_on =
            a_part(part) || (first && an_old_word(part)) ||
            (part == token::section_token && joined == "window") ||
            (part == token::text_token && joined == "window.display") ||
            (part == token::display_token &&
             (joined == "widgets" || joined == "window" ||
              (a_method_period(code_at(row, k + 2)) && a_maker_part(code_at(row, k + 3)))));
        if (!goes_on)
            break;
        under = under || code_at(row, k) == token::satellite_method_token;
        const std::string name = token::method_name_of(part);
        written += std::string(code_at(row, k) == token::satellite_method_token ? ".satellite." : ".") + name;
        joined += (first ? "" : ".") + name;
        if (a_last_part(part))
            break;
    }
    const std::string head = receiver + ".satellite.";
    // EVERY PART RIGHT AND IN ITS PLACE: written bare, the old spelling of a numbered word, said as every one is;
    // with `.satellite.` again inside it, the word as it is written.
    if (joined == "window.section" || joined == "window.display.file" || joined == "window.display.text.input" ||
        joined == "scroll.vertical" || joined == "scroll.horizontal") {
        if (!under)
            return period_written_wrong(receiver, joined, satelliteObject::window, false);
        return written + " is written " + head + joined +
               " -- it is one word, written with .satellite. once, straight before it, and plain periods inside it";
    }
    // THE OLD SPELLINGS (rule 8), and any mix of the old parts that ends in a maker: named as the word it is now.
    const std::string last = joined.substr(joined.rfind('.') == std::string::npos ? 0 : joined.rfind('.') + 1);
    const char *now = last == "file" ? "window.display.file"
                      : last == "input" ? "window.display.text.input"
                      : last == "section" ? "window.section"
                                          : nullptr;
    if (now != nullptr)
        return written + " is written " + head + now + " now -- a window's sections and displays are under "
                                                       "satellite.window";
    // BEGUN AND NOT FINISHED: the words it begins -- the two of the plan's sentence for window (SEC-0), and the old
    // family's begun pointed at the new.
    if (joined.rfind("scroll", 0) == 0)
        return written + " -- scroll is the start of a word: " + head + "scroll.vertical(on) or " + head +
               "scroll.horizontal(on)";
    if (joined.rfind("window.display", 0) == 0)
        return written + " -- window.display is the start of a word: " + head + "window.display.file(...) or " +
               head + "window.display.text.input(...)";
    if (joined.rfind("window", 0) == 0)
        return written + " -- window is the start of a word: " + head + "window.section(...) or " + head +
               "window.display.file(...)";
    return written + " -- a window's sections and displays are under satellite.window now: " + head +
           "window.section(...) or " + head + "window.display.file(...)";
}

// AND WHICH OF THEM MAKES A DISPLAY, for the checker's pass over every name a program gives a widget: the two
// SEC-0 builds (SATELLITE_WINDOW_SECTIONS.md rule 6), each a word the lexer reads whole after `.satellite.`, so
// never an object's capsule.
bool makes_a_widget(Code method)
{
    return method == token::window_display_file_token || method == token::window_display_text_input_token;
}

// A DISPLAY GIVEN A SIZE (SEC-0, rule 8): `.file("files", 100%, 100%)` until then, which a section of that size holds
// now. `spelled` is the call as written, `named` its name and `sizes` its width and height as written when they are
// known before the run, and empty when not -- the run has their values, not their text, and says a_width, a_height.
std::string a_display_takes_no_size(Code display, const std::string &spelled, const std::string &named,
                                    const std::string &sizes)
{
    const std::string name = named.empty() ? "a_name" : named;
    return spelled + " takes its name and no size -- a display fills the section it is made in (the author, "
                     "2026-10-25), so the size is a section's: .satellite.window.section(\"" + name + "_section\", " +
           (sizes.empty() ? "a_width, a_height" : sizes) + "), and in it " + name + "_section.satellite." +
           token::method_name_of(display) + "(\"" + name + "\")";
}

// WHERE A DISPLAY OR A SECTION MAY NOT GO (SEC-0, rules 6 and 7), one sentence each, said after the call as written:
// by the checker where it sees the lines (program_check.cpp's where_it_goes_refused), and by window_widget_made
// when the line runs. The author, 2026-10-25: "a display always fills the section".
std::string a_place_refused(APlace place)
{
    switch (place) {
    case APlace::a_second_display:
        return "the section shows a display already, and a display fills its section, so each display goes in a "
               "section of its own";
    case APlace::a_display_among_sections:
        return "the section holds sections, and a section shows one display or holds sections, not both: make the "
               "display in one of its sections";
    case APlace::a_section_beside_a_display:
        return "the section shows a display, and a section shows one display or holds sections, not both";
    case APlace::a_display_over_sections:
        return "the window has sections, and a window shows displays across its top or has sections, not both: make "
               "the display in a section";
    case APlace::a_section_under_displays:
        return "the window shows a display across its top, and a window shows displays across its top or has "
               "sections, not both: make that display in a section";
    }
    return std::string();
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
    // TWO NAMES A WIDGET NEVER HAD TO AVOID UNTIL ITS NAME WAS A NAME (SATELLITE_WINDOW_SECTIONS.md SEC-0, rule 4):
    // `satellite`, which no name may be (satellite_legal.hpp), and one shaped like a hex or a binary number, which
    // the lexer reads as that number (bytecode_registry.cpp) -- so neither could be written bare after its line.
    if (name == "satellite") {
        why = "\"satellite\" is the language's own name, and nothing else is called it -- give it a name of its own: "
              "\"satellite_display\"";
        return false;
    }
    bool a_number = name.size() > 1 && (name[0] == 'b' || name[0] == 'x');
    for (std::size_t at = 1; at < name.size() && a_number; ++at) {
        const char c = name[at];
        a_number = name[0] == 'b' ? (c == '0' || c == '1')
                                  : (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }
    if (a_number) {
        why = "\"" + name + "\" is how " + (name[0] == 'b' ? "a binary" : "a hex") + " number is written, so " + name +
              " on a line would be that number and never this name -- give it a name of its own: \"" + name +
              "_display\"";
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
