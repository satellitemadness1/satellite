#pragma once
// satellite/machine/satellite_of_its_own.hpp -- A NEW satl IN A CONSOLE OF ITS OWN: the console menu's
// New window and Open… (satellite_variable_window/console_menu.cpp), and since 2026-10-20 a program
// started at the prompt in satl's own console (satl/prompt_run.cpp), which hands it to the desk where the
// menu's runs (window_calls.hpp, start_a_satellite_on_the_desk).
//
// (the author, 2026-10-20) "when you type in "interpret program.satl" and it spawns an interpreter ... I
// have to be able to type back into the satellite console that created the program, I need to be free to
// type into the original satellite console that created the other console, and to navigate into a
// different directory, and to start another program ... I could run.. 24 of these programs or something"
// -- and then: "Just create another satellite, for each program". The menu already made one. This is the
// menu's own start of 2026-09-22, moved here so that the prompt starts a program the same way and there is
// one start to keep true, not two.
//
// WHAT THE NEW satl IS GIVEN, the menu's rules, unchanged:
//   - THIS satl, by /proc/self/exe and not PATH, so it is this build with these libraries beside it, and
//     --console first, so it opens a console of its own whatever its output is pointed at;
//   - stdin /dev/null, and stdout and stderr where THIS satl was started -- the C stderr, which
//     keep_the_c_streams_off_the_pty (console_launch.cpp) left there -- never this console's pty, or its
//     first words would land in this window; and no other file this satl has open;
//   - setsid(), so this console's Ctrl-C and its hangup are never the new one's, and closing this console
//     leaves it running; GLib's five signals -- SIGPIPE, SIGINT, SIGTERM, SIGHUP, SIGCHLD -- back to their
//     defaults, and none blocked;
//   - the stack satl was given (stack_share.hpp), and the folder this satl is in.
//
// NEVER WAITED FOR, AND NEVER LEFT A ZOMBIE. A middle process starts it and ends at once, and only the
// middle one is waited for -- the double fork GLib's g_spawn_async made for the menu until this day -- so
// the new satl is the machine's reaper's (init, or the desktop's systemd) from the moment the middle one
// ends, and never this satl's child.
//
// FROM ANY THREAD: it calls no GTK, and between each fork and the exec only what is safe there -- _Fork
// for the second fork, setsid, signal, sigprocmask, dup2, close_range, setrlimit, execv, write, _exit.
//
// Answers true once the new satl is running -- its exec has succeeded -- and false with `why` when it
// could not be started, or the middle one was ended before it could fork. AN EXEC THAT FAILED IS KNOWN
// HERE, through a pipe the exec closes, and not left to a report nobody would see: a console opened before
// an install still holds the path of the satl it was, which /proc/self/exe then names "... (deleted)".

#include <string>
#include <vector>

namespace satellite004 {

// `satl --console <words...>`, as above. `called` is the new satl's argv[0], which a program reads as
// satellite.arguments.argument1: a satl typed in a shell passes its own, so the program sees `satl` as it
// would have in that satl (structured-library.cpp); empty is the path /proc/self/exe names, the menu's.
bool start_a_satellite_of_its_own(const std::vector<std::string> &words, std::string &why,
                                  const std::string &called = std::string());

} // namespace satellite004
