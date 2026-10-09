#pragma once
// satellite/arguments/argument_words.hpp -- THE ARGUMENTS WORDS, BUILT INTO satl: one table,
// one row a word, and the function each word runs.
//
// (the author, 2026-10-03) "Let's build satellite.variable.arguments into the satl interpreter
// itself, and build each of the 33 arguments .so file's into the interpreter ... so that it can
// be read alot faster than opening 33 files". Until then every one of these words was a library
// of its own under satellite-numbers/, and satl opened all 33 at every start -- 1.57 ms of a
// 10.8 ms hello world (measured that day), for words most programs never say.
//
// NOTHING ELSE CHANGED, AND THAT IS WHY IT IS A TABLE OF THE SAME ROWS. NumberIndex::load
// (satellite-numbers/call_number.satellite.cpp) files each row here exactly as it filed a
// library's description -- a name, its numbers, its Scenarios -- before it opens the libraries
// that are still files. The function table, the arguments variable, the checker and the walker
// read a word through the same NumberRow they always did, and cannot tell the two apart.
//
// A ROW NAMES ITS WORD AND NOTHING ELSE. Its numbers come from the word table (words.tsv, through
// word_codes.hpp) when it is filed, so a row here can never disagree with words.tsv about where a
// word lives -- the thing every library's satellite_number_describe had to type out by hand.
//
// HEADER-ONLY, so anything that compiles call_number.satellite.cpp -- satl, and the race and
// string-method harnesses beside it -- has the rows without another .cpp to link.

