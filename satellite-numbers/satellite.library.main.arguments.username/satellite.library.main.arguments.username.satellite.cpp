// satellite.library.main.arguments.username  `1 14 1 1 3` -- the login name this interpreter is running as.
//
// SATELLITE_ARGUMENTS B7-B11: one of "the values that are free" -- a /proc or
// sysconf read the interpreter already did somewhere, needing a library and a
// row rather than a new reader. The reader itself is in
// satellite-numbers/machine_facts.hpp, shared because each library is compiled
// from exactly one .cpp.
//
// READ EVERY TIME, NEVER CACHED. See machine_facts.hpp for why.
//
// A FAILURE IS SAID, NOT ANSWERED AS 0. A machine that does not state this
// refuses with machine_fact_not_read (36) and names what it could not read --
// because 0 is a number a program would divide by.

#include "../machine_facts.hpp"

namespace {

using satellite004::FactReply;

FactReply answer_arguments_username()
{
    std::string said;
    if (satellite004::machine_facts::username(said) == false)
        return satellite004::machine_facts::could_not_read("a passwd entry for this user", satellite004::machine_fact_not_read);
    return satellite004::machine_facts::some_text(std::move(said));
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.username";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 3;
    row->depth = 5;
    row->scenarios.fact = &answer_arguments_username;
    return satellite004::success;
}
