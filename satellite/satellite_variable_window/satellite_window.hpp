#pragma once
// satellite/satellite_variable_window/satellite_window.hpp -- satellite.variable.window,
// the thirteenth arm. SATELLITE_WINDOW.md WIN-2 and WIN-3.
//
// The author, 2026-09-19: *"we are wiring in GTK+ so that satl and satl-term
// become a single application"*, and the shape he wrote:
//
//     satellite.variable.window my_window = satellite.window.new("window_title", 800, 600)
//     my_window.append(satellite.window.button("text"), 400, 300)
//
// THERE IS NO GTK IN THIS HEADER, ON PURPOSE. satellite_object.hpp includes it,
// so every file that holds a value would otherwise include gtk/gtk.h -- 004's
// own object model would stop compiling on a machine with no GTK, and satl is
// the one thing 047-window.mk promises builds everywhere. The widget is a
// `void *` here and a GtkWidget * in exactly three files -- window_desk.cpp,
// satellite_window.cpp and window_pieces.cpp -- every one of which is compiled
// only when pkg-config finds gtk4.
//
// A WINDOW AND A BUTTON ARE ONE TYPE, not two arms. `satellite.window.button()`
// answers something that is only ever handed straight to `.append`, and the
// author's own line never declares a name for it. Two arms would have meant two
// entries in every switch over Kind to say the same thing twice; a `piece` that
// says which it is costs one byte and no switch at all.
//
// THE HANDLE IS SHARED, as a file's and an infinity's are: two names for one
// window are the same window, because a window is a thing on a screen and not a
// value to be copied. It is also shared with the desk (window_desk.hpp), which
// holds its own strong reference for exactly as long as the window is open --
// that is what makes the `destroy` signal safe to handle after the program has
// dropped its last name for it.

#include <memory>
#include <string>
#include <vector>

namespace satellite004 {

class satellite_window;
using WindowHandle = std::shared_ptr<satellite_window>;

// ENABLE_SHARED_FROM_THIS, and it is here for the press (WIN-11). GTK hands a
// signal handler a raw pointer, and a press must answer the capsule with the
// PIECE that was pressed -- which is a Value, which is a handle. Without this
// the desk would have to search every open window's `pieces` for a matching
// address to turn one back into the other, which is a lookup that can fail and
// a null nobody knows what to do with.
//
// IT IS ALWAYS VALID WHERE IT IS USED. Every satellite_window is made with
// make_shared, and the desk holds its own strong reference for as long as the
// piece is on a screen -- which is the only time a press can arrive.
class satellite_window : public std::enable_shared_from_this<satellite_window> {
public:
    // WHICH PIECE OF A WINDOW THIS IS. `window` is the thing with a frame;
    // everything after it is a thing put inside one -- and since GTK-7 some of
    // those hold pieces of their own.
    //
    // `how_many_pieces` IS NOT A PIECE, it is the count -- and it is what makes
    // adding one below without naming it a COMPILE ERROR rather than a widget
    // that is refused as "a button". See kPieceNames under this class.
    // `a_switch` AND NOT `switch`, which is a C++ keyword. The name a person
    // sees is in kPieceNames and is "a switch"; this is the only place the
    // language's spelling and C++'s disagree, and renaming the piece would have
    // been letting C++ choose satellite's words.
    enum Piece { window, button, label, text_box, text_area, checkbox, a_switch,
                 slider, number_box, progress, choice,
                 row, column, grid, picture, how_many_pieces };

    Piece piece = window;

    // THE GtkWidget *, AS A void *. Only window_desk.cpp, satellite_window.cpp and
    // window_pieces.cpp ever cast it back, and all three are compiled only where
    // GTK is.
    void *widget = nullptr;

    // THE GtkFixed INSIDE A WINDOW, which is what `.append` puts a piece into.
    // GTK4 has no absolute position in a box, so a window that a program places
    // things in BY COORDINATE must hold a GtkFixed (WIN-3).
    void *inside = nullptr;

    // WHAT WAS APPENDED INTO IT, held so that a window going away can say so to
    // every piece inside it. GTK destroys a window's children with the window,
    // and a button's handle that a program still holds would otherwise keep a
    // GtkWidget * that GTK has already freed.
    std::vector<WindowHandle> pieces;

