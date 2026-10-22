#pragma once
// satellite/machine/console_store.hpp -- where a line entered at a console goes (KEY_PLAN.md KEY-1).
//
// THE STORE IS satl/console_files.hpp's ConsoleFiles: the day's folder under ~/.satl/console and the
// console's file in it (KEY-2), and the console's lines (KEY-3). run_satl makes it once, when satl's
// stdin and stdout are both terminals (KEY_PLAN.md rule 1: a console is what a person types into), and
// sets both pointers below. They are null in every other run of satl and in every harness.
//
// WHY A HOOK AND NOT A CALL ON THE STORE. The prompt's LineReader is the first to see an Enter, and it
// is linked into build/prompt_cases and build/prompt_reader too, which have nothing of satl/ in them
// (065-tests.mk's PROMPT_SOURCES). So the reader calls a function pointer, as input() calls
// input_source(), and needs nothing from satl/ to link: a harness leaves it null, and nothing is
// called. It is the write hook KEY-3 names, called with every line entered: the first call makes the
// day's folder and claims the console's file in it, and every call, the first among them, writes its
// line into that file.
//
// stop_flag.hpp's shape and stop_flag.hpp's reason: two pointers, each set once, instead of a parameter
// threaded through the reader, the session and every caller of input(). They hold nothing a program
// wrote -- there are still no globals in the language.

#include <string>

namespace satellite004 {

class ConsoleFiles;   // satl/console_files.hpp, which the pointer below needs no more of

namespace prompt {
class ConsoleLines;   // prompt/console_lines.hpp, which the hook below needs no more of
}

// THE STORE, while satl has a console; null when it has none. Never deleted (run_satl).
inline ConsoleFiles *&console_files()
{
    static ConsoleFiles *store = nullptr;
    return store;
}

// A LINE WAS ENTERED AT A CONSOLE: the reader's answer at a terminal, typed or pasted -- the prompt's,
// and a program's input() at the prompt or in a file run (KEY_PLAN.md KEY-4). Never a piped line,
// never a line stopped with Ctrl-C, never the end of the input. It may be called from a program's
// threads, which input() runs on.
//
// `kept_in` IS WHERE THE LINE IS ALREADY KEPT, so that it is kept once whichever way it came
// (KEY_PLAN.md KEY-3): the console's lines the reader was handed, and has just added it to -- in satl
// the store's own (session.cpp, structured-library.cpp) -- or null when nothing has kept it yet: a
// reader handed no lines. The store adds the line to its own lines unless they are `kept_in`, then
// writes it.
using LineEntered = void (*)(const std::string &line, const prompt::ConsoleLines *kept_in);

inline LineEntered &line_entered()
{
    static LineEntered entered = nullptr;
    return entered;
}

} // namespace satellite004
