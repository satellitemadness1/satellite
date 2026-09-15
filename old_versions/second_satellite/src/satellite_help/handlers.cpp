// The three rows behind `satellite.help`. See satellite_help/handlers.hpp for
// why this file is a join rather than part of either module it needs.

#include "satellite_help/handlers.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_console/console.hpp"
#include "satellite_help/built.hpp"
#include "satellite_help/render.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::help {

namespace {

// WRITTEN AND NOT DISPLAYED, which is a one-word difference with a reason.
// `display` appends a newline because it prints ONE VALUE; an answer here is
// already a document, ends in a newline of its own, and a second one would put
// a blank line after every ask. The console's queue is the same either way --
// DESIGN §10.1's printer thread is what carries it -- so this is about the text
// and not about the mechanism.
void print(const std::string &text)
{
    console::Console::the().write(text);
}

// `satellite.help` `1 19` and `satellite.help()` `1 19 0`.
bool everything(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    print(answer_for(BuiltSet::now(), static_cast<words::PathId>(words::NodeId::SATELLITE)));
    *answer = Value::nothing();
    return true;
}

// `satellite.help(x)` `1 19 1`.
//
// THE ARGUMENT IS A PATH THE COMPILER ALREADY DECIDED, folded to a constant by
// Compiler::topic() -- so what arrives here is the canonical text of a node,
// never a value the program computed and never anything a person typed
// directly. That is what `1 19 1`'s row in words.def's fifth list buys, and it
// is why this function has no case for a wrong type: a wrong argument was
// refused at compile time with S1102 or S1103 and never reaches an op that
// dispatches.
bool about(eval::Machine &m, const Value *arguments, uint32_t, Value *answer)
{
    const std::string path = text_of(arguments[0]);
    const words::Walk found = words::walk(path);
    if (found.error != words::WalkError::NONE) {
        // UNREACHABLE THROUGH THE COMPILER, AND A REFUSAL RATHER THAN AN
        // ASSERT. What is folded in is `words::path_text` of a node, so it
        // walks back by construction -- but "by construction" is a property of
        // two files agreeing, and the day they stop agreeing a sentence is a
        // better answer than a crash.
        m.refuse(errors::make<errors::Code::HELP_NOT_A_TOPIC>(
            m.span_of(m.here()), "`" + path + "`"));
        return false;
    }

    const BuiltSet built = BuiltSet::now();
    if (!built.contains(found.id)) {
        // THE DONE-WHEN'S SECOND SELF-VERIFYING CHECK.
        // `satellite.help(satellite.network)` refuses in plain words rather
        // than printing seven shapes nobody has written -- and it now refuses
        // through the mechanism that is ABOUT being unbuilt, where before this
        // milestone it refused with S0721 because the argument itself died.
        m.refuse(errors::make<errors::Code::HELP_NOT_BUILT>(
            m.span_of(m.here()), query_text(found.id)));
        return false;
    }

    print(answer_for(built, found.id));
    *answer = Value::nothing();
    return true;
}

} // namespace

void install_handlers()
{
    const auto row = [](words::NodeId id, eval::HandlerFn fn, uint32_t arity) {
        eval::Handlers::table().install(static_cast<words::PathId>(id),
                                        eval::Handler{fn, false, arity, "M18"});
    };
    // NONE OF THEM BINDS A RECEIVER. `help` is a word under `satellite` and
    // `satellite` is the runtime rather than a value with methods, so DESIGN
    // §6.4 qualification 2's tag is false for all three -- the same answer
    // `satellite.console.display` gives, for the same reason.
    row(words::NodeId::HELP, everything, 0);
    row(words::NodeId::HELP_0, everything, 0);
    // ARITY ONE, AND THE ONE IS THE FOLDED PATH. The written argument and the
    // handler's argument are the same count here, which is what separates a
    // topic from a place: `input(prompt, target)`'s arity is one less than what
    // is written because the place never reaches its handler, and a topic does
    // reach it -- as a constant the compiler put there instead of as an
    // expression the machine ran.
    row(words::NodeId::HELP_X, about, 1);
}

} // namespace satellite::help
