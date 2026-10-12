// satellite/satellite_variable_window/window_widgets.cpp -- THE AUTHOR'S WIDGETS (2026-10-09):
// `my_window.widgets.displays.file("file_display")` and `.input("text_input")`, each reached after
// by its name -- `my_window.file_display`, `my_window.file_display.text_input` -- and a file
// display's `.directory`.
//
// HIS WORDS, the day it was asked for: "the next thing I need in the source editor is a gtk file
// display ... and I need to put another gtk widget on top of the file display -- a text input where
// you can type in a directory and it changes the file display to that directory, and this has to be
// programmed in satellite"; "windows have .widgets and .widget objects have .widgets, so that is how
// I want the syntax to look". And to the questions put to him: a widget's own widgets sit at the top
// of it, top to bottom; `.directory("/home/you/")` changes the directory shown and starts in the
// home folder, the input starting with that path; a directory that is not there leaves the display
// where it was and the program running.
//
// A WIDGET IS A PIECE (satellite_window.hpp), held in its owner's `pieces` as `.append` holds one,
// so a window that closes lets go of it as of every piece; and in its owner's `widgets` too, under
// its name, which is what `my_window.a_name` reads. The NAMES are the interpreter's alone -- the desk
// never sees one -- and every GTK call is on the desk, inside on_the_desk().
//
// THE FILE DISPLAY IS A COLUMN: across its top the widgets made in it, and under them its list -- a
// GtkListView over the directory GIO lists, folders first and then by name, and hidden files left
// out, as a file chooser leaves them out. THE PROGRAM DECIDES WHAT HAPPENS: the list shows, and the
// program says which directory -- "this has to be programmed in satellite".
//
// SECTIONS (the author, 2026-10-10): `main_window.section("section_name", 100%, 50%)`, and then
// `main_window.section_name.widgets.displays.file("my_file_display", 100%, 100%)` "takes up 100% width
// and 100% height of half of the window, and we can accept plain numbers as pixels, and percentages
// get converted into pixels" -- "to program GUIs in satellite you must learn the section code
// scheme". A SECTION IS AN AREA: what is made in it is laid out LEFT TO RIGHT, a row at a time, each
// as wide and as tall as it was told -- a share of the area or pixels -- and the next row starts
// under the tallest of the row before. Two `50%, 100%` sit side by side; two `100%, 50%` are the top
// and bottom halves. A share is worked out each time GTK lays the area out, so 50% stays half of the
// window however it is resized. A window's sections, and the widgets made in it with a size, go in
// an area of its own under its widgets box. A section holds sections. (My reading of the three
// questions put to him that day -- left to right and wrapping, a share kept on resize, sections in
// sections -- written down so a different answer is a change in one place: an_area_is_laid_out.)
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace satellite004 {
namespace {

// THE HOME FOLDER, where a file display starts (his "yes", 2026-10-09), with the / a person types
// after a directory -- his own spelling was "/home/" + arguments.username + "/".
std::string the_home_folder()
{
    const char *home = std::getenv("HOME");
    std::string folder = home != nullptr && home[0] != '\0' ? home : "/";
    if (folder.back() != '/')
        folder += '/';
    return folder;
}

// ONE ROW OF THE LIST: a label, its name, a folder's ending in / as it is typed. ON THE DESK.
void a_row_is_made(GtkSignalListItemFactory *, GObject *object, gpointer)
{
    GtkWidget *label = gtk_label_new(nullptr);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_list_item_set_child(GTK_LIST_ITEM(object), label);
}

void a_row_is_shown(GtkSignalListItemFactory *, GObject *object, gpointer)
{
    GtkListItem *item = GTK_LIST_ITEM(object);
    GFileInfo *info = G_FILE_INFO(gtk_list_item_get_item(item));
    const char *name = g_file_info_get_display_name(info);
    std::string shown = name != nullptr ? name : "";
    if (g_file_info_get_file_type(info) == G_FILE_TYPE_DIRECTORY)
        shown += '/';
    gtk_label_set_text(GTK_LABEL(gtk_list_item_get_child(item)), shown.c_str());
}

// FOLDERS FIRST, THEN BY NAME, capitals or not -- the order a person reads a directory in.
int folders_first(gconstpointer a, gconstpointer b, gpointer)
{
    GFileInfo *one = G_FILE_INFO(const_cast<gpointer>(a));
    GFileInfo *two = G_FILE_INFO(const_cast<gpointer>(b));
    const bool one_a_folder = g_file_info_get_file_type(one) == G_FILE_TYPE_DIRECTORY;
    const bool two_a_folder = g_file_info_get_file_type(two) == G_FILE_TYPE_DIRECTORY;
    if (one_a_folder != two_a_folder)
        return one_a_folder ? GTK_ORDERING_SMALLER : GTK_ORDERING_LARGER;
    const char *one_name = g_file_info_get_display_name(one);
    const char *two_name = g_file_info_get_display_name(two);
    gchar *first = g_utf8_casefold(one_name != nullptr ? one_name : "", -1);
    gchar *second = g_utf8_casefold(two_name != nullptr ? two_name : "", -1);
    const int order = g_utf8_collate(first, second);
    g_free(first);
    g_free(second);
    return order < 0 ? GTK_ORDERING_SMALLER : order > 0 ? GTK_ORDERING_LARGER : GTK_ORDERING_EQUAL;
}

gboolean not_hidden(gpointer item, gpointer)
{
    return !g_file_info_get_is_hidden(G_FILE_INFO(item));
}

// A FILE DISPLAY, ON THE DESK: the column, its top for the widgets made in it, and the list of
// `folder`. `raw->directory_list` keeps a reference of its own, let go of with the piece
// (window_desk.cpp), so `.directory` never reaches a list GTK has already freed.
GtkWidget *a_file_display_of(satellite_window &raw, const std::string &folder)
{
    GFile *where = g_file_new_for_path(folder.c_str());
    GtkDirectoryList *listed =
        gtk_directory_list_new("standard::display-name,standard::type,standard::is-hidden", where);
    g_object_unref(where);
    raw.directory_list = g_object_ref(listed);
    GtkFilterListModel *shown = gtk_filter_list_model_new(G_LIST_MODEL(listed),
                                                          GTK_FILTER(gtk_custom_filter_new(not_hidden, nullptr, nullptr)));
    GtkSortListModel *sorted = gtk_sort_list_model_new(G_LIST_MODEL(shown),
                                                       GTK_SORTER(gtk_custom_sorter_new(folders_first, nullptr, nullptr)));
    GtkListItemFactory *rows = gtk_signal_list_item_factory_new();
    g_signal_connect(rows, "setup", G_CALLBACK(a_row_is_made), nullptr);
    g_signal_connect(rows, "bind", G_CALLBACK(a_row_is_shown), nullptr);
    GtkWidget *list = gtk_list_view_new(GTK_SELECTION_MODEL(gtk_no_selection_new(G_LIST_MODEL(sorted))), rows);

    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), list);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroller, TRUE);

    GtkWidget *top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    raw.widgets_box = top;
    GtkWidget *column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(column), top);
    gtk_box_append(GTK_BOX(column), scroller);
    gtk_widget_set_hexpand(column, TRUE);
    gtk_widget_set_vexpand(column, TRUE);
    return column;
}

