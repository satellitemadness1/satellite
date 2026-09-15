// What the keyboard means in a satl-term window. See keys.hpp for the split.
//
// THE KEY IS ONLY EVER TAKEN WHEN THERE IS NOBODY TO GIVE IT TO, and that one
// sentence is the whole safety argument for handling Ctrl-C in a window at all.
//
// A capture phase controller runs BEFORE the focused widget -- terminal.cpp
// already relies on that, and says so -- which means a handler here that
// answers GDK_EVENT_STOP consumes the keystroke and VTE never writes anything
// to the pty master. That is a loaded gun pointed at M22: DESIGN §10.2 gives
// Ctrl-C two meanings, and the prompt's meaning IS the byte 0x03 arriving
// through the pty, because raw mode turns ISIG off so the key never reaches a
// signal handler. A window that swallowed Ctrl-C would make the prompt it was
// built to host uncancellable, and the failure would appear one milestone
// later in a file that does not mention windows.
//
// So the rule below is not "handle Ctrl-C". It is: while there is a child on
// the other side of the pty, the key is the CHILD'S, and this file's only job
// is to get out of its way. Every branch that consumes the key first
// establishes that there is no child left to consume it.
//
// THE FILE MENU TAKES NO KEY AT ALL, and that is this paragraph's rule applied
// to the newest way of asking for something. Every item in it is reachable by
// mouse only -- there is no Ctrl-O, no Ctrl-S, no Ctrl-N -- so nothing the menu
// can do is spelled with a chord some program on the other side of a pty might
// have wanted. It is the one design here that cannot go wrong later, because
// there is nothing to go wrong.

#include "programs/satl-term/keys.hpp"
#include "programs/satl-term/terminal.hpp"

#include <gdk/gdkkeysyms.h>
#include <vte/vte.h>

