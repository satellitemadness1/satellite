// The consoles before this one. See earlier_consoles.hpp.

#include "earlier_consoles.hpp"

#include <utility>

namespace satellite004::prompt {

void EarlierConsoles::add(Read read)
{
    const std::lock_guard<std::mutex> held(lock_);
    consoles_.push_back(Console{std::move(read), nullptr, false});
}

std::size_t EarlierConsoles::size() const
{
    const std::lock_guard<std::mutex> held(lock_);
    return consoles_.size();
}

const ConsoleLines *EarlierConsoles::lines(std::size_t back)
{
    const std::lock_guard<std::mutex> held(lock_);
    if (back >= consoles_.size())
        return nullptr;
    Console &console = consoles_[back];
    // READ ONCE, WHATEVER IT ANSWERED: a console passed over is not tried again at the next Left, so a
    // file that went stays gone for the run, and one read stays as it was read.
    if (!console.tried) {
        console.tried = true;
        auto lines = std::make_unique<ConsoleLines>();
        if (console.read && console.read(*lines))
            console.lines = std::move(lines);
        console.read = nullptr;   // what it held -- the file's place -- is needed no more
    }
    return console.lines.get();
}

} // namespace satellite004::prompt
