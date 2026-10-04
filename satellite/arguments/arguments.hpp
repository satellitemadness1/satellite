#pragma once
// `arguments` -- everything the program is told when it starts: the author's
// satellite_config.hpp, the command line, and the machine it is running on.
//
// Each entry holds ONE kind of value:
//
//     text   a string         arguments.system.hostname   "siege3"
//     count  a whole number   arguments.machine.threads   24
//     number a satellite_number, as satellite_config.hpp writes one -- any number
//            of digits (the author, 2026-09-16: "just make everything a
//            satellite number")    arguments.infinity   128
//     flag   true or false    arguments.debug_mode        true
//     size   an amount of memory or disk, as a long double plus its unit:
//            "bytes", "kilobytes", "megabytes", "gigabytes" or "terabytes"
//     list   some words, in order     arguments.cpu.features   {"sse", "avx", "avx2" ...}
//            (2026-09-23, the author: "all that in a single list"; cpu_facts.hpp)
//
// WHY A long double FOR SIZES: it is the sharpest standard C++ float (18 exact
// digits here, against double's 15), it holds every 64-bit byte count exactly,
// and dividing by 1024 is exact too. Measured 2026-09-14: the largest 64-bit
// count went to terabytes and back unchanged, where a double was off by one.

#include "command_line.hpp"
#include "../satellite_variable_number/satellite_number.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

enum class ArgumentKind { text, count, number, flag, size, list };

struct Argument {
    std::string name;
    ArgumentKind kind = ArgumentKind::text;
    std::string text;
    unsigned long long int count = 0;
    satellite_number number;
    bool flag = false;
    long double size = 0.0L;
    std::string unit;              // only for a size
    std::vector<std::string> items;  // only for a list
};

class Arguments {
public:
    // Every row of the author's return_arguments_vector() (satellite_config.hpp).
    // Answers config_value_not_understood for a row that cannot mean what its
    // name asks. Called first, because --version needs it.
    signed long long int gather_config();

    // Fill every entry from the command line and the machine. Answers a
    // machine code; the program can still run if a fact could not be read.
    signed long long int gather(const CommandLine &command_line);

    void add_text(const std::string &name, const std::string &value);
    void add_count(const std::string &name, unsigned long long int value);
    void add_number(const std::string &name, satellite_number value);
    void add_flag(const std::string &name, bool value);
    void add_bytes(const std::string &name, unsigned long long int bytes);
    void add_list(const std::string &name, std::vector<std::string> items);

    const Argument *find(const std::string &name) const;
    bool flag(const std::string &name) const;
    // The row's satellite_number, or 0 when there is no number row of that name.
    const satellite_number &number(const std::string &name) const;
    std::string text(const std::string &name) const;
    const std::vector<Argument> &all() const { return entries_; }

    // EVERY ROW, THE COMMAND LINE FIRST (MS-1, the author 2026-10-03: "arguments returns a
    // note about every argument that exists, starting from argv at argument1, and the
    // entire arguments special variable"): argument1 onwards, then length, then every other
    // row in the order it was added. Wherever all of them are shown -- the variable a
    // program displays, satl --debug -- this is the order.
    std::vector<const Argument *> every_row_arguments_first() const;

private:
    Argument &add(const std::string &name, ArgumentKind kind);

    std::vector<Argument> entries_;
    std::unordered_map<std::string, size_t> where_;
    size_t facts_start_ = 0;  // entries_ before this index came from the config
    std::string overwritten_; // the first config row gather() found a second value for
};

// "24", "true", "62.5 gigabytes", "sse, avx, avx2", or the text itself.
std::string describe(const Argument &argument);

// Whether satl fills this name in itself -- from the command line or the machine
// -- on SOME run, so satellite_config.hpp may never hold it. arguments.argument3
// is satl's whether or not this run was given three words -- under every spelling
// of it, and under the old argument_3 too, so no program writes one either.
bool filled_in_by_satl(const std::string &name);

// THE ROW A SPELLING OF A COMMAND-LINE WORD NAMES (MS-1, the author 2026-10-03:
// "arguments.arg1 || args1 || arguments1 and no _underscore?? ... you could alias them
// as the same thing"): arg3, args3, argument3 and arguments3 -- and argument03 -- are
// all argument3, ONE row and never four. A key that is none of them answers itself, and
// so does one numbered 0 (names_argument_zero says so).
std::string the_argument_row_spelled(const std::string &key);
bool names_argument_zero(const std::string &key);

// THE OLD SPELLING. argument_1 was the first word AFTER the file, which is argument3
// now that argument1 is satl itself and argument2 the file -- so argument_N answers
// the row it means now, argument(N+2), and "" for a key that is not one. It is never
// READ as that row: a program written before 2026-10-03 is told the new name, rather
// than quietly handed satl's own path where it expected its first word.
std::string the_row_argument_underscore_is_now(const std::string &key);

} // namespace satellite004
