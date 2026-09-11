// `satellite.system.home` `1 22 3` and `satellite.system.environment` `1 22 2`
// and `(name)` `1 22 9`. See satellite_system/host_methods.hpp.
//
// SPLIT FROM THE MEMORY VERBS BY SUBJECT. Everything in memory_methods.cpp is a
// quantity of bytes reported in a unit; nothing here is a quantity of anything.
// PLAN M20 asks for exactly this split, because v1 answered both out of one
// 321-line file.
//
// `environment` IS THE ONE PATH IN THIS MILESTONE THAT IS NOT A PORT. v1 never
// built it and said so in its own comment -- "environment in general is
// satellite.system.environment(name)'s job" -- so the shape was an open
// question until the author settled it on 2026-09-09: BARE ANSWERS A MAP of the
// whole environment, and `(name)` answers the one variable. M16 has maps, so
// the bare form costs nothing that did not already exist, and the pair is the
// same read-all / read-one shape `satellite.system.memory` carries.

#include "satellite_system/host_methods.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_containers/containers.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/facts.hpp"

#include <string>

extern char **environ;

namespace satellite::system {

namespace {

std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

// `satellite.system.home` `1 22 3` -- where the person running this lives.
//
// THE READER IS M6'S AND THIS IS ONLY THE WORD FOR IT. facts::home_dir() asks
// $HOME first and the password database when that is empty, because $HOME is
// both forgeable and absent under `env -i` while the password database is the
// authority. That argument lives at the reader and is not repeated here.
//
// SPELLED WITHOUT PARENTHESES, WHICH IS A DEPARTURE FROM v1 AND DELIBERATE.
// v1's registry writes `satellite.system.home()` at arity 0; `words.def`
// numbers `home` `1 22 3` as a plain word with no call shape under it, the same
// way `bit` and `frequency` are plain words where v1 wrote `bit()` and
// `frequency()`. All three moved together, so it is one decision rather than
// three omissions -- and a program ported from v1 drops the parentheses.
bool read_home(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::string(encode_raw(facts::home_dir()));
    return true;
}

// The whole environment, as a map.
//
// READ FROM `environ` RATHER THAN WITH getenv() IN A LOOP, because there is no
// list of names to loop over -- getenv answers a name and cannot enumerate.
// `environ` is the array POSIX gives for exactly this.
//
// A ROW WITH NO `=` IS SKIPPED. POSIX does not promise there is none, and a
// name with no value is not a variable a program can use; taking the whole
// string as a key with an empty value would invent an entry the environment
// does not have.
//
// A REPEATED NAME KEEPS THE FIRST, which is what getenv() does -- so the map
// and `environment(name)` cannot disagree about a duplicate. map_with()
// overwrites, so the check is here rather than left to it.
bool read_environment(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    MapBody body;
    for (char **row = environ; row && *row; row++) {
        const std::string entry = *row;
        const size_t equals = entry.find('=');
        if (equals == std::string::npos)
            continue;

        const std::string name = entry.substr(0, equals);
        if (body.index.find(name) != body.index.end())
            continue;               // the first wins, as getenv() does

        MapBody next;
        if (containers::map_with(body, Value::string(encode_raw(name)),
                                 Value::string(encode_raw(entry.substr(equals + 1))),
                                 next))
            body = std::move(next);
    }
    *answer = Value::map(std::move(body));
    return true;
}

// `satellite.system.environment(name)` `1 22 9` -- one variable.
//
// A NAME THAT IS NOT SET ANSWERS NOTHING, and that is M12's askable state doing
// the job it exists for. An empty string would be the wrong answer twice over:
// it is a real value a variable can legitimately hold, so it would make "unset"
// and "set to empty" indistinguishable -- and those are different facts that
// programs branch on.
bool read_environment_named(eval::Machine &m, const Value *arguments, uint32_t,
                            Value *answer)
{
    const Str *text = std::get_if<Str>(&arguments[0]);
    if (!text) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "a `satellite.variable.string` name", type_name(arguments[0])));
        return false;
    }

    const std::string name = *text ? decode(**text) : std::string();
    const char *value = name.empty() ? nullptr : getenv(name.c_str());
    *answer = value ? Value::string(encode_raw(value)) : Value::nothing();
    return true;
}

} // namespace

void install_host()
{
    using words::NodeId;

    eval::Handlers::table().install(
        static_cast<words::PathId>(NodeId::SYSTEM_HOME),
        {read_home, false, 0, "M20"});
    eval::Handlers::table().install(
        static_cast<words::PathId>(NodeId::SYSTEM_ENVIRONMENT),
        {read_environment, false, 0, "M20"});
    eval::Handlers::table().install(
        static_cast<words::PathId>(NodeId::SYSTEM_ENVIRONMENT_NAME),
        {read_environment_named, false, 1, "M20"});
}

} // namespace satellite::system