    // WHETHER THIS PIECE HOLDS OTHERS (GTK-7). A row, a column and a grid do;
    // everything else is a leaf. It is asked in four places -- what `.append`
    // takes, where a piece is put, what a teardown has to walk, and which window
    // a press happened in -- so it is one question here and not four tests.
    bool holds_pieces() const { return piece == row || piece == column || piece == grid; }

    // AND THE WINDOW A PIECE IS IN, the other way along that same line. WEAK,
    // because the window already holds this piece STRONGLY: two strong
    // references in a ring is a window and a button that keep each other alive
    // for ever, and a weak one back costs nothing and cannot.
    //
    // It is what lets a pressed capsule reach the window it was pressed in --
    // a button's own `.close()` is refused, because only a window closes.
    //
    // IT POINTS AT THE IMMEDIATE PARENT AND NOT AT THE WINDOW (GTK-7). A button
    // in a row in a window points at the ROW. `the_window_holding()` below is
    // what walks the rest of the way, and everything that wants "the window this
    // happened in" must go through it -- reading `inside_of` directly was right
    // when a window was the only thing a piece could be in, and has been wrong
    // since rows existed.
    std::weak_ptr<satellite_window> inside_of;

    std::string title;            // what it was made with, and what .title reads back

    // WHAT SIZE A WINDOW ASKED FOR (GTK-8). A window that is not on the screen
    // yet has no size at all -- gtk_widget_get_width answers 0 until a
    // compositor has mapped it -- and 0 is not what a program that just wrote
    // `new("t", 800, 600)` means by `.width`. This is what it answers instead,
    // and it stops being the answer the moment there is a real one.
    //
    // A COMPOSITOR MAY NOT GIVE A WINDOW THE SIZE IT ASKED FOR. Tiling ones
    // routinely do not, so the real size wins wherever there is one: a window
    // that reports its wish rather than its size is the failure this project
    // keeps naming.
    int asked_wide = 0;
    int asked_tall = 0;

    // THE WORDS ON A PIECE: a button's label, a label's line, what a person
    // typed. FOR A BUTTON AND A LABEL THIS IS THE TRUTH -- nothing but satellite
    // ever writes them. FOR A TEXT BOX IT IS A CACHE and GTK holds the original:
    // a person typing changes the widget and tells us nothing, so `.text`
    // refreshes this out of the widget before answering (window_text_of).
    //
    // WRITTEN ONLY ON THE INTERPRETER'S THREAD. The desk reads widgets; it never
    // writes this.
    std::string text;
    bool on_the_screen = false;   // false once it is closed, whoever closed it

    // THE CAPSULE A PRESS RUNS, by name, and empty for a piece that answers
    // nobody (WIN-11). A NAME AND NOT A CAPSULE: a capsule is arm 5 of the
    // object model and nothing in the language makes one yet, and the walker
    // finds a user's capsule BY NAME anyway (program_walk.hpp's CapsuleTable) --
    // so the name IS the reference, and no body is copied to hold it.
    //
    // WRITTEN AND READ ON THE DESK'S THREAD ONLY. `.pressed()` sets it inside
    // an on_the_desk() lambda and the `clicked` handler reads it there, so the
    // two never race and no second mutex is needed for one string.
    std::string when_pressed;
    bool press_is_connected = false;   // `clicked` is connected once, not once a call

    // AND THE SAME AGAIN FOR THE OTHER TWO THINGS A PIECE CAN SAY (GTK-9).
    // `when_changed` is a person changing this piece -- typing in it, moving it,
    // ticking it, picking in it. `when_closed` is a WINDOW going away, whoever
    // took it away.
    //
    // THREE STRINGS AND NOT ONE MAP, because there are three and a map would be
    // a lookup on the desk's thread inside a signal handler to answer a question
    // with three possible keys. A fourth signal is a fourth string; a tenth
    // would be the map.
    std::string when_changed;
    bool changed_is_connected = false;
    std::string when_closed;

