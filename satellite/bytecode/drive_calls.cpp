// satellite/bytecode/drive_calls.cpp -- satellite.drive's words. drive_calls.hpp says which are built and why
// they have no library.

#include "drive_calls.hpp"

#include "word_codes.hpp"
#include "../machine/console_lock.hpp"
#include "../machine/drive_mounts.hpp"
#include "../machine/filesystems.hpp"
#include "../satl/drives.hpp"

#include <iostream>

namespace satellite004 {

bool is_drive_word(token::Code code)
{
    return code == word::fixed_code<1, 36, 1>;
}

signed long long int drive_word_refused(token::Code code, std::size_t given, std::string &why)
{
    if (!is_drive_word(code) || given == 0)
        return success;
    why = "satellite.drive.list() takes nothing -- satellite.drive.list() -- and was given " + std::to_string(given) +
          (given == 1 ? " argument" : " arguments");
    return satl_line_not_understood;
}

Value call_drive_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    std::string why;
    if (const signed long long int refused = drive_word_refused(code, arguments.size(), why); refused != success) {
        context.refuse(refused, why);
        return Value();
    }
    // UNDER THE CONSOLE'S LOCK, as every display is (the review, 2026-10-23): the table goes into std::cout's
    // buffer, which another thread's display hands to the printing satellite -- unlocked, two threads listing
    // while a third displayed lost a table, or wrote NULs, 4 runs in 15.
    const std::string table = drives_listed(every_drive(DriveRoots{}, filesystems::mounted_drives()));
    ConsoleHold hold;
    std::cout << table << std::flush;
    return Value();
}

} // namespace satellite004
