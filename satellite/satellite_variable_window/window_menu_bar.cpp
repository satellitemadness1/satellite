// satellite/satellite_variable_window/window_menu_bar.cpp -- A MENU MEETING A
// WINDOW, OR A MENU: `my_window.menu(file)` and `file.menu(recent)`.
// GTK_AND_NO_DEPENDENCIES.md GTK-12.
//
// SPLIT OUT OF window_menu.cpp ON 2026-09-22 (window_menu.hpp says how). That
// file is a menu's own MODEL and touches gtk once, to put a changed heading
// back on the bar the menu is already on; this one is where a menu meets a
// window, the only place the bar -- a GtkPopoverMenuBar -- is MADE and
// gtk_widget_insert_action_group is called. The decision the whole thing
// rests on, no GtkApplication, is written at the top of window_menu.cpp and
// holds here: the actions go on the WINDOW, under each menu's own prefix.
//
// EVERY GTK AND GIO CALL HAPPENS INSIDE on_the_desk(), like its neighbours.
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "window_menu.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

namespace satellite004 {
namespace {

// THE ACTIONS OF A MENU, AND OF EVERY MENU INSIDE IT, PUT ON A WINDOW. ON THE
// DESK. Under each menu's own prefix, which is how "menu3.item2" in a model
// finds the GSimpleAction to fire. A GtkApplicationWindow would have done this
// under "win." and cost a session bus; see the top of this file.
void put_the_actions_on(GtkWidget *window, satellite_window &menu)
{
    gtk_widget_insert_action_group(window, menu.action_prefix.c_str(), G_ACTION_GROUP(actions_of(menu)));
    for (const WindowHandle &under : menu.pieces)
        put_the_actions_on(window, *under);
}

// A MENU THAT REACHED A WINDOW PUTS EVERY MENU INSIDE IT ON THE SCREEN WITH IT,
// which is the rule `.append` already has for a row filled before it is placed.
void now_on_the_screen(satellite_window &menu)
{
    menu.on_the_screen = true;
    for (const WindowHandle &under : menu.pieces)
        now_on_the_screen(*under);
}

} // namespace

bool window_menu(satellite_window &which, const WindowHandle &menu, std::string &why)
{
    const bool on_a_window = which.piece == satellite_window::window;
    if (!on_a_window && which.piece != satellite_window::menu) {
        why = "only a window has a menu across its top, and only a menu has one inside it";
        return false;
    }
    // A WINDOW MUST BE ON A SCREEN; A MENU NEED NOT BE. A menu is built in full
    // before it goes anywhere, which is the ordinary order to write those lines
    // in -- the same rule `.append` has for a row.
    if (on_a_window ? (which.widget == nullptr || !which.on_the_screen) : which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    if (menu == nullptr || menu->piece != satellite_window::menu) {
        why = ".menu takes a menu, and was given " +
              std::string(menu == nullptr ? "nothing" : menu->piece_name()) +
              " -- satellite.window.menu(\"File\") makes one";
        return false;
    }
    if (menu->widget == nullptr) {
        why = "that menu is closed -- it went with the window it was on";
        return false;
    }
    if (menu.get() == &which) {
        why = "a menu cannot be put inside itself";
        return false;
    }
    // ALREADY SOMEWHERE. Putting one GMenu on two bars would draw it twice
    // and fire its actions from whichever window has the group; satellite says
    // it in a sentence first, as `.append` does for a piece already somewhere.
    if (const WindowHandle holding = menu->inside_of.lock()) {
        why = holding->piece == satellite_window::window ? "that menu is already across the top of a window"
                                                         : "that menu is already inside a menu";
        return false;
    }
    // AND NOT INTO A MENU IT ALREADY HOLDS, which is the only way `inside_of`
    // could be made to loop -- the same refusal `.append` carries, for the same
    // walk in the_window_holding().
    for (WindowHandle above = which.weak_from_this().lock(); above != nullptr; above = above->inside_of.lock()) {
        if (above.get() == menu.get()) {
            why = "a menu cannot be put inside a menu it already holds";
            return false;
        }
    }
    satellite_window *holder = &which;
    satellite_window *raw = menu.get();
    if (on_a_window) {
        on_the_desk([holder, raw] {
            GtkWidget *bar = static_cast<GtkWidget *>(holder->bar);
            if (bar == nullptr) {
                // THE BAR IS MADE ONCE, ON THE FIRST MENU, and it goes ABOVE the
                // fixed in the window's column -- which is why window_new() has
                // held a column since GTK-12. A window with no menu never pays
                // for one.
                GMenu *bar_model = g_menu_new();
                bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(bar_model));
                // THE BAR HOLDS THE MODEL NOW. Ours is spent.
                g_object_unref(bar_model);
                GtkWidget *column = gtk_widget_get_parent(static_cast<GtkWidget *>(holder->inside));
                gtk_box_prepend(GTK_BOX(column), bar);
                holder->bar = bar;
            }
            GMenuModel *bar_model = gtk_popover_menu_bar_get_menu_model(GTK_POPOVER_MENU_BAR(bar));
            g_menu_append_submenu(G_MENU(bar_model), raw->text.c_str(), G_MENU_MODEL(model_of(*raw)));
            put_the_actions_on(static_cast<GtkWidget *>(holder->widget), *raw);
        });
    } else {
        // INSIDE A MENU: an item with an arrow, in the parent's current section.
        // Its actions go on the window the parent is on, if it is on one yet;
        // if not, they go there with the parent's when the parent arrives.
        const WindowHandle in = the_window_holding(which);
        GtkWidget *window = in == nullptr ? nullptr : static_cast<GtkWidget *>(in->widget);
        on_the_desk([holder, raw, window] {
            g_menu_append_submenu(section_of(*holder), raw->text.c_str(), G_MENU_MODEL(model_of(*raw)));
            if (window != nullptr)
                put_the_actions_on(window, *raw);
        });
    }
    // THE SAME BOOKKEEPING `.append` DOES, for the same reasons: the holder
    // keeps the menu so a teardown can let go of it, and the menu knows what
    // it is in so a picked item's capsule can be handed its window.
    which.pieces.push_back(menu);
    menu->inside_of = which.weak_from_this();
    if (on_a_window || which.on_the_screen)
        now_on_the_screen(*menu);
    return true;
}
} // namespace satellite004
