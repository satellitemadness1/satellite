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
#include <sys/utsname.h>
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

// ---------------------------------------------------------------------------
// THE ANSWERS THEMSELVES, SO AN ALIAS CANNOT DRIFT FROM WHAT IT ALIASES.
//
// The author, 2026-09-18: *"let's use the longer choice for each one, can we
// have an alias for them though?"* -- so `arguments.machine.cores` is the name
// and `arguments.cores` is a second way to write it. An alias is a second word
// ROW and a second `.so`, because a code is one number and one library; what it
// must NOT be is a second copy of the answer. These functions are the one copy,
// and every library -- canonical or alias -- is a dozen lines pointing here.
//
// So an alias cannot answer a different number from the word it aliases, which
// is the only way an alias can really go wrong.
// ---------------------------------------------------------------------------

inline FactReply answer_memory_total()
{
    unsigned long long int said = 0;
    if (meminfo_bytes("MemTotal:", said) == false)
        return could_not_read("MemTotal in /proc/meminfo", machine_fact_not_read);
    return a_count(said);
}

inline FactReply answer_memory_free()
{
    unsigned long long int said = 0;
    if (meminfo_bytes("MemAvailable:", said) == false)
        return could_not_read("MemAvailable in /proc/meminfo", machine_fact_not_read);
    return a_count(said);
}

inline FactReply answer_memory_used()
{
    unsigned long long int whole = 0, spare = 0;
    if (meminfo_bytes("MemTotal:", whole) == false || meminfo_bytes("MemAvailable:", spare) == false)
        return could_not_read("MemTotal and MemAvailable in /proc/meminfo", machine_fact_not_read);
    // TOTAL LESS MemAvailable, NOT TOTAL LESS MemFree. MemFree leaves out the
    // page cache, which the kernel hands back the moment anything wants it -- so
    // `free` off MemFree reads as almost nothing on a machine that is perfectly
    // healthy, and `used` off it reads as almost everything.
    return a_count(spare > whole ? 0 : whole - spare);
}

inline FactReply answer_cores()
{
    unsigned long long int said = 0;
    if (cores_online(said) == false)
        return could_not_read("a count of online processors", machine_fact_not_read);
    return a_count(said);
}

inline FactReply answer_username()
{
    std::string said;
    if (username(said) == false)
        return could_not_read("a passwd entry for this user", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_directory()
{
    std::string said;
    if (working_directory(said) == false)
        return could_not_read("a working directory", machine_fact_not_read);
    return some_text(std::move(said));
}

// ---------------------------------------------------------------------------
// THE REST OF THE FREE ONES. Each is a uname field, a sysconf call, a getenv or
// a compiler macro -- nothing here opens anything that is not already open.
// ---------------------------------------------------------------------------

// `uname` gives five of the words in one call. A field the system left empty is
// a field this refuses, rather than answering "".
inline bool uname_field(int which, std::string &out)
{
    struct utsname said {};
    if (uname(&said) != 0) return false;
    const char *field = nullptr;
    switch (which) {
    case 0: field = said.nodename; break;   // system.hostname
    case 1: field = said.sysname;  break;   // system.kernel
    case 2: field = said.release;  break;   // system.kernel_version
    case 3: field = said.machine;  break;   // machine.architecture
    case 4: field = said.version;  break;   // system.name
    default: return false;
    }
    if (field == nullptr || field[0] == '\0') return false;
    out = field;
    return true;
}

// A named value out of /etc/os-release: NAME, ID, VERSION_ID. Quotes stripped,
// because the file writes them and nobody wants them in a string.
inline bool os_release(const char *key, std::string &out)
{
    std::ifstream file("/etc/os-release");
    if (!file.is_open()) return false;
    const std::string wanted = std::string(key) + "=";
    std::string line;
    while (std::getline(file, line)) {
        if (line.compare(0, wanted.size(), wanted) != 0) continue;
        std::string value = line.substr(wanted.size());
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (value.empty()) return false;
        out = value;
        return true;
    }
    return false;
}

// The first "model name" in /proc/cpuinfo -- what a person calls their CPU.
inline bool cpu_model(std::string &out)
{
    std::ifstream file("/proc/cpuinfo");
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        if (key != "model name") continue;
        std::string value = line.substr(colon + 1);
        std::size_t from = value.find_first_not_of(" \t");
        if (from == std::string::npos) return false;
        out = value.substr(from);
        return true;
    }
    return false;
}

// An environment variable, refused when it is unset or empty. A daemon started
// with a scrubbed environment has none of these, which is a real state and not a
// reason to answer "".
inline bool from_environment(const char *name, std::string &out)
{
    const char *said = std::getenv(name);
    if (said == nullptr || said[0] == '\0') return false;
    out = said;
    return true;
}

// WHICH WAY ROUND THIS MACHINE STORES A NUMBER, asked of the machine rather than
// of a macro, so it is true for whatever this was compiled on.
inline bool byte_order(std::string &out)
{
    const unsigned int one = 1u;
    unsigned char first = 0;
    __builtin_memcpy(&first, &one, 1);
    out = (first == 1) ? "little" : "big";
    return true;
}

inline FactReply answer_hostname()
{
    std::string said;
    if (uname_field(0, said) == false) return could_not_read("a hostname", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_kernel()
{
    std::string said;
    if (uname_field(1, said) == false) return could_not_read("a kernel name", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_kernel_version()
{
    std::string said;
    if (uname_field(2, said) == false) return could_not_read("a kernel version", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_architecture()
{
    std::string said;
    if (uname_field(3, said) == false) return could_not_read("an architecture", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_distribution()
{
    std::string said;
    if (os_release("NAME", said) == false) return could_not_read("NAME in /etc/os-release", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_distribution_id()
{
    std::string said;
    if (os_release("ID", said) == false) return could_not_read("ID in /etc/os-release", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_distribution_version()
{
    std::string said;
    if (os_release("VERSION_ID", said) == false)
        return could_not_read("VERSION_ID in /etc/os-release", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_cpu()
{
    std::string said;
    if (cpu_model(said) == false) return could_not_read("a model name in /proc/cpuinfo", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_byte_order()
{
    std::string said;
    byte_order(said);
    return some_text(std::move(said));
}

inline FactReply answer_page_size()
{
    const long said = sysconf(_SC_PAGESIZE);
    if (said <= 0) return could_not_read("a page size", machine_fact_not_read);
    return a_count(static_cast<unsigned long long int>(said));
}

// HOW WIDE A POINTER IS ON THIS BUILD. sizeof, not a guess: a 32-bit build on a
// 64-bit machine answers 32, which is the true answer for the interpreter that
// is actually running.
inline FactReply answer_pointer_bits()
{
    return a_count(static_cast<unsigned long long int>(sizeof(void *) * 8));
}

inline FactReply answer_process_id()
{
    return a_count(static_cast<unsigned long long int>(getpid()));
}

inline FactReply answer_process_parent()
{
    return a_count(static_cast<unsigned long long int>(getppid()));
}

inline FactReply answer_shell()
{
    std::string said;
    if (from_environment("SHELL", said) == false) return could_not_read("$SHELL", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_terminal()
{
    std::string said;
    if (from_environment("TERM", said) == false) return could_not_read("$TERM", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_language()
{
    std::string said;
    if (from_environment("LANG", said) == false) return could_not_read("$LANG", machine_fact_not_read);
    return some_text(std::move(said));
}

inline FactReply answer_home()
{
    std::string said;
    if (from_environment("HOME", said) == false) return could_not_read("$HOME", machine_fact_not_read);
    return some_text(std::move(said));
}

} // namespace machine_facts
} // namespace satellite004