// ---------------------------------------------------------------------------
// AN AREA -- a section, or a window's sections area -- AND HOW IT LAYS OUT WHAT IS IN IT.
// ---------------------------------------------------------------------------

// WHAT THE DESK LAYS A WIDGET OUT BY: a copy of the size it was told, kept on its GtkWidget and let
// go of with it, so the layout -- on the desk, at every allocation -- never reaches into the
// interpreter's handle. A widget with none in a section is all of it: 100% by 100%.
constexpr const char *kSizeKey = "satellite-size";

void forget_a_size(gpointer size)
{
    delete static_cast<ASize *>(size);
}

// ONE EXTENT IN PIXELS, OF `whole`: a share of it ROUNDED DOWN -- so two 50%s of 801 pixels are 400
// each and fit side by side, where rounding up would have wrapped the second -- or the pixels given.
int pixels_of(const AnExtent &extent, int whole)
{
    const long long int pixels = extent.share ? static_cast<long long int>(whole) * extent.amount / 1000000 : extent.amount;
    return static_cast<int>(std::clamp<long long int>(pixels, 0, 32767));
}

// AN AREA MAY BE AS SMALL AS NOTHING AND ASKS FOR NOTHING: its size is its holder's to give, and what
// is in it is cut off at its edge (GTK_OVERFLOW_HIDDEN) rather than pushing the window wider.
void an_area_is_measured(GtkWidget *, GtkOrientation, int, int *minimum, int *natural, int *, int *)
{
    *minimum = 0;
    *natural = 0;
}

