// The window's tabs. See tabs.hpp for the split.
//
// A NOTEBOOK IS THE WHOLE OF IT. GTK draws the bar, remembers the order and
// answers which page is in front; what this file adds is the two questions the
// rest of the folder asks -- which terminal a keystroke or a menu item is for,
// and what happens to a page whose interpreter has finished.

#include "programs/satl-term/tabs.hpp"
#include "programs/satl-term/keys.hpp"
#include "programs/satl-term/terminal.hpp"

namespace satellite {
namespace {

GtkWidget *the_window = nullptr;
GtkNotebook *the_notebook = nullptr;
bool hold_clean_exit = false;

// A page and its terminal, each pointing at the other, STORED RATHER THAN
// DERIVED. gtk_scrolled_window_get_child() would answer today, because VTE is
// a GtkScrollable and a scrolled window parents those directly -- and would
// silently answer with an interposed viewport the day that stopped being true,
// which reads as tabs that will not close. Two pointers set where the pair is
// made cost nothing and cannot be wrong.
const char *PAGE_OF = "satl-term-page";
const char *TERMINAL_OF = "satl-term-terminal";

GtkWidget *terminal_of(GtkWidget *page)
{
    return (GtkWidget *)g_object_get_data(G_OBJECT(page), TERMINAL_OF);
}

GtkWidget *page_of(GtkWidget *terminal)
{
    return (GtkWidget *)g_object_get_data(G_OBJECT(terminal), PAGE_OF);
}

// THE BAR APPEARS WITH THE SECOND TAB AND LEAVES WITH IT. A window nobody has
// asked for a second terminal in looks exactly like the window M1.5 built, and
// a row of chrome saying "1" over a single terminal is a widget that has never
// once helped anybody.
void show_the_bar_only_when_there_is_a_choice()
{
    gtk_notebook_set_show_tabs(the_notebook,
                               gtk_notebook_get_n_pages(the_notebook) > 1);
}

// What the tab says. The file's own name and not its path -- a tab is an inch
// wide and the leading directories are the half nobody needs to tell two tabs
// apart. An empty file is the prompt, which is what `satl --repl` is.
std::string label_for(const std::string &file)
{
    if (file.empty())
        return "prompt";

    const size_t slash = file.rfind('/');
    return slash == std::string::npos ? file : file.substr(slash + 1);
}

void give_the_keyboard_to(GtkWidget *terminal)
{
    if (terminal)
        gtk_widget_grab_focus(terminal);
}

// A terminal that is finished with -- terminal.cpp's exit policy, or Ctrl-C in
// a held tab with nothing highlighted. Both arrive here.
//
// THE LAST PAGE TAKES THE WINDOW WITH IT, which is what keeps M1.5's behaviour
// exactly as it was: one tab, a clean exit, and the window closes. It is also
// the only place this file destroys anything, so "when does a satl-term window
// go away by itself" has one answer and it is these four lines.
void close_the_tab(GtkWidget *terminal)
{
    GtkWidget *page = page_of(terminal);
    const int number = page ? gtk_notebook_page_num(the_notebook, page) : -1;
    if (number < 0)
        return;

    gtk_notebook_remove_page(the_notebook, number);

    if (gtk_notebook_get_n_pages(the_notebook) == 0) {
        gtk_window_destroy(GTK_WINDOW(the_window));
        return;
    }

    show_the_bar_only_when_there_is_a_choice();
    give_the_keyboard_to(tabs_terminal_in_front());
}

// THE PAGE COMES FROM THE SIGNAL AND NOT FROM THE NOTEBOOK. "switch-page" is
// emitted while the change is being made, so asking which page is current here
// can still answer with the one being left.
void on_switch_page(GtkNotebook *, GtkWidget *page, guint, gpointer)
{
    give_the_keyboard_to(terminal_of(page));
}

} // namespace

GtkWidget *tabs_new(GtkWidget *window, bool hold_always)
{
    the_window = window;
    hold_clean_exit = hold_always;

    GtkWidget *notebook = gtk_notebook_new();
    the_notebook = GTK_NOTEBOOK(notebook);

    // Scrollable, so twenty tabs shrink the bar's contents rather than the
    // terminal under it; no border, because the window is a terminal and a
    // frame drawn around it is a line of pixels that means nothing.
    gtk_notebook_set_scrollable(the_notebook, TRUE);
    gtk_notebook_set_show_border(the_notebook, FALSE);
    gtk_notebook_set_show_tabs(the_notebook, FALSE);

    g_signal_connect(notebook, "switch-page",
                     G_CALLBACK(on_switch_page), nullptr);

    // THE KEYBOARD GOES ON BEFORE THE FIRST TERMINAL EXISTS, and the order is
    // load-bearing rather than tidy. It is terminal.cpp's old argument moved
    // here with the call: every failure path in that file ends by holding a
    // screen that says "press any key to close", and a promise like that made
    // before the controller exists is a window that answers no key at all.
    install_key_bindings(window, tabs_terminal_in_front);

    return notebook;
}

void tabs_open_tab(const std::string &file,
                   const std::vector<std::string> &args)
{
    GtkWidget *terminal = terminal_new(file, args, hold_clean_exit,
                                       close_the_tab);

    GtkWidget *page = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(page), terminal);

