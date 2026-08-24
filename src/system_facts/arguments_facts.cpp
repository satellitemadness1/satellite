// One fact each, for the arguments object: the environment, /etc/os-release,
// what compiled the interpreter, and the single append that every entry goes
// through.
//
// These were an anonymous namespace inside system.cpp. arguments_for() still
// calls them and had to stay behind -- it reads the version macros, which only
// system.o is compiled with -- so they are declared in system_internal.hpp and
// defined here, and the two constants they share moved into that header with
// them. Nothing else changed: the default argument on add() is on the
// declaration now, because that is where a header puts one.
//
// Part of src/system_facts/, split from a 653-line system.cpp.

#include "system_facts/system_internal.hpp"

namespace satellite {

// $NAME, or "" if unset or empty. Empty rather than the fallback word, because
// every caller here wants to try a second source before giving up.
std::string env_or_empty(const char *name)
{
    const char *v = getenv(name);
    return (v && *v) ? std::string(v) : std::string();
}

// One VAR=value out of /etc/os-release, with the surrounding quotes taken off.
//
// Parsed here rather than shelled out to, because a program that forks
// `sh -c ". /etc/os-release"` to learn its own distribution is a program that
// fails inside a container with no shell. The format is one KEY=VALUE per
// line, optionally quoted, and that is all this reads.
std::string os_release(const char *key)
{
    FILE *f = fopen("/etc/os-release", "re");
    if (!f)
        return {};

    std::string want = std::string(key) + "=";
    char line[1024];
    std::string found;
    while (fgets(line, sizeof line, f)) {
        std::string text(line);
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            text.pop_back();
        if (text.compare(0, want.size(), want) != 0)
            continue;
        found = text.substr(want.size());
        if (found.size() >= 2 && (found.front() == '"' || found.front() == '\'') &&
            found.back() == found.front())
            found = found.substr(1, found.size() - 2);
        break;
    }
    fclose(f);
    return found;
}

// The C++ standard the interpreter was compiled to, as a year rather than as
// __cplusplus's raw 202002L, because the year is what anyone reading it knows.
const char *cxx_standard_name()
{
#if __cplusplus >= 202302L
    return "C++23";
#elif __cplusplus >= 202002L
    return "C++20";
#elif __cplusplus >= 201703L
    return "C++17";
#else
    return "pre-C++17";
#endif
}

// Which standard library, and which release of it. Both libstdc++ and libc++
// are supported targets — the Makefile builds with clang against libstdc++ —
// so neither may be assumed.
std::string standard_library_name()
{
#if defined(_GLIBCXX_RELEASE)
    return "libstdc++ " + std::to_string(_GLIBCXX_RELEASE);
#elif defined(_LIBCPP_VERSION)
    return "libc++ " + std::to_string(_LIBCPP_VERSION);
#else
    return UNRECORDED;
#endif
}

std::string c_library_name()
{
#if defined(__GLIBC__)
    return "glibc " + std::to_string(__GLIBC__) + "." +
           std::to_string(__GLIBC_MINOR__);
#else
    return UNRECORDED;
#endif
}

// Append one entry, keeping `index` in step. Called only while the body is
// still being built and before make_arguments freezes it.
//
// A blank value becomes the fallback word here, in ONE place, rather than at
// each of the thirty call sites — which is what stops an entry from quietly
// being "" on a machine where the fact was missing.
void add(Arguments &args, std::string name, std::string value,
         const char *missing)
{
    if (value.empty())
        value = missing;
    args.index[name] = args.entries.size();
    args.entries.push_back(
        ArgumentEntry{std::move(name),
                      std::make_shared<const Value>(make_string(encode_raw(value)))});
}

} // namespace satellite
