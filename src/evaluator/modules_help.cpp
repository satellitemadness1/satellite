#include "evaluator/eval_internal.hpp"

#include <iostream>
#include <optional>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

// satellite.help, the bare module constants, and satellite.analyze
//
// One arm of Evaluator::call_module, which was 771 lines in a single function
// before the 2026-08-24 split. The branch bodies below are UNCHANGED -- they
// were moved, not rewritten.
//
// The return type is what makes that possible. `std::nullopt` means "not mine,
// keep looking"; an ENGAGED optional means this arm handled the call, and the
// ValuePtr inside may still be null because a null return is how a failed call
// reports itself after fail() has run (§8.3.1's rule that a failure is a value).
// So every `return <expr>;` in the moved code converts to an engaged optional on
// its own and needed no edit at all.
//
// Part of src/evaluator/modules.cpp -- see eval_internal.hpp for why an
// anonymous namespace could not simply be split.

namespace satellite {

std::optional<ValuePtr> Evaluator::module_help_and_analyze(
    const std::string &full, const std::vector<std::string> &path,
    const std::vector<ValuePtr> &argv, Span span)
{
    // the interpreter has no way to tell it anything.
    if (full == "satellite.help") {
        if (argv.empty())
            return make_value(encode_raw(help_overview()));
        if (argv.size() == 1) {
            // A STRING that names a topic is a request for that topic, and
            // only then. Every other string still answers with what a
            // satellite.variable.string can do, so nothing that worked before
            // this line answers differently -- the split is on the CONTENT,
            // and the set of contents it fires for is exactly the topic table.
            if (const SatString *named = as_string(*argv[0])) {
                const std::string topic = help_for_topic(decode(*named));
                if (!topic.empty())
                    return make_value(encode_raw(topic));
            }
            return make_value(encode_raw(help_for(*argv[0])));
        }
        fail(span, arity_message("satellite", "help", 1, argv.size()));
        return nullptr;
    }

    // satellite.directory() -- a module asked what it answers to. Two segments
    // and no third, which today was "no such module function: satellite.
    // directory": an answer that is true and useless, since the reason to type
    // it is not knowing the third segment yet. The bare form without
    // parentheses is handled beside satellite.help in helpers.cpp, so both
    // spellings work for the same reason help's do.
    if (path.size() == 2 && path[0] == "satellite") {
        const std::string listing = help_for_module(path[1]);
        if (!listing.empty()) {
            if (!argv.empty()) {
                fail(span, "satellite." + path[1] +
                           " takes no arguments -- it names a module, and "
                           "answers with what that module can do");
                return nullptr;
            }
            return make_value(encode_raw(listing));
        }
    }

    // satellite.analyze(path) -- what is in a spaceship. Directly under
    // satellite rather than under a module, because the subject is a FILE of
    // the language rather than any one module's business: satellite.help's
    // shape, not satellite.directory's. The walk itself is in analyze.cpp.
    if (full == "satellite.analyze") {
        if (argv.size() != 1) {
            fail(span, arity_message("satellite", "analyze", 1, argv.size()));
            return nullptr;
        }
        const SatString *where = as_string(*argv[0]);
        if (!where) {
            fail(span, "satellite.analyze wants a satellite.variable.string, "
                       "got " + to_string(*argv[0]));
            return nullptr;
        }
        const std::string file = decode(*where);
        std::string error;
        const std::string report = analyze_spaceship(file, error);
        if (!error.empty()) {
            // Loud, for satellite.directory.list's reason: the report is a
            // value with no room in it for "there was no file", and an empty
            // one would read as a spaceship that declares nothing.
            fail(span, "satellite.analyze " + error + " \"" + file + "\"");
            return nullptr;
        }
        return make_value(encode_raw(report));
    }

    return std::nullopt;
}

} // namespace satellite
