// satellite/bytecode/window_calls.cpp -- satellite.window's WORDS: the one
// table every widget is a row of, and the call that makes a piece from one.
// The header says why they are the object model's and not libraries.
// window_readers.hpp says how this file was cut on 2026-09-22, at 1278 lines.

#include "window_calls.hpp"

#include "window_readers.hpp"
#include "word_codes.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_variable_window/window_desk.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

using token::Code;

Code window_word() { return word::fixed_code<1, 27>; }

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
    // WHICH FAMILY IT IS UNDER, AND WHAT THAT FAMILY IS CALLED: 27 and "window"
    // for every piece, 5 and "console" for the one word that is under
    // satellite.console instead (GTK-17). Two families in one table, because
    // the table is what every arity, refusal and `.append` sentence is written
    // FROM, and a second table would be the stale-list mistake again.
    unsigned int family;
    const char *family_name;
    unsigned int number;                  // its number under 1 <family>
    const char *spelling;                 // satellite.<family>.<this>
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
    {27, "window", 1, "new", 3, satellite_window::window, AWord::words,
     "satellite.window.new takes a title, a width and a height: "
     "satellite.window.new(\"my window\", 800, 600)"},
    {27, "window", 2, "button", 1, satellite_window::button, AWord::words,
     "satellite.window.button takes the text on it: satellite.window.button(\"press me\")"},
    {27, "window", 3, "label", 1, satellite_window::label, AWord::words,
     "satellite.window.label takes the text it shows: satellite.window.label(\"a line of text\")"},
    {27, "window", 4, "text_box", 1, satellite_window::text_box, AWord::words,
     "satellite.window.text_box takes the text already in it, and \"\" for an empty one: "
     "satellite.window.text_box(\"\")"},
    {27, "window", 5, "text_area", 1, satellite_window::text_area, AWord::words,
     "satellite.window.text_area takes the text already in it, and \"\" for an empty one: "
     "satellite.window.text_area(\"\")"},
    {27, "window", 6, "checkbox", 1, satellite_window::checkbox, AWord::words,
     "satellite.window.checkbox takes the text beside it: satellite.window.checkbox(\"I agree\")"},
    {27, "window", 7, "switch", 0, satellite_window::a_switch, AWord::words,
     "satellite.window.switch takes nothing -- a switch says nothing, it is only on or off: "
     "satellite.window.switch()"},
    {27, "window", 8, "slider", 2, satellite_window::slider, AWord::numbers,
     "satellite.window.slider takes the least and the most it runs between: "
     "satellite.window.slider(0, 100)"},
    {27, "window", 9, "number_box", 2, satellite_window::number_box, AWord::numbers,
     "satellite.window.number_box takes the least and the most it runs between: "
     "satellite.window.number_box(1, 12)"},
    {27, "window", 10, "progress", 0, satellite_window::progress, AWord::numbers,
     "satellite.window.progress takes nothing -- how far along it is, is .value: "
     "satellite.window.progress()"},
    {27, "window", 11, "choice", 1, satellite_window::choice, AWord::items,
     "satellite.window.choice takes a list of what a person may pick: "
     "satellite.window.choice({\"red\", \"green\"})"},
    {27, "window", 12, "row", 0, satellite_window::row, AWord::words,
     "satellite.window.row takes nothing -- what goes in it is .append'ed: satellite.window.row()"},
    {27, "window", 13, "column", 0, satellite_window::column, AWord::words,
     "satellite.window.column takes nothing -- what goes in it is .append'ed: "
     "satellite.window.column()"},
    {27, "window", 14, "grid", 0, satellite_window::grid, AWord::words,
     "satellite.window.grid takes nothing -- what goes in it is .append'ed at a cell: "
     "satellite.window.grid()"},
    {27, "window", 15, "picture", 1, satellite_window::picture, AWord::a_file,
     "satellite.window.picture takes the name of a file to show: "
     "satellite.window.picture(\"logo.png\")"},
    {27, "window", 16, "scroll", 0, satellite_window::scroll, AWord::words,
     "satellite.window.scroll takes nothing -- the one piece it shows is .append'ed: "
     "satellite.window.scroll()"},
    {27, "window", 17, "frame", 1, satellite_window::frame, AWord::words,
     "satellite.window.frame takes the words on its edge, and \"\" for none: "
     "satellite.window.frame(\"a title\")"},
    {27, "window", 18, "split", 0, satellite_window::split, AWord::words,
     "satellite.window.split takes nothing -- the two pieces either side are .append'ed: "
     "satellite.window.split()"},
    // A MENU TAKES ITS HEADING, and that is GTK's ruling: a menu bar drops a
    // top-level item that has no submenu, silently, so a menu with no word on
    // the bar would be a menu that is nowhere (window_menu.cpp).
    {27, "window", 19, "menu", 1, satellite_window::menu, AWord::words,
     "satellite.window.menu takes the word that goes on the bar: satellite.window.menu(\"File\")"},
    {27, "window", 20, "canvas", 2, satellite_window::canvas, AWord::a_size,
     "satellite.window.canvas takes how wide and how tall it is: satellite.window.canvas(400, 300)"},
    // A SET OF TABS TAKES NOTHING: each tab is named by its PIECE's own .title
    // (GTK-16), so there is nothing here for the adding to carry.
    {27, "window", 21, "tabs", 0, satellite_window::tabs, AWord::words,
     "satellite.window.tabs takes nothing -- each piece .append'ed is a tab, named by that "
     "piece's .title: satellite.window.tabs()"},
    // THE RADIO (GTK-3, built 2026-09-22 as the recommendation): one word that
    // draws many buttons, made from a list as a choice is, and asked `.chosen`
    // as a choice is. The other spelling -- `.group(other_checkbox)` on a
    // checkbox -- stays the author's to ask for; nothing here forecloses it.
    {27, "window", 22, "one_of", 1, satellite_window::one_of, AWord::items,
     "satellite.window.one_of takes a list of what a person may pick one of: "
     "satellite.window.one_of({\"small\", \"large\"})"},
    // A CONSOLE IS UNDER satellite.console AND NOT satellite.window (GTK-17, the
    // author's own spelling of 2026-09-22: "satellite.console.new"): a window
    // whose whole inside is a terminal, made from the three things a window is
    // made from. `1 5 10`, the next number free under satellite.console -- the
    // first word 004 has put under satellite.console (003 ended at 1 5 9).
    {5, "console", 10, "new", 3, satellite_window::console, AWord::words,
     "satellite.console.new takes a title, a width and a height: "
     "satellite.console.new(\"my console\", 800, 600)"},
};

