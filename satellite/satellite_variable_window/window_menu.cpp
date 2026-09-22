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

GMenu *model_of(const satellite_window &which) { return static_cast<GMenu *>(which.widget); }
GMenu *section_of(const satellite_window &which) { return static_cast<GMenu *>(which.section); }
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