// LEFT TO RIGHT, A ROW AT A TIME. Each piece is as wide and as tall as it was told, and never less
// than the least GTK can draw it in -- measured first, as GTK asks of every allocation -- and one that
// would pass the right edge starts the next row, under the tallest of this one. One wider than the
// whole area has a row to itself, and is cut off at the edge.
void an_area_is_laid_out(GtkWidget *area, int width, int height, int)
{
    static const ASize all;
    int across = 0, down = 0, row_tall = 0;
    for (GtkWidget *child = gtk_widget_get_first_child(area); child != nullptr; child = gtk_widget_get_next_sibling(child)) {
        if (!gtk_widget_should_layout(child))
            continue;
        const ASize *size = static_cast<const ASize *>(g_object_get_data(G_OBJECT(child), kSizeKey));
        if (size == nullptr)
            size = &all;
        int wide = pixels_of(size->wide, width);
        int tall = pixels_of(size->tall, height);
        int least = 0;
        gtk_widget_measure(child, GTK_ORIENTATION_HORIZONTAL, -1, &least, nullptr, nullptr, nullptr);
        wide = std::max(wide, least);
        gtk_widget_measure(child, GTK_ORIENTATION_VERTICAL, wide, &least, nullptr, nullptr, nullptr);
        tall = std::max(tall, least);
        if (across > 0 && across + wide > width) {
            across = 0;
            down += row_tall;
            row_tall = 0;
        }
        const GtkAllocation place = {across, down, wide, tall};
        gtk_widget_size_allocate(child, &place, -1);
        across += wide;
        row_tall = std::max(row_tall, tall);
    }
}

// THE AREA ITSELF: a widget that holds what is put in it, lays it out as above, and lets go of it
// when it goes -- console_shadow.cpp's ground is the same kind of thing.
G_DECLARE_FINAL_TYPE(SatlArea, satl_area, SATL, AREA, GtkWidget)

struct _SatlArea {
    GtkWidget parent_instance;
};

G_DEFINE_FINAL_TYPE(SatlArea, satl_area, GTK_TYPE_WIDGET)

void the_area_goes(GObject *object)
{
    while (GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(object)))
        gtk_widget_unparent(child);
    G_OBJECT_CLASS(satl_area_parent_class)->dispose(object);
}

void satl_area_class_init(SatlAreaClass *kind)
{
    G_OBJECT_CLASS(kind)->dispose = the_area_goes;
    gtk_widget_class_set_css_name(GTK_WIDGET_CLASS(kind), "section");
}

void satl_area_init(SatlArea *area)
{
    gtk_widget_set_layout_manager(GTK_WIDGET(area), gtk_custom_layout_new(nullptr, an_area_is_measured,
                                                                          an_area_is_laid_out));
    gtk_widget_set_overflow(GTK_WIDGET(area), GTK_OVERFLOW_HIDDEN);
}

GtkWidget *an_area()
{
    return GTK_WIDGET(g_object_new(satl_area_get_type(), nullptr));
}

// THE FRAME AFTER A SECTION, OR A WIDGET WITH A SIZE, WAS MADE (the review, 2026-10-10). A tick runs at
// the START of a frame, and GTK lays out and paints in the same pass (gdkframeclock.c runs the phases
// in one go), so a `.width` the interpreter sends to the desk once this has run is answered after the
// layout. Once: it is the one frame that was waited for.
gboolean it_was_laid_out(GtkWidget *, GdkFrameClock *, gpointer user_data)
{
    the_desk_saw_it_laid_out(static_cast<satellite_window *>(user_data));
    return G_SOURCE_REMOVE;
}

