/* vendor/gtk/hello/hello-static.c -- the experiment that decides whether satellite can
 * ship as ONE executable.
 *
 * It is not a hello-world for its own sake. Linking a static GTK proves only that the
 * CODE is inside the binary; GTK also wants DATA at run time -- GSettings schemas, an
 * icon theme, gdk-pixbuf loaders, GIO modules, fontconfig's configuration -- and it
 * looks for all of it through XDG_DATA_DIRS and dlopen(), neither of which the linker
 * touches. A binary that links clean and dies on a bare machine is the failure this
 * file exists to find, EARLY, before any satellite.window word is minted on top of it.
 *
 * So it reports before it draws. Run it three ways, hardest last:
 *
 *     ./hello-static                        the developer's machine, everything present
 *     env -i ./hello-static                 no environment at all
 *     env -i XDG_DATA_DIRS=/nonexistent ... the schemas and icons are gone
 *
 * NO GtkApplication ON PURPOSE. GtkApplication is GApplication, which registers on the
 * D-Bus session bus, and a bare machine may have no bus at all. gtk_window_new() asks
 * for none of that, and if it is enough for a window then it is the shape satl should
 * use. If satellite later needs GtkApplication for the launcher, that is a separate
 * decision with a separate cost -- see satl-term/window.cpp, which passes
 * G_APPLICATION_NON_UNIQUE for a related reason.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

/* What the process can see of the outside world. Printed BEFORE gtk_init, because
 * gtk_init is the call that may not come back. */
static void say_what_we_can_see(void)
{
    static const char *const names[] = {
        "DISPLAY", "WAYLAND_DISPLAY", "XDG_DATA_DIRS", "XDG_RUNTIME_DIR",
        "GSETTINGS_SCHEMA_DIR", "GSETTINGS_BACKEND", "DBUS_SESSION_BUS_ADDRESS",
        "GDK_PIXBUF_MODULE_FILE", "FONTCONFIG_FILE", "GSK_RENDERER", "GDK_BACKEND",
        "XKB_CONFIG_ROOT", "XDG_CONFIG_HOME", "GTK_A11Y", "GIO_MODULE_DIR",
    };
    fputs("environment:\n", stderr);
    for (unsigned i = 0; i < sizeof names / sizeof *names; i++) {
        const char *v = getenv(names[i]);
        fprintf(stderr, "  %-26s %s\n", names[i], v ? v : "(unset)");
    }
    fprintf(stderr, "compiled against GTK %d.%d.%d, running on %d.%d.%d\n",
            GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION,
            gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version());
}

/* Whether the schema GTK itself reads is reachable. This is the single most likely
 * hard failure: g_settings_new() on a schema that is not installed is documented to
 * ABORT the process, not to return NULL, so asking politely first is the difference
 * between a diagnosis and a core dump. */
static void say_whether_schemas_are_there(void)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    if (source == NULL) {
        fputs("schemas: NO DEFAULT SOURCE -- g_settings_new() would abort\n", stderr);
        return;
    }
    static const char *const wanted[] = {
        "org.gtk.gtk4.Settings.FileChooser",
        "org.gtk.gtk4.Settings.ColorChooser",
        "org.gtk.gtk4.Settings.Debug",
    };
    for (unsigned i = 0; i < sizeof wanted / sizeof *wanted; i++) {
        GSettingsSchema *s = g_settings_schema_source_lookup(source, wanted[i], TRUE);
        fprintf(stderr, "schemas: %-38s %s\n", wanted[i], s ? "found" : "MISSING");
        if (s != NULL)
            g_settings_schema_unref(s);
    }
}

/* WHETHER A KEYMAP CAN BE BUILT AT ALL -- asked here because the alternative is a
 * SEGFAULT WITH NO MESSAGE. gdk/wayland/gdkkeymap-wayland.c:478-486 runs at seat
 * creation, inside gtk_init(), and checks nothing:
 *
 *     context = xkb_context_new (0);
 *     keymap->xkb_keymap = xkb_keymap_new_from_names (context, &names, 0);
 *     keymap->xkb_state  = xkb_state_new (keymap->xkb_keymap);
 *
 * With no xkeyboard-config data on the machine, xkb_context_new() returns NULL (it has
 * no include path to add), xkb_keymap_new_from_names() returns NULL, and xkb_state_new()
 * dereferences it. The process dies before a window exists and before anything is
 * printed, so the one thing that must not happen is finding this out from gtk_init.
 *
 * Code links in; THIS DATA CANNOT. libxkbcommon reads files and only files -- there is
 * no API to hand it bytes, and no meson option that embeds them. A static satl has to
 * carry the tree as a GResource and spill it to a writable directory before gtk_init,
 * then set XKB_CONFIG_ROOT to it. An INCOMPLETE tree fails exactly like no tree.
 */
