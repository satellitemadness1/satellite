#pragma once
// satellite/prompt/earlier_consoles.hpp -- THE CONSOLES BEFORE THIS ONE (KEY_PLAN.md KEY-5), newest first:
// what Left reaches past the start of a line, and Right comes back through (KEY-6). The author's fourth
// milestone of 2026-10-20, "we build the code that loads the month-and-day named folders into
// satellite.variable.string objects", and his fifth, "that can be cycled through with left and right
// arrows ... saving the most recent 32mb worth of files".
//
// ONE ENTRY A CONSOLE, AND ITS LINES READ ONLY WHEN LEFT FIRST REACHES IT (rule 6): each is added with how
// to read it, and read the first time lines() is asked for it. A console read once stays read for the
// run, and one that cannot be read -- its file gone since the walk, or another file in its place -- is
// passed over, every time it is asked for.
//
// IT KEEPS NO FILE, as the console's lines keep none (console_lines.hpp): the console's store fills it
// from its walk of the day folders, with a read of its own for each console (satl/console_files.cpp), and
// build/prompt_cases fills it by hand. So prompt/ links nothing of satl/ for it.
//
// ASKED UNDER A LOCK: two readers may reach the same console -- the session's and a program's input()
// through it, and KEY-9's readers on threads of their own -- and a console is read once.

#include "console_lines.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace satellite004::prompt {

class EarlierConsoles {
public:
    // HOW A CONSOLE'S LINES ARE READ: each line into `into` (ConsoleLines::add, so KEY-3's filter has
    // it), and false when the console cannot be read, which passes it over.
    using Read = std::function<bool(ConsoleLines &into)>;

    EarlierConsoles() = default;
    EarlierConsoles(const EarlierConsoles &) = delete;
    EarlierConsoles &operator=(const EarlierConsoles &) = delete;

    // THE NEXT CONSOLE BACK, older than every one added before it. Nothing is read yet.
    void add(Read read);

    std::size_t size() const;

    // THE LINES OF THE CONSOLE `back` BEFORE THIS ONE -- 0 the console just before, size() - 1 the oldest
    // Left reaches -- read the first time they are asked for. Null when it was passed over, or `back`
    // is past the oldest. The lines stay where they are for the run, so the pointer does too.
    const ConsoleLines *lines(std::size_t back);

private:
    struct Console {
        Read read;
        std::unique_ptr<ConsoleLines> lines;   // null until read, and after a read that failed
        bool tried = false;
    };

    mutable std::mutex lock_;
    std::vector<Console> consoles_;   // newest first
};

} // namespace satellite004::prompt
