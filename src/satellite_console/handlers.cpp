// `satellite.console.display` `1 5 1`, behind the table. See
// satellite_console/handlers.hpp for why this file is the join between two
// modules that must not know about each other.

#include "satellite_console/handlers.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_console/console.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::console {

namespace {

// `satellite.console.display(x)` `1 5 1`.
//
// IT TAKES ANY VALUE AND NOT A STRING, which is DESIGN §7.7 read from the other
// end: "Displaying it bare prints all of it" is said there about `arguments`,
// which is a `satellite.container.list` and not a string, so `display` was
// never a function of one type. `satellite_value/render.hpp` is the one place a
// value becomes characters and its own comment has said since M9 that "M10's
// `satellite.console.display` will print through it".
//
// SO THERE IS NO CONVERSION HERE AND NO REFUSAL EITHER. A number prints as its
// digits, a string as its text with DESIGN §5's six live codes answered, a bool
// as `true` or `false`, and the runtime singleton as `satellite`. That is
// rendering rather than conversion -- the language still has none, and
// `"x" + 1` is as much an error after this milestone as before it.
//
// IT ANSWERS `nothing`, WHICH IS A VALUE AND NOT A GAP. Every expression op
// leaves exactly one value (evaluator/machine.hpp's contract), and a statement
// that prints has nothing to say afterwards; `op_expression` drops it. Since
// M12 a program can ask what it got (DESIGN §8.7).
bool display(eval::Machine &, const Value *arguments, uint32_t, Value *answer)
{
    Console::the().display(text_of(arguments[0]));
    *answer = Value::nothing();
    return true;
}

} // namespace

void install_handlers()
{
    eval::Handlers::table().install(
        static_cast<words::PathId>(words::NodeId::CONSOLE_DISPLAY),
        // NOT A RECEIVER, WHICH IS THE TAG DOING ITS JOB RATHER THAN BEING
        // SPARE. DESIGN §6.4 qualification 2's flag says whether argument 0 is
        // the thing the call was written ON; `satellite.console.display("x")`
        // is a path called with one written argument and `console` is a
        // namespace rather than a value, so it is false. The first true one is
        // a method on a value -- `satellite.variable.file.new` `1 6 2 1` at
        // M19, which WORD_NUMBERS §4 pairs with `satellite.file.new` `1 8 1`.
        eval::Handler{display, false, 1, "M10"});
}

} // namespace satellite::console
