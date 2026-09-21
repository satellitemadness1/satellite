// satellite/satellite_variable_window/window_state.cpp -- WHAT A PIECE IS SET
// TO, read and written: `.on`, `.value` and `.chosen`.
// GTK_AND_NO_DEPENDENCIES.md GTK-3, GTK-4 and GTK-5.
//
// SPLIT OUT OF window_asks.cpp AT GTK-5, at 398 lines against the author's "try
// to build for 300 lines". Its neighbour keeps a piece's WORDS; this keeps its
// STATE, and the difference is not filing -- a label's words are satellite's
// own and were handed to the factory, while a checkbox's tick, a slider's
// position and a choice's pick were NEVER ours. Every read here goes to the
// desk, because the thing being read belongs to the person using the window.
//
// NOTHING HERE FALLS BACK ON THE HANDLE, and that is the other half of the same
// point. `.text` on a closed button still answers, because satellite wrote those
// words. `.on`, `.value` and `.chosen` on a closed piece are REFUSED: whatever
// they last were was the person's, and it went with the window.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

namespace satellite004 {
namespace {

// ---------------------------------------------------------------------------
// ON AND OFF (GTK-3).
// ---------------------------------------------------------------------------
//
// ONLY A CHECKBOX AND A SWITCH, and everything else is refused by name rather
// than answered `false`. A label is not "off"; it has no such question, and
// inventing an answer for it is exactly the kind of quiet wrongness this
// language refuses.

bool is_turned_on_or_off(const satellite_window &which)
{
    return which.piece == satellite_window::checkbox || which.piece == satellite_window::a_switch;
}

// WHY `which` AND NOT A RAW WIDGET: a checkbox is a GtkCheckButton and a switch
// is a GtkSwitch, and their getters are different functions on unrelated types.
// The Piece is the only thing that can tell them apart.
bool is_it_on(GtkWidget *widget, satellite_window::Piece piece)
{
    if (piece == satellite_window::checkbox)
        return gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)) != FALSE;
    return gtk_switch_get_active(GTK_SWITCH(widget)) != FALSE;
}

} // namespace

bool window_on_of(satellite_window &which, bool &out, std::string &why)
{
    if (!is_turned_on_or_off(which)) {
        why = std::string(which.piece_name()) + " is not something that is turned on or off -- "
              "a checkbox and a switch are";
        return false;
    }
    // NOTHING TO FALL BACK ON, unlike `.text`. A checkbox's state was never
    // satellite's, so a closed one has no last-known answer that is worth
    // anything -- it is refused, and the sentence is the same one `.text` uses.
    if (which.widget == nullptr) {
        why = "it is closed -- read .on while the window is still open";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    bool got = false;
    on_the_desk([widget, piece, &got] { got = is_it_on(widget, piece); });
    out = got;
    return true;
}

bool window_set_on(satellite_window &which, bool on, std::string &why)
{
    if (!is_turned_on_or_off(which)) {
        why = std::string(which.piece_name()) + " is not something that is turned on or off -- "
              "a checkbox and a switch are";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    on_the_desk([widget, piece, on] {
        if (piece == satellite_window::checkbox)
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), on ? TRUE : FALSE);
        else
            gtk_switch_set_active(GTK_SWITCH(widget), on ? TRUE : FALSE);
    });
    return true;
}

// ---------------------------------------------------------------------------
// THE NUMBER A PIECE IS AT (GTK-4).
// ---------------------------------------------------------------------------
//
// THE UNITS ARE THE PIECE'S OWN and satellite_window.hpp says which: a slider
// and a number box are whole numbers, and a progress bar is MILLIONTHS. The
// millionths exist because GTK holds a double and satellite's percentage is
// exact to 32 digits; a whole number of millionths is the largest unit both can
// say without either of them rounding. window_calls.cpp does the rest.
namespace {

constexpr long long int kMillion = 1000000;

bool has_a_value(const satellite_window &which)
{
    return which.piece == satellite_window::slider || which.piece == satellite_window::number_box ||
           which.piece == satellite_window::progress;
}

} // namespace

