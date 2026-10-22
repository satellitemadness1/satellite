// satellite/satellite_variable_window/window_wayland_fixes.cpp -- wl_fixes_interface, the one
// wayland symbol the carried GTK needs that wayland 1.23 does not have.
//
// THE CARRIED GTK WAS BUILT AGAINST WAYLAND 1.24 (AlmaLinux 10.2; vendor/stage/pkgconfig-system/
// wayland-client.pc), and GDK binds the wl_fixes global that 1.24 added to the core protocol:
// vendor/gtk/gtk-4.24.0/gdk/wayland/gdkdisplay-wayland.c names &wl_fixes_interface where it binds
// it and its .name where it matches it. That table is DATA, and it lives in libwayland-client 1.24.
// Measured 2026-10-15: the stack references 49 wayland symbols, Debian 13's libwayland 1.23.1 has 48
// of them, and this is the 49th -- the link stopped at it, and `ldd -r` on a satl linked on AlmaLinux
// says the same thing at load time, so that binary could not start on Debian 13 at all.
//
// SO satl CARRIES THE TABLE ITSELF (the author, 2026-10-15: "the wl_fixes shim"). A table is not
// code: libwayland marshals any interface from its name and its message signatures, so 1.23's
// library sends a wl_fixes request exactly as 1.24's does -- and where the compositor offers no
// wl_fixes global, GDK never binds one. Nothing else in the core protocol changed between 1.23.1
// and 1.24.0 but one enum value (wl_keyboard's key_state.repeated, at wl_seat 10), which no table
// checks. THE FLOOR IS THEN WAYLAND 1.22, not 1.24: the newest call the stack makes is
// wl_proxy_marshal_flags (1.20), but GDK binds wl_compositor up to 6 and wl_seat up to 10, and the
// events of wl_compositor 6 and wl_seat 9 are in no table before 1.22's -- an older libwayland,
// under a compositor that offers them, would drop the connection. Ubuntu 24.04 ships 1.22.0 and
// Debian 13 1.23.1 (the wayland.xml of each release, compared 2026-10-15).
//
// HIDDEN, so the table is satl's GTK's alone. The link resolves GDK's reference to a regular
// object's definition ahead of any libwayland's, and a hidden symbol never reaches the dynamic
// symbol table, where a GPU driver's own wayland calls could find it. Where the machine's
// libwayland has its own copy, both exist, and they agree on every request this GTK sends
// (1.24.0's is version 1 and this one version 2 -- below).
//
// VERSION 2 -- wayland main's table on 2026-10-15 -- where this GTK was compiled for version 1,
// 1.24.0's: gdkdisplay-wayland.c:132 picks the version by WL_FIXES_ACK_GLOBAL_REMOVE, and only
// gdkdisplay-wayland.c.o references the symbol (GTK's input method binds it only at version 2).
// Version 2 is version 1 with one request added at the end, so this GTK -- which binds version 1
// and sends requests 0 and 1 -- is answered exactly as 1.24.0's table would answer it, and a GTK
// rebuilt against newer headers, which also sends request 2, finds it here instead of past the end
// of a shorter table. Every entry below is `wayland-scanner private-code` output for
// <interface name="wl_fixes">, copied. A GTK built against a wayland whose wl_fixes is newer than
// version 2 needs this table regenerated the same way.
//
// COMPILED ONLY FOR THE CARRIED GTK (047-window.mk): `make GTK=system` links the system's GTK
// against the system's own libwayland, and those two agree already.

#include <wayland-util.h>

extern "C" {

extern const struct wl_interface wl_registry_interface;

__attribute__((visibility("hidden"))) extern const struct wl_interface wl_fixes_interface;

// What each request's arguments are: "destroy" has none, "destroy_registry" one wl_registry,
// "ack_global_remove" a wl_registry and a uint -- the slots wayland-scanner points them at.
static const struct wl_interface *wl_fixes_types[] = {
    nullptr,
    &wl_registry_interface,
    &wl_registry_interface,
    nullptr,
};

static const struct wl_message wl_fixes_requests[] = {
    { "destroy", "", wl_fixes_types + 0 },
    { "destroy_registry", "o", wl_fixes_types + 1 },
    { "ack_global_remove", "2ou", wl_fixes_types + 2 },
};

const struct wl_interface wl_fixes_interface = {
    "wl_fixes", 2,
    3, wl_fixes_requests,
    0, nullptr,
};

}
