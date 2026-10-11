// satellite/satellite_variable_window/window_canvas.cpp -- A CANVAS: satellite
// draws it itself. GTK_AND_NO_DEPENDENCIES.md GTK-15.
//
// THE FIRST TIME SATELLITE'S OWN CODE CALLS CAIRO, AND THE FIRST TIME IT CALLS
// PANGO. Both have been linked since the first vendored build and every call
// into either so far was GTK's (Part 00's Table B). What is called here is small
// and named in full: cairo for a line, a box and a circle; pango_cairo for the
// words, on a layout GTK made from the widget's own font.
//
// THE PEN HAS FOUR PARTS SINCE 2026-09-22: a colour, a font, a thickness and
// whether it outlines. `.arc` joined the strokes the same day, and `.across`
// and `.down` read where a click landed -- those two live in window_answers.cpp
// (the desk noticing it), bytecode/window_run.cpp (the interpreter copying it
// onto the piece) and bytecode/window_questions.cpp (answering it), because a
// click is an event and not a stroke. All four
// were GTK-15's open questions and were built as the recommendation, still
// reversible; GTK_AND_NO_DEPENDENCIES.md says what changes if the author rules
// otherwise.
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
// SPLIT ON 2026-09-22, at 442 lines against the author's "try to build for 300
// lines", at the seam between who is drawing: this file is the DESK drawing --
// the replay, GTK's draw function and the factory that hands GTK that function
// -- and window_strokes.cpp is a PROGRAM drawing: every stroke, the pen, .clear
// and .save. window_canvas.hpp is the one function both need.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "window_canvas.hpp"

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
// A LINE LANDS ON THE PIXEL IT NAMES. Cairo's coordinates run between pixels,
// so a one-pixel line at a whole number is two half-covered pixels -- a grey
// smear where a person asked for a line. The half is added here so nobody
// writing satellite ever has to know that; a pen of EVEN width sits evenly
// across the line between two pixels already and wants no half. An outline
// is a line too, so it gets the same half; a filled shape has no edge to
// land, and is drawn where it was asked.
double on_the_pixel(double thickness)
{
    return (static_cast<long long int>(thickness) % 2) == 1 ? 0.5 : 0.0;
}

} // namespace

// AN ARC IS PART OF A CIRCLE, CLOCKWISE FROM ONE ANGLE TO THE NEXT, in degrees
// with 0 at three o'clock (GTK-15's leftover, 2026-09-22). That is cairo's own
// convention, and on a screen whose `down` grows downward it is a clock's.
// cairo_arc always runs the increasing way round, adding a turn when `to` is
// behind `from`, so `arc(.., 270, 0)` is the quarter from twelve to three.
// FILLED IT IS A SLICE from the centre -- a pie chart's, a clock face's --
// and as an OUTLINE it is the curve alone, which is what a drawing wants.
void replay(const satellite_window &canvas, GtkWidget *widget, cairo_t *cr)
{
    for (const AStroke &stroke : canvas.drawn) {
        cairo_set_source_rgba(cr, stroke.red, stroke.green, stroke.blue, stroke.alpha);
        cairo_set_line_width(cr, stroke.thickness);
        const double off = on_the_pixel(stroke.thickness);
        switch (stroke.shape) {
        case AStroke::a_line:
            cairo_move_to(cr, stroke.x + off, stroke.y + off);
            cairo_line_to(cr, stroke.a + off, stroke.b + off);
            cairo_stroke(cr);
            break;
        case AStroke::a_box:
            if (stroke.outline) {
                cairo_rectangle(cr, stroke.x + off, stroke.y + off, stroke.a, stroke.b);
                cairo_stroke(cr);
            } else {
                cairo_rectangle(cr, stroke.x, stroke.y, stroke.a, stroke.b);
                cairo_fill(cr);
            }
            break;
        case AStroke::a_circle:
            cairo_new_path(cr);
            if (stroke.outline) {
                cairo_arc(cr, stroke.x + off, stroke.y + off, stroke.a, 0.0, 2.0 * M_PI);
                cairo_stroke(cr);
            } else {
                cairo_arc(cr, stroke.x, stroke.y, stroke.a, 0.0, 2.0 * M_PI);
                cairo_fill(cr);
            }
            break;
        case AStroke::an_arc: {
            const double from = stroke.b * M_PI / 180.0, to = stroke.c * M_PI / 180.0;
            cairo_new_path(cr);
            if (stroke.outline) {
                cairo_arc(cr, stroke.x + off, stroke.y + off, stroke.a, from, to);
                cairo_stroke(cr);
            } else {
                cairo_move_to(cr, stroke.x, stroke.y);
                cairo_arc(cr, stroke.x, stroke.y, stroke.a, from, to);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            break;
        }
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

namespace {

// GTK ASKING FOR THE CANVAS TO BE DRAWN. ON THE DESK'S THREAD, and it walks
// nothing: the list is what it draws. `user_data` is the piece, and it is
// always valid here -- the widget is drawn only while it is in a window, and
// the window holds the piece in `pieces` until the desk lets go of it, which
// nulls `widget` before GTK could ever draw it again.
void draw_the_list(GtkDrawingArea *area, cairo_t *cr, int, int, gpointer user_data)
{
    replay(*static_cast<satellite_window *>(user_data), GTK_WIDGET(area), cr);
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

} // namespace satellite004
