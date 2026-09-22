// satellite/satellite_variable_window/window_menu.cpp -- A MENU ACROSS THE TOP
// OF A WINDOW, and the items on it. GTK_AND_NO_DEPENDENCIES.md GTK-12.
//
// THE ONLY MILESTONE THAT IS gio AND NOT gtk. A menu is a GMenu -- a MODEL of
// items -- and a GSimpleActionGroup with one action an item; nothing of GTK's is
// touched until the menu meets a window, and then it is one call:
// gtk_widget_insert_action_group on the GtkWindow. The bar that draws it is
// the window's, made the first time a menu is put on it and shared by every
// menu after.
//
// NO GtkApplication, AND THAT IS THE ONE DECISION THIS FILE RESTS ON. A
// GtkApplicationWindow would give these actions their "win." prefix for free
// -- and a GtkApplication is a GApplication, which registers on the D-Bus
// session bus, and a wedged portal hangs gtk_init_check for ever with nothing
// printed (Q-WIN-11a). satellite_window.cpp refused it on 2026-09-20 for the
// window; this refuses it for the menu. The prefix is handed out here instead.
//
// A MENU IS THE ONE PIECE THAT IS NOT A WIDGET. Its `widget` holds the GMenu,
// and satellite_window.hpp's is_drawn() is what keeps every gtk_widget_* caller
// from handing GTK a model. What that costs is five refusals by name --
// measuring, dressing, watching for a click, putting it in a fixed, and reading
// its words off a widget -- and what it buys is ONE bar for a whole window, with
// the keyboard walking across it, which two bars side by side would not give.
//
// satl-term/menu.cpp IS THE SAME GMenu AND THE SAME ACTIONS, ported from 003,
// and was read before this was written. What differs is exactly the two things
// above: no GtkApplication, and the model belongs to a satellite piece.
//
// EVERY GTK AND GIO CALL HAPPENS INSIDE on_the_desk(), like its neighbours.
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

#include <string>