// A WINDOW AND A CONSOLE ARE FRAMES; everything else is a piece that goes in
// one. Asked wherever the table's `makes` used to be compared with `window`.
constexpr bool a_frame(satellite_window::Piece piece)
{
    return piece == satellite_window::window || piece == satellite_window::console;
}

// WHICH ROW OF kWords A CODE IS, AS A TABLE THE COMPILER FILLS IN: one byte a word code,
// so word_at is one read. IT WAS A LINEAR SCAN, calling code_of() -- a binary search --
// twice a row, on the belief that it was "asked once a window word in a program, not once
// a line". It was asked on EVERY word call: call_word asks is_window_word before any word
// reaches its own path, and the checker asks it of every call it reads. callgrind on
// build 0108 (2026-09-26): ~8,600 instructions a satellite.console.display, three
// quarters of what a display cost, spent here.
//
// FIRST ROW WINS, as the scan's order did: a code is filled in only while its byte is empty.
//
// A WORD THAT TAKES NOTHING IS **TWO ROWS** IN words.tsv, and both answer the
// one row here. That is satellite.infinity's own shape -- `1 26` is the name and
// `1 26 0` is the call -- and it exists for the refusal rather than for the
// call: with only `satellite.window.switch()` registered, writing
// `satellite.window.switch("on")` matches no word at all and is refused as **"no
// capsule named switch"**, which tells a person nothing. Measured 2026-09-21;
// `satellite.window.nosuchword("on")` says exactly the same thing, which is what
// proved it was the unregistered NAME and not the switch.
constexpr std::size_t kWordRows = sizeof kWords / sizeof kWords[0];
static_assert(kWordRows < 255, "a row is one byte, and 255 means no row");
constexpr unsigned char kNoRow = 255;

struct RowOfCode {
    unsigned char row[word::kWordFactsCount];
};

