// satellite.library.main.arguments.access  `1 14 1 1 10` -- the valve on the
// last-known name, type and value, compiled as a library with no main.
//
// THE AUTHOR'S BRIEF, 2026-09-18: *"we keep a list of everything inside of
// satellite, but we keep it in satellite.history, satellite.library is cleaned
// up, history is the lasting copy -- as long as the interpreter runs, we keep
// the last name, type and value of everything.... but only the last"*, and the
// control for it is *"a single satellite.variable.bool history_valve =
// true/false, and it lives at arguments.access = true/false"*.
//
// WHAT THIS WORD IS AND IS NOT, TODAY. It is the VALVE and it is real: written,
// it lasts, and it reads back across runs. It does not yet turn anything off,
// because the last-known store it governs is not built. That is the honest order
// -- the switch before the thing it switches -- and it is the same order
// SATELLITE_ARGUMENTS.md's A1 puts `arguments_active` in, for the same reason:
// a valve built after its machinery has nowhere to be tested.
//
// AND IT IS NOT `arguments.history`. `1 14 1 1 11` is the OTHER valve, the one
// that writes every value to disk rather than keeping the last in memory. The
// author, 2026-09-18: *"let's leave history alone for right now, and add the
// access stuff"*. It is numbered and it has no library, which answers
// `not_built_yet` with its own name -- exactly what a person can act on.
//
// ON BY DEFAULT, AND THE AUTHOR SAID SO: *"arguments.access will always be on,
// on this machine, as we are testing it"*. A machine with no config.ini reads
// true, so the feature is on until somebody turns it off, and turning it off is
// what has to be written down rather than turning it on.

#include "../number_row.hpp"
#include "../../satellite/config/config_file.hpp"
#include "../../satellite/machine/machine_codes.hpp"

#include <string>

namespace {

using satellite004::SettingReply;

// THE KEY IN THE FILE IS THE WORD WITHOUT ITS PATH. A person opening
// config.ini reads `access = true`, not
// `satellite.library.main.arguments.access = true`. The path is how the
// LANGUAGE spells it; the file is a person's and spells it the short way.
const char *const kKey = "access";

// The default when nothing has been written. See the header comment: on.
constexpr bool kDefault = true;

SettingReply satellite_arguments_access_setting(bool writing, bool value)
{
    SettingReply reply;

    if (!writing) {
        bool said = kDefault;
        satellite004::config_file::read_flag(kKey, said);   // false leaves the default
        reply.code = satellite004::success;
        reply.flag = said;
        return reply;
    }

    std::string why;
    const signed long long int wrote =
        satellite004::config_file::write_flag(kKey, value, why);
    reply.code = wrote;
    reply.reason = why;
    // WHAT IT SAYS NOW, AND ON A FAILED WRITE THAT IS THE OLD VALUE. Answering
    // the value that was asked for would make a refused write look like a
    // successful one to anything reading the reply's flag instead of its code.
    if (wrote == satellite004::success) {
        reply.flag = value;
    } else {
        bool said = kDefault;
        satellite004::config_file::read_flag(kKey, said);
        reply.flag = said;
    }
    return reply;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    row->name = "satellite.library.main.arguments.access";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 10;
    row->depth = 5;
    row->scenarios.flag_setting = satellite_arguments_access_setting;
    return satellite004::success;
}