// WHERE A WINDOW'S SECTIONS GO, made the first time one is -- and the widgets made in the window
// with a size: an area that is the REST OF THE WINDOW, under its menu bar and under the widgets made
// across its top, which keep only the height they need. The fixed area `.append` places pieces in
// is HIDDEN: a window laid out in sections appends nothing (window_lays_out_in_sections), and an
// empty fixed area kept a strip of its own under the sections, so that 100% of the window was 46
// pixels short of it (the review, 2026-10-10). ON THE DESK.
GtkWidget *a_windows_sections_area(satellite_window &window)
{
    if (window.sections_area != nullptr)
        return static_cast<GtkWidget *>(window.sections_area);
    GtkWidget *area = an_area();
    gtk_widget_set_hexpand(area, TRUE);
    gtk_widget_set_vexpand(area, TRUE);
    GtkWidget *column = gtk_window_get_child(GTK_WINDOW(static_cast<GtkWidget *>(window.widget)));
    GtkWidget *fixed_area = static_cast<GtkWidget *>(window.scroller);
    GtkWidget *before = fixed_area != nullptr ? gtk_widget_get_prev_sibling(fixed_area) : nullptr;
    gtk_box_insert_child_after(GTK_BOX(column), area, before);
    if (fixed_area != nullptr) {
        gtk_widget_set_vexpand(fixed_area, FALSE);
        gtk_widget_set_visible(fixed_area, FALSE);
    }
    if (window.widgets_box != nullptr)
        gtk_widget_set_vexpand(static_cast<GtkWidget *>(window.widgets_box), FALSE);
    window.sections_area = area;
    return area;
}

// WHERE A WINDOW'S WIDGETS GO, made the first time one is: a column across the top of the window,
// above the fixed area `.append` places pieces in -- and that area stops taking the height, so the
// widgets have the window. A window holding no widgets is exactly as it was. ON THE DESK.
//
// ALWAYS ABOVE THE SECTIONS AREA, whichever was made first, and only as tall as what is in it once
// there is one -- the sections have the rest (the review, 2026-10-10: made after a section, the box
// went under it, and the two split the window's height). The fixed area is hidden while nothing has
// been appended (`nothing_appended`, asked on the interpreter's thread), and `.append` shows it.
GtkWidget *a_windows_widgets_box(satellite_window &window, bool nothing_appended)
{
    if (window.widgets_box != nullptr)
        return static_cast<GtkWidget *>(window.widgets_box);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_vexpand(box, window.sections_area == nullptr);
    GtkWidget *column = gtk_window_get_child(GTK_WINDOW(static_cast<GtkWidget *>(window.widget)));
    GtkWidget *fixed_area = static_cast<GtkWidget *>(window.scroller);
    GtkWidget *under = window.sections_area != nullptr ? static_cast<GtkWidget *>(window.sections_area) : fixed_area;
    GtkWidget *before = under != nullptr ? gtk_widget_get_prev_sibling(under) : nullptr;
    gtk_box_insert_child_after(GTK_BOX(column), box, before);
    if (fixed_area != nullptr) {
        gtk_widget_set_vexpand(fixed_area, FALSE);
        if (nothing_appended)
            gtk_widget_set_visible(fixed_area, FALSE);
    }
    window.widgets_box = box;
    return box;
}

} // namespace

