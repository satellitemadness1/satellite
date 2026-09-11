// The one arguments object a run builds: the command line, and the directory
// it was started in. See satellite_arguments/arguments.hpp for the seam.

#include "satellite_arguments/arguments.hpp"

#include "satellite_string/satellite_string.hpp"
#include "satellite_value/value_arguments.hpp"
#include "system_facts/facts.hpp"

#include <string>
#include <utility>
#include <vector>

namespace satellite::arguments {

namespace {

// THE OBJECT ITSELF, AND IT IS A `Value` RATHER THAN AN `Arg`. What every
// reader wants is something to hand back, and a `Value` holding the handle is
// sixteen bytes plus a tag -- so building it once here saves every `display`
// and every subscript from making one. It is written exactly once, by start(),
// before any thread but the main one exists.
Value &held()
{
    static Value one;
    return one;
}

} // namespace

void start(const std::vector<std::string> &words)
{
    Arguments body;
    body.words.reserve(words.size());
    for (size_t i = 0; i < words.size(); i++) {
        // `program` AND `argument_1`..., WHICH IS v1's ANSWER KEPT. argv[0] is
        // the script, so it gets the name a shell would call it by; the rest
        // are numbered from 1 so that `argument_1` IS `arguments[1]` and a
        // reader never has to work out whether a name is off by one.
        std::string name = i == 0 ? std::string("program")
                                  : "argument_" + std::to_string(i);
        body.words.push_back(
            CommandLineWord{std::move(name),
                            Value::string(encode_raw(words[i]))});
    }

    // THE ONE FACT THAT CANNOT WAIT. `satellite.directory.change` `1 18 1` has
    // been built since M19, so a program that changes directory and then reads
    // `arguments.session.directory` would get two different answers from an
    // eager build and a lazy one. The arguments object is what the program was
    // STARTED with -- DESIGN §7.7's "the machine it woke up on" -- so this is
    // read here, with the command line, and not with the deferred thirty-two.
    body.directory = facts::cwd();

    held() = Value(std::make_shared<const Arguments>(std::move(body)));
}

const Value &object()
{
    return held();
}

} // namespace satellite::arguments
