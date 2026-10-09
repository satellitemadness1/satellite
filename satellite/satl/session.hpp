#pragma once
// satellite/satl/session.hpp -- `satl --repl`: one statement a line, run out of
// the same 16-bit bytecode a file runs from (PLAN M0.6).
//
// THERE IS NO SECOND RUNNER HERE, and that is the whole shape of this file. A
// typed line is tokenised by the lexer that tokenises a .satl, judged by the
// checker that judges a capsule's body, and walked by run_statements -- the six
// shapes in program_walk.hpp, the same fast paths. What the session adds is the
// loop around them: read a line, refuse the spellings that have nowhere to live
// yet, run it, say what happened, and go again.
//
// WHAT IT KEEPS BETWEEN LINES is the number index, the function table, the
// working directory, the history -- and, since 2026-09-24, EVERY NAME A LINE
// DECLARES, with its value, until the session ends (the author: "the prompt has to
// remember what you type in"). program_walk.hpp's TypedLineMemory holds them, and a
// file one holds is saved on the way out. NOTHING TYPED IS KEPT AS TEXT AND RUN
// AGAIN, which is 003's ERROR #26: the values are kept, not the lines that made them.
//
// CTRL-C IS TWO KEYS AND WHICH ONE IS DECIDED BY WHAT IS RUNNING. While a line is
// being typed the terminal is raw with ISIG off, so it arrives as the byte 0x03
// and the line reader abandons the line. While a line RUNS the terminal is
// cooked, so it arrives as SIGINT: the handler raises the flag a directory
// listing reads between entries (stop_flag.hpp) and the line answers
// `interrupted`. A SECOND press does not wait to be noticed -- it restores the
// terminal and exits 130.
//
// FROM A PIPE there is no banner and no prompt text (D0.6.2), because nobody is
// there to read them; the session's status is the FIRST failing line's code, so a
// script that fails says so, and 0 when every line was understood.

#include "../arguments/arguments.hpp"
#include "../bytecode/function_table.hpp"
#include "../threads/startup_threads.hpp"

namespace satellite004 {

struct MachineState;

// Runs the prompt until `exit`, `quit`, Ctrl-D or the end of the input. Answers
// success, or the first line's code that stopped.
signed long long int run_session(const Arguments &arguments, const FunctionTable &functions,
                                 StartupThreads &threads, MachineState &state);

} // namespace satellite004
