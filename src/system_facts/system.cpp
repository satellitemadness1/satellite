// The two answers that are built into the binary rather than read off the
// machine: where the .satl library lives, and everything satellite.main is
// handed about its command line, its system and its build.
//
// Both stay HERE, and this is the reason the split left them alone: system.o
// is the one object the Makefile compiles with -DSATELLITE_LIB_DIR and
// VERSION_DEFS. A sibling file is built by the generic rule, would get neither,
// and would answer from the #ifndef fallbacks immediately below -- `unrecorded`
// for a build fact that WAS recorded, /usr/local for a prefix nobody chose.
//
// The live machine facts moved out to host_facts.cpp, memory_facts.cpp and
// stack_facts.cpp, and the one-fact-each readers this file's arguments_for()
// calls moved to arguments_facts.cpp. See system_internal.hpp.

#include "system_facts/system_internal.hpp"

// Tier 3 of library_path(), baked from the Makefile's `prefix`. The fallback
// is here so that a build with no -D at all -- the test binaries compile these
// sources directly, and so does anyone reaching for a bare clang++ -- still
// compiles and still resolves somewhere defensible.
#ifndef SATELLITE_LIB_DIR
#define SATELLITE_LIB_DIR "/usr/local/share/satellite/lib"
#endif

// The build facts the arguments object reports. Every one has a fallback for
// the same reason SATELLITE_LIB_DIR does: the test binaries compile these
// sources directly with no -D at all, and so does anyone reaching for a bare
// clang++. `unrecorded` is a truthful answer and an empty string is not.
#ifndef SATELLITE_BUILD_CXX
#define SATELLITE_BUILD_CXX "unrecorded"
#endif
#ifndef SATELLITE_BUILD_FLAGS
#define SATELLITE_BUILD_FLAGS "unrecorded"
#endif
#ifndef SATELLITE_BUILD_MAKE
#define SATELLITE_BUILD_MAKE "unrecorded"
#endif
#ifndef SATELLITE_BUILT
#define SATELLITE_BUILT "unrecorded"
#endif
#ifndef SATELLITE_VERSION
#define SATELLITE_VERSION "002"
#endif
#ifndef SATELLITE_REVISION
#define SATELLITE_REVISION "01"
#endif

namespace satellite {

static bool is_directory(const std::string &path)
{
    struct stat info;
    return !path.empty() && stat(path.c_str(), &info) == 0 &&
           S_ISDIR(info.st_mode);
}

// The install root the running binary sits under: two components up from
// /proc/self/exe, so /usr/local/bin/satl answers /usr/local.
//
// Two components rather than a match on the literal "/bin/satl", because
// satl-term is the other binary in that directory and it must reach the
// same answer.
static std::string exe_prefix()
{
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf);
    // A readlink that exactly fills the buffer may have been truncated, and
    // there is no way to tell which -- so it is refused. A truncated path is
    // worse than no path: it would name a directory nobody meant.
    if (n <= 0 || static_cast<size_t>(n) >= sizeof buf)
        return {};

    std::string path(buf, static_cast<size_t>(n));
    for (int up = 0; up < 2; ++up) {
        size_t slash = path.rfind('/');
        if (slash == std::string::npos)
            return {};
        path.resize(slash);
    }
    return path;
}

std::string library_path(LibraryPathSource *source)
{
    auto answer = [source](LibraryPathSource found, std::string path) {
        if (source)
            *source = found;
        return path;
    };

    if (const char *env = getenv("SATELLITE_PATH"); env && *env)
        if (is_directory(env))
            return answer(LibraryPathSource::Environment, env);

    if (std::string prefix = exe_prefix(); !prefix.empty()) {
        std::string relative = prefix + "/share/satellite/lib";
        if (is_directory(relative))
            return answer(LibraryPathSource::Relative, std::move(relative));
    }

    if (is_directory(SATELLITE_LIB_DIR))
        return answer(LibraryPathSource::Compiled, SATELLITE_LIB_DIR);

    return answer(LibraryPathSource::None, {});
}


// ---------------------------------------------------------------------------
// The arguments object — see system.hpp for what it promises.
// ---------------------------------------------------------------------------

