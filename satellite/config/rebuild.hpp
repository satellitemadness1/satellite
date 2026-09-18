#pragma once
// `satl --rebuild` -- compose every setting into the one binary, write it, and
// say what it turned on.
//
// The author, 2026-09-18: *"satl --rebuild is the command to rebuild the
// arguments string, so we have to set the arguments how we want them, then run
// satl --rebuild to rebuild the binary string that is stored inside of
// config.ini, it saves it and then it just loads that single value"*.
//
// AND: *"this way it remains fast, and the --rebuild takes a long time only,
// that is what is slow"*.
//
// THAT SENTENCE IS THE WHOLE DESIGN AND IT IS WORTH SAYING PLAINLY: **nothing
// here is on a path anybody waits on twice.** A person sets their settings,
// runs this once, and every run afterwards reads ONE value. So this step may do
// work that would be unthinkable per run -- check every key, name every feature
// that is listed and not built, print a table -- and the only mistake available
// is putting something in start-up that belongs in here.
//
// IT IS THE SECOND COMMAND OF THAT SHAPE, WHICH MAKES IT A PATTERN.
// SATELLITE_ARGUMENTS Phase 8 has `satl --config`: a thread probe that takes 9.6
// seconds and is "unthinkable at every startup and nothing at all once per
// machine". Same argument, same answer. `--config` measures what the MACHINE can
// do; `--rebuild` composes what the PERSON asked for. Whether they should be one
// command is SATELLITE_ERROR red note 13.
//
// WHAT IT READS AND WHAT IT WRITES. config.ini keeps BOTH forms, on purpose:
//
//     access = true            <- the person's, readable, editable by hand
//     history = false             and written by a running program
//     ...
//     features = b00000000000001  <- composed from them; the ONE value start-up reads
//
// The named keys are the truth a person edits. `features` is the truth the
// interpreter reads, and it is DERIVED -- which is why it can be regenerated at
// any time and why editing it by hand is pointless rather than dangerous.
//
// SO A SETTING CHANGED BY A PROGRAM DOES NOT TAKE EFFECT UNTIL --rebuild RUNS.
// That is a real consequence of the author's design and it is stated here rather
// than discovered: `arguments.access = satellite.bool.false` writes `access =
// false`, and the next run still reads the OLD `features` until somebody rebuilds.
// start_register() below is what keeps that from being a trap -- it notices the
// two disagreeing and says so.

#include "config_file.hpp"
#include "feature_register.hpp"
#include "../machine/critical_report.hpp"
#include "../machine/machine_codes.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace satellite004 {

// Compose the register from the named keys, each falling back to its default.
inline FeatureRegister compose_from_settings()
{
    FeatureRegister made = FeatureRegister::defaults();
    for (unsigned i = 0; i < kFeatureCount; ++i) {
        bool said = feature_facts()[i].on_by_default;
        if (config_file::read_flag(feature_facts()[i].name, said))
            made.set(static_cast<Feature>(i), said);
    }
    return made;
}

// WHAT START-UP DOES, AND IT IS ONE READ.
//
// THREE ANSWERS AND NOT TWO, which is SATELLITE_ERROR Part 7's rule 3 applied to
// this file: "a section that failed to gather prints as that, never as empty".
// A register that is ABSENT and a register that is THERE AND UNREADABLE both end
// up using the defaults, and they are not the same thing -- the first is a fresh
// install, the second is a damaged file somebody should be told about. Saying
// "no register saved" for both would be the quiet kind of wrong.
struct RegisterReading {
    FeatureRegister features;
    bool found = false;        // a readable register was in the file
    bool unreadable = false;   // the key was there and could not be read
    bool disagrees = false;    // it does not match what the named keys compose to
    std::string said;          // what the file actually held, when unreadable
};

inline RegisterReading start_register()
{
    RegisterReading reading;
    reading.features = FeatureRegister::defaults();

    std::string said;
    if (!config_file::read_value(kRegisterKey, said))
        return reading;                       // absent: a fresh install

    FeatureRegister stored;
    if (!read_written(said, stored)) {
        reading.unreadable = true;            // present and damaged
        reading.said = said;
        return reading;
    }

    reading.features = stored;
    reading.found = true;
    reading.disagrees = compose_from_settings().bits != stored.bits;
    return reading;
}

// The table --rebuild prints. Verbose on purpose: this runs once, and a person
// running it is asking exactly this question.
inline std::string register_table(const FeatureRegister &reg)
{
    std::string out;
    out += "    bit  feature              state     built  what it does\n";
    out += "    ---  -------------------  --------  -----  ------------------------------------\n";
    for (unsigned i = 0; i < kFeatureCount; ++i) {
        const FeatureFact &fact = feature_facts()[i];
        std::string row = "    ";
        const std::string number = std::to_string(i);
        row += std::string(3 - (number.size() < 3 ? number.size() : 3), ' ') + number + "  ";
        std::string name = fact.name;
        name.resize(19, ' ');
        row += name + "  ";
        std::string state = reg.on(static_cast<Feature>(i)) ? "ON" : "off";
        state.resize(8, ' ');
        row += state + "  ";
        row += fact.built ? "yes    " : "NO     ";
        row += fact.what;
        out += row + "\n";
    }
    return out;
}

// `satl --rebuild`. Answers a machine code.
inline signed long long int run_rebuild()
{
    const FeatureRegister made = compose_from_settings();
    const std::string value = written(made);

    std::cout << "satl --rebuild: composing every setting into one binary\n\n";
    std::cout << register_table(made);

    // EVERY FEATURE THAT IS LISTED AND NOT BUILT IS NAMED, and this is the kind
    // of work that only belongs in a once-per-machine step. A person who turns
    // on `trace` and sees nothing happen has no way to tell a broken feature
    // from an unbuilt one; saying it here costs a run nothing.
    std::vector<std::string> asked_for_but_unbuilt;
    for (unsigned i = 0; i < kFeatureCount; ++i)
        if (made.on(static_cast<Feature>(i)) && !feature_facts()[i].built)
            asked_for_but_unbuilt.push_back(feature_facts()[i].name);

    std::cout << "\n    " << kRegisterKey << " = " << value << "\n";
    std::cout << "    " << kFeatureCount << " features, "
              << (made.any() ? "some ON" : "all off") << "\n";

    if (!asked_for_but_unbuilt.empty()) {
        std::cout << "\n    turned on but NOT BUILT YET, so nothing will happen:\n";
        for (const std::string &name : asked_for_but_unbuilt)
            std::cout << "        " << name << "\n";
        std::cout << "    SATELLITE_ERROR.md Parts 8 and 12 are the milestones that owe them.\n";
    }

    std::string why;
    const signed long long int wrote = config_file::write_value(kRegisterKey, value, why);
    if (wrote != success) {
        CriticalReport failed;
        failed.code = "S0722";
        failed.name = "REGISTER_NOT_WRITTEN";
        failed.description =
            "satl --rebuild composed the feature register and could not save it, so the next run "
            "will read whatever was there before. Every setting still has its built-in default, so "
            "nothing is broken -- but nothing was changed either.";
        failed.directory = config_file::path().empty() ? std::string("$HOME is not set, so there is no ~/.satl")
                                                       : config_file::path();
        if (!why.empty())
            failed.notes.push_back(why);
        print_critical(failed);
        return wrote;
    }

    std::cout << "\n    written to " << config_file::path() << "\n";
    std::cout << "    every run from now reads that one value.\n";
    return success;
}

} // namespace satellite004