    g_object_set_data(G_OBJECT(terminal), PAGE_OF, page);
    g_object_set_data(G_OBJECT(page), TERMINAL_OF, terminal);

    // Ellipsised in the MIDDLE, because a long name's two informative halves
    // are its start and its extension: "advanced_examp....satl" is still a
    // satellite program and still tells you which one.
    //
    // BOTH WIDTHS, AND THE FIRST ONE IS NOT OPTIONAL. A label that may ellipsize
    // asks for no minimum width at all -- it can always shrink by dropping more
    // characters -- so a notebook is free to hand it the smallest box it will
    // take, and the tab that came out of max_width_chars alone said `p... t`
    // where it meant `prompt`. Measured on the first run of this file. The pair
    // is a floor and a ceiling: never narrower than ten characters, never wider
    // than twenty.
    GtkWidget *label = gtk_label_new(label_for(file).c_str());
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_width_chars(GTK_LABEL(label), 10);
    gtk_label_set_max_width_chars(GTK_LABEL(label), 20);

    const int number = gtk_notebook_append_page(the_notebook, page, label);
    gtk_notebook_set_current_page(the_notebook, number);

    show_the_bar_only_when_there_is_a_choice();
    give_the_keyboard_to(terminal);
}

void tabs_run(const std::string &file, const std::vector<std::string> &args)
{
    GtkWidget *terminal = tabs_terminal_in_front();

    // A LIVE INTERPRETER IS NOT THIS MENU'S TO END. The tab in front is busy,
    // so the program goes in a tab of its own -- the same answer keys.cpp gives
    // Ctrl-C, arrived at from the other direction: what is already running is
    // somebody's, and nothing in this window may take it away from them.
    if (!terminal || terminal_is_running(terminal)) {
        tabs_open_tab(file, args);
        return;
    }

    terminal_run(terminal, file, args);

    // The label is set through the label WIDGET rather than through
    // gtk_notebook_set_tab_label_text, which builds a plain new one and would
    // quietly drop the ellipsis the tab was made with.
    GtkWidget *label = gtk_notebook_get_tab_label(the_notebook,
                                                  page_of(terminal));
    if (GTK_IS_LABEL(label))
        gtk_label_set_text(GTK_LABEL(label), label_for(file).c_str());

    give_the_keyboard_to(terminal);
}

GtkWidget *tabs_terminal_in_front()
{
    if (!the_notebook)
        return nullptr;

    const int number = gtk_notebook_get_current_page(the_notebook);
    if (number < 0)
        return nullptr;

    GtkWidget *page = gtk_notebook_get_nth_page(the_notebook, number);
    return page ? terminal_of(page) : nullptr;
}

} // namespace satellite
