// satl -- what to do when nobody started us from a console.
//
// THE PROBLEM THIS SOLVES IS INVISIBLE FROM A TERMINAL. Double-click satl in a
// file manager, or open a .satl file with it, and every line it prints goes
// somewhere nobody is looking -- /dev/null, or the session journal. The program
// runs correctly and the person sees nothing at all, which is indistinguishable
// from a program that did not start. DESIGN sec 1.1 is the rule this breaks:
// do absolutely everything for the user, and never do it behind their back.
//
// So satl hands itself over to satl-term, which has a screen. The window then
// spawns the satl sitting beside it (src/programs/terminal.cpp), and that copy
// runs with a pty for a console and prints where it can be read.
//
// THERE IS NO LOOP AND IT IS NOT LUCK. satl-term spawns its child on a pty, so
// the child HAS a controlling terminal and answers this question the other way
// on the first line it asks it. The recursion terminates because the condition
// is a fact about the process, not a flag we forgot to clear.

#ifndef SATELLITE_PROGRAMS_WINDOW_HANDOVER_HPP
#define SATELLITE_PROGRAMS_WINDOW_HANDOVER_HPP

namespace satellite {

// Replace this process with satl-term, if and only if this process was started
// with nothing to print to and there is a display to open a window on.
//
// RETURNS ONLY WHEN satl SHOULD CARRY ON HERE. On a successful handover it
// never returns -- execv() has replaced the image. Every other path is a
// deliberate refusal to hand over, and window_handover.cpp names all six.
//
// argv is passed through untouched, so `satl thing.satl` becomes
// `satl-term --hold thing.satl` and the file reaches the same interpreter by a
// longer road.
void hand_over_to_the_window(char **argv);

} // namespace satellite

#endif
