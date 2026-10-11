// satellite/satellite_variable_window/window_strokes.cpp -- WHAT A PROGRAM
// DRAWS ON A CANVAS: a line, a box, a circle, an arc and some words; the pen
// they are drawn with; and the list cleared or saved to a picture.
// GTK_AND_NO_DEPENDENCIES.md GTK-15, and its four leftovers of 2026-09-22.
//
// SPLIT OUT OF window_canvas.cpp ON 2026-09-22 (window_canvas.hpp says how).
// That file is the DESK drawing a canvas; this one is a PROGRAM drawing on it.
// Every stroke here appends to the display list and asks GTK to redraw, and
// GTK's draw function replays the list with no satellite code running at all
// -- which is what makes a canvas impossible to deadlock, and window_canvas.cpp
// says why that was the design.
//
// THE PEN IS COPIED ONTO EACH STROKE AT THE MOMENT IT IS MADE -- the colour and
// the font since GTK-15, the thickness and whether it outlines since the
// leftovers -- so `.colour`, `.font`, `.thickness` and `.outline` change what
// is drawn after them and nothing drawn before.
//
// EVERY GTK AND CAIRO CALL HAPPENS INSIDE on_the_desk(), like its neighbours.
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "window_canvas.hpp"

#include "window_desk.hpp"

#include <cairo.h>
#include <gtk/gtk.h>

#include <string>

