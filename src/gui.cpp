// The satellite GUI console.
//
// A GTK4 window with a scrolling read-only transcript on top and a command
// entry underneath.  It is a front end for exactly the same interpreter seam
// the CLI uses (satellite/interpret.hpp), so it gains capability automatically
// as each stage of that seam lands.
//
// Version 001 is preliminary.  There is no parser and no virtual machine, so
// ordinary satellite code is NOT executed here or anywhere else -- only the
// satellite.cxx blocks inside a file actually run.  The banner says so on
// startup and the status line never claims otherwise.
//
// Commands:
//   interpret <file>   run the interpreter, show diagnostics and output
//   check     <file>   lex only, report token counts
//   lex       <file>   dump the token stream
//   clear              clear the output
//   help               list the commands
//   quit               close the window
#include "satellite/container.hpp"
#include "satellite/interpret.hpp"
#include "satellite/lexer.hpp"

#include <gtk/gtk.h>

#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace satellite;

static const char* kVersion = "001";

namespace {

struct App {
    GtkWidget*     window = nullptr;
    GtkWidget*     view   = nullptr;
    GtkWidget*     entry  = nullptr;
    GtkWidget*     status = nullptr;
    GtkTextBuffer* buffer = nullptr;
};

// ---------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------

// Scrolling straight after an insert can land in the wrong place: the view has
// not laid the new text out yet.  Deferring to an idle callback lets GTK size
// the line first, so the last line is genuinely visible.
gboolean scroll_to_end_idle(gpointer data) {
    App* app = static_cast<App*>(data);
    GtkTextMark* mark = gtk_text_buffer_get_mark(app->buffer, "scroll-end");
    if (mark)
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->view), mark,
                                     0.0, TRUE, 0.0, 1.0);
    return G_SOURCE_REMOVE;
}

void append(App* app, const std::string& text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->buffer, &end);
    gtk_text_buffer_insert(app->buffer, &end, text.c_str(),
                           static_cast<int>(text.size()));
}

void line(App* app, const std::string& text) { append(app, text + "\n"); }

void set_status(App* app, const std::string& text) {
    gtk_label_set_text(GTK_LABEL(app->status), text.c_str());
}

