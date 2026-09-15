#pragma once
// `arguments` -- everything the program is told when it starts: the command
// line and the machine it is running on.
//
// Each entry holds ONE kind of value:
//
//     text   a string         arguments.system.hostname   "siege3"
//     count  a whole number   arguments.machine.threads   24
//     flag   true or false    arguments.debug_mode        true
//     size   an amount of memory or disk, as a long double plus its unit:
//            "bytes", "kilobytes", "megabytes", "gigabytes" or "terabytes"
//
// WHY A long double FOR SIZES: it is the sharpest standard C++ float (18 exact
// digits here, against double's 15), it holds every 64-bit byte count exactly,
// and dividing by 1024 is exact too. Measured 2026-09-14: the largest 64-bit
// count went to terabytes and back unchanged, where a double was off by one.

#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

enum class ArgumentKind { text, count, flag, size };

struct Argument {
    std::string name;
    ArgumentKind kind = ArgumentKind::text;
    std::string text;
    unsigned long long int count = 0;
    bool flag = false;
    long double size = 0.0L;
    std::string unit;              // only for a size
};

class Arguments {
public:
    // Fill every entry from the command line and the machine. Answers a
    // machine code; the program can still run if a fact could not be read.
    signed long long int gather(int argc, char **argv);

    void add_text(const std::string &name, const std::string &value);
    void add_count(const std::string &name, unsigned long long int value);
    void add_flag(const std::string &name, bool value);
    void add_bytes(const std::string &name, unsigned long long int bytes);

    const Argument *find(const std::string &name) const;
    bool flag(const std::string &name) const;
    std::string text(const std::string &name) const;
    const std::vector<Argument> &all() const { return entries_; }

private:
    Argument &add(const std::string &name, ArgumentKind kind);

    std::vector<Argument> entries_;
    std::unordered_map<std::string, size_t> where_;
};

// "24", "true", "62.5 gigabytes", or the text itself.
std::string describe(const Argument &argument);

} // namespace satellite004
