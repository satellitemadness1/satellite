#pragma once
// satellite-numbers/machine_facts.hpp -- THE READERS BEHIND arguments.*
// SATELLITE_ARGUMENTS B7-B11.
//
// Shared by every numbered library that answers a fact about the machine,
// because each library is compiled from exactly one .cpp -- so anything two of
// them need has to be inline in a header. Same rule as critical_report.hpp.
//
// NOTHING HERE IS CACHED, AND THAT IS THE DESIGN. A machine fact is read every
// time it is asked for. `arguments.memory.free` that answers what was free a
// minute ago is a wrong answer wearing a right answer's face, and
// SATELLITE_ARGUMENTS says the same of the feature register: "composing them
// would mean caching a fact that changes".
//
// A FAILURE IS AN ANSWER. Every reader says whether it could read, and the
// library turns that into a refusal naming the file it could not read -- rather
// than answering 0, which is a number a program would happily divide by.

#include "number_row.hpp"
#include "../satellite/machine/machine_codes.hpp"

#include <pwd.h>
#include <unistd.h>

#include <fstream>
#include <string>

namespace satellite004 {
namespace machine_facts {

// A named kilobyte count out of /proc/meminfo: "MemTotal:", "MemFree:",
// "MemAvailable:", "SwapTotal:", "SwapFree:". The file states kB and this
// answers BYTES, because a program asking for memory means bytes unless it says
// otherwise -- and the unit words (B4-B6) are not built yet, so there is nowhere
// to say otherwise.
inline bool meminfo_bytes(const char *wanted, unsigned long long int &out)
{
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return false;
    std::string name;
    while (file >> name) {
        if (name == wanted) {
            unsigned long long int kilobytes = 0;
            if (!(file >> kilobytes)) return false;
            out = kilobytes * 1024ULL;
            return true;
        }
        std::string rest;
        std::getline(file, rest);
    }
    return false;
}

// The login name of the user this interpreter is running as.
//
// getpwuid AND NOT $USER, deliberately: the environment's copy is whatever was
// exported and the passwd entry is who the process really is, and under `sudo`
// they disagree. SATELLITE_ERROR's environment allowlist keeps BOTH for exactly
// that reason -- but a program asking `arguments.username` wants the true one.
inline bool username(std::string &out)
{
    const struct passwd *entry = getpwuid(geteuid());
    if (entry == nullptr || entry->pw_name == nullptr) return false;
    out = entry->pw_name;
    return true;
}

// The working directory this interpreter is in now.
inline bool working_directory(std::string &out)
{
    std::string room(1024, '\0');
    while (room.size() <= (1u << 20)) {
        if (getcwd(&room[0], room.size()) != nullptr) {
            out = room.c_str();      // getcwd wrote a NUL; trim to it
            return true;
        }
        room.assign(room.size() * 2, '\0');   // the path is longer than the room
    }
    return false;
}

// THE FACTS THAT ARE A `sysconf` CALL. `_SC_NPROCESSORS_ONLN` is the count of
// CPUs the kernel will schedule on NOW, which is the number a person means by
// "cores" -- not the count the hardware has, some of which may be offline.
inline bool cores_online(unsigned long long int &out)
{
    const long said = sysconf(_SC_NPROCESSORS_ONLN);
    if (said <= 0) return false;
    out = static_cast<unsigned long long int>(said);
    return true;
}

// A reply that failed, naming what could not be read. One spelling, so every
// library refuses in the same words.
inline FactReply could_not_read(const char *what, signed long long int code)
{
    FactReply reply;
    reply.code = code;
    reply.reason = std::string("this machine does not state ") + what;
    return reply;
}

inline FactReply a_count(unsigned long long int value)
{
    FactReply reply;
    reply.code = success;
    reply.count = value;
    return reply;
}

inline FactReply some_text(std::string value)
{
    FactReply reply;
    reply.code = success;
    reply.is_text = true;
    reply.text = std::move(value);
    return reply;
}

} // namespace machine_facts
} // namespace satellite004