namespace satellite004 {
namespace {

bool a_canvas_that_is_open(satellite_window &which, std::string &why)
{
    if (which.piece != satellite_window::canvas) {
        why = "only a canvas is drawn on -- " + std::string(which.piece_name()) + " is not";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    return true;
}

// ONE MORE STROKE, IN THE CANVAS'S COLOUR AND FONT OF THIS MOMENT, and a redraw
// asked for. ON THE DESK.
//
// THE PEN IS THE CANVAS'S OWN `.colour`, PARSED HERE, AND NOT THE WIDGET'S
// COMPUTED STYLE -- and that was measured, not assumed. The first draft read
// gtk_widget_get_color, and a canvas not yet in a window that was told
// `.colour` twice drew the second batch in the FIRST colour: adding the css
// class invalidates the node, but reloading a provider does not reach a widget
// that has no root, so the computed style stayed where it was. `a_colour` is
// written on this thread by window_look.cpp and is the truth of what was asked
// for; gdk_rgba_parse reads the same spellings CSS does. The theme's own
// foreground is what a canvas nobody dressed draws in, and gtk_widget_get_color
// is right for that -- there is nothing pending to miss.
//
// THE FONT IS THE SAME STORY: `a_font` and `a_font_size`, as pango spells a
// description, and "" for a canvas nobody gave one. AND THE REST OF THE PEN
// (2026-09-22): the thickness and whether it outlines, copied the same way,
// so a `.thickness(3)` after a box changes nothing about that box.
void stroke_it(satellite_window &canvas, AStroke stroke)
{
    stroke.thickness = static_cast<double>(canvas.pen_thickness);
    stroke.outline = canvas.pen_outline;
    GtkWidget *widget = static_cast<GtkWidget *>(canvas.widget);
    GdkRGBA colour;
    if (canvas.a_colour.empty() || !gdk_rgba_parse(&colour, canvas.a_colour.c_str()))
        gtk_widget_get_color(widget, &colour);
    stroke.red = colour.red;
    stroke.green = colour.green;
    stroke.blue = colour.blue;
    stroke.alpha = colour.alpha;
    if (!canvas.a_font.empty())
        stroke.font = canvas.a_font + " " + std::to_string(canvas.a_font_size > 0 ? canvas.a_font_size : 12) + "px";
    canvas.drawn.push_back(std::move(stroke));
    gtk_widget_queue_draw(widget);
}

} // namespace


bool window_line(satellite_window &which, long long int x1, long long int y1, long long int x2,
                 long long int y2, std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    satellite_window *raw = &which;
    AStroke stroke;
    stroke.shape = AStroke::a_line;
    stroke.x = static_cast<double>(x1);
    stroke.y = static_cast<double>(y1);
    stroke.a = static_cast<double>(x2);
    stroke.b = static_cast<double>(y2);
    on_the_desk([raw, &stroke] { stroke_it(*raw, stroke); });
    return true;
}

bool window_box(satellite_window &which, long long int x, long long int y, long long int wide,
                long long int tall, std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    // NEGATIVE IS REFUSED WHERE IT IS WRITTEN. Cairo would draw the box the
    // other way from its corner, which is an answer nobody asked for; a box of
    // NO size draws nothing, which is exactly what it is, and is let through.
    if (wide < 0 || tall < 0) {
        why = "a box's width and height are 0 or more -- it is drawn from its top-left corner";
        return false;
    }
    satellite_window *raw = &which;
    AStroke stroke;
    stroke.shape = AStroke::a_box;
    stroke.x = static_cast<double>(x);
    stroke.y = static_cast<double>(y);
    stroke.a = static_cast<double>(wide);
    stroke.b = static_cast<double>(tall);
    on_the_desk([raw, &stroke] { stroke_it(*raw, stroke); });
    return true;
}

bool window_circle(satellite_window &which, long long int x, long long int y, long long int radius,
                   std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    if (radius < 0) {
        why = "a circle's radius is 0 or more";
        return false;
    }
    satellite_window *raw = &which;
    AStroke stroke;
    stroke.shape = AStroke::a_circle;
    stroke.x = static_cast<double>(x);
    stroke.y = static_cast<double>(y);
    stroke.a = static_cast<double>(radius);
    on_the_desk([raw, &stroke] { stroke_it(*raw, stroke); });
    return true;
}

bool window_arc(satellite_window &which, long long int x, long long int y, long long int radius,
                long long int from_degrees, long long int to_degrees, std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    if (radius < 0) {
        why = "an arc's radius is 0 or more";
        return false;
    }
    // THE ANGLES ARE POSITIONS ON A CLOCK FACE, so 370 is 10 and -90 is 270
    // -- AND CAIRO DOES NOT READ THEM SO (a fresh reader, 2026-09-22): cairo
    // reads `to - from` as a SWEEP and adds a turn only when `to` is behind
    // `from`, so 0 to 370 handed straight through would be a full turn and
    // ten degrees more, a whole disc where a person asked for a sliver. Both
    // angles are brought onto the face here, and an arc from an angle round
    // to the same angle is the whole circle: clockwise from three o'clock all
    // the way round IS a circle, so `arc(.., 0, 0)` and `arc(.., 0, 360)` both
    // draw one, and there is no sweep a program can write that draws nothing.
    const long long int from = ((from_degrees % 360) + 360) % 360;
    long long int to = ((to_degrees % 360) + 360) % 360;
    if (to <= from)
        to += 360;
    satellite_window *raw = &which;
    AStroke stroke;
    stroke.shape = AStroke::an_arc;
    stroke.x = static_cast<double>(x);
    stroke.y = static_cast<double>(y);
    stroke.a = static_cast<double>(radius);
    stroke.b = static_cast<double>(from);
    stroke.c = static_cast<double>(to);
    on_the_desk([raw, &stroke] { stroke_it(*raw, stroke); });
    return true;
}

bool window_write(satellite_window &which, long long int x, long long int y, const std::string &words,
                  std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    satellite_window *raw = &which;
    AStroke stroke;
    stroke.shape = AStroke::some_words;
    stroke.x = static_cast<double>(x);
    stroke.y = static_cast<double>(y);
    stroke.words = words;
    on_the_desk([raw, &stroke] { stroke_it(*raw, stroke); });
    return true;
}

// ---------------------------------------------------------------------------
// THE REST OF THE PEN: its thickness and whether it outlines (2026-09-22).
// ---------------------------------------------------------------------------
namespace {

bool a_canvas_with_a_pen(satellite_window &which, std::string &why)
{
    if (which.piece != satellite_window::canvas) {
        why = "only a canvas has a pen -- " + std::string(which.piece_name()) + " does not";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    return true;
}

} // namespace

bool window_set_thickness(satellite_window &which, long long int pixels, std::string &why)
{
    if (!a_canvas_with_a_pen(which, why))
        return false;
    // A PEN OF NO WIDTH DRAWS LINES NOBODY CAN SEE, which is the answer that is
    // wrong and does not say so; past a thousand it is a typo and not a wish.
    if (pixels <= 0 || pixels > 1000) {
        why = "a pen's thickness is between 1 and 1000 pixels";
        return false;
    }
    satellite_window *raw = &which;
    const int wide = static_cast<int>(pixels);
    on_the_desk([raw, wide] { raw->pen_thickness = wide; });
    return true;
}

bool window_set_outline(satellite_window &which, bool outline, std::string &why)
{
    if (!a_canvas_with_a_pen(which, why))
        return false;
    satellite_window *raw = &which;
    on_the_desk([raw, outline] { raw->pen_outline = outline; });
    return true;
}

bool window_pen_of(satellite_window &which, bool the_thickness, long long int &out, std::string &why)
{
    if (!a_canvas_with_a_pen(which, why))
        return false;
    satellite_window *raw = &which;
    long long int got = 0;
    // READ ON THE DESK, because that is the thread that writes the pen -- the
    // same crossing `.on` makes for a checkbox, and for the same reason.
    on_the_desk([raw, the_thickness, &got] {
        got = the_thickness ? raw->pen_thickness : (raw->pen_outline ? 1 : 0);
    });
    out = got;
    return true;
}

bool window_clear(satellite_window &which, std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    satellite_window *raw = &which;
    on_the_desk([raw] {
        raw->drawn.clear();
        gtk_widget_queue_draw(static_cast<GtkWidget *>(raw->widget));
    });
    return true;
}

bool window_save(satellite_window &which, const std::string &path, std::string &why)
{
    if (!a_canvas_that_is_open(which, why))
        return false;
    if (path.empty()) {
        why = "a picture needs the name of a file to save it as, and was given \"\"";
        return false;
    }
    satellite_window *raw = &which;
    std::string went_wrong;
    on_the_desk([raw, &path, &went_wrong] {
        GtkWidget *widget = static_cast<GtkWidget *>(raw->widget);
        const int wide = gtk_drawing_area_get_content_width(GTK_DRAWING_AREA(widget));
        const int tall = gtk_drawing_area_get_content_height(GTK_DRAWING_AREA(widget));
        cairo_surface_t *picture = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, wide, tall);
        cairo_t *cr = cairo_create(picture);
        // WHAT IS BEHIND IT, IF THE PROGRAM SAID: `.background(...)` is CSS on
        // the screen, and the picture paints the same colour first so the file
        // looks like the window. Nothing said is nothing painted, and the PNG
        // is transparent there -- which is the truth of what was drawn.
        GdkRGBA behind;
        if (!raw->a_background.empty() && gdk_rgba_parse(&behind, raw->a_background.c_str())) {
            cairo_set_source_rgba(cr, behind.red, behind.green, behind.blue, behind.alpha);
            cairo_paint(cr);
        }
        replay(*raw, widget, cr);
        cairo_destroy(cr);
        const cairo_status_t status = cairo_surface_write_to_png(picture, path.c_str());
        cairo_surface_destroy(picture);
        if (status != CAIRO_STATUS_SUCCESS)
            went_wrong = cairo_status_to_string(status);
    });
    if (!went_wrong.empty()) {
        why = "the picture could not be written to " + path + " -- " + went_wrong;
        return false;
    }
    return true;
}

} // namespace satellite004
