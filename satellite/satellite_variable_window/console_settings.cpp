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
//
// AND THE CONSOLE'S TWO COLOURS AND ITS SHADOW (the author, 2026-10-05): "we can
// have a window where you can pick the two colors to use, one for the background
// and one for the text, and an option to turn the drop shadow off ... this way we
// can set themes later". console.background, console.text and console.shadow, the
// rows console_shadow.cpp reads; each is put on this console the moment it is
// picked and kept at once, as the size is. The shadow's steps are worked out from
// the background, so a new background brings its own.

#include "satellite_window.hpp"

#if SATELLITE_HAS_CONSOLE

#include "console_shadow.hpp"
#include "window_console.hpp"
#include "../config/config_file.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace satellite004 {
namespace {

constexpr const char *kFontRow = "console.font_size";
constexpr long long int kDefaultPoints = 11;

// THE WEIGHT OF ITS TEXT (the author, 2026-10-05: "can you make the default text
// thicker? like, "500" whatever that is called?"): Pango's weight, 100 the thinnest
// to 900 the heaviest, 400 regular. 500 is Medium, a face satl carries
// (vendor/fonts/ibm-plex-mono, all seven of them), and the default on his word.
// A row that is not a number from 100 to 900 is 500, as a size that is not a
// number is 11.
constexpr const char *kWeightRow = "console.font_weight";
constexpr long long int kDefaultWeight = 500;
constexpr long long int kLightestWeight = 100;
constexpr long long int kHeaviestWeight = 900;
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

// A COLOUR AS config.ini KEEPS ONE: #RRGGBB.
std::string as_hex(const GdkRGBA &colour)
{
    const auto byte = [](float part) {
        const int value = static_cast<int>(part * 255.0f + 0.5f);
        return value < 0 ? 0 : value > 255 ? 255 : value;
    };
    char text[8];
    std::snprintf(text, sizeof text, "#%02X%02X%02X", byte(colour.red), byte(colour.green), byte(colour.blue));
    return text;
}

// KEPT, OR WHY NOT -- said in the window itself, never in the terminal (console_menu.cpp's rule).
void say_whether_it_was_kept(SettingsWindow *settings, const char *row, const std::string &value,
                             const std::string &what)
{
    std::string why;
    const std::string kept =
        config_file::write_value(row, value, why) == success
            ? "Kept in " + config_file::path() + " -- every new window opens with it."
            : "This window has " + what + ", but it could not be kept for the next: " + why;
    gtk_label_set_text(GTK_LABEL(settings->said), kept.c_str());
}

void when_a_colour_is_picked(GObject *button, GParamSpec *, gpointer user_data)
{
    SettingsWindow *settings = static_cast<SettingsWindow *>(user_data);
    const GdkRGBA *picked = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(button));
    const bool behind = g_object_get_data(button, "satl-behind") != nullptr;
    if (settings->console->widget != nullptr && settings->console->terminal != nullptr) {
        if (behind)
            vte_terminal_set_color_background(terminal_of(*settings->console), picked);
        else
            vte_terminal_set_color_foreground(terminal_of(*settings->console), picked);
    }
    say_whether_it_was_kept(settings, behind ? kConsoleBackgroundRow : kConsoleTextRow, as_hex(*picked),
                            behind ? "this background" : "this text colour");
}

void when_the_shadow_is_switched(GObject *button, GParamSpec *, gpointer user_data)
{
    SettingsWindow *settings = static_cast<SettingsWindow *>(user_data);
    const bool on = gtk_switch_get_active(GTK_SWITCH(button)) != FALSE;
    if (settings->console->widget != nullptr && settings->console->terminal != nullptr)
        the_ground_casts_shadows(static_cast<GtkWidget *>(settings->console->terminal), on);
    say_whether_it_was_kept(settings, kConsoleShadowRow, on ? "on" : "off", on ? "the shadow" : "no shadow");
}

// ONE ROW OF THE WINDOW: its name, and what sets it, at the right.
GtkWidget *a_row(const char *name, GtkWidget *control)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *label = gtk_label_new(name);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_widget_set_valign(control, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), control);
    return row;
}

// A BUTTON THAT SHOWS A COLOUR AND OPENS GTK'S COLOUR PICKER, titled `title`.
GtkWidget *a_colour_button(const char *title, const GdkRGBA &now)
{
    GtkColorDialog *dialog = gtk_color_dialog_new();
    gtk_color_dialog_set_title(dialog, title);
    gtk_color_dialog_set_with_alpha(dialog, FALSE);
    GtkWidget *button = gtk_color_dialog_button_new(dialog);   // the button keeps the dialog
    gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(button), &now);
    return button;
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