    // WHAT THIS PIECE IS WEARING (GTK-10). GTK4 has no per-widget colour setter
    // -- everything is CSS -- so a piece that has been dressed carries a css
    // class nobody else has and a GtkCssProvider scoped to it, and the parts are
    // kept because a provider holds ONE stylesheet: setting `.colour` after
    // `.font` has to say the font again or it would take it away.
    //
    // ALL OF THEM WRITTEN AND READ ON THE DESK'S THREAD ONLY, inside
    // on_the_desk(), which is the same rule `when_pressed` follows.
    void *look = nullptr;             // the GtkCssProvider, as a void * (no GTK in this header)
    std::string style_class;          // "satl-7", handed out by the desk and nobody else
    std::string a_colour;
    std::string a_background;
    std::string a_font;
    int a_font_size = 0;

    satellite_window() = default;
    explicit satellite_window(Piece which) : piece(which) {}

    // WHAT THIS PIECE IS CALLED. A TABLE AND NOT A `?:` SINCE GTK-1: two pieces
    // fit in a conditional and eighteen do not, and every refusal in this module
    // names the piece it was given -- so the naming is one place a new piece is
    // added to, or the tenth widget is refused as "a button".
    const char *piece_name() const;     // "a label"  -- inside a refusal
    const char *piece_shown() const;    // "label"    -- what display() prints
};

// THE WINDOW A PIECE IS IN, however deep it is (GTK-7). Walks `inside_of` up
// until it finds the thing with a frame, and answers null for a piece that is in
// nothing yet -- which is every piece before it is appended.
//
// IT CANNOT LOOP. `.append` refuses to put a piece into something already inside
// it, so the chain it walks is a tree by construction; the step count is capped
// anyway, because a bug in that refusal must not become a hang.
WindowHandle the_window_holding(const satellite_window &piece);

// EVERY PIECE, NAMED ONCE, IN Piece's OWN ORDER. `the` is what a refusal calls
// it; `shown` is the word a program sees when it displays one -- (label "text").
//
// THE static_assert IS THE WHOLE POINT OF THE TABLE. A `Piece` added to the enum
// and not given a row here does not compile, which is the only way a table like
// this stays true -- a short initialiser would zero-fill and answer nullptr at
// the moment somebody was being told what they did wrong.
struct PieceNames {
    const char *the;
    const char *shown;
};

inline constexpr PieceNames kPieceNames[] = {
    {"a window", "window"},
    {"a button", "button"},
    {"a label", "label"},
    {"a text box", "text box"},
    {"a text area", "text area"},
    {"a checkbox", "checkbox"},
    {"a switch", "switch"},
    {"a slider", "slider"},
    {"a number box", "number box"},
    {"a progress bar", "progress bar"},
    {"a choice", "choice"},
    {"a row", "row"},
    {"a column", "column"},
    {"a grid", "grid"},
    {"a picture", "picture"},
};

static_assert(sizeof(kPieceNames) / sizeof(*kPieceNames) == satellite_window::how_many_pieces,
              "a Piece was added without a name -- give it a row in kPieceNames");

inline const char *satellite_window::piece_name() const { return kPieceNames[piece].the; }
inline const char *satellite_window::piece_shown() const { return kPieceNames[piece].shown; }

// ---------------------------------------------------------------------------
// WHAT A PROGRAM CAN DO TO ONE.
// ---------------------------------------------------------------------------
//
// EVERY ONE OF THESE IS CALLED ON THE INTERPRETER'S THREAD and does its work on
// the desk's (window_desk.hpp): GTK4 is not thread-safe and every call must
// happen on the thread that called gtk_init. None of them throws; a failure is
// `why` filled in and a null handle or false.
//
// THEY EXIST IN BOTH BUILDS. When pkg-config found no gtk4 the bodies are not
// compiled at all -- bytecode/window_calls.cpp refuses before it reaches one,
// which is the single place this satl says it was built without a window.

// `satellite.window.new(title, width, height)`. A width or a height of 0 is
// refused, because a window nobody can see is not what was asked for.
WindowHandle window_new(const std::string &title, unsigned long long int width,
                        unsigned long long int height, std::string &why);

// `satellite.window.button(text)`, `satellite.window.label(text)`, and every
// piece after them that is made from one line of text.
//
// ONE FUNCTION AND NOT ONE A WIDGET, because they differ in exactly one thing --
// which GtkWidget is asked for. Eighteen near-identical declarations would be
// eighteen places for the nineteenth to be left out of, and the table in
// bytecode/window_calls.cpp already says which word answers which `Piece`; this
// is what turns that Piece into a widget.
//
// A LABEL DRAWS AND ANSWERS NOBODY. It is not focusable and cannot be pressed,
// so `a_label.pressed(c)` is refused by the sentence that already refuses it on
// a window -- "only a button is pressed".
//
// A CHECKBOX AND A SWITCH ARE THE SAME QUESTION DRAWN TWICE, and they are two
// pieces for the reason the text box and the text area are: GTK makes them two
// widgets, and a person choosing between them is choosing how it looks to
// somebody. A switch takes no words at all -- it is the one piece so far whose
// word takes nothing.
//
// A RADIO GROUP IS NOT HERE. GTK4 makes one by giving a check button ANOTHER as
// its group, so a radio is not a widget -- it is two pieces that know about each
// other, and satellite has no spelling for that. GTK-3 leaves it to the author.
//
// A TEXT BOX IS ONE LINE AND A TEXT AREA IS MANY, and they are TWO PIECES rather
// than one with a flag, because GTK makes them two widgets and a person typing a
// name and a person typing a page are different things. One word with a flag
// would be satellite hiding a distinction it cannot actually hide: a text area's
// words live in a GtkTextBuffer and a text box's in the widget itself.
//
// NOT ON ANY SCREEN until it is appended. A null handle with `why` filled in
// when there is no display to draw on, or when the Piece is not one made this
// way -- a window is not, and window_new above is what makes one.
WindowHandle window_piece_of_text(satellite_window::Piece which, const std::string &text,
                                  std::string &why);

// AND THE PIECES WHOSE WORD TAKES NUMBERS INSTEAD (GTK-4): a slider and a number
// box are made from the range they run over, and a progress bar from nothing --
// it is here rather than beside the text pieces because nothing about it is
// words.
//
// `least` AND `most` MUST NOT BE THE SAME. GTK takes a zero-width range and
// draws a slider that cannot move, which is an answer that is wrong and does not
// say so; it is refused where it is written.
WindowHandle window_piece_of_numbers(satellite_window::Piece which, long long int least,
                                     long long int most, std::string &why);

// AND THE THIRD SHAPE (GTK-5): a piece made from a LIST of words.
//
// THE ITEMS ARE COPIED, and that is the whole ownership decision. A live view of
// a satellite list into a GTK model would be a second ownership story across the
// interpreter/desk line, and this project already has one of those and knows
// what it costs. A choice made from a list is a choice made from what the list
// SAID; changing the list afterwards changes nothing on the screen.
//
// AN EMPTY LIST IS REFUSED: a choice with nothing to choose from is a control a
// person can do nothing with.
WindowHandle window_piece_of_items(satellite_window::Piece which,
                                   const std::vector<std::string> &items, std::string &why);

// AND THE FOURTH (GTK-6): a piece made from a FILE on the disk.
//
// A FILE THAT IS NOT THERE IS REFUSED WHERE IT IS WRITTEN.
// gtk_picture_new_for_filename() on a path that does not exist gives a widget
// that draws nothing and says nothing, which is the one kind of answer this
// project will not ship -- so the texture is loaded HERE, with its GError, and a
// missing or unreadable or corrupt file comes back as a sentence.
//
// IT IS WHAT MAKES gdk-pixbuf, libpng, libjpeg-turbo AND libtiff REACHABLE.
// Those four are ~2.6 MB of satl and had been carried since the first vendored
// build with no satellite word able to touch a line of them
// (GTK_AND_NO_DEPENDENCIES.md Part 00, Table B).
WindowHandle window_piece_of_a_file(satellite_window::Piece which, const std::string &path,
                                    std::string &why);

// `a_piece.text("what it says now")` -- the words ON a piece. A window is
// REFUSED here and told to use `.title` instead: a window's words are its title,
// and answering the title to `.text` would be two names for one thing, which is
// the shape this language spends its refusals avoiding.
//
bool window_set_text(satellite_window &which, const std::string &text, std::string &why);

// `a_piece.text` -- THE WORDS ON A PIECE, READ BACK, and the first thing
// satellite ever reads back OUT of GTK (GTK-2).
//
// A button's and a label's words are ours: nothing but satellite writes them, so
// the handle is the truth. A TEXT BOX'S ARE NOT -- a person typing changes the
// widget and tells us nothing -- so this crosses to the desk, copies what is
// there into the handle, and answers that. `on_the_desk()` already waits for the
// job it posts, so bringing a value back is the mechanism that was there and had
// never been used in this direction.
//
// A CLOSED BUTTON OR LABEL STILL ANSWERS, because its words were always ours.
// A CLOSED TEXT BOX REFUSES, and names when to read it instead. There is no
// moment in a GTK teardown at which a widget's words can still be asked for --
// gtk_entry_dispose clears them before the `destroy` signal it would have been
// rescued in, and window_desk.cpp carries the measurement -- so the choice is
// between refusing and answering whatever satellite last happened to write. This
// project does not ship the second kind of answer.
//
// A WINDOW IS REFUSED, and told to ask for `.title` -- as it is when it tries to
// SET `.text`, and for the same reason.
//
// `text` HAS EXACTLY ONE WRITER AND IT IS THIS THREAD. The desk never touches
// it: this reads the widget inside an on_the_desk() lambda that has finished
// before the assignment happens. That is deliberate -- a std::string written on
// one thread and read on another is undefined behaviour, and the only other
// cross-thread fields in this class are a pointer and two bools.
bool window_text_of(satellite_window &which, std::string &out, std::string &why);

// `a_checkbox.on` AND `a_checkbox.on(1)` -- WHETHER A THING IS TURNED ON, read
// and written (GTK-3). Only a checkbox and a switch have one; everything else is
// refused by name, because a piece that is neither on nor off answering `false`
// would be an answer that is wrong and does not say so.
//
// READ CROSSES TO THE DESK, as `.text` does and for the same reason: a person
// clicking a checkbox changes the widget and tells satellite nothing. Unlike
// `.text` there is nothing to cache and nothing to lose, so a CLOSED one is
// refused outright -- there is no "it was ours all along" half here.
bool window_on_of(satellite_window &which, bool &out, std::string &why);
bool window_set_on(satellite_window &which, bool on, std::string &why);

// `a_slider.value` AND `a_slider.value(50)` -- THE NUMBER A PIECE IS AT (GTK-4).
//
// THE UNITS ARE THE PIECE'S OWN, AND THIS IS THE ONE THING TO READ BEFORE
// CALLING IT. A slider and a number box answer the whole number they are at. A
// PROGRESS BAR ANSWERS MILLIONTHS -- 0 to 1,000,000 -- because what it holds is
// a `double` fraction of GTK's and a percentage is exact to 32 digits, so the
// two cannot meet without a unit that is a whole number. bytecode/window_calls.cpp
// is what turns millionths into a satellite.variable.percentage and back, and
// the conversion is exact in both directions: a percentage is held as itself
// times 10^32, so one millionth of the whole is exactly 10^28 of those.
//
// WHY NOT A double THROUGH THIS BOUNDARY: because satellite has no binary
// fraction anywhere and is not going to gain one in a window. GTK's slider is a
// double; satellite's slider is whole numbers, and a slider that needs fractions
// waits for the float arm (arm 14, not built).
bool window_value_of(satellite_window &which, long long int &out, std::string &why);
bool window_set_value(satellite_window &which, long long int to, std::string &why);

// `a_choice.chosen` AND `a_choice.chosen("green")` -- WHICH ITEM IS PICKED, AS
// TEXT (GTK-5).
//
// TEXT AND NOT AN INDEX, on purpose. A program that wanted the position has the
// list it made the choice from and can `.index_of` it; a program that has the
// position and not the list has a number that means nothing on its own. And a
// choice with nothing picked answers "" rather than a number no item has.
//
// SETTING IT REFUSES WHAT IS NOT THERE. `a_choice.chosen("purple")` on a choice
// of red and green is a program saying something untrue about itself, and
// silently picking nothing -- which is what GTK does -- is the answer that is
// wrong and does not say so.
bool window_chosen_of(satellite_window &which, std::string &out, std::string &why);
bool window_set_chosen(satellite_window &which, const std::string &to, std::string &why);

// `a_picture.path` AND `a_picture.path("other.png")` -- THE FILE A PICTURE SHOWS
// (GTK-6).
//
// `.path` AND NOT `.text`, and `.text` on a picture is refused and told to write
// this instead -- the same pairing a window has with `.title`. A path is not
// words on a piece; it is where a piece got what it draws.
//
// READING NEVER CROSSES TO THE DESK and answers after the window has closed,
// because a picture's path is satellite's own: we opened that file. It is a
// label's rule, not a text box's.
bool window_path_of(satellite_window &which, std::string &out, std::string &why);
bool window_set_path(satellite_window &which, const std::string &to, std::string &why);

// `my_window.append(piece, x, y)` -- BY ITS CENTRE (WIN-3): 400, 300 is the
// middle of an 800x600 window, not a corner. The piece's own measured size is
// halved and taken off at placement, which is the whole difference.
// `my_window.append(piece, x, y)` -- BY ITS CENTRE (WIN-3), and
// `a_row.append(piece)` -- ONE AFTER ANOTHER (GTK-7).
//
// `by_place` IS FALSE FOR A ROW OR A COLUMN, which have no coordinates at all,
// and true for a window (pixels) and a grid (CELLS, counting from 1 the way
// satellite counts a file's lines). Giving the wrong one is refused with a
// sentence naming the piece it was actually given, because WHICH ONE IS RIGHT IS
// THE RECEIVER'S and the checker cannot know it -- a satellite.variable.window
// name may hold a window or a row, and which it holds is not decided until the
// line that makes it RUNS.
bool window_append(satellite_window &into, const WindowHandle &piece, bool by_place,
                   long long int x, long long int y, std::string &why);

// `my_button.pressed(when_pressed)` -- the capsule to run when it is pressed
// (WIN-11). ONLY A BUTTON, because only a button is pressed; a window is
// refused here rather than silently answering nobody.
//
// THE CAPSULE IS NOT RUN HERE AND NOT ON THIS THREAD. A press puts the name on
// the desk's queue (window_desk.hpp) and the INTERPRETER's thread takes it off
// and walks it -- see the_desk_waits_for_something for why that is the only safe
// thread for it.
bool window_pressed(satellite_window &which, const std::string &capsule, std::string &why);

// `my_button.press()` -- THE PROGRAM PRESSES IT, as a person clicking would
// (WIN-11, the author 2026-09-21: *"It's just a mouse click, so it's not like
// there's any arguments to clicking on something"*). It takes nothing and it
// runs whatever `.pressed(...)` named, through the same path a real click takes.
//
// IT EMITS `clicked` AND DOES NOT CALL gtk_widget_activate(). Activate is the
// KEYBOARD path: for a button it needs the widget REALIZED and does nothing at
// all when it is not (gtkbutton.c:827), while still answering TRUE -- an answer
// that is wrong and does not say so. A mouse release emits `clicked` directly
// (gtkbutton.c:802), which is what this does.
bool window_press(satellite_window &which, std::string &why);

// `a_piece.changed(when_changed)` -- THE CAPSULE A PERSON CHANGING THIS PIECE
// RUNS (GTK-9). Typed in, moved, ticked, picked in -- one question and one
// method, not a name a widget.
//
// ONLY A PIECE THAT A PERSON CAN CHANGE. A label, a picture, a row and a
// progress bar are refused by name: nothing a PERSON does changes any of them,
// and a capsule wired to one would simply never run, which is the quietest
// possible way for a program to be wrong. A BUTTON is refused too, and sent to
// `.pressed` -- a button is not changed, it is pressed.
//
// WHAT THE CAPSULE IS HANDED IS WHAT A PRESS IS HANDED: nothing, the piece, or
// the piece and its window. **IT IS NOT HANDED THE NEW VALUE**, and that is the
// author's open question in GTK-9 -- the recommendation was that it should not
// be, because the piece is already there and `the_piece.text` is one short line.
bool window_changed(satellite_window &which, const std::string &capsule, std::string &why);

// `my_window.closed(when_closed)` -- THE CAPSULE A WINDOW GOING AWAY RUNS
// (GTK-9), whoever took it away: the program calling `.close()` and a person
// clicking the close button are the same event, because both are GTK's
// `destroy`. That is the same reason the desk's own bookkeeping lives there.
//
// IT IS ALWAYS DRAINED AND NEVER WAITED FOR. By the time it runs the window is
// gone -- `w.ok` is false and `.text` on a text box inside it refuses -- so a
// capsule that wants what was in a window must read it before closing, not
// after. NO SIGNAL IS CONNECTED HERE: `destroy` is already connected by
// window_new, and this only writes the name that handler reads.
bool window_closed(satellite_window &which, const std::string &capsule, std::string &why);

bool window_close(satellite_window &which, std::string &why);
bool window_focus(satellite_window &which, std::string &why);
bool window_set_title(satellite_window &which, const std::string &title, std::string &why);

// `my_window.resize(1024, 768)` -- ASK FOR A SIZE (GTK-8). ONLY A WINDOW: a
// piece inside one is sized by what holds it, and a button told to be 300 wide
// in a row is a button arguing with the row.
//
// ASKED, NEVER TAKEN, which is the same sentence `.focus()` carries. A
// compositor decides; a tiling one will ignore this entirely, and that is not a
// failure and must not be reported as one.
bool window_resize(satellite_window &which, long long int wide, long long int tall, std::string &why);

// `a_piece.width` AND `a_piece.height` -- HOW BIG IT IS ON THE SCREEN RIGHT NOW.
// Any piece, not just a window: what a button turned out to measure is a real
// question, and it is the one `.append` answers by centring.
//
// WHAT IT ANSWERS WHEN THERE IS NOTHING ON A SCREEN YET, in this order: the real
// size if a compositor has given it one; the size a WINDOW asked for; and for
// any other piece its own MEASURED natural size, which is what GTK would give it
// if it were placed now. Never 0 for a piece that exists, because 0 is an answer
// a program would act on and it would be acting on nothing.
bool window_size_of(satellite_window &which, bool the_height, long long int &out, std::string &why);

// `my_window.fullscreen` AND `my_window.fullscreen(1)` -- WHETHER IT FILLS THE
// SCREEN, read and written (GTK-8). ONLY A WINDOW.
//
// ASKED, NEVER TAKEN, again: gtk_window_fullscreen() is a request, and reading
// it back straight afterwards can still answer false because the compositor has
// not answered yet. A program that wants to know should ask when it matters,
// not immediately after asking.
bool window_fullscreen_of(satellite_window &which, bool &out, std::string &why);
bool window_set_fullscreen(satellite_window &which, bool on, std::string &why);

// `a_piece.colour("#00ff88")`, `a_piece.background("#222222")` and
// `a_piece.font("IBM Plex Mono", 12)` -- WHAT A PIECE LOOKS LIKE (GTK-10).
//
// ANY PIECE, INCLUDING A WINDOW. GTK styles a GtkWindow like anything else, and
// a program that wants a dark window should be able to say so.
//
// THERE IS NO READING THEM BACK, and that is on purpose rather than unfinished.
// What a piece is WEARING is not what it LOOKS like: a theme, a parent's rule
// and the desktop's own settings all reach a widget, and answering only the part
// satellite wrote would be an answer that is wrong and does not say so. A
// program that wants to know what it asked for already knows.
//
// A COLOUR OR A FONT NAME SATELLITE WILL NOT PASS ON IS REFUSED. GTK parses
// these out of a stylesheet and a bad one is a parse error that takes the whole
// rule with it -- so `.colour("orange juice")` would leave the piece unstyled
// and say nothing at all.
bool window_set_colour(satellite_window &which, const std::string &colour, bool behind,
                       std::string &why);
bool window_set_font(satellite_window &which, const std::string &face, long long int size,
                     std::string &why);

// THE RUN DOES NOT END WHILE A WINDOW IS OPEN. Called once, from main(), after
// the program has returned: a program that opens a window and stops would
// otherwise take the window down with it before anybody saw it. Returns at once
// when nothing was ever opened, so a program with no window pays nothing.
//
// `the_program_finished` FALSE TAKES THE WINDOWS DOWN INSTEAD OF WAITING, and
// that difference was measured rather than designed: a program refused at a line
// AFTER it opened a window printed its report and then hung, because the window
// was still up and nothing was ever going to close it. A person who has just been
// told their program stopped is owed the prompt back, not a wait with no end.
void windows_stay_open_until_closed(bool the_program_finished);

} // namespace satellite004
