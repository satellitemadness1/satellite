// The thirty-two facts the arguments object answers about this machine and
// this build. See system_facts/arguments_facts.hpp for the seam and for why
// the assembly is deferred to the first ask.
//
// PORTED FROM THE FIRST SATELLITE'S arguments_facts.cpp AND system.cpp's
// arguments_for(), which PLAN M20 names as the split: the one-fact-each
// readers and the assembler that calls them come here, and system.cpp's
// library_path() and its -DSATELLITE_LIB_DIR build coupling go to M25 with
// `satellite.include` of another file.
//
// THE PORT CHANGED THREE THINGS AND EACH IS MARKED WHERE IT HAPPENS: a count
// is stored as a count rather than rendered to text, the CPU model name is new
// work with no reader in either tree, and nothing loops on a fixed buffer.

#include "system_facts/arguments_facts.hpp"

#include "system_facts/facts.hpp"
#include "system_facts/version.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>

// The make that ran, when the Makefile said. The fallback is here for the same
// reason version.hpp's are: the test binaries compile these sources directly
// with no -D at all, and so does anyone reaching for a bare clang++.
#ifndef SATELLITE_BUILD_MAKE
#define SATELLITE_BUILD_MAKE "unrecorded"
#endif

namespace satellite::facts {

namespace {

// $NAME, or "" if unset or empty. Empty rather than the fallback word, because
// every caller here wants to try a second source before giving up.
std::string env_or_empty(const char *name)
{
    const char *value = getenv(name);
    return (value && *value) ? std::string(value) : std::string();
}

// One VAR=value out of /etc/os-release, with the surrounding quotes taken off.
//
// PARSED HERE RATHER THAN SHELLED OUT TO, which is v1's reason kept whole: a
// program that forks `sh -c ". /etc/os-release"` to learn its own distribution
// is a program that fails inside a container with no shell. The format is one
// KEY=VALUE per line, optionally quoted, and that is all this reads.
std::string os_release(const char *key)
{
    FILE *file = fopen("/etc/os-release", "re");
    if (!file)
        return {};

    const std::string want = std::string(key) + "=";
    std::string found;
    char line[1024];
    while (fgets(line, sizeof line, file)) {
        std::string text(line);
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            text.pop_back();
        if (text.compare(0, want.size(), want) != 0)
            continue;
        found = text.substr(want.size());
        if (found.size() >= 2 &&
            (found.front() == '"' || found.front() == '\'') &&
            found.back() == found.front())
            found = found.substr(1, found.size() - 2);
        break;
    }
    fclose(file);
    return found;
}

// What the processor calls itself -- /proc/cpuinfo's "model name".
//
// NEW WORK AND THE ONE ROW HERE WITH NO READER IN EITHER TREE. PLAN M20 says
// so: nothing in v1 or in src/ reads this, and v1's `architecture` is
// uname.machine (`x86_64`), which is a different fact and keeps its own field.
// The two are next to each other in the struct precisely so nobody merges them.
//
// THE FIRST MATCH WINS because every core prints its own line and they are the
// same string; a machine with two different processors in it would answer for
// the first, which is a truthful answer to "what is this processor" and is
// what every other tool that reads this file does.
std::string cpu_model()
{
    FILE *file = fopen("/proc/cpuinfo", "re");
    if (!file)
        return {};

    std::string found;
    char line[1024];
    while (fgets(line, sizeof line, file)) {
        std::string text(line);
        // "model name" on x86; "Model" on some ARM kernels, which is why the
        // separator rather than the key is what this splits on.
        const size_t colon = text.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = text.substr(0, colon);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
            key.pop_back();
        if (key != "model name" && key != "Model")
            continue;
        found = text.substr(colon + 1);
        const size_t first = found.find_first_not_of(" \t");
        found = first == std::string::npos ? std::string() : found.substr(first);
        while (!found.empty() &&
               (found.back() == '\n' || found.back() == '\r' ||
                found.back() == ' '))
            found.pop_back();
        break;
    }
    fclose(file);
    return found;
}

// The C++ standard this was compiled to, as a year rather than as
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
// are supported targets -- the Makefile builds with clang against libstdc++ --
// so neither may be assumed.
std::string standard_library_name()
{
#if defined(_GLIBCXX_RELEASE)
    return "libstdc++ " + std::to_string(_GLIBCXX_RELEASE);
#elif defined(_LIBCPP_VERSION)
    return "libc++ " + std::to_string(_LIBCPP_VERSION);
#else
    return kUnrecorded;
#endif
}

std::string c_library_name()
{
#if defined(__GLIBC__)
    return "glibc " + std::to_string(__GLIBC__) + "." +
           std::to_string(__GLIBC_MINOR__);
#else
    return kUnrecorded;
#endif
}

// The running binary's own path.
//
// NO FIXED BUFFER, WHICH IS THE PORT'S THIRD CHANGE. v1 read into `char
// buf[4096]` and refused an answer that exactly filled it -- right, because a
// readlink that fills its buffer may have been truncated and a truncated path
// names a file nobody meant -- but the ceiling is a constant deciding how deep
// a tree the user may install into, which DESIGN §7.5 forbids. user_facts.cpp
// made the same change to cwd() at M9; this is that loop again.
std::string exe_path()
{
    std::vector<char> buffer(256);
    for (;;) {
        const ssize_t wrote = readlink("/proc/self/exe", buffer.data(),
                                       buffer.size());
        if (wrote <= 0)
            return {};
        if (static_cast<size_t>(wrote) < buffer.size())
            return std::string(buffer.data(), static_cast<size_t>(wrote));
        buffer.resize(buffer.size() * 2);
    }
}

// A fact, or the word that says the machine was asked and had nothing.
std::string or_unrecorded(std::string value)
{
    return value.empty() ? std::string(kUnrecorded) : value;
}

MachineAnswers assemble()
{
    MachineAnswers answers;

    // --- machine ------------------------------------------------------------
    answers.cpu = or_unrecorded(cpu_model());
    answers.cores = physical_cores();
    answers.threads = hardware_threads();
    // ANSWERED FROM THE COMPILER'S MACRO WHERE THERE IS ONE AND BY INSPECTING
    // A VALUE WHERE THERE IS NOT, so this is right on a target the macro does
    // not cover rather than merely usually right. v1's probe, kept.
    {
        const uint16_t probe = 1;
        answers.byte_order =
            *reinterpret_cast<const unsigned char *>(&probe) == 1 ? "little"
                                                                  : "big";
    }
    {
        const long page = sysconf(_SC_PAGESIZE);
        answers.page_size = page > 0 ? static_cast<unsigned long long>(page) : 0;
    }
    answers.pointer_bits = sizeof(void *) * 8;

    // --- memory -------------------------------------------------------------
    answers.memory_total_mb = mem_total_mb();

    // --- who, and the operating system --------------------------------------
    // $HOME AND $USER ARE BOTH FORGEABLE AND BOTH ABSENT UNDER `env -i`, so
    // the password database is the second source. It is the authority for the
    // name; $USER is merely what the shell was told.
    std::string home = env_or_empty("HOME");
    std::string user = env_or_empty("USER");
    if (home.empty() || user.empty()) {
        if (const passwd *pw = getpwuid(getuid())) {
            if (home.empty() && pw->pw_dir)
                home = pw->pw_dir;
            if (user.empty() && pw->pw_name)
                user = pw->pw_name;
        }
    }
    answers.username = or_unrecorded(user);
    answers.home = or_unrecorded(home);

    {
        // ONE FACT PER FIELD AND NEVER ONE "os info" STRING, because a program
        // that wants the architecture should not have to parse a sentence.
        utsname machine{};
        const bool ok = uname(&machine) == 0;
        answers.name = or_unrecorded(ok ? machine.sysname : "");
        answers.kernel = or_unrecorded(ok ? machine.release : "");
        answers.kernel_version = or_unrecorded(ok ? machine.version : "");
        answers.architecture = or_unrecorded(ok ? machine.machine : "");
    }
    answers.distribution = or_unrecorded(os_release("PRETTY_NAME"));
    answers.distribution_id = or_unrecorded(os_release("ID"));
    answers.distribution_version = or_unrecorded(os_release("VERSION_ID"));
    {
        // THE KERNEL'S OWN CEILING, ASKED FOR, and not a number written here
        // deciding how long a name a machine may have -- DESIGN §7.5 again.
        const long most = sysconf(_SC_HOST_NAME_MAX);
        std::vector<char> buffer((most > 0 ? most : 255) + 1, '\0');
        answers.hostname = or_unrecorded(
            gethostname(buffer.data(), buffer.size()) == 0
                ? std::string(buffer.data())
                : std::string());
    }

    // --- what built this ----------------------------------------------------
    answers.compiler = or_unrecorded(SATELLITE_BUILD_CXX);
#ifdef __VERSION__
    answers.compiler_version = or_unrecorded(__VERSION__);
#else
    answers.compiler_version = kUnrecorded;
#endif
    answers.standard = cxx_standard_name();
    answers.flags = or_unrecorded(SATELLITE_BUILD_FLAGS);
    answers.make = or_unrecorded(SATELLITE_BUILD_MAKE);
    answers.standard_library = standard_library_name();
    answers.c_library = c_library_name();
    answers.built = or_unrecorded(SATELLITE_BUILT);

    // --- the interpreter ----------------------------------------------------
    answers.interpreter = or_unrecorded(exe_path());
    // ONE FUNCTION AND NOT A SECOND LITERAL. version.hpp's whole argument: the
    // first satellite kept the prompt's version as its own string and the two
    // drifted, with nothing in the build able to notice.
    answers.version = version_line();

    // --- this process -------------------------------------------------------
    answers.process_id = static_cast<unsigned long long>(getpid());
    answers.parent_process_id = static_cast<unsigned long long>(getppid());

    // --- the session it was started in --------------------------------------
    answers.shell = or_unrecorded(env_or_empty("SHELL"));
    answers.terminal = or_unrecorded(env_or_empty("TERM"));
    answers.language = or_unrecorded(env_or_empty("LANG"));

    return answers;
}

} // namespace

const MachineAnswers &machine_answers()
{
    // A FUNCTION-LOCAL STATIC, so the deferral costs no mutex and no flag a
    // later reader has to remember to check: C++ guarantees exactly one thread
    // runs the initialiser and the rest wait on it. satl runs a printer thread
    // and a watchdog thread, so this is a real guarantee and not a formality.
    static const MachineAnswers answers = assemble();
    return answers;
}

} // namespace satellite::facts