namespace satellite {
namespace {

// The one thing a keystroke has to know: how to find the terminal it was meant
// for.
//
// A STATIC IS NOT A LIMIT OF ONE WINDOW HERE, which is the objection it would
// otherwise deserve. window.cpp registers the application G_APPLICATION_NON_UNIQUE
// and argues why: a second `satl-term` is a second PROCESS, not a second window
// in this one -- which is also what File > New window does, deliberately. The
// day M24's library opens two windows in one process this becomes per-window
// state, and that is a change to this file rather than to its callers.
//
// WHAT IS NO LONGER HERE is the state of the terminal itself, and the window
// pointer with it. A window now holds as many terminals as somebody has opened
// tabs, so "is it running" and "is it being held" moved to the terminal that
// can answer them for itself; and closing goes out through terminal_finish
// rather than through gtk_window_destroy, so this file no longer needs to know
// which window it is in to end what is in front of it.
GtkWidget *(*in_front)() = nullptr;

// Control held, and nothing stranger than Shift with it.
//
// SHIFT IS ALLOWED THROUGH rather than matched exactly, because Ctrl-Shift-C is
// the same intention typed by somebody whose caps lock is on or whose fingers
// learned another terminal, and answering that press with silence is worse than
// answering it with what they meant. Alt and Super are NOT allowed: those are
// window manager territory and a chord this file does not recognise must reach
// whoever does.
//
// DELIBERATELY ABSENT: Ctrl-Shift-C as a SEPARATE binding meaning "copy even
// while a program is running". Every terminal on this machine has it and this
// one does not, because the author asked for Ctrl-C to stop a running
// interpreter and a second spelling that does the opposite of the first is a
// decision rather than an omission. Said here so that its absence is a choice
// somebody made and not a gap somebody missed.
bool is_control_chord(GdkModifierType state)
{
    if (!(state & GDK_CONTROL_MASK))
        return false;

    return !(state & (GDK_ALT_MASK | GDK_SUPER_MASK));
}

// A key that is only ever pressed on the way to another key.
//
// THIS IS WHAT MAKES CTRL-C REACHABLE IN A HELD WINDOW AT ALL, and it is not a
// nicety: "any key closes this window" reads a bare Control_L press -- which is
// the FIRST half of every Ctrl-C -- and would destroy the window a fraction
// before the C arrived. The copy binding below could never once have fired. A
// modifier is not an answer to "press any key"; it is somebody starting to
// type one.
bool is_only_a_modifier(guint keyval)
{
    switch (keyval) {
    case GDK_KEY_Control_L: case GDK_KEY_Control_R:
    case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
    case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
    case GDK_KEY_Super_L:   case GDK_KEY_Super_R:
    case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
    case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_Caps_Lock: case GDK_KEY_Num_Lock:
        return true;
    default:
        return false;
    }
}

// Ctrl-C, and it means three different things because the terminal in front is
// in three different states. In the order they are asked:
//
// 1. A RUNNING INTERPRETER IS NOT THIS WINDOW'S TO STOP, so the key is passed
//    through untouched and the kernel does the work. VTE writes 0x03 to the
//    pty master, the line discipline turns it into SIGINT for the foreground
//    process group, and satl's own handler -- system_facts/interrupt.hpp, M11 --
//    sets a flag the walk reads at its next statement boundary. That road is
//    why an interrupted program can still say WHERE it stopped: S0730 puts a
//    caret under the statement that did not run, the console drains so
//    everything the program said is above it, and the exit is 130.
//
//    KILLING THE CHILD HERE WOULD BE THE OBVIOUS VERSION AND IT IS WRONG. It
//    produces the same 130 with none of the meaning behind it -- no boundary,
//    no caret, no drain -- and opening.cpp promises the user that 130 means
//    "Ctrl-C stopped a running program at a statement boundary". The number
//    would be right and the sentence would be a lie. DESIGN §10.2 rules the
//    third meaning out in one line: neither half of Ctrl-C "is the same as
//    taking the session". A second press escalates, and that escalation is the
//    handler's, inside the process that knows whether the first press landed.
//
// 2. NOTHING RUNNING AND TEXT HIGHLIGHTED -- copy it. There is no child to
//    take the key, so taking it costs nothing, and a terminal holding the only
//    copy of an error report that cannot be copied out of is a terminal that
//    wastes the reason it was held open for.
//
// 3. NOTHING RUNNING AND NOTHING HIGHLIGHTED -- close it. Ctrl-C keeps one
//    promise across all three: it stops what is in front of you. With no run to
//    stop and no selection to lift, what is in front of you is this terminal --
//    which is the window when there is one tab, and the tab when there are more.
//    It goes out through terminal_finish so that a tab closes by the same road
//    whether its child ended or somebody dismissed it.
gboolean control_c(GtkWidget *terminal)
{
    if (terminal_is_running(terminal))
        return GDK_EVENT_PROPAGATE;

    if (vte_terminal_get_has_selection(VTE_TERMINAL(terminal))) {
        // _format AND NOT vte_terminal_copy_clipboard(), which still exists in
        // this header and is marked deprecated in it. VTE_FORMAT_TEXT is what
        // the selection looks like to every other program; VTE_FORMAT_HTML
        // would paste this window's colours into whatever received it.
        vte_terminal_copy_clipboard_format(VTE_TERMINAL(terminal), VTE_FORMAT_TEXT);
        return GDK_EVENT_STOP;
    }

    terminal_finish(terminal);
    return GDK_EVENT_STOP;
}

// Ctrl-V, and it is ALWAYS taken -- the one binding here that consumes a key
// with a live child on the other side of the pty.
//
// THAT IS THE POINT RATHER THAN AN EXCEPTION TO IT. Paste has exactly one
// meaning and a running program is the case that wants it: somebody with a
// path, a number or a line of text on their clipboard is answering
// `satellite.console.input`, and telling them to type it out by hand because
// the interpreter is busy is the window getting in the way of the person.
// DESIGN §1.1 -- do everything for the user.
//
// The byte this replaces is 0x16, which no part of this language asks for, so
// unlike Ctrl-C there is nothing downstream being denied its meaning.
// vte_terminal_paste_clipboard writes the clipboard's text to the child
// exactly as if it had been typed, which is what makes it work at a cooked
// prompt and at M22's raw one without either of them learning about clipboards.
gboolean control_v(GtkWidget *terminal)
{
    vte_terminal_paste_clipboard(VTE_TERMINAL(terminal));
    return GDK_EVENT_STOP;
}

gboolean on_key_pressed(GtkEventControllerKey *, guint keyval, guint,
                        GdkModifierType state, gpointer)
{
    // WHICH TERMINAL, ASKED FIRST AND ASKED EVERY TIME. The tab in front is
    // whatever the notebook says it is at this instant, and a key that arrives
    // when there is none -- the last tab closing, the window going away -- is
    // nobody's to answer.
    GtkWidget *terminal = in_front ? in_front() : nullptr;
    if (!terminal)
        return GDK_EVENT_PROPAGATE;

    if (is_control_chord(state)) {
        if (keyval == GDK_KEY_c || keyval == GDK_KEY_C)
            return control_c(terminal);
        if (keyval == GDK_KEY_v || keyval == GDK_KEY_V)
            return control_v(terminal);
    }

    // The held terminal's "press any key", asked LAST so that the two bindings
    // above keep their meaning in the state where the screen has already said
    // any key will close it.
    if (terminal_is_held(terminal) && !is_only_a_modifier(keyval)) {
        terminal_finish(terminal);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

} // namespace

void install_key_bindings(GtkWidget *window, GtkWidget *(*terminal_in_front)())
{
    in_front = terminal_in_front;

    // ON THE WINDOW AND IN THE CAPTURE PHASE. The terminal has keyboard focus
    // whenever there is anything to type at, and it keeps it after its child is
    // gone; a controller on the terminal in the bubble phase would be asked
    // only about keys VTE had already decided it did not want -- which is every
    // key except the ones this file exists to answer.
    //
    // ON THE WINDOW ALSO MEANS ONCE, WHICH IS WHY TABS COST THIS FILE NOTHING:
    // a controller per terminal would be a second controller for the same
    // press the moment there were two tabs, which is the thing the header
    // refuses.
    GtkEventController *keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
    g_signal_connect(keys, "key-pressed", G_CALLBACK(on_key_pressed), nullptr);
    gtk_widget_add_controller(window, keys);
}

} // namespace satellite
