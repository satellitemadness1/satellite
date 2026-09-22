// satellite/satellite_variable_window/window_menu.cpp -- A MENU ACROSS THE TOP
// OF A WINDOW, the items on it, the line between groups of them, and a menu
// inside a menu. GTK_AND_NO_DEPENDENCIES.md GTK-12.
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
// A MENU'S MODEL HOLDS SECTIONS AND NOTHING ELSE (the separator, 2026-09-21).
// One section from the day the menu is made, one more for every `.separator()`,
// and the items go into the last one. GTK draws the line between one section
// and the next, which is the only way a GMenu has of drawing a line at all --
// there is no separator item. A menu that never says `.separator()` has one
// section and looks exactly as it did before sections existed.
//
// A MENU INSIDE A MENU IS `.menu` ON A MENU. The same method that puts a menu
// across a window's top puts one under another, as an item with an arrow; its
// heading is the word on that item, which `satellite.window.menu("Recent")`
// already carries -- the piece carries its own name, which is what GTK-16 says
// of a tab and what the recommendation said of a submenu. Its actions go on the
// window with its parent's: at once if the parent is on one, and when the
// parent gets there if not.
//
// satl-term/menu.cpp IS THE SAME GMenu AND THE SAME ACTIONS, ported from 003,
// and was read before this was written. What differs is exactly the two things
// above: no GtkApplication, and the model belongs to a satellite piece.
//
// SPLIT ON 2026-09-22, at 384 lines against the author's "try to build for 300
// lines", at the seam between a menu's MODEL and the BAR that draws it: this
// file is the model -- made, its items, its sections, its heading -- and
// window_menu_bar.cpp is `.menu`, where a menu meets a window or another menu.
// window_menu.hpp holds the three casts both files make.
//
// EVERY GTK AND GIO CALL HAPPENS INSIDE on_the_desk(), like its neighbours.
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "window_menu.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

#include <string>

namespace satellite004 {
namespace {

// A PREFIX NO OTHER MENU HAS. ON THE DESK'S THREAD ONLY, which is what makes a
// plain counter right -- window_look.cpp's style classes are handed out the
// same way and for the same reason.
unsigned long long int menus_so_far = 0;

// AND A NAME NO OTHER ITEM HAS, across every menu. It used to be the count of
// items on the menu's model plus one, and since the model holds SECTIONS that
// count is the number of separators -- two items either side of one would have
// shared a name and the second would have replaced the first's action. An
// action's name need only be unique in its own group; a counter that is unique
// everywhere is that, with nothing left to think about.
unsigned long long int items_so_far = 0;

// WHAT AN ACTION KNOWS: which menu it is on, and which capsule it runs. Owned by
// the action's closure and freed with it, so an item can never outlive the
// action GTK would fire it from.
//
// THE MENU IS A RAW POINTER AND IT IS ALWAYS VALID WHERE IT IS USED. An action
// fires only through the bar, the bar lives only while its window does, and
// the window holds every menu on it in `pieces` -- and every menu holds the
// menus inside it the same way -- which is the argument that makes a button's
// `clicked` handler safe (satellite_window.hpp, ENABLE_SHARED_FROM_THIS).
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

// WHERE A MENU SITS IN A MODEL, or -1 when it is not there. ON THE DESK. The
// bar's model and a section both link each heading to the menu's own GMenu,
// and a link comes back with a reference on it that must be given back.
int place_in(GMenuModel *model, GMenu *menu)
{
    const gint many = g_menu_model_get_n_items(model);
    for (gint at = 0; at < many; ++at) {
        GMenuModel *under = g_menu_model_get_item_link(model, at, G_MENU_LINK_SUBMENU);
        const bool this_one = under == G_MENU_MODEL(menu);
        if (under != nullptr)
            g_object_unref(under);
        if (this_one)
            return at;
    }
    return -1;
}

// A NEW SECTION ON THE END OF A MENU'S MODEL, which is where its items go from
// now on. ON THE DESK.
void open_a_section(satellite_window &menu)
{
    GMenu *section = g_menu_new();
    g_menu_append_section(model_of(menu), nullptr, G_MENU_MODEL(section));
    // THE MODEL HOLDS IT NOW. Ours is spent, and the raw pointer stays good for
    // exactly as long as the model does -- which is as long as `widget` is.
    g_object_unref(section);
    menu.section = section;
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
        open_a_section(*raw);
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
        // ONE ACTION AN ITEM, NAMED BY ITS NUMBER. "item3" and not the label,
        // because a label is a person's own words and an action name may hold
        // only letters, digits and hyphens -- and two items may say the same
        // thing and still be two items.
        const std::string name = "item" + std::to_string(++items_so_far);
        GSimpleAction *action = g_simple_action_new(name.c_str(), nullptr);
        g_signal_connect_data(action, "activate", G_CALLBACK(an_item_was_picked),
                              new AnItem{raw, capsule}, forget_the_item, GConnectFlags(0));
        g_action_map_add_action(G_ACTION_MAP(actions_of(*raw)), G_ACTION(action));
        // THE MAP HOLDS IT NOW. Ours is spent.
        g_object_unref(action);
        const std::string detailed = raw->action_prefix + "." + name;
        g_menu_append(section_of(*raw), label.c_str(), detailed.c_str());
    });
    return true;
}