constexpr RowOfCode rows_of_codes()
{
    RowOfCode made{};
    for (unsigned char &each : made.row)
        each = kNoRow;
    const auto fill = [&made](Code code, std::size_t row) {
        const bool a_word = code >= word::kFirst && code < word::kFirst + word::kWordFactsCount;
        if (a_word && made.row[code - word::kFirst] == kNoRow)
            made.row[code - word::kFirst] = static_cast<unsigned char>(row);
    };
    for (std::size_t at = 0; at < kWordRows; ++at) {
        fill(word::code_of(1, kWords[at].family, kWords[at].number), at);
        if (kWords[at].arity == 0)
            fill(word::code_of(1, kWords[at].family, kWords[at].number, 0), at);
    }
    return made;
}

constexpr RowOfCode kRowOfCode = rows_of_codes();

const AWord *word_at(Code code)
{
    if (code < word::kFirst || code >= word::kFirst + word::kWordFactsCount)
        return nullptr;
    const unsigned char row = kRowOfCode.row[code - word::kFirst];
    return row == kNoRow ? nullptr : &kWords[row];
}

} // namespace

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
        if (!a_frame(row.makes))
            ++left;
    for (const AWord &row : kWords) {
        if (a_frame(row.makes))
            continue;
        if (out.empty())
            out = "satellite.window." + std::string(row.spelling) + "(\"text\")";
        else
            out += (left == 1 ? " or ." : ", .") + std::string(row.spelling);
        --left;
    }
    return out;
}

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

// ---------------------------------------------------------------------------
// THE WORD, CALLED. Twice -- once for a satl with a window and once for one
// without -- and both halves answer the same shapes, in this one file.
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
    const std::string called = "satellite." + std::string(row->family_name) + "." + std::string(row->spelling);

    // EVERY PIECE MADE FROM ONE LINE OF TEXT GOES THROUGH HERE, and a widget
    // added to the table above needs no branch of its own -- the Piece in its row
    // is what window_pieces.cpp turns into a GtkWidget. `new` is the one word
    // that is not this shape, and it falls past.
    if (!a_frame(row->makes)) {
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
            // AND A satl BUILT WITHOUT VTE SAYS SO WITH THE BUILD'S CODE, not the
            // machine's (GTK-17): "built without a console" is not_built_yet,
            // told apart by the sentence as "no display" is.
            const bool not_built = why.find("built without a console") != std::string::npos;
            const signed long long int code =
                a_bad_file ? (why.find("No such file or directory") != std::string::npos
                                  ? file_not_found
                                  : file_unreadable)
                           : (the_program ? satl_line_not_understood
                                          : not_built ? not_built_yet : no_display);
            context.refuse(code, called + " could not be made -- " + why);
            return Value();
        }
        return Value::of_window(std::move(made));
    }

    // A FRAME: a window, or a console (GTK-17), made from the same three things.
    std::string title;
    unsigned long long int wide = 0, tall = 0;
    if (!text_of(arguments[0], title, called, context) ||
        !size_of(arguments[1], wide, called + "'s width", context) ||
        !size_of(arguments[2], tall, called + "'s height", context))
        return Value();
    WindowHandle made = row->makes == satellite_window::console ? console_new(title, wide, tall, why)
                                                                : window_new(title, wide, tall, why);
    if (made == nullptr) {
        // NO SCREEN IS THE MACHINE'S ANSWER AND A BAD SIZE IS THE PROGRAM'S, and
        // they are told apart by which one window_new checked first: it refuses a
        // size before it ever asks for a display. A satl WITHOUT VTE is the
        // build's, and the sentence says so (window_console.cpp).
        const bool the_program = wide == 0 || tall == 0 || wide > 32767 || tall > 32767;
        const bool not_built = why.find("built without a console") != std::string::npos;
        context.refuse(the_program ? satl_line_not_understood : not_built ? not_built_yet : no_display,
                       called + " could not open a window -- " + why);
        return Value();
    }
    return Value::of_window(std::move(made));
}

#else

Value call_window_word(Code code, const std::vector<Value> &, ExpressionContext &context)
{
    return no_window_here(word::spelling_of(code), context);
}

#endif

} // namespace satellite004