bool window_value_of(satellite_window &which, long long int &out, std::string &why)
{
    if (!has_a_value(which)) {
        why = std::string(which.piece_name()) + " has no number -- a slider, a number box and a "
              "progress bar do";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed -- read .value while the window is still open";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    double got = 0.0;
    on_the_desk([widget, piece, &got] {
        if (piece == satellite_window::progress)
            got = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(widget)) * static_cast<double>(kMillion);
        else if (piece == satellite_window::number_box)
            got = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
        else
            got = gtk_range_get_value(GTK_RANGE(widget));
    });
    // ROUNDED, NOT TRUNCATED. GTK's double for a slider left exactly on 50 can
    // come back as 49.999999999999996, and a language whose numbers are whole
    // must not answer 49 for a slider a person put on 50.
    out = static_cast<long long int>(got < 0.0 ? got - 0.5 : got + 0.5);
    return true;
}

bool window_set_value(satellite_window &which, long long int to, std::string &why)
{
    if (!has_a_value(which)) {
        why = std::string(which.piece_name()) + " has no number -- a slider, a number box and a "
              "progress bar do";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    on_the_desk([widget, piece, to] {
        if (piece == satellite_window::progress) {
            // CLAMPED HERE AND NOT REFUSED. A progress bar past its end is a
            // program counting slightly wrong, not a program that has gone
            // wrong -- and GTK draws a fraction above 1.0 as a bar longer than
            // its own frame. A slider and a number box need no clamp: GTK holds
            // them inside the range they were made with.
            const double fraction = static_cast<double>(to) / static_cast<double>(kMillion);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget),
                                          fraction < 0.0 ? 0.0 : (fraction > 1.0 ? 1.0 : fraction));
        } else if (piece == satellite_window::number_box) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), static_cast<double>(to));
        } else {
            gtk_range_set_value(GTK_RANGE(widget), static_cast<double>(to));
        }
    });
    return true;
}

// ---------------------------------------------------------------------------
// WHICH ITEM IS PICKED (GTK-5).
// ---------------------------------------------------------------------------
//
// THE MODEL IS THE ONE COPY OF THE ITEMS. The handle does not keep a second
// list, so there is no way for the two to disagree about what a choice offers --
// and searching a GtkStringList is a few strcmp on a list a person is going to
// read, which is not a size worth indexing.
namespace {

// GTK_INVALID_LIST_POSITION is what a drop-down answers when nothing is picked,
// and it is not a position -- it is 0xFFFFFFFF, so treating it as one reads off
// the end of the model.
bool what_is_picked(GtkWidget *widget, std::string &out)
{
    GtkDropDown *choice = GTK_DROP_DOWN(widget);
    const guint at = gtk_drop_down_get_selected(choice);
    if (at == GTK_INVALID_LIST_POSITION)
        return false;
    GtkStringList *items = GTK_STRING_LIST(gtk_drop_down_get_model(choice));
    const char *got = items == nullptr ? nullptr : gtk_string_list_get_string(items, at);
    out = got == nullptr ? std::string() : std::string(got);
    return true;
}

bool pick_it(GtkWidget *widget, const std::string &wanted)
{
    GtkDropDown *choice = GTK_DROP_DOWN(widget);
    GtkStringList *items = GTK_STRING_LIST(gtk_drop_down_get_model(choice));
    if (items == nullptr)
        return false;
    const guint many = g_list_model_get_n_items(G_LIST_MODEL(items));
    for (guint at = 0; at < many; ++at) {
        const char *got = gtk_string_list_get_string(items, at);
        if (got != nullptr && wanted == got) {
            gtk_drop_down_set_selected(choice, at);
            return true;
        }
    }
    return false;
}

} // namespace

bool window_chosen_of(satellite_window &which, std::string &out, std::string &why)
{
    if (which.piece != satellite_window::choice) {
        why = std::string(which.piece_name()) + " has nothing to choose from -- a choice does";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed -- read .chosen while the window is still open";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    std::string got;
    // NOTHING PICKED IS "" AND NOT A REFUSAL. A choice a person has not touched
    // is an ordinary state of a choice, not a mistake anybody made.
    on_the_desk([widget, &got] { what_is_picked(widget, got); });
    out = got;
    return true;
}

bool window_set_chosen(satellite_window &which, const std::string &to, std::string &why)
{
    if (which.piece != satellite_window::choice) {
        why = std::string(which.piece_name()) + " has nothing to choose from -- a choice does";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    bool found = false;
    on_the_desk([widget, &to, &found] { found = pick_it(widget, to); });
    if (!found) {
        // REFUSED AND NOT SILENTLY IGNORED. gtk_drop_down_set_selected on a
        // position that is not there simply picks nothing, and a program that
        // asked for an item this choice does not offer has said something untrue
        // about itself -- it should hear so.
        why = "there is no \"" + to + "\" to choose here";
        return false;
    }
    return true;
}

} // namespace satellite004