Arguments arguments_for(const List &command_line)
{
    Arguments args;

    // --- the command line, first and in order -------------------------------
    // argv[0] is the script (interp.hpp), so it gets the name a shell would
    // call it: the program. Everything after it is numbered from 1, matching
    // the index the program reads it at, so argument_1 IS args[1].
    for (size_t i = 0; i < command_line.size(); i++) {
        std::string name = i == 0 ? std::string("program")
                                  : "argument_" + std::to_string(i);
        args.index[name] = args.entries.size();
        // The caller's handle, not a copy: these Values already exist and are
        // immutable, which is the whole reason a ValuePtr may be shared.
        args.entries.push_back(ArgumentEntry{std::move(name), command_line[i]});
    }
    args.command_line_count = args.entries.size();

    // Deliberately AFTER command_line_count is fixed, so it is a named entry
    // and not a positional one — it describes the command line, it is not part
    // of it, and counting itself would be wrong by one.
    add(args, "argument_count", std::to_string(command_line.size()));

    // --- where the program is -----------------------------------------------
    {
        char buf[4096];
        add(args, "current_directory",
            getcwd(buf, sizeof buf) ? std::string(buf) : std::string());
    }

    std::string home = env_or_empty("HOME");
    std::string user = env_or_empty("USER");
    if (home.empty() || user.empty()) {
        // $HOME and $USER are both forgeable and both absent under a bare
        // `env -i`, so the password database is the second source. It is the
        // authority for the name; $USER is merely what the shell was told.
        if (const struct passwd *pw = getpwuid(getuid())) {
            if (home.empty() && pw->pw_dir)
                home = pw->pw_dir;
            if (user.empty() && pw->pw_name)
                user = pw->pw_name;
        }
    }
    add(args, "home_directory", home);
    add(args, "username", user);

    {
        char buf[256];
        add(args, "hostname",
            gethostname(buf, sizeof buf) == 0 ? std::string(buf) : std::string());
    }

    // --- the operating system, one fact per entry ---------------------------
    // Separate entries rather than one "os info" string, because a program
    // that wants the architecture should not have to parse a sentence.
    {
        struct utsname u;
        bool ok = uname(&u) == 0;
        add(args, "operating_system", ok ? u.sysname : "");
        add(args, "kernel_release",   ok ? u.release : "");
        add(args, "kernel_version",   ok ? u.version : "");
        add(args, "architecture",     ok ? u.machine : "");
    }
    add(args, "distribution",         os_release("PRETTY_NAME"));
    add(args, "distribution_id",      os_release("ID"));
    add(args, "distribution_version", os_release("VERSION_ID"));

    // --- what built the interpreter -----------------------------------------
    // Baked in at compile time, so these describe the binary that is running
    // and not whatever compiler happens to be installed now. That distinction
    // is the reason they are worth carrying at all.
    add(args, "cxx_compiler",         SATELLITE_BUILD_CXX,  UNRECORDED);
    add(args, "cxx_compiler_version",
#ifdef __VERSION__
        __VERSION__,
#else
        "",
#endif
        UNRECORDED);
    add(args, "cxx_standard",         cxx_standard_name(),  UNRECORDED);
    add(args, "cxx_flags",            SATELLITE_BUILD_FLAGS, UNRECORDED);
    add(args, "make_version",         SATELLITE_BUILD_MAKE, UNRECORDED);
    add(args, "standard_library",     standard_library_name(), UNRECORDED);
    add(args, "c_library",            c_library_name(),     UNRECORDED);
    add(args, "built",                SATELLITE_BUILT,      UNRECORDED);

    // --- the interpreter itself ---------------------------------------------
    {
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof buf);
        // The same truncation refusal exe_prefix() makes above: a path that
        // exactly fills the buffer may have been cut, and a truncated path
        // names a file nobody meant.
        add(args, "interpreter",
            (n > 0 && static_cast<size_t>(n) < sizeof buf)
                ? std::string(buf, static_cast<size_t>(n))
                : std::string());
    }
    add(args, "interpreter_version",
        std::string(SATELLITE_VERSION) + " revision " + SATELLITE_REVISION);

    {
        LibraryPathSource source = LibraryPathSource::None;
        std::string path = library_path(&source);
        add(args, "library_path", path);
        // Which of the three tiers answered — the same fact `--where` prints,
        // because "it found a library" and "it found the one you installed"
        // are different answers and only one of them is reassuring.
        const char *how = "none";
        switch (source) {
        case LibraryPathSource::Environment: how = "SATELLITE_PATH"; break;
        case LibraryPathSource::Relative:    how = "alongside the binary"; break;
        case LibraryPathSource::Compiled:    how = "compiled in"; break;
        case LibraryPathSource::None:        how = "none"; break;
        }
        add(args, "library_path_source", how);
    }

    // --- the process --------------------------------------------------------
    add(args, "process_id",        std::to_string(getpid()));
    add(args, "parent_process_id", std::to_string(getppid()));
    {
        unsigned n = std::thread::hardware_concurrency();
        add(args, "thread_count", n ? std::to_string(n) : std::string());
    }
    {
        long page = sysconf(_SC_PAGESIZE);
        add(args, "page_size", page > 0 ? std::to_string(page) : std::string());
    }
    add(args, "pointer_bits", std::to_string(sizeof(void *) * 8));
    // Answered from the compiler's own macro where there is one, and by
    // inspecting a value where there is not, so this is right on a target the
    // macro does not cover rather than merely usually right.
    {
        const uint16_t probe = 1;
        bool little = *reinterpret_cast<const unsigned char *>(&probe) == 1;
        add(args, "byte_order", little ? "little" : "big");
    }

    // --- the environment it was started in ----------------------------------
    // Four named variables and no mechanism for a fifth. Reading the
    // environment in general is satellite.system.environment(name)'s job and
    // it does not exist yet; these three are here because they describe the
    // session rather than the program's own configuration.
    add(args, "shell",    env_or_empty("SHELL"));
    add(args, "terminal", env_or_empty("TERM"));
    add(args, "language", env_or_empty("LANG"));

    return args;
}

} // namespace satellite
