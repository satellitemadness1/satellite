#pragma once
// satellite/satellite_variable_window/window_canvas.hpp -- THE ONE THING BOTH
// HALVES OF A CANVAS SHARE: replaying its display list. GTK-15.
//
// SPLIT OUT OF window_canvas.cpp ON 2026-09-22, at 442 lines against the
// author's "try to build for 300 lines". The seam is who is drawing:
//
//   window_canvas.cpp    the DESK draws it: the replay, GTK's draw function, and
//                        the factory that hands GTK that function
//   window_strokes.cpp   a PROGRAM draws on it: .line .box .circle .arc .write,
//                        the pen (.thickness .outline), .clear and .save
//
// `.save` is why this header exists: it replays the same list into a picture,
// so a saved file and the screen cannot differ. GTK AND CAIRO ARE IN THIS
// HEADER, which is allowed here and nowhere public -- it is included by two
// files compiled only where pkg-config found gtk4.

#include "satellite_window.hpp"

#include <cairo.h>
#include <gtk/gtk.h>

namespace satellite004 {

// THE LIST, REPLAYED, ON THE DESK'S THREAD -- from the draw function and from
// `.save`. `widget` is what a layout for the words is made from, so with no
// `.font` they are drawn the way a label's would be.
void replay(const satellite_window &canvas, GtkWidget *widget, cairo_t *cr);

} // namespace satellite004