void finish_output(App* app) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->buffer, &end);
    gtk_text_buffer_move_mark_by_name(app->buffer, "scroll-end", &end);
    g_idle_add(scroll_to_end_idle, app);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool read_file(const std::string& path, std::string* out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    *out = ss.str();
    return true;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Splits "interpret hello.satl" into "interpret" and "hello.satl".  The
// argument keeps its internal spaces, so a path with a space in it survives.
void split_command(const std::string& in, std::string* verb, std::string* arg) {
    std::string s = trim(in);
    size_t sp = s.find_first_of(" \t");
    if (sp == std::string::npos) {
        *verb = s;
        arg->clear();
    } else {
        *verb = s.substr(0, sp);
        *arg  = trim(s.substr(sp));
    }
}

// Resolves the typed path and reports the failure in the transcript if it does
// not exist.  Returns empty on failure.
std::string resolve_or_report(App* app, const std::string& typed) {
    std::string path = resolve_satl_path(typed);
    if (path.empty()) {
        line(app, "no such file: " + typed);
        line(app, "  (tried \"" + typed + "\" and \"" + typed + ".satl\")");
        set_status(app, "file not found");
        return "";
    }
    // Worth saying out loud -- the user typed one thing and got another.
    if (path != typed) line(app, "resolved \"" + typed + "\" -> " + path);
    return path;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

void cmd_interpret(App* app, const std::string& typed) {
    if (typed.empty()) {
        line(app, "interpret needs a file");
        set_status(app, "interpret: missing argument");
        return;
    }
    std::string path = resolve_or_report(app, typed);
    if (path.empty()) return;

    // Nothing from the interpreter may reach the GTK main loop: an exception
    // thrown through C frames is undefined behaviour, and a dialogless abort
    // would look like the GUI itself crashed.
    InterpretResult r;
    try {
        r = interpret_file(path);
    } catch (const std::exception& e) {
        line(app, std::string("internal error: ") + e.what());
        set_status(app, "internal error");
        return;
    } catch (...) {
        line(app, "internal error: unknown exception");
        set_status(app, "internal error");
        return;
    }

    for (const auto& d : r.diagnostics) line(app, d.format(r.file));
    for (const auto& o : r.output)      line(app, o);

    line(app, std::string("stage reached: ") + stage_name(r.reached));
    if (!r.summary.empty()) line(app, r.summary);

    set_status(app, r.summary.empty()
                        ? (r.ok ? "ok" : "failed")
                        : r.summary);
}

void cmd_check(App* app, const std::string& typed) {
    if (typed.empty()) {
        line(app, "check needs a file");
        set_status(app, "check: missing argument");
        return;
    }
    std::string path = resolve_or_report(app, typed);
    if (path.empty()) return;

    std::string src;
    if (!read_file(path, &src)) {
        line(app, "cannot read " + path);
        set_status(app, "read failed");
        return;
    }

    Lexer lx(src);
    auto ts = lx.scan();

    int sat = 0, ident = 0, str = 0, num = 0, blocks = 0, stmts = 0;
    for (const auto& t : ts) {
        switch (t.kind) {
            case Tok::Satellite: ++sat;    break;
            case Tok::Ident:     ++ident;  break;
            case Tok::Str:       ++str;    break;
            case Tok::Int:
            case Tok::Real:      ++num;    break;
            case Tok::CxxBlock:  ++blocks; break;
            case Tok::Term:      ++stmts;  break;
            default: break;
        }
    }

    for (const auto& e : lx.errors())
        line(app, path + ":" + std::to_string(e.line) + ":" +
                      std::to_string(e.col) + ": error: " + e.message);

    line(app, path + ": " + (lx.ok() ? "ok" : "FAILED"));
    line(app, "  " + std::to_string(ts.size()) + " tokens, " +
                  std::to_string(stmts) + " statements");
    line(app, "  " + std::to_string(sat) + " satellite keywords, " +
                  std::to_string(ident) + " identifiers");
    line(app, "  " + std::to_string(str) + " strings, " +
                  std::to_string(num) + " numbers, " +
                  std::to_string(blocks) + " cxx blocks");

    set_status(app, lx.ok()
                        ? (std::to_string(ts.size()) + " tokens, no lex errors")
                        : (std::to_string(lx.errors().size()) + " lex error(s)"));
}

void cmd_lex(App* app, const std::string& typed) {
    if (typed.empty()) {
        line(app, "lex needs a file");
        set_status(app, "lex: missing argument");
        return;
    }
    std::string path = resolve_or_report(app, typed);
    if (path.empty()) return;

    std::string src;
    if (!read_file(path, &src)) {
        line(app, "cannot read " + path);
        set_status(app, "read failed");
        return;
    }

    Lexer lx(src);
    auto ts = lx.scan();

    size_t shown = 0;
    for (const auto& t : ts) {
        if (t.kind == Tok::End) break;
        char pos[32];
        g_snprintf(pos, sizeof pos, "%4d:%-3d  ", t.line, t.col);
        std::string row = std::string(pos);
        std::string name = tok_name(t.kind);
        name.resize(10, ' ');
        row += name;
        switch (t.kind) {
            case Tok::Ident:
            case Tok::Int:
            case Tok::Real:
                row += "  " + t.text;
                break;
            case Tok::Str:
                row += "  \"" + t.text + "\"";
                break;
            case Tok::CxxBlock:
                row += "  <" + std::to_string(t.text.size()) + " bytes of C++>";
                break;
            default: break;
        }
        line(app, row);
        ++shown;
    }

    for (const auto& e : lx.errors())
        line(app, path + ":" + std::to_string(e.line) + ":" +
                      std::to_string(e.col) + ": error: " + e.message);

    set_status(app, std::to_string(shown) + " token(s) from " + path);
}

void cmd_help(App* app) {
    line(app, "commands:");
    line(app, "  interpret <file>   run the interpreter, show diagnostics and output");
    line(app, "  check     <file>   lex only, report token counts");
    line(app, "  lex       <file>   dump the token stream");
    line(app, "  clear              clear the output");
    line(app, "  help               list the commands");
    line(app, "  quit               close the window");
    line(app, "");
    line(app, "the .satl extension is optional: `interpret hello` finds hello.satl");
    set_status(app, "help");
}

void banner(App* app) {
    line(app, std::string("satellite ") + kVersion + " -- GUI console (preliminary)");
    line(app, "");
    line(app, "There is no parser and no virtual machine in this version, so");
    line(app, "ordinary satellite code is not executed.  Only satellite.cxx { }");
    line(app, "blocks actually run.  Everything else is lexed and checked.");
    line(app, "");
    line(app, "Type `help` for the command list.");
    line(app, "");
    set_status(app, "ready -- no parser or VM yet; satellite.cxx blocks only");
    finish_output(app);
}

void run_command(App* app, const std::string& input) {
    std::string verb, arg;
    split_command(input, &verb, &arg);
    if (verb.empty()) return;

    line(app, "> " + trim(input));

    if (verb == "interpret")   cmd_interpret(app, arg);
    else if (verb == "check")  cmd_check(app, arg);
    else if (verb == "lex")    cmd_lex(app, arg);
    else if (verb == "clear") {
        gtk_text_buffer_set_text(app->buffer, "", 0);
        set_status(app, "cleared");
    }
    else if (verb == "help")   cmd_help(app);
    else if (verb == "quit" || verb == "exit") {
        gtk_window_destroy(GTK_WINDOW(app->window));
        return;   // app is about to go away; do not touch its widgets
    }
    else {
        line(app, "unknown command: " + verb);
        line(app, "type `help` for the command list");
        set_status(app, "unknown command: " + verb);
    }

    line(app, "");
    finish_output(app);
}

// ---------------------------------------------------------------------------
// Signals
// ---------------------------------------------------------------------------

void on_entry_activate(GtkEntry* entry, gpointer data) {
    App* app = static_cast<App*>(data);
    const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));
    std::string input = text ? text : "";

    gtk_editable_set_text(GTK_EDITABLE(entry), "");

    if (trim(input).empty()) return;

    // The command may destroy the window (quit), so nothing below may assume
    // the entry is still alive.
    GtkWidget* w = app->window;
    run_command(app, input);
    if (GTK_IS_WIDGET(w) && gtk_widget_get_visible(w))
        gtk_widget_grab_focus(app->entry);
}

