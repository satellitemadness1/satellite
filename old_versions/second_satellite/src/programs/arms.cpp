// The arms and what each installs. See arms.hpp for why this is one file.

#include "programs/arms.hpp"

#include "satellite_arguments/arguments.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_directory/handlers.hpp"
#include "satellite_file/handlers.hpp"
#include "satellite_help/handlers.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_thread/handlers.hpp"
#include "satellite_time/handlers.hpp"

namespace satellite::arms {
namespace {

// THE DIFFERENCES THAT ARE ON PURPOSE, AND EVERY ONE OF THEM WAS PROSE FIRST.
// Each `why` below is a sentence that already existed as a comment in the arm
// it describes; moving it here is what lets `satl --arms` answer the question
// and what lets tests/arms_test tell an intended gap from M23's accident.
//
// A ROW HERE IS A DECISION AND NOT A TODO. Anything missing because nobody got
// to it yet does NOT belong in this table -- it belongs in the arm, failing the
// audit, until somebody either installs it or decides it should never be there.
constexpr Exception kExceptions[] = {
    // `--call` runs one capsule with no `satellite.main` in front of it and no
    // printer behind it, so `display` under it answers S0721 -- which is the
    // honest answer: there is nothing to print through.
    { Arm::Call, Group::Console,
      "--call runs one capsule with no printer behind it" },

    // Help's whole answer is printed, so a help installed where `display` is
    // not would be a row that reaches a console this arm never started.
    { Arm::Call, Group::Help,
      "help's whole answer is printed, and --call starts no printer" },

    // `arguments::start()` is never called under `--call`, because the
    // arguments object is the COMMAND LINE and that arm does not have one.
    //
    // THE PROMPT HAD A ROW HERE UNTIL 2026-09-13 AND IT WAS WRONG ABOUT WHAT A
    // COMMAND LINE IS. Its argv is satl's own, but `run <file> a b` is a command
    // line the user typed, and a program that reads `arguments.length()` ran
    // from a file and answered S0721 from satl-term. The prompt now builds the
    // object per `run` -- satellite_prompt/session.cpp's run_file().
    { Arm::Call, Group::Arguments,
      "--call has no command line of its own to report" },
};

void install_group(Group group)
{
    switch (group) {
    case Group::Console:    console::install_handlers();    break;
    case Group::Scalars:    scalars::install_handlers();    break;
    case Group::Containers: containers::install_handlers(); break;
    case Group::Random:     random::install_handlers();     break;
    case Group::Time:       time::install_handlers();       break;
    case Group::Thread:     thread::install_handlers();     break;
    case Group::System:     system::install_handlers();     break;
    case Group::Help:       help::install_handlers();       break;
    case Group::File:       file::install_handlers();       break;
    case Group::Directory:  directory::install_handlers();  break;
    case Group::Arguments:  arguments::install_handlers();  break;
    case Group::Count:      break;
    }
}

} // namespace

const Exception *exceptions(std::size_t *count)
{
    if (count)
        *count = sizeof kExceptions / sizeof kExceptions[0];
    return kExceptions;
}

bool declared_absent(Arm arm, Group group, const char **why)
{
    for (const Exception &row : kExceptions) {
        if (row.arm == arm && row.group == group) {
            if (why)
                *why = row.why;
            return true;
        }
    }
    return false;
}

const char *arm_name(Arm arm)
{
    switch (arm) {
    case Arm::Run:    return "run";
    case Arm::Prompt: return "prompt";
    case Arm::Call:   return "call";
    case Arm::Count:  break;
    }
    return "?";
}

const char *group_name(Group group)
{
    switch (group) {
    case Group::Console:    return "console";
    case Group::Scalars:    return "scalars";
    case Group::Containers: return "containers";
    case Group::Random:     return "random";
    case Group::Time:       return "time";
    case Group::Thread:     return "thread";
    case Group::System:     return "system";
    case Group::Help:       return "help";
    case Group::File:       return "file";
    case Group::Directory:  return "directory";
    case Group::Arguments:  return "arguments";
    case Group::Count:      break;
    }
    return "?";
}

// EVERY GROUP, MINUS THE DECLARED EXCEPTIONS. Written as a loop over the whole
// enum rather than as a list per arm, because a list per arm is exactly the
// thing that drifted.
void install_for(Arm arm)
{
    for (unsigned char g = 0; g < static_cast<unsigned char>(Group::Count); ++g) {
        const Group group = static_cast<Group>(g);
        if (!declared_absent(arm, group, nullptr))
            install_group(group);
    }
}

std::string report()
{
    std::string out = "\nthe arms, and what each installs\n\n";

    out += "                 ";
    for (unsigned char a = 0; a < static_cast<unsigned char>(Arm::Count); ++a) {
        out += arm_name(static_cast<Arm>(a));
        out += "   ";
    }
    out += "\n";

    for (unsigned char g = 0; g < static_cast<unsigned char>(Group::Count); ++g) {
        const Group group = static_cast<Group>(g);

        std::string name = group_name(group);
        out += "  " + name;
        for (std::size_t pad = name.size(); pad < 15; ++pad)
            out += ' ';

        for (unsigned char a = 0; a < static_cast<unsigned char>(Arm::Count); ++a) {
            const Arm arm = static_cast<Arm>(a);
            const bool absent = declared_absent(arm, group, nullptr);
            const std::string mark = absent ? "-" : "yes";
            out += mark;
            for (std::size_t pad = mark.size();
                 pad < std::string(arm_name(arm)).size() + 3; ++pad)
                out += ' ';
        }
        out += "\n";
    }

    out += "\n  `-` is a difference this build declares on purpose:\n\n";
    std::size_t count = 0;
    const Exception *rows = exceptions(&count);
    for (std::size_t i = 0; i < count; ++i) {
        out += "    ";
        out += arm_name(rows[i].arm);
        out += " has no ";
        out += group_name(rows[i].group);
        out += " -- ";
        out += rows[i].why;
        out += "\n";
    }

    out += "\n  anything NOT listed above is installed in every arm, and\n"
           "  tests/arms_test fails the day that stops being true. See\n"
           "  ERROR_HANDLING.md section 4.3 for what this prevents.\n";
    return out;
}

bool audit(std::string *complaints)
{
    // THE AUDIT CANNOT READ THE TABLE AND MUST NOT TRY. Installing an arm is a
    // process-wide side effect, so a check that installed each arm in turn
    // would be checking the last one three times. What is audited instead is
    // the DECLARATION -- install_for() is a loop over this same data, so a
    // group reachable here is a group it installs, and the two cannot disagree
    // without this file being edited in two places at once.
    bool fine = true;

    for (unsigned char a = 0; a < static_cast<unsigned char>(Arm::Count); ++a) {
        for (unsigned char g = 0; g < static_cast<unsigned char>(Group::Count); ++g) {
            const Arm arm = static_cast<Arm>(a);
            const Group group = static_cast<Group>(g);

            const char *why = nullptr;
            if (!declared_absent(arm, group, &why))
                continue;

            if (why == nullptr || *why == '\0') {
                fine = false;
                if (complaints) {
                    *complaints += "  ";
                    *complaints += arm_name(arm);
                    *complaints += " declares no ";
                    *complaints += group_name(group);
                    *complaints += " and gives no reason\n";
                }
            }
        }
    }
    return fine;
}

} // namespace satellite::arms
