// satellite/satellite_variable_window/console_settings.cpp -- FILE > SETTINGS… in
// satl's own console, and the font size it keeps.
//
// (the author, 2026-09-22) "the user of satl needs to be able to set font size, we
// need a file > settings like right away, I want to make my font way smaller".
//
// THE SIZE IS A ROW OF ~/.satl/config.ini, `console.font_size = 9`, in points --
// the person's file (config/config_file.hpp), written by write_value, which
// rewrites that one row and leaves every other line where it was. Every console
// reads it when it is dressed (dress_the_terminal), so a new window, a program's
// console and the next start of satl all wear it. A row that is absent, or is not
// a whole number from 1 up, is 11, the size satl-term wore.
//
// THE WINDOW CHANGES THE FONT AS THE NUMBER CHANGES, so the person sees the size
// before they keep it -- and it is kept at once, because there is no Cancel: the
// number shown is the number in the file. What could not be written is said in
// the window itself, never in the terminal, which is a transcript of what a
// program did (console_menu.cpp's rule).
//
// MODAL TO ITS CONSOLE: one Settings window a console, and no second one opened
// behind it. It goes when its console goes.

#include "satellite_window.hpp"

#if SATELLITE_HAS_CONSOLE

#include "window_console.hpp"
#include "../config/config_file.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>

namespace satellite004 {
namespace {

constexpr const char *kFontRow = "console.font_size";
constexpr long long int kDefaultPoints = 11;
constexpr double kMostPointsOffered = 500;

// Where the Settings window says whether the size was kept.
struct SettingsWindow {
    satellite_window *console;
    GtkWidget *said;
};

void when_the_size_changes(GtkSpinButton *spin, gpointer user_data)
{
    SettingsWindow *settings = static_cast<SettingsWindow *>(user_data);
    const long long int points = gtk_spin_button_get_value_as_int(spin);
    if (settings->console->widget != nullptr && settings->console->terminal != nullptr)
        set_console_font_points(terminal_of(*settings->console), points);
    std::string why;
    const std::string kept =
        config_file::write_value(kFontRow, std::to_string(points), why) == success
            ? "Kept in " + config_file::path() + " -- every new window opens at this size."
            : "This window is at " + std::to_string(points) + ", but it could not be kept for the next: " + why;
    gtk_label_set_text(GTK_LABEL(settings->said), kept.c_str());
}

// THE SIZE THE TERMINAL IS WEARING NOW, in points -- which is the row's unless a
// size in pixels was put on it, and then the row's is the honest number to show.
long long int points_it_wears(VteTerminal *terminal)
{
    const PangoFontDescription *font = vte_terminal_get_font(terminal);
    if (font == nullptr || pango_font_description_get_size_is_absolute(font) ||
        pango_font_description_get_size(font) <= 0)
        return console_font_points();
    return (pango_font_description_get_size(font) + PANGO_SCALE / 2) / PANGO_SCALE;
}

} // namespace

long long int console_font_points()
{
    std::string text;
    if (!config_file::read_value(kFontRow, text))
        return kDefaultPoints;
    text = config_file::trimmed(text);
    char *end = nullptr;
    errno = 0;
    const long long int points = std::strtoll(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0' || points < 1)
        return kDefaultPoints;
    return points;
}

void set_console_font_points(VteTerminal *terminal, long long int points)
{
    // THE FAMILY LIST dress_the_terminal always used: IBM Plex Mono, which satl
    // carries (WIN-1), falling through to the system's monospace.
    const std::string wanted = "IBM Plex Mono,monospace " + std::to_string(points);
    PangoFontDescription *font = pango_font_description_from_string(wanted.c_str());
    vte_terminal_set_font(terminal, font);
    pango_font_description_free(font);
}

void open_the_console_settings(satellite_window &console)
{
    if (console.widget == nullptr || console.terminal == nullptr)
        return;
    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Settings");
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(static_cast<GtkWidget *>(console.widget)));
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    GtkWidget *column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(column, 18);
    gtk_widget_set_margin_bottom(column, 18);
    gtk_widget_set_margin_start(column, 18);
    gtk_widget_set_margin_end(column, 18);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *name = gtk_label_new("Font size");
    gtk_widget_set_hexpand(name, TRUE);
    gtk_label_set_xalign(GTK_LABEL(name), 0);
    const long long int now = points_it_wears(terminal_of(console));
    GtkWidget *size = gtk_spin_button_new_with_range(1, now > kMostPointsOffered ? now : kMostPointsOffered, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(size), static_cast<double>(now));
    gtk_box_append(GTK_BOX(row), name);
    gtk_box_append(GTK_BOX(row), size);

    GtkWidget *said = gtk_label_new("Changes as you set it, and is kept for every new window.");
    gtk_label_set_wrap(GTK_LABEL(said), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(said), 40);
    gtk_label_set_xalign(GTK_LABEL(said), 0);

    GtkWidget *close = gtk_button_new_with_label("Close");
    gtk_widget_set_halign(close, GTK_ALIGN_END);
    g_signal_connect_swapped(close, "clicked", G_CALLBACK(gtk_window_destroy), window);

    gtk_box_append(GTK_BOX(column), row);
    gtk_box_append(GTK_BOX(column), said);
    gtk_box_append(GTK_BOX(column), close);
    gtk_window_set_child(GTK_WINDOW(window), column);

    // FREED WITH THE WINDOW, which is the last thing that can call back into it.
    SettingsWindow *settings = new SettingsWindow{&console, said};
    g_object_set_data_full(G_OBJECT(window), "satl-settings", settings,
                           [](gpointer data) { delete static_cast<SettingsWindow *>(data); });
    g_signal_connect(size, "value-changed", G_CALLBACK(when_the_size_changes), settings);
    gtk_window_present(GTK_WINDOW(window));
}

} // namespace satellite004

#endif