WindowHandle window_widget_made(satellite_window &owner, satellite_window::Piece kind, const std::string &name,
                                const ASize *size, std::string &why)
{
    // A WINDOW, A SECTION AND A WIDGET THAT HOLDS WIDGETS -- the file display -- make them; nothing else
    // has room. A CONSOLE IS A WINDOW (is_a_window) whose terminal fills it, so it is told that by name.
    if (owner.piece == satellite_window::console) {
        why = "a console holds nothing but its terminal";
        return nullptr;
    }
    if (!owner.is_a_window() && owner.piece != satellite_window::file_display &&
        owner.piece != satellite_window::a_section) {
        why = std::string(owner.piece_name()) + " holds no widgets -- a window does, a section does, and a file "
                                                "display does: my_window.satellite.widgets.display.file(\"files\")";
        return nullptr;
    }
    // A FILE DISPLAY'S WIDGETS SIT ACROSS ITS TOP, one under another, as the author asked on 2026-10-09
    // -- so a section, or a size, is given where there is an area to lay it out in.
    if (owner.piece == satellite_window::file_display && kind == satellite_window::a_section) {
        why = "a file display holds widgets across its top, and no sections -- make the section in the window "
              "or in a section: my_window.satellite.section(\"" + name + "\", 100%, 50%)";
        return nullptr;
    }
    if (owner.piece == satellite_window::file_display && size != nullptr) {
        why = "a widget made in a file display sits across its top and takes no size -- give the size to a "
              "section, and make the widget in it: my_window.satellite.section(\"top\", 100%, 10%)";
        return nullptr;
    }
    // ONE OR THE OTHER, FOR NOW (satellite_window.hpp): a window that places pieces with `.append` gets
    // no sections, and window_append refuses the other way round.
    const bool in_an_area = owner.piece == satellite_window::a_section || size != nullptr;
    if (owner.is_a_window() && in_an_area && window_holds_appended_pieces(owner)) {
        why = std::string(owner.piece_name()) + " places pieces with .satellite.append, by its own corner, and "
              "sections lay a window out left to right -- a window does one or the other, for now: make the " +
              (kind == satellite_window::a_section ? std::string("section") : std::string("widget with a size")) +
              " in a window with nothing appended";
        return nullptr;
    }
    if (owner.widget == nullptr || !owner.on_the_screen) {
        why = "it is closed";
        return nullptr;
    }
    if (!a_widget_name_fits(name, why))
        return nullptr;
    // ONE NAME, ONE WIDGET, while it is open. A widget that closed with its window is made again under
    // the same name when the window is opened again, so its old entry gives way.
    std::size_t again = owner.widgets.size();
    for (std::size_t at = 0; at < owner.widgets.size(); ++at) {
        if (owner.widgets[at].first != name)
            continue;
        if (owner.widgets[at].second->widget != nullptr) {
            why = "a widget named " + name + " is already in it";
            return nullptr;
        }
        again = at;
    }

    WindowHandle made = std::make_shared<satellite_window>(kind);
    satellite_window *raw = made.get();
    satellite_window *holder = &owner;
    // WHAT IT STARTS WITH: a file display, the home folder; a text input made in a file display, that
    // display's directory -- "the input starts out showing that path".
    const std::string folder = the_home_folder();
    const std::string text = owner.piece == satellite_window::file_display ? owner.directory : std::string();
    // IN AN AREA, LEFT TO RIGHT: everything made in a section, and a section or a widget with a size
    // made in a window. A widget with no size in a window goes across its top, as on 2026-10-09.
    //
    // THE WINDOW ITSELF, for its next frame: an area is laid out in it, and `.width` waits for that.
    const WindowHandle frame = the_window_holding(owner);
    satellite_window *window = frame.get();
    const bool nothing_appended = !window_holds_appended_pieces(owner);
    on_the_desk([raw, holder, kind, size, in_an_area, window, nothing_appended, &folder, &text] {
        // CLOSED SINCE THE INTERPRETER LOOKED (the review, 2026-10-10): a person's click on the close
        // button is the desk's own event and can land between the question above and this. Then nothing
        // is made and GTK is not touched -- its window, scroller and areas are freed -- and the line is
        // told the window is closed.
        if (holder->widget == nullptr || window == nullptr || window->widget == nullptr)
            return;
        GtkWidget *widget = kind == satellite_window::file_display ? a_file_display_of(*raw, folder)
                            : kind == satellite_window::a_section  ? an_area()
                                                                   : gtk_entry_new();
        if (kind == satellite_window::text_box)
            gtk_editable_set_text(GTK_EDITABLE(widget), text.c_str());
        raw->widget = widget;
        if (in_an_area) {
            GtkWidget *area = holder->piece == satellite_window::a_section ? static_cast<GtkWidget *>(holder->widget)
                                                                         : a_windows_sections_area(*holder);
            if (size != nullptr)
                g_object_set_data_full(G_OBJECT(widget), kSizeKey, new ASize(*size), forget_a_size);
            gtk_widget_set_parent(widget, area);
            if (the_desk_lays_it_out_again(window))
                gtk_widget_add_tick_callback(static_cast<GtkWidget *>(window->widget), it_was_laid_out, window, nullptr);
            return;
        }
        GtkWidget *box = holder->is_a_window() ? a_windows_widgets_box(*holder, nothing_appended)
                                                : static_cast<GtkWidget *>(holder->widgets_box);
        gtk_box_append(GTK_BOX(box), widget);
    });
    if (raw->widget == nullptr) {
        why = "it is closed";
        return nullptr;
    }
    if (size != nullptr) {
        made->size = *size;
        made->sized = true;
    }
    if (kind == satellite_window::file_display)
        made->directory = folder;
    else
        made->text = text;
    made->on_the_screen = true;
    made->inside_of = owner.weak_from_this();
    owner.pieces.push_back(made);
    if (again < owner.widgets.size())
        owner.widgets[again].second = made;
    else
        owner.widgets.emplace_back(name, made);
    return made;
}

