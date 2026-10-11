// satellite/satellite_variable_window/console_shadow.cpp -- THE SHADOW UNDER A
// CONSOLE'S TEXT, the ground it falls on, and the console's two colours
// (console_shadow.hpp).
//
// HIS WORDS, IN ORDER. 2026-10-04: "Would it be at all possible to give the text
// in the console a drop shadow? Just to see the drop shadow of the text, i'll pick
// the color... 757575". 2026-10-05: white text with "1 PIXEL LIGHT RED DROP SHADOW
// AND A 3 PIXEL FUZZY BLACK DROP SHADOW"; the light red "so weird"; a dark blue,
// #147ab5, that "still looks really off"; and then:
//
//   "can you take the color that is in between white and our light blue color
//   console background, take 1 color that is 25% of the light blue, and 75%
//   white, and use it as the 1 pixel drop shadow, then take 1 color that is 50% of
//   the light blue and 50% white and use it as the 2nd 1 pixel drop shadow, and do
//   that 2 more times, to have 4 colors of drop shadow into the light blue
//   background and then the text is still 000000! Then we can have a setting that
//   sets the background and the text color, then it calculates the drop shadow
//   colors using that 25%/75% then 50%/50% then 75%/25% then 100%/0% formula??"
//
// AND, ON SEEING THE STEPS FROM WHITE: "for some reason there's green in there...
// just change what you have that is green to the dark blue that we had and
// nothing else". SO THE SHADOW IS FOUR STEPS, A PIXEL APART, FROM HIS DARK BLUE,
// #147AB5, INTO THE BACKGROUND: one pixel right and down, 25% background and 75%
// dark blue; two, half and half; three, 75% background; four, the background
// itself -- his formula's last step, which cannot be seen against the background,
// drawn lowest. The steps are kept nowhere:
// they are worked out from the colour VTE paints the background in, at the moment
// the ground is drawn, so a background picked in File > Settings…, given by
// `.background(...)` or printed by satellite.terminal.background (OSC 11) brings
// its own steps.
//
// WHY THERE IS A GROUND. A shadow is drawn BEHIND what casts it, and VTE fills
// the whole terminal with its background before it draws a letter: a shadow
// behind an opaque field is never seen. So the terminal stops painting its
// background (vte_terminal_set_clear_background), and a ground under it paints it
// instead -- the colour VTE would have, when VTE would have
// (vte_terminal_get_color_background_for_draw, VTE's own words: "if you disable
// drawing the background ... and then need to draw the background yourself") --
// then the terminal on top, with the shadows pushed under it, which have only what
// VTE draws to fall from: the letters, the cursor and a selection. A CSS colour
// copied once stayed light blue through `.background` and OSC 11 (the harness,
// 2026-10-04); VTE redraws itself when its colour changes, and GTK redraws every
// widget above one that does, so the ground reads the new colour on that frame.
//
// A WIDGET OF ITS OWN, the one GObject type satl defines: a GtkBox paints its
// background from CSS, and CSS cannot ask VTE for anything. It clips to itself,
// or the shadows of the bottom row, the cursor or a selection fall onto the status
// bar and the menu bar (the fresh reader, 2026-10-04).
//
// AND VTE'S OWN CSS BACKGROUND GOES. VTE puts a stylesheet on every terminal (vte
// widget.cc) -- `background-color: @text_view_bg`, white, at
// GTK_STYLE_PROVIDER_PRIORITY_APPLICATION - 2 -- and GTK draws it under VTE's
// text, inside the shadows. Left there, the console came out white (the harness).
// One rule at APPLICATION, put on the display once, makes a grounded terminal's
// transparent.
//
// THE ROWS ARE THE PERSON'S (~/.satl/config.ini, read when a console is made, as
// console.font_size is, and written by File > Settings…): console.background, the
// light blue #90D5FF; console.text, #000000; console.shadow, on unless it says
// none, false or off, in any case. A colour is written as satl writes one anywhere
// -- #90D5FF, 90D5FF, or satl's own hex, x90D5FF -- and one GTK cannot read is the
// author's, as a font size that is not a number is 11 (console_settings.cpp).
// console.shadow_fuzzy is the fuzzy shadow of 2026-10-04 (the author's #757575,
// two pixels right and down, CSS's 2px of blur), under the steps, and only when
// the row names a colour: drawn beside the steps (the harness, 2026-10-05) its
// grey ran on past them, and the steps no longer faded "into the light blue
// background".
//
// COMPILED WHERE VTE WAS FOUND, as every console file's VTE half is
// (SATELLITE_HAS_CONSOLE, make_support/047-window.mk): a satl built with gtk4 and
// no VTE draws windows and refuses the console words, and has no terminal to put
// a ground under.