namespace satellite004 {
namespace {

// A PREFIX NO OTHER MENU HAS. ON THE DESK'S THREAD ONLY, which is what makes a
// plain counter right -- window_look.cpp's style classes are handed out the
// same way and for the same reason.
unsigned long long int menus_so_far = 0;

// WHAT AN ACTION KNOWS: which menu it is on, and which capsule it runs. Owned by
// the action's closure and freed with it, so an item can never outlive the
// action GTK would fire it from.
//
// THE MENU IS A RAW POINTER AND IT IS ALWAYS VALID WHERE IT IS USED. An action
// fires only through the bar, the bar lives only while its window does, and
// the window holds every menu on it in `pieces` -- the same argument that makes
// a button's `clicked` handler safe (satellite_window.hpp, ENABLE_SHARED_FROM_THIS).
struct AnItem {
    satellite_window *menu;
    std::string capsule;
};

// A PERSON PICKED AN ITEM. ON THE DESK'S THREAD, in GSimpleAction's `activate`,
// doing the one thing that is safe here -- writing the capsule's name on the
// desk's queue. NOT COLLAPSED: picking Open twice is two picks, as three clicks
// are three presses.
void an_item_was_picked(GSimpleAction *, GVariant *, gpointer user_data)
{
    AnItem *item = static_cast<AnItem *>(user_data);
    the_desk_saw_something(item->capsule, item->menu->shared_from_this());
}

void forget_the_item(gpointer data, GClosure *) { delete static_cast<AnItem *>(data); }

GMenu *model_of(const satellite_window &which) { return static_cast<GMenu *>(which.widget); }
GSimpleActionGroup *actions_of(const satellite_window &which)
{
    return static_cast<GSimpleActionGroup *>(which.actions);
}

bool a_menu_that_is_open(satellite_window &which, std::string &why)
{
    if (which.piece != satellite_window::menu) {
        why = "only a menu has items -- " + std::string(which.piece_name()) + " does not";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    return true;
}

// WHERE A MENU SITS ON ITS BAR, or -1 when it is not on one. ON THE DESK. The
// bar's model links each heading to the menu's own GMenu, and a link comes back
// with a reference on it that must be given back.
int place_on_the_bar(GMenuModel *bar_model, GMenu *menu)
{
    const gint many = g_menu_model_get_n_items(bar_model);
    for (gint at = 0; at < many; ++at) {
        GMenuModel *under = g_menu_model_get_item_link(bar_model, at, G_MENU_LINK_SUBMENU);
        const bool this_one = under == G_MENU_MODEL(menu);
        if (under != nullptr)
            g_object_unref(under);
        if (this_one)
            return at;
    }
    return -1;
}

} // namespace

WindowHandle window_piece_of_a_menu(const std::string &heading, std::string &why)
{
    // A MENU WITH NO WORD ON THE BAR IS A MENU NOBODY CAN FIND, and GTK would
    // draw the item as a blank the width of its padding. Refused where it is
    // written, before the desk is opened.
    if (heading.empty()) {
        why = "a menu needs the word that goes on the bar, and was given \"\"";
        return nullptr;
    }
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(satellite_window::menu);
    made->text = heading;
    satellite_window *raw = made.get();
    on_the_desk([raw] {
        raw->widget = g_menu_new();
        raw->actions = g_simple_action_group_new();
        raw->action_prefix = "menu" + std::to_string(++menus_so_far);
    });
    return made;
}

bool window_item(satellite_window &which, const std::string &capsule, const std::string &label,
                 std::string &why)
{
    if (!a_menu_that_is_open(which, why))
        return false;
    if (label.empty()) {
        why = "an item needs the words a person picks it by, and was given \"\"";
        return false;
    }
    satellite_window *raw = &which;
    on_the_desk([raw, &capsule, &label] {
        // ONE ACTION AN ITEM, NAMED BY ITS PLACE. "item3" and not the label,
        // because a label is a person's own words and an action name may hold
        // only letters, digits and hyphens -- and two items may say the same
        // thing and still be two items.
        const std::string name = "item" + std::to_string(g_menu_model_get_n_items(G_MENU_MODEL(model_of(*raw))) + 1);
        GSimpleAction *action = g_simple_action_new(name.c_str(), nullptr);
        g_signal_connect_data(action, "activate", G_CALLBACK(an_item_was_picked),
                              new AnItem{raw, capsule}, forget_the_item, GConnectFlags(0));
        g_action_map_add_action(G_ACTION_MAP(actions_of(*raw)), G_ACTION(action));
        // THE MAP HOLDS IT NOW. Ours is spent.
        g_object_unref(action);
        const std::string detailed = raw->action_prefix + "." + name;
        g_menu_append(model_of(*raw), label.c_str(), detailed.c_str());
    });
    return true;
}

bool window_menu(satellite_window &which, const WindowHandle &menu, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window has a menu across its top";
        return false;
    }
    if (which.widget == nullptr || !which.on_the_screen) {
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
    // ALREADY ON A WINDOW. Putting one GMenu on two bars would draw it twice
    // and fire its actions from whichever window has the group; satellite says
    // it in a sentence first, as `.append` does for a piece already somewhere.
    if (menu->inside_of.lock() != nullptr) {
        why = "that menu is already across the top of a window";
        return false;
    }
    satellite_window *window = &which;
    satellite_window *raw = menu.get();
    on_the_desk([window, raw] {
        GtkWidget *bar = static_cast<GtkWidget *>(window->bar);
        if (bar == nullptr) {
            // THE BAR IS MADE ONCE, ON THE FIRST MENU, and it goes ABOVE the
            // fixed in the window's column -- which is why window_new() has
            // held a column since GTK-12. A window with no menu never pays
            // for one.
            GMenu *bar_model = g_menu_new();
            bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(bar_model));
            // THE BAR HOLDS THE MODEL NOW. Ours is spent.
            g_object_unref(bar_model);
            GtkWidget *column = gtk_widget_get_parent(static_cast<GtkWidget *>(window->inside));
            gtk_box_prepend(GTK_BOX(column), bar);
            window->bar = bar;
        }
        GMenuModel *bar_model = gtk_popover_menu_bar_get_menu_model(GTK_POPOVER_MENU_BAR(bar));
        g_menu_append_submenu(G_MENU(bar_model), raw->text.c_str(), G_MENU_MODEL(model_of(*raw)));
        // THE ACTIONS GO ON THE WINDOW, under this menu's own prefix, which is
        // how "menu3.item2" in the model finds the GSimpleAction to fire. A
        // GtkApplicationWindow would have done this under "win." and cost a
        // session bus; see the top of this file.
        gtk_widget_insert_action_group(static_cast<GtkWidget *>(window->widget),
                                       raw->action_prefix.c_str(), G_ACTION_GROUP(actions_of(*raw)));
    });
    // THE SAME BOOKKEEPING `.append` DOES, for the same reasons: the window
    // holds the menu so a teardown can let go of it, and the menu knows its
    // window so a picked item's capsule can be handed it.
    menu->on_the_screen = true;
    which.pieces.push_back(menu);
    menu->inside_of = which.weak_from_this();
    return true;
}

bool window_menu_heading(satellite_window &which, const std::string &heading, std::string &why)
{
    if (!a_menu_that_is_open(which, why))
        return false;
    if (heading.empty()) {
        why = "a menu needs the word that goes on the bar, and was given \"\"";
        return false;
    }
    const WindowHandle on = which.inside_of.lock();
    satellite_window *raw = &which;
    GtkWidget *bar = on == nullptr ? nullptr : static_cast<GtkWidget *>(on->bar);
    on_the_desk([raw, bar, &heading] {
        if (bar == nullptr)
            return;
        // TAKEN OFF AND PUT BACK AT THE SAME PLACE. A GMenu item's label is
        // not writable once appended; the bar tracks the model, so a remove
        // and an insert is one heading changing and not a menu flickering
        // to the end of the bar.
        GMenuModel *bar_model = gtk_popover_menu_bar_get_menu_model(GTK_POPOVER_MENU_BAR(bar));
        const int at = place_on_the_bar(bar_model, model_of(*raw));
        if (at < 0)
            return;
        g_menu_remove(G_MENU(bar_model), at);
        g_menu_insert_submenu(G_MENU(bar_model), at, heading.c_str(), G_MENU_MODEL(model_of(*raw)));
    });
    which.text = heading;
    return true;
}

} // namespace satellite004
