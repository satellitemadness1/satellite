// satellite/machine/own_environment.cpp -- the header says what this is for.

#include "own_environment.hpp"

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <utility>

extern char **environ;

namespace satellite004 {
namespace {

// EVERY NAME satl HAS SET FOR ITSELF, with the machine's value from before -- or none, when the
// machine had not set it.
struct Kept {
    std::mutex lock;
    std::vector<std::pair<std::string, std::optional<std::string>>> names;
};

Kept &kept()
{
    static Kept one;
    return one;
}

} // namespace

void keep_the_machines(const char *name)
{
    const std::lock_guard<std::mutex> hold(kept().lock);
    for (const auto &each : kept().names)
        if (each.first == name)
            return;   // the first time only: after that the value is satl's own
    const char *now = std::getenv(name);
    kept().names.emplace_back(name, now != nullptr ? std::optional<std::string>(now) : std::nullopt);
}

char *const *environment_for_a_program(std::vector<std::string> &storage, std::vector<char *> &pointers)
{
    const std::lock_guard<std::mutex> hold(kept().lock);
    if (kept().names.empty())
        return environ;
    storage.clear();
    pointers.clear();
    for (char **entry = environ; entry != nullptr && *entry != nullptr; ++entry) {
        const char *equals = std::strchr(*entry, '=');
        const std::size_t length = equals != nullptr ? static_cast<std::size_t>(equals - *entry) : std::strlen(*entry);
        bool satls = false;
        for (const auto &each : kept().names)
            satls = satls || (each.first.size() == length && std::strncmp(*entry, each.first.c_str(), length) == 0);
        if (!satls)
            storage.emplace_back(*entry);
    }
    for (const auto &each : kept().names)
        if (each.second)
            storage.push_back(each.first + "=" + *each.second);
    pointers.reserve(storage.size() + 1);
    for (std::string &each : storage)
        pointers.push_back(each.data());
    pointers.push_back(nullptr);
    return pointers.data();
}

} // namespace satellite004