bool window_lays_out_in_sections(const satellite_window &window)
{
    for (const auto &[name, widget] : window.widgets)
        if (widget != nullptr && widget->widget != nullptr && (widget->piece == satellite_window::a_section || widget->sized))
            return true;
    return false;
}

bool window_holds_appended_pieces(const satellite_window &window)
{
    // EVERY WIDGET IS ONE OF ITS PIECES TOO (window_widget_made), so a piece more than the widgets is one
    // `.append` put there -- counted, not searched for, because a window making a thousand sections asks
    // this a thousand times. A menu is not appended (it goes across the top), and what closed is gone.
    std::size_t pieces = 0, widgets = 0;
    for (const WindowHandle &piece : window.pieces)
        if (piece != nullptr && piece->widget != nullptr && piece->is_drawn())
            ++pieces;
    for (const auto &[name, widget] : window.widgets)
        if (widget != nullptr && widget->widget != nullptr)
            ++widgets;
    return pieces > widgets;
}

WindowHandle window_widget_named(const satellite_window &owner, const std::string &name, std::string &why)
{
    for (const auto &[named, widget] : owner.widgets)
        if (named == name)
            return widget;
    why = std::string(owner.piece_name()) + " has no widget named " + name + " -- " +
          (owner.is_a_window() || owner.piece == satellite_window::a_section
               ? ".satellite.widgets.display.file(\"" + name + "\") or .satellite.section(\"" + name +
                     "\", 100%, 50%) makes one"
           : owner.piece == satellite_window::file_display
               ? ".satellite.widgets.display.file(\"" + name + "\") makes one"
               : "a window's widgets are made with my_window.satellite.widgets.display.file(\"" + name + "\")");
    return nullptr;
}

bool window_set_directory(satellite_window &which, const std::string &path, bool &went, std::string &why)
{
    if (which.piece != satellite_window::file_display) {
        why = std::string(which.piece_name()) + " shows no directory -- a file display does: "
                                                "my_window.satellite.widgets.display.file(\"files\")";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    // NOT A DIRECTORY IS NOT A REFUSAL (his "yes", 2026-10-09): a person typing a path passes through
    // every half-typed name on the way to the one they mean, and the display waits for it.
    std::error_code trouble;
    if (path.empty() || !std::filesystem::is_directory(path, trouble)) {
        went = false;
        return true;
    }
    satellite_window *raw = &which;
    on_the_desk([raw, &path] {
        if (raw->directory_list == nullptr)
            return;
        GFile *where = g_file_new_for_path(path.c_str());
        gtk_directory_list_set_file(GTK_DIRECTORY_LIST(raw->directory_list), where);
        g_object_unref(where);
    });
    which.directory = path;
    went = true;
    return true;
}

} // namespace satellite004
