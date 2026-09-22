// satellite/satellite_variable_window/window_look.cpp -- WHAT A PIECE LOOKS
// LIKE: its colour, what is behind it, and the font its words are drawn in.
// GTK_AND_NO_DEPENDENCIES.md GTK-10.
//
// GTK4 HAS NO PER-WIDGET COLOUR SETTER AT ALL. Everything is CSS, so this file
// is really "satellite generates a stylesheet", which is a bigger idea than the
// three methods above it look like.
//
// ONE CLASS AND ONE PROVIDER A PIECE, and that is what makes it per-piece at
// all. A provider added to the DISPLAY styles every window, so each piece gets a
// css class nobody else has -- `satl-1`, `satl-2` -- and its provider is scoped
// to that class. The piece keeps its provider so a second `.colour` REPLACES the
// first rather than piling another rule on top of it, which would leave whichever
// GTK happened to sort last winning.
//
// NOT gtk_widget_get_style_context(), which GTK deprecated in 4.10 and 4.24 has
// removed the header for -- gtkstylecontext.h is not in the tarball. The class
// and the display are the way that is left, and it is the better one anyway.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

#include <string>

namespace satellite004 {
namespace {

// A NAME NO OTHER PIECE HAS. ON THE DESK'S THREAD ONLY, which is what makes a
// plain counter right: there is one desk, so there is one thread handing these
// out, and no two pieces can be given the same number.
unsigned long long int so_far = 0;

// A COLOUR SATELLITE WILL PASS ON, and nothing else. GTK parses a colour out of
// a stylesheet and a bad one is a PARSE ERROR that takes the whole rule with it
// -- so `.colour("orange juice")` would silently leave the piece unstyled, which
// is the answer that is wrong and does not say so.
//
// WHAT IS ALLOWED: letters, digits, `#`, `(`, `)`, `,`, `.`, `%` and spaces.
// That covers `#00ff88`, `red`, `rgb(0, 255, 136)` and `rgba(0,0,0,0.5)`, and it
// refuses the `;` and `}` that are the only way a colour could carry a second
// rule in with it.
bool a_colour_we_will_pass_on(const std::string &colour)
{
    if (colour.empty() || colour.size() > 64)
        return false;
    for (const char c : colour) {
        const bool fine = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                          c == '#' || c == '(' || c == ')' || c == ',' || c == '.' || c == '%' ||
                          c == ' ';
        if (!fine)
            return false;
    }
    return true;
}

// AND A FONT NAME, by the same rule and for the same reason. A face name is
// letters, digits, spaces and hyphens; it is quoted into the stylesheet, so what
// must not get through is a quote.
bool a_font_we_will_pass_on(const std::string &face)
{
    if (face.empty() || face.size() > 64)
        return false;
    for (const char c : face) {
        const bool fine = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                          c == ' ' || c == '-' || c == '_';
        if (!fine)
            return false;
    }
    return true;
}

// GTK COULD NOT PARSE WHAT WE GAVE IT, and without this nobody would ever know.
// gtk_css_provider_load_from_string() answers nothing at all: a rule it cannot
// read is DROPPED, the piece stays exactly as it was, and the program carries on
// believing it asked for something. That is the answer that is wrong and does
// not say so, and a stylesheet is the easiest place in this whole module to
// produce one -- `.colour("orange juice")` gets through any character filter
// worth writing and means nothing to GTK.
//
// THE SIGNAL IS THE ONLY WAY TO HEAR ABOUT IT. It fires DURING the load, on this
// thread, so a plain bool beside the call is enough and no lock is wanted.
bool the_sheet_was_bad = false;

void it_would_not_parse(GtkCssProvider *, GtkCssSection *, const GError *, gpointer)
{
    the_sheet_was_bad = true;
}

// THE RULE THIS PIECE IS WEARING, REWRITTEN AND RELOADED. ON THE DESK.
//
// EVERY PART IS KEPT AND ALL OF THEM ARE WRITTEN EACH TIME, because a provider
// holds one stylesheet: setting `.colour` after `.font` has to say the font
// again or it would take it away.
bool wear_it(satellite_window &which, GtkWidget *widget)
{
    if (which.style_class.empty()) {
        which.style_class = "satl-" + std::to_string(++so_far);
        gtk_widget_add_css_class(widget, which.style_class.c_str());
    }
    std::string sheet = "." + which.style_class + " { ";
    if (!which.a_colour.empty())
        sheet += "color: " + which.a_colour + "; ";
    if (!which.a_background.empty())
        sheet += "background: " + which.a_background + "; ";
    if (!which.a_font.empty())
        sheet += "font-family: \"" + which.a_font + "\"; ";
    if (which.a_font_size > 0)
        sheet += "font-size: " + std::to_string(which.a_font_size) + "px; ";
    sheet += "}";

    GtkCssProvider *wearing = static_cast<GtkCssProvider *>(which.look);
    if (wearing == nullptr) {
        wearing = gtk_css_provider_new();
        which.look = wearing;
        // THE DISPLAY AND NOT THE WIDGET, because GTK 4.24 has removed the
        // header for the per-widget way. The class above is what keeps this rule
        // from reaching every other window.
        gtk_style_context_add_provider_for_display(gtk_widget_get_display(widget),
                                                   GTK_STYLE_PROVIDER(wearing),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_signal_connect(wearing, "parsing-error", G_CALLBACK(it_would_not_parse), nullptr);
    }
    the_sheet_was_bad = false;
    gtk_css_provider_load_from_string(wearing, sheet.c_str());
    return !the_sheet_was_bad;
}

// WHAT EVERY ONE OF THESE NEEDS TO BE TRUE. A window IS allowed: GTK styles a
// GtkWindow, and a program that wants a dark window should be able to say so.
bool it_can_be_dressed(satellite_window &which, std::string &why)
{
    // A MENU WEARS ITS WINDOW'S LOOK (GTK-12). The window's bar draws it, and
    // GTK's own CSS hands a window's colour and font down to everything in it
    // -- so `my_window.font(...)` reaches the menu, and a menu of its own to
    // dress would be a class on a model GTK never draws.
    if (!which.is_drawn()) {
        why = "a menu wears its window's look -- dress the window, and the menu across "
              "its top follows";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    return true;
}

} // namespace

bool window_set_colour(satellite_window &which, const std::string &colour, bool behind,
                       std::string &why)
{
    if (!it_can_be_dressed(which, why))
        return false;
    if (!a_colour_we_will_pass_on(colour)) {
        why = "\"" + colour + "\" is not a colour satl will pass on -- write it as #00ff88, as a "
              "name like red, or as rgb(0, 255, 136)";
        return false;
    }
    satellite_window *raw = &which;
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    // THE OLD ONE IS PUT BACK IF GTK WILL NOT HAVE THE NEW ONE, so a refused
    // colour leaves the piece wearing exactly what it was wearing -- and not a
    // rule with a hole in it where the bad line was dropped.
    std::string before = behind ? which.a_background : which.a_colour;
    bool worn = false;
    on_the_desk([raw, widget, &colour, behind, &before, &worn] {
        (behind ? raw->a_background : raw->a_colour) = colour;
        worn = wear_it(*raw, widget);
        if (!worn) {
            (behind ? raw->a_background : raw->a_colour) = before;
            wear_it(*raw, widget);
        }
    });
    if (!worn) {
        why = "GTK could not read \"" + colour + "\" as a colour, so nothing was changed";
        return false;
    }
    return true;
}

bool window_set_font(satellite_window &which, const std::string &face, long long int size,
                     std::string &why)
{
    if (!it_can_be_dressed(which, why))
        return false;
    if (!a_font_we_will_pass_on(face)) {
        why = "\"" + face + "\" is not a font name satl will pass on -- letters, digits, spaces, "
              "- and _ only";
        return false;
    }
    // A SIZE OF 0 IS NOT A SIZE, and GTK draws a 0px font as nothing at all --
    // words that are there and cannot be seen. 400 is past any screen's useful
    // range and is a typo rather than a wish.
    if (size <= 0 || size > 400) {
        why = "a font size is between 1 and 400";
        return false;
    }
    satellite_window *raw = &which;
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const int points = static_cast<int>(size);
    const std::string was_face = which.a_font;
    const int was_size = which.a_font_size;
    bool worn = false;
    on_the_desk([raw, widget, &face, points, &was_face, was_size, &worn] {
        raw->a_font = face;
        raw->a_font_size = points;
        worn = wear_it(*raw, widget);
        if (!worn) {
            raw->a_font = was_face;
            raw->a_font_size = was_size;
            wear_it(*raw, widget);
        }
    });
    if (!worn) {
        why = "GTK could not read \"" + face + "\" as a font, so nothing was changed";
        return false;
    }
    return true;
}

} // namespace satellite004