void when_the_weight_changes(GtkSpinButton *spin, gpointer user_data)
{
    SettingsWindow *settings = static_cast<SettingsWindow *>(user_data);
    const long long int weight = gtk_spin_button_get_value_as_int(spin);
    if (settings->console->widget != nullptr && settings->console->terminal != nullptr) {
        VteTerminal *terminal = terminal_of(*settings->console);
        set_console_font(terminal, points_it_wears(terminal), weight);
    }
    say_whether_it_was_kept(settings, kWeightRow, std::to_string(weight), "this weight");
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

long long int console_font_weight()
{
    std::string text;
    if (!config_file::read_value(kWeightRow, text))
        return kDefaultWeight;
    text = config_file::trimmed(text);
    char *end = nullptr;
    errno = 0;
    const long long int weight = std::strtoll(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0' || weight < kLightestWeight || weight > kHeaviestWeight)
        return kDefaultWeight;
    return weight;
}

void set_console_font(VteTerminal *terminal, long long int points, long long int weight)
{
    // THE FAMILY LIST dress_the_terminal always used: IBM Plex Mono, which satl
    // carries (WIN-1), falling through to the system's monospace. The weight is
    // set on the description, not spelled into it: "IBM Plex Mono Medium" would
    // be read as a family nobody has.
    const std::string wanted = "IBM Plex Mono,monospace " + std::to_string(points);
    PangoFontDescription *font = pango_font_description_from_string(wanted.c_str());
    pango_font_description_set_weight(font, static_cast<PangoWeight>(weight));
    vte_terminal_set_font(terminal, font);
    pango_font_description_free(font);
}

void set_console_font_points(VteTerminal *terminal, long long int points)
{
    set_console_font(terminal, points, console_font_weight());
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

    // THE WEIGHT, a hundred at a time: 100 to 900, as the row keeps it.
    GtkWidget *weight = gtk_spin_button_new_with_range(static_cast<double>(kLightestWeight),
                                                       static_cast<double>(kHeaviestWeight), 100);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(weight), static_cast<double>(console_font_weight()));

    // THE TWO COLOURS AND THE SHADOW, as config.ini has them -- what a new window opens with.
    GtkWidget *background =
        a_colour_button("Background", a_console_colour(kConsoleBackgroundRow, kAuthorsBackground));
    g_object_set_data(G_OBJECT(background), "satl-behind", GINT_TO_POINTER(1));
    GtkWidget *text = a_colour_button("Text", a_console_colour(kConsoleTextRow, kAuthorsText));
    GtkWidget *shadow = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(shadow), the_console_shadow_is_on());

    GtkWidget *said = gtk_label_new("Changes as you set it, and is kept for every new window.");
    gtk_label_set_wrap(GTK_LABEL(said), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(said), 40);
    gtk_label_set_xalign(GTK_LABEL(said), 0);

    GtkWidget *close = gtk_button_new_with_label("Close");
    gtk_widget_set_halign(close, GTK_ALIGN_END);
    g_signal_connect_swapped(close, "clicked", G_CALLBACK(gtk_window_destroy), window);

    gtk_box_append(GTK_BOX(column), row);
    gtk_box_append(GTK_BOX(column), a_row("Text weight", weight));
    gtk_box_append(GTK_BOX(column), a_row("Background", background));
    gtk_box_append(GTK_BOX(column), a_row("Text", text));
    gtk_box_append(GTK_BOX(column), a_row("Drop shadow", shadow));
    gtk_box_append(GTK_BOX(column), said);
    gtk_box_append(GTK_BOX(column), close);
    gtk_window_set_child(GTK_WINDOW(window), column);

    // FREED WITH THE WINDOW, which is the last thing that can call back into it.
    SettingsWindow *settings = new SettingsWindow{&console, said};
    g_object_set_data_full(G_OBJECT(window), "satl-settings", settings,
                           [](gpointer data) { delete static_cast<SettingsWindow *>(data); });
    g_signal_connect(size, "value-changed", G_CALLBACK(when_the_size_changes), settings);
    // CONNECTED AFTER THEIR VALUES ARE SET, so opening the window writes nothing.
    g_signal_connect(weight, "value-changed", G_CALLBACK(when_the_weight_changes), settings);
    g_signal_connect(background, "notify::rgba", G_CALLBACK(when_a_colour_is_picked), settings);
    g_signal_connect(text, "notify::rgba", G_CALLBACK(when_a_colour_is_picked), settings);
    g_signal_connect(shadow, "notify::active", G_CALLBACK(when_the_shadow_is_switched), settings);
    gtk_window_present(GTK_WINDOW(window));
}

} // namespace satellite004

#endif