void on_activate(GtkApplication* gtk_app, gpointer data) {
    App* app = static_cast<App*>(data);

    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "satellite console");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 600);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);
    gtk_widget_set_margin_start(box, 6);
    gtk_widget_set_margin_end(box, 6);
    gtk_window_set_child(GTK_WINDOW(app->window), box);

    // Output transcript.
    app->view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app->view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app->view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app->view), 6);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(app->view), 6);
    app->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->view));

    // Right gravity keeps this mark pinned to the end as text is inserted.
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->buffer, &end);
    gtk_text_buffer_create_mark(app->buffer, "scroll-end", &end, FALSE);

    GtkWidget* scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), app->view);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_widget_set_hexpand(scroller, TRUE);
    gtk_box_append(GTK_BOX(box), scroller);

    // Command entry.
    app->entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->entry),
                                   "interpret hello.satl");
    gtk_widget_set_hexpand(app->entry, TRUE);
    g_signal_connect(app->entry, "activate",
                     G_CALLBACK(on_entry_activate), app);
    gtk_box_append(GTK_BOX(box), app->entry);

    // Status line.
    app->status = gtk_label_new("");
    gtk_widget_set_halign(app->status, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(app->status), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(app->status), 0.0f);
    gtk_box_append(GTK_BOX(box), app->status);

    banner(app);

    gtk_window_present(GTK_WINDOW(app->window));
    gtk_widget_grab_focus(app->entry);
}

}  // namespace

int main(int argc, char** argv) {
    init_tables();

    App app;
    GtkApplication* gtk_app =
        gtk_application_new("org.satellite.console", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(on_activate), &app);

    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    g_object_unref(gtk_app);
    return status;
}