#include "../../satellite-numbers/number_row.hpp"
#include "../../satellite-numbers/machine_facts.hpp"
#include "argument_settings.hpp"
#include "../config/config_file.hpp"
#include "../config/machine_probe.hpp"
#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {
namespace argument_words {

// ARGUMENTS.ACCESS -- THE VALVE ON THE LAST-KNOWN NAME, TYPE AND VALUE. The author, 2026-09-18:
// *"we keep a list of everything inside of satellite, but we keep it in satellite.history,
// satellite.library is cleaned up, history is the lasting copy -- as long as the interpreter
// runs, we keep the last name, type and value of everything.... but only the last"*, and the
// control for it is *"a single satellite.variable.bool history_valve = true/false, and it lives
// at arguments.access = true/false"*. Written, it lasts in config.ini and reads back across runs.
//
// ON BY DEFAULT, on his word: *"arguments.access will always be on, on this machine, as we are
// testing it"*. A machine with no config.ini reads true; turning it off is what gets written.
//
// THE KEY IN THE FILE IS THE WORD WITHOUT ITS PATH: a person opening config.ini reads
// `access = true`. The path is how the LANGUAGE spells it; the file spells it the short way.
//
// SINCE 2026-10-06 A RUN READS ITS OWN ROW, `arguments.access`, made at start-up from this same key
// (arguments.cpp), so a file's line can change it for that run only and the prompt for good
// (bytecode/setting_writes.hpp). This reader answers where there is no row -- a binary that never
// ran satl's start -- and its default is the table's, so the two cannot disagree.
inline constexpr const char *kAccessKey = "access";
inline constexpr bool kAccessDefault = kAccessOnByDefault;

inline SettingReply access_setting(bool writing, bool value)
{
    SettingReply reply;
    if (!writing) {
        bool said = kAccessDefault;
        config_file::read_flag(kAccessKey, said);   // false leaves the default
        reply.code = success;
        reply.flag = said;
        return reply;
    }
    std::string why;
    const signed long long int wrote = config_file::write_flag(kAccessKey, value, why);
    reply.code = wrote;
    reply.reason = why;
    // WHAT IT SAYS NOW, AND ON A FAILED WRITE THAT IS THE OLD VALUE: answering the value asked
    // for would make a refused write look like a successful one to anything reading the flag.
    if (wrote == success) {
        reply.flag = value;
    } else {
        bool said = kAccessDefault;
        config_file::read_flag(kAccessKey, said);
        reply.flag = said;
    }
    return reply;
}

// ARGUMENTS.THREADS -- HOW MANY THREADS THE INTERPRETER MAY CREATE ON THIS MACHINE. The author,
// 2026-09-25: *"I want arguments.threads or arguments.thread = how many the interpreter can
// create, and arguments.machine.thread = how many physical threads exist on the machine"*.
// The measured count when `satl --config` has run here, and the lowest ceiling /proc states
// when it has not. NEVER PROBES: reading a word must not cost nine seconds and three gigabytes,
// which is the whole reason --config is a separate, once-per-machine command (C6).
inline FactReply answer_threads()
{
    const unsigned long long int said = threads_this_machine_allows();
    if (said == 0)
        return machine_facts::could_not_read("any thread ceiling", machine_fact_not_read);
    return machine_facts::a_count(said);
}

// ONE ROW A WORD: its name after the arguments' own -- `machine.cores`, the name main's
// arguments variable files it under and config.ini spells it by -- and the function it runs.
struct BuiltInWord {
    const char *key;
    Scenarios scenarios;
};

// WHERE EVERY ROW IS FILED, ONCE UNDER EACH. The author, 2026-10-03: "we make it a special
// variable IN satellite.library at the location of: satellite.library.arguments ... so the
// entire interpreter can use it, because satellite.library.main is just for satellite.main".
// So satellite.library.arguments (1 14 3) is where the arguments live, first.
//
// THE OLD PLACE STILL ANSWERS, AND THAT IS A CHOICE HE MAY REVERSE. satellite.library.main.
// arguments (1 14 1 1) is where every one of these words lived until that day, and a word's
// number is frozen once given (words/make_words.py), so its rows stay in words.tsv either way.
// Filed here too, a program written the old way reads the same reader through the same row;
// dropping this second entry retires the old spelling, and nothing else has to change.
inline constexpr const char *kPlaces[] = {"satellite.library.arguments", "satellite.library.main.arguments"};

// THE ALIASES ARE ROWS OF THEIR OWN WITH THE SAME FUNCTION -- cores, dir, ram, user and the two
// bare memory words. The author, 2026-09-18: *"let's use the longer choice for each one, can we
// have an alias for them though?"* The long name is the word and the short one a second way to
// write it, and because both rows point at ONE reader in machine_facts.hpp they cannot answer
// differently, which is the only way an alias really goes wrong.
inline constexpr BuiltInWord kWords[] = {
    {"access", {.flag_setting = &access_setting}},
    {"cores", {.fact = &machine_facts::answer_cores}},
    {"dir", {.fact = &machine_facts::answer_directory}},
    {"directory", {.fact = &machine_facts::answer_directory}},
    {"machine.architecture", {.fact = &machine_facts::answer_architecture}},
    {"machine.byte_order", {.fact = &machine_facts::answer_byte_order}},
    {"machine.cores", {.fact = &machine_facts::answer_cores}},
    {"machine.cpu", {.fact = &machine_facts::answer_cpu}},
    {"machine.page_size", {.fact = &machine_facts::answer_page_size}},
    {"machine.pointer_bits", {.fact = &machine_facts::answer_pointer_bits}},
    {"machine.threads", {.fact = &machine_facts::answer_hardware_threads}},
    {"memory()", {.fact = &machine_facts::answer_memory_total}},
    {"memory", {.fact = &machine_facts::answer_memory_total}},
    {"memory.free", {.fact = &machine_facts::answer_memory_free}},
    {"memory.total", {.fact = &machine_facts::answer_memory_total}},
    {"memory.used", {.fact = &machine_facts::answer_memory_used}},
    {"process.id", {.fact = &machine_facts::answer_process_id}},
    {"process.parent", {.fact = &machine_facts::answer_process_parent}},
    {"ram", {.fact = &machine_facts::answer_memory_total}},
    {"session.directory", {.fact = &machine_facts::answer_directory}},
    {"session.home", {.fact = &machine_facts::answer_home}},
    {"session.language", {.fact = &machine_facts::answer_language}},
    {"session.shell", {.fact = &machine_facts::answer_shell}},
    {"session.terminal", {.fact = &machine_facts::answer_terminal}},
    {"system.distribution", {.fact = &machine_facts::answer_distribution}},
    {"system.distribution_id", {.fact = &machine_facts::answer_distribution_id}},
    {"system.distribution_version",
     {.fact = &machine_facts::answer_distribution_version}},
    {"system.hostname", {.fact = &machine_facts::answer_hostname}},
    {"system.kernel", {.fact = &machine_facts::answer_kernel}},
    {"system.kernel_version", {.fact = &machine_facts::answer_kernel_version}},
    {"threads", {.fact = &answer_threads}},
    {"user", {.fact = &machine_facts::answer_username}},
    {"username", {.fact = &machine_facts::answer_username}},
};

} // namespace argument_words
} // namespace satellite004
