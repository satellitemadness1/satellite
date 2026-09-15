// Who is running this and where: the username, the home directory and the
// working directory. See system_facts/facts.hpp.
//
// PORTED FROM THE FIRST SATELLITE'S host_facts.cpp AT M9, WHICH IS WHERE ITS
// OWN NOTE SAID THEY WOULD COME. M6 took hardware_threads() out of that file
// and left these three behind on the argument that "bringing them now would put
// three functions in this tree that nothing calls"; their caller is
// satellite_value/render.cpp, and it exists as of this milestone.
//
// THE THREE ARE satellite_string's CODES 95, 96 AND 100, and that is the whole
// of why they are facts rather than something the value module reads for
// itself. DESIGN §5's code table calls them live, LAYOUT.md draws the seam
// between a reader of the machine and a policy about what it answers, and every
// other live code in that table -- 97, 98 and 99 -- already comes from here.
//
// cwd() DOES NOT CARRY A BUFFER SIZE, AND v1's DID. The port is otherwise
// line for line; this one line is not, because `char buf[4096]` is a constant in
// a header deciding how long a thing the user may have, which DESIGN §7.5
// forbids in as many words. Linux has no PATH_MAX that binds getcwd() -- a
// directory nested past 4096 bytes is legal and reachable, and v1 answered "?"
// for it. The loop below doubles until it fits, so the bound is memory.
//
// EVERY READ IS FRESH, which is facts.hpp's rule for the whole module and is
// load-bearing for exactly one of these three: `\cwd` in a string must answer
// where the program is NOW, and a satellite program that changes directory and
// prints the same literal twice must see two answers.

#include "system_facts/facts.hpp"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <vector>

#include <pwd.h>
#include <unistd.h>

namespace satellite::facts {

std::string username()
{
    // THE PASSWORD DATABASE FIRST AND THE ENVIRONMENT SECOND, which is the
    // opposite order from home_dir() below and is v1's, kept. A user may point
    // HOME wherever they like and the shell will honour it, so the environment
    // is the better answer there; USER is a courtesy the shell sets and the
    // kernel's idea of who this process is, is the better answer here.
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
    // ERANGE IS THE ONLY FAILURE WORTH RETRYING, and the loop stops on any
    // other one -- a deleted working directory answers ENOENT forever and
    // doubling a buffer at it would run until the watchdog noticed.
    std::vector<char> buffer(256);
    for (;;) {
        if (getcwd(buffer.data(), buffer.size()))
            return buffer.data();
        if (errno != ERANGE)
            return "?";
        buffer.resize(buffer.size() * 2);
    }
}

} // namespace satellite::facts