bool window_separator(satellite_window &which, std::string &why)
{
    if (!a_menu_that_is_open(which, why))
        return false;
    satellite_window *raw = &which;
    bool nothing_above = false;
    on_the_desk([raw, &nothing_above] {
        // A SECTION WITH NOTHING IN IT DRAWS NO LINE AT ALL, so a separator
        // written first, or two in a row, would be a line a person asked for
        // and cannot see. Refused, and the sentence says where a line goes.
        nothing_above = g_menu_model_get_n_items(G_MENU_MODEL(section_of(*raw))) == 0;
        if (!nothing_above)
            open_a_section(*raw);
    });
    if (nothing_above) {
        why = "a separator goes under the items above it, and there is nothing above it yet";
        return false;
    }
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
    // WHERE IT IS: on a window's bar, inside another menu, or nowhere yet -- in
    // which case the handle simply remembers, and the word is read when it goes
    // somewhere.
    const WindowHandle on = which.inside_of.lock();
    satellite_window *raw = &which;
    GtkWidget *bar = on != nullptr && on->piece == satellite_window::window
                         ? static_cast<GtkWidget *>(on->bar) : nullptr;
    satellite_window *parent = on != nullptr && on->piece == satellite_window::menu ? on.get() : nullptr;
    on_the_desk([raw, bar, parent, &heading] {
        // TAKEN OFF AND PUT BACK AT THE SAME PLACE. A GMenu item's label is
        // not writable once appended; the bar tracks the model, so a remove
        // and an insert is one heading changing and not a menu flickering
        // to the end of the bar.
        if (bar != nullptr) {
            GMenuModel *bar_model = gtk_popover_menu_bar_get_menu_model(GTK_POPOVER_MENU_BAR(bar));
            const int at = place_in(bar_model, model_of(*raw));
            if (at < 0)
                return;
            g_menu_remove(G_MENU(bar_model), at);
            g_menu_insert_submenu(G_MENU(bar_model), at, heading.c_str(), G_MENU_MODEL(model_of(*raw)));
            return;
        }
        if (parent == nullptr)
            return;
        // INSIDE A MENU it sits in one of the parent's sections: the same
        // remove and insert, in whichever section has it.
        GMenuModel *sections = G_MENU_MODEL(model_of(*parent));
        const gint many = g_menu_model_get_n_items(sections);
        for (gint s = 0; s < many; ++s) {
            GMenuModel *section = g_menu_model_get_item_link(sections, s, G_MENU_LINK_SECTION);
            if (section == nullptr)
                continue;
            const int at = place_in(section, model_of(*raw));
            if (at >= 0) {
                g_menu_remove(G_MENU(section), at);
                g_menu_insert_submenu(G_MENU(section), at, heading.c_str(), G_MENU_MODEL(model_of(*raw)));
            }
            g_object_unref(section);
            if (at >= 0)
                return;
        }
    });
    which.text = heading;
    return true;
}

} // namespace satellite004
