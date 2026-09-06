#pragma once

// The `satl` on the other side of the pty -- where it is, and what it is told.
//
// SEPARATE FROM terminal.cpp because they are two jobs: that file is a widget
// and an exit policy, and this one is a process being started. The split is by
// subject and not by line count, and it happened when File > Open gave the
// spawn a SECOND caller -- a finished tab being handed another program. Two
// callers is the moment "what satl is told" stops being a detail of building a
// terminal and becomes a thing with one right answer.
//
// THE ONE FILE IN THIS FOLDER THAT KNOWS THE INTERPRETER'S COMMAND LINE.
// `--repl`, `--run` and the SATL_TERM the child is handed are spelled here and
// nowhere else, so a flag that changes has one place to change and no second
// spelling left behind to disagree with it.
//
// It still knows nothing of what a satellite program is. It knows which file on
// disk is the interpreter and which words go after it.

#include <vte/vte.h>

#include <string>
#include <vector>

namespace satellite {

// Spawn the sibling `satl` into this terminal's pty.
//
// `file` empty means the prompt -- `satl --repl`; otherwise `satl --run file`
// followed by `args`. `done` is VTE's own spawn callback and `for_done` is the
// pointer it will be handed, so the caller's state reaches the answer without
// this file having to know what that state is.
//
// FALSE MEANS THE INTERPRETER COULD NOT BE FOUND BESIDE US, and nothing was
// spawned. It is said on stderr here, for the person who typed `satl-term` in a
// shell; putting it on the SCREEN is the caller's, because a message to the
// person looking at the window belongs with the policy that holds the window
// open to show it.
bool child_spawn(VteTerminal *terminal,
                 const std::string &file,
                 const std::vector<std::string> &args,
                 VteTerminalSpawnAsyncCallback done,
                 gpointer for_done);

} // namespace satellite