#if SATELLITE_HAS_CONSOLE

#include "console_shadow.hpp"

#include "../config/config_file.hpp"

#include <cctype>
#include <string>

namespace satellite004 {
namespace {

constexpr const char *kOnAGround = "satl-on-a-ground";

// THE FUZZY ONE: CSS's `drop-shadow(2px 2px 2px)` -- GSK's radius is 4 for CSS's
// 2, because GTK doubles a drop-shadow filter's blur (gtkcssshadowvalue.c).
constexpr const char *kFuzzyRow = "console.shadow_fuzzy";
constexpr const char *kAuthorsFuzzy = "#757575";
constexpr float kFuzzyFalls = 2;
constexpr float kFuzzyBlur = 4;

// THE STEPS: how many, and the colour they begin from -- the author's dark blue,
// #147AB5 (the author, 2026-10-05: "Let me decide the dark blue color").
constexpr int kSteps = 4;
constexpr GdkRGBA kDarkBlue = {0x14 / 255.0f, 0x7A / 255.0f, 0xB5 / 255.0f, 1.0f};

// THE ROW'S TEXT, folded to lower case, or "" when the row is absent.
std::string the_row_folded(const char *row, std::string &said)
{
    said.clear();
    if (!config_file::read_value(row, said))
        return std::string();
    said = config_file::trimmed(said);
    std::string folded = said;
    for (char &letter : folded)
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    return folded;
}

bool says_off(const std::string &folded)
{
    return folded == "none" || folded == "false" || folded == "off";
}

// A COLOUR AS satl WRITES ONE: #757575, 757575, x757575 or 0x757575 -- satl's own
// hex and C's were read as nothing (the fresh reader, 2026-10-04) -- or a name GTK
// knows. False when GTK cannot read it.
bool read_a_colour(const std::string &said, GdkRGBA &colour)
{
    std::string folded = said;
    for (char &letter : folded)
        letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    const std::string digits = folded.rfind("0x", 0) == 0 ? folded.substr(2)
                               : folded.rfind("x", 0) == 0 ? folded.substr(1)
                                                           : folded;
    const bool hex = !digits.empty() && digits.find_first_not_of("0123456789abcdef") == std::string::npos &&
                     (digits.size() == 3 || digits.size() == 4 || digits.size() == 6 || digits.size() == 8);
    return gdk_rgba_parse(&colour, hex ? ("#" + digits).c_str() : said.c_str()) != FALSE;
}

// `share` OF THE BACKGROUND AND THE REST DARK BLUE -- one of his steps.
GdkRGBA a_step(const GdkRGBA &background, float share)
{
    return GdkRGBA{kDarkBlue.red * (1 - share) + background.red * share,
                   kDarkBlue.green * (1 - share) + background.green * share,
                   kDarkBlue.blue * (1 - share) + background.blue * share, 1.0f};
}

// THE GROUND: one child, the terminal, laid over the whole of it.
G_DECLARE_FINAL_TYPE(SatlConsoleGround, satl_console_ground, SATL, CONSOLE_GROUND, GtkWidget)

struct _SatlConsoleGround {
    GtkWidget parent_instance;
    gboolean shadowed;          // console.shadow, or the switch in File > Settings…
    gboolean fuzzy;             // console.shadow_fuzzy is a colour
    GdkRGBA fuzzy_colour;
};

G_DEFINE_FINAL_TYPE(SatlConsoleGround, satl_console_ground, GTK_TYPE_WIDGET)

void the_ground_is_drawn(GtkWidget *widget, GtkSnapshot *snapshot)
{
    GtkWidget *terminal = gtk_widget_get_first_child(widget);
    if (terminal == nullptr)
        return;
    const SatlConsoleGround *ground = SATL_CONSOLE_GROUND(widget);
    GdkRGBA background;
    vte_terminal_get_color_background_for_draw(VTE_TERMINAL(terminal), &background);
    const graphene_rect_t whole = GRAPHENE_RECT_INIT(0, 0, static_cast<float>(gtk_widget_get_width(widget)),
                                                     static_cast<float>(gtk_widget_get_height(widget)));
    gtk_snapshot_append_color(snapshot, &background, &whole);
    if (!ground->shadowed) {
        gtk_widget_snapshot_child(widget, terminal, snapshot);
        return;
    }
    // IN THE ORDER GSK DRAWS THEM, the first lowest and the last nearest the letters
    // (gskshadownode.c): the fuzzy one, then the steps from the farthest in.
    GskShadow shadows[kSteps + 1];
    gsize count = 0;
    if (ground->fuzzy)
        shadows[count++] = GskShadow{ground->fuzzy_colour, kFuzzyFalls, kFuzzyFalls, kFuzzyBlur};
    for (int step = kSteps; step >= 1; --step) {
        const float falls = static_cast<float>(step);
        shadows[count++] = GskShadow{a_step(background, 0.25f * static_cast<float>(step)), falls, falls, 0};
    }
    gtk_snapshot_push_shadow(snapshot, shadows, count);
    gtk_widget_snapshot_child(widget, terminal, snapshot);
    gtk_snapshot_pop(snapshot);
}

void the_ground_goes(GObject *object)
{
    if (GtkWidget *terminal = gtk_widget_get_first_child(GTK_WIDGET(object)))
        gtk_widget_unparent(terminal);
    G_OBJECT_CLASS(satl_console_ground_parent_class)->dispose(object);
}

void satl_console_ground_class_init(SatlConsoleGroundClass *kind)
{
    G_OBJECT_CLASS(kind)->dispose = the_ground_goes;
    GTK_WIDGET_CLASS(kind)->snapshot = the_ground_is_drawn;
    gtk_widget_class_set_layout_manager_type(GTK_WIDGET_CLASS(kind), GTK_TYPE_BIN_LAYOUT);
}

void satl_console_ground_init(SatlConsoleGround *)
{
}

} // namespace

GdkRGBA a_console_colour(const char *row, const char *authors)
{
    std::string said;
    the_row_folded(row, said);
    GdkRGBA colour;
    if (said.empty() || !read_a_colour(said, colour))
        gdk_rgba_parse(&colour, authors);
    return colour;
}

bool the_console_shadow_is_on()
{
    std::string said;
    return !says_off(the_row_folded(kConsoleShadowRow, said));
}

GtkWidget *a_ground_under(GtkWidget *terminal)
{
    // ON THE DESK, LIKE EVERY CALL HERE, so the one rule needs no lock.
    static GtkCssProvider *transparent = nullptr;
    if (transparent == nullptr) {
        transparent = gtk_css_provider_new();
        gtk_css_provider_load_from_string(transparent,
                                          (std::string("vte-terminal.") + kOnAGround +
                                           " { background-color: transparent; }").c_str());
        gtk_style_context_add_provider_for_display(gtk_widget_get_display(terminal), GTK_STYLE_PROVIDER(transparent),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    SatlConsoleGround *ground = SATL_CONSOLE_GROUND(g_object_new(satl_console_ground_get_type(), nullptr));
    ground->shadowed = the_console_shadow_is_on();
    std::string said;
    const std::string fuzzy = the_row_folded(kFuzzyRow, said);
    ground->fuzzy = !fuzzy.empty() && !says_off(fuzzy);
    if (!ground->fuzzy || !read_a_colour(said, ground->fuzzy_colour))
        gdk_rgba_parse(&ground->fuzzy_colour, kAuthorsFuzzy);
    // ITS SHADOWS STAY INSIDE IT (the fresh reader, 2026-10-04).
    gtk_widget_set_overflow(GTK_WIDGET(ground), GTK_OVERFLOW_HIDDEN);
    vte_terminal_set_clear_background(VTE_TERMINAL(terminal), FALSE);
    gtk_widget_add_css_class(terminal, kOnAGround);
    gtk_widget_set_parent(terminal, GTK_WIDGET(ground));
    gtk_widget_set_hexpand(GTK_WIDGET(ground), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(ground), TRUE);
    return GTK_WIDGET(ground);
}

void the_ground_casts_shadows(GtkWidget *terminal, bool on)
{
    GtkWidget *ground = gtk_widget_get_parent(terminal);
    if (ground == nullptr || !G_TYPE_CHECK_INSTANCE_TYPE(ground, satl_console_ground_get_type()))
        return;
    SATL_CONSOLE_GROUND(ground)->shadowed = on;
    gtk_widget_queue_draw(ground);
}

} // namespace satellite004

#endif