#if defined(__has_include)
#  if __has_include(<xkbcommon/xkbcommon.h>)
#    define SATL_HAVE_XKB 1
#  endif
#endif

#ifdef SATL_HAVE_XKB
#include <xkbcommon/xkbcommon.h>

static void say_whether_a_keymap_can_be_built(void)
{
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (context == NULL) {
        fputs("xkb: NO CONTEXT -- gtk_init() would segfault at gdkkeymap-wayland.c:485\n",
              stderr);
        return;
    }

    /* The same names GDK asks for: rules=evdev, model=pc105, layout=us. */
    struct xkb_rule_names names;
    names.rules = "evdev";
    names.model = "pc105";
    names.layout = "us";
    names.variant = NULL;
    names.options = NULL;

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, &names,
                                                          XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap == NULL) {
        fputs("xkb: NO KEYMAP -- gtk_init() would segfault at gdkkeymap-wayland.c:486\n",
              stderr);
        xkb_context_unref(context);
        return;
    }
    fprintf(stderr, "xkb: keymap built, %u layout(s) -- gtk_init() will survive the seat\n",
            xkb_keymap_num_layouts(keymap));
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
}
#else
static void say_whether_a_keymap_can_be_built(void)
{
    fputs("xkb: no xkbcommon header at build time -- not probed\n", stderr);
}
#endif

static void on_clicked(GtkButton *button, gpointer user_data)
{
    (void)button; (void)user_data;
    fputs("the button was pressed -- input reaches a static GTK\n", stderr);
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    say_what_we_can_see();
    say_whether_schemas_are_there();
    say_whether_a_keymap_can_be_built();

    /* The renderer is worth knowing before it is chosen for us: -Dvulkan=disabled means
     * GSK falls back to GL, and libepoxy resolves GL by dlopen() at run time even when
     * it is statically linked. "cairo" here would mean software, which still proves the
     * window works. */
    fputs("calling gtk_init()...\n", stderr);
    if (!gtk_init_check()) {
        fputs("gtk_init_check() FAILED -- no display, or no backend\n", stderr);
        return 2;
    }
    fputs("gtk_init() returned\n", stderr);

    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "satellite -- static GTK");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    /* A LABEL AND A BUTTON, because they fail differently. The label proves text
     * rendering -- pango, fontconfig and freetype all found a font and shaped it. The
     * button proves the theme and input. A window with no text in it is a window that
     * technically opened and cannot be used. */
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

    GtkWidget *label = gtk_label_new("If you can read this, pango found a font.");
    GtkWidget *button = gtk_button_new_with_label("press me");
    g_signal_connect(button, "clicked", G_CALLBACK(on_clicked), NULL);

    gtk_box_append(GTK_BOX(box), label);
    gtk_box_append(GTK_BOX(box), button);
    gtk_window_set_child(GTK_WINDOW(window), box);

    gtk_window_present(GTK_WINDOW(window));
    fputs("window presented\n", stderr);

    /* SATL_HELLO_SECONDS lets the harness run this without a human: the window opens,
     * proves itself, and closes on its own. Unset, it waits for a person. */
    const char *seconds = getenv("SATL_HELLO_SECONDS");
    if (seconds != NULL && seconds[0] != '\0') {
        guint n = (guint)strtoul(seconds, NULL, 10);
        fprintf(stderr, "closing in %u second(s)\n", n);
        g_timeout_add_seconds(n, (GSourceFunc)gtk_window_destroy, window);
    }

    /* ONE handler, and _swapped, because a "destroy" callback is called as
     * (window, user_data) -- connecting g_main_loop_quit plainly would hand it the
     * WINDOW where it expects a GMainLoop*. The first version of this file did exactly
     * that, and the window opened, the timer fired, and the loop never quit. */
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_signal_connect_swapped(window, "destroy", G_CALLBACK(g_main_loop_quit), loop);
    g_main_loop_run(loop);

    fputs("clean exit\n", stderr);
    return 0;
}
