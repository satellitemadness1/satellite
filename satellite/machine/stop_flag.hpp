#pragma once
// satellite/machine/stop_flag.hpp -- the one flag a library may look at while it
// runs: the session's Ctrl-C (PLAN M0.6).
//
// WHY A POINTER HERE AND NOT AN ARGUMENT EVERYWHERE. A directory listing of a
// million names is the first thing in this tree that can take long enough to be
// worth stopping, and the flag has to reach the library that reads the entries --
// through call_word, through the expression, through run_statements. Threading a
// parameter through all of those would change five signatures so that one library
// can read one flag; this is one pointer, set once when a session starts and null
// in every other run of satl, which is exactly how `satl file.satl` behaves today.
//
// volatile sig_atomic_t IS THE TYPE A SIGNAL HANDLER MAY WRITE. The handler sets
// it and nothing else; a library reads it between entries and answers
// `interrupted`. Nothing here allocates, locks or decides anything.
//
// THIS IS NOT A LANGUAGE GLOBAL. "There are no globals" (the author, 2026-09-16)
// is about a satellite program's names -- a capsule cannot see its caller's
// variables, and that is enforced by construction in value.hpp. This is the
// interpreter's own wiring for a key press, and it holds nothing a program wrote.

#include <csignal>

namespace satellite004 {

// The session's flag while a session is running, null otherwise.
inline const volatile sig_atomic_t *&stop_flag()
{
    static const volatile sig_atomic_t *flag = nullptr;
    return flag;
}

} // namespace satellite004
