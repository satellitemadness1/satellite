// satellite/satellite_variable_window/window_canvas.cpp -- A CANVAS: satellite
// draws it itself. GTK_AND_NO_DEPENDENCIES.md GTK-15.
//
// THE FIRST TIME SATELLITE'S OWN CODE CALLS CAIRO, AND THE FIRST TIME IT CALLS
// PANGO. Both have been linked since the first vendored build and every call
// into either so far was GTK's (Part 00's Table B). What is called here is small
// and named in full: cairo for a line, a box and a circle; pango_cairo for the
// words, on a layout GTK made from the widget's own font.
//
// A DISPLAY LIST AND NOT A DRAW CAPSULE, and that is the whole design. GTK's
// draw function must return having drawn. Queueing a satellite capsule to the
// interpreter's thread and WAITING for it would have the desk blocked on the
// interpreter while the interpreter may be blocked on the desk -- a deadlock,
// and every piece needed to build one was already here. So a program draws
// into the canvas with `.line`, `.box`, `.circle` and `.write`; satellite keeps
// the list on the piece; and the draw function replays it with no satellite
// code running at all. It is faster, it cannot deadlock, and it is the only one
// of the two shapes that works before the walker is re-entrant. GTK-15 said so
// before this was written and it was the recommendation; the other shape stays
// the author's to ask for.
//
// EVERY GTK, CAIRO AND PANGO CALL HAPPENS INSIDE on_the_desk() OR INSIDE THE
// DRAW FUNCTION, which is the desk's own. The list is read and written on that
// thread and no other, like `when_pressed`.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <cairo.h>
#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include <cmath>
#include <string>

namespace satellite004 {
namespace {

// THE LIST, REPLAYED. ON THE DESK'S THREAD -- from the draw function, and from
// `.save` inside on_the_desk(). One replay for both, so a saved picture is the
// picture on the screen and not a second drawing that could drift from it.
//
// A LINE IS ONE PIXEL WIDE AND IT LANDS ON THE PIXEL IT NAMES. Cairo's
// coordinates run between pixels, so a one-pixel line at a whole number is two
// half-covered pixels -- a grey smear where a person asked for a line. The
// half is added here so nobody writing satellite ever has to know that.
void replay(const satellite_window &canvas, GtkWidget *widget, cairo_t *cr)
{
    for (const AStroke &stroke : canvas.drawn) {
        cairo_set_source_rgba(cr, stroke.red, stroke.green, stroke.blue, stroke.alpha);
        switch (stroke.shape) {
        case AStroke::a_line:
            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, stroke.x + 0.5, stroke.y + 0.5);
            cairo_line_to(cr, stroke.a + 0.5, stroke.b + 0.5);
            cairo_stroke(cr);
            break;
        case AStroke::a_box:
            cairo_rectangle(cr, stroke.x, stroke.y, stroke.a, stroke.b);
            cairo_fill(cr);
            break;
        case AStroke::a_circle:
            cairo_arc(cr, stroke.x, stroke.y, stroke.a, 0.0, 2.0 * M_PI);
            cairo_fill(cr);
            break;
        case AStroke::some_words: {
            // A LAYOUT FROM THE WIDGET, so with no `.font` the words are drawn
            // the way a label's would be; with one, the font the stroke carries
            // -- which is the canvas's `.font` as it was when the words were
            // written, so a later `.font` changes only what comes after it.
            PangoLayout *layout = gtk_widget_create_pango_layout(widget, stroke.words.c_str());
            if (!stroke.font.empty()) {
                PangoFontDescription *face = pango_font_description_from_string(stroke.font.c_str());
                pango_layout_set_font_description(layout, face);
                pango_font_description_free(face);
            }
            cairo_move_to(cr, stroke.x, stroke.y);
            pango_cairo_show_layout(cr, layout);
            g_object_unref(layout);
            break;
        }
        }
    }
}

// GTK ASKING FOR THE CANVAS TO BE DRAWN. ON THE DESK'S THREAD, and it walks
// nothing: the list is what it draws. `user_data` is the piece, and it is
// always valid here -- the widget is drawn only while it is in a window, and
// the window holds the piece in `pieces` until the desk lets go of it, which
// nulls `widget` before GTK could ever draw it again.
void draw_the_list(GtkDrawingArea *area, cairo_t *cr, int, int, gpointer user_data)
{
    replay(*static_cast<satellite_window *>(user_data), GTK_WIDGET(area), cr);
}

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
// description, and "" for a canvas nobody gave one.
void stroke_it(satellite_window &canvas, AStroke stroke)
{
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

WindowHandle window_piece_of_a_size(satellite_window::Piece which, long long int wide,
                                    long long int tall, std::string &why)
{
    if (which != satellite_window::canvas) {
        why = "that is not a piece made from a size";
        return nullptr;
    }
    // THE SAME TWO REFUSALS A WINDOW HAS, for the same reasons: a canvas of no
    // size is one nobody can see, and one past int is a typo.
    if (wide <= 0 || tall <= 0) {
        why = "a canvas's width and height must both be more than 0";
        return nullptr;
    }
    if (wide > 32767 || tall > 32767) {
        why = "a canvas's width and height must each be 32767 or less";
        return nullptr;
    }
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(which);
    satellite_window *raw = made.get();
    const int to_wide = static_cast<int>(wide), to_tall = static_cast<int>(tall);
    on_the_desk([raw, to_wide, to_tall] {
        GtkWidget *area = gtk_drawing_area_new();
        gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(area), to_wide);
        gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), to_tall);
        // THE DRAW FUNCTION IS SET ONCE, HERE, AND HANDED THE PIECE. No destroy
        // notify: the piece outlives the widget (the window holds it), so there
        // is nothing to free when GTK lets the function go.
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), draw_the_list, raw, nullptr);
        raw->widget = area;
    });
    return made;
}

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
