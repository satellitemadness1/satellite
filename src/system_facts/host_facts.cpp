// Who is running this and on what: the username, the home directory, the
// working directory, and how many hardware threads the machine has.
//
// Part of src/system_facts/, split from a 653-line system.cpp. See
// system_internal.hpp for what the pieces share and why system.cpp kept what
// it kept.

#include "system_facts/system_internal.hpp"

namespace satellite {

std::string username()
{
    if (const passwd *pw = getpwuid(getuid()); pw && pw->pw_name)
        return pw->pw_name;
    if (const char *env = getenv("USER"))
        return env;
    return "unknown";
}

std::string home_dir()
{
    if (const char *env = getenv("HOME"))
        return env;
    if (const passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
    return "/";
}

std::string cwd()
{
    char buf[4096];
    if (getcwd(buf, sizeof buf))
        return buf;
    return "?";
}

unsigned hardware_threads()
{
    unsigned n = std::thread::hardware_concurrency();
    return n ? n : 1;
}

} // namespace satellite
