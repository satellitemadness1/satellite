// The console's nine rows behind the table -- `display` `1 5 1` since M10,
// and the other eight children of `console` since M14, which finished the
// namespace: §2.2 has no tenth child. See satellite_console/handlers.hpp for
// why this file is the join between two modules that must not know about
// each other.

#include "satellite_console/handlers.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_console/console.hpp"
#include "satellite_console/reader.hpp"
#include "satellite_string/satellite_string.hpp"
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
bool display(eval::Machine &m, const Value *arguments, uint32_t, Value *answer)
{
    // `end=` -- M30's first named option, and v1's `display(text, end="")`
    // back under the grammar that dropped it. What is written after the text in
    // place of the newline, as ONE queued unit: text and ending queued
    // separately would be two units, and DESIGN §10.1's atomicity is a
    // property of the unit (Console::display says so). `Console::write` has
    // been waiting for this spelling since M10.
    if (const Value *end = m.option("end")) {
        Console::the().write(text_of(arguments[0]) + text_of(*end));
    } else {
        Console::the().display(text_of(arguments[0]));
    }
    *answer = Value::nothing();
    return true;
}

// The named options display takes -- M30. Colour, style and position join
// this list as they are built; the dispatch refuses any name not on it.
const char *const kDisplayOptions[] = {"end", nullptr};

// The read every `input` shape shares -- "ask, and wait, in three shapes",
// one place that prompts, one place that drains, one place that reads (v1's
// own consolidation, kept). The prompt goes out UN-NEWLINED and as ONE piece,
// so another thread's line cannot tear it; the drain is the barrier DESIGN
// §10.1 keeps for exactly this call -- "a prompt written with no trailing
// newline" is queued, not printed, and a read that did not wait for it would
// block on an empty-looking terminal while the prompt sat behind it.
//
// THREE ANSWERS, NOT ONE EMPTY STRING. A line -- the empty line included,
// return pressed IS a line -- answers a string. The end of input refuses,
// S1001, loud rather than empty forever. And a Ctrl-C answers NOTHING and
// lets the walk stop itself at the next statement boundary with S0730's own
// caret -- the same shape `satellite.time.sleep` takes, one mechanism for
// every blocked wait, and `eof()`'s old job -- telling a closed stdin from an
// interrupted read -- is done by the reader's queue instead: the two arrive
// as different answers and cannot be confused (§6's do-not-rediscover
// regression, retired by construction).
bool ask(eval::Machine &m, const std::string &prompt, Value *answer)
{
    Console &out = Console::the();
    if (!prompt.empty())
        out.write(prompt);
    out.drain();

    std::string line;
    switch (Reader::the().read_line(&line, m.policy().interrupted)) {
    case Reader::Got::line:
        *answer = Value::string(encode_raw(line));
        return true;
    case Reader::Got::interrupted:
        *answer = Value::nothing();
        return true;
    case Reader::Got::end:
        break;
    }
    m.refuse(errors::make<errors::Code::CONSOLE_END_OF_INPUT>(
        m.span_of(m.here()), std::string(m.text_of(m.here()))));
    return false;
}

// `satellite.console.input()` `1 5 2` -- ask with nothing to say first.
bool input_bare(eval::Machine &m, const Value *, uint32_t, Value *answer)
{
    return ask(m, std::string(), answer);
}

// `satellite.console.input(prompt)` `1 5 3`. The prompt is ANY value,
// rendered the way `display` renders it -- a number can prompt.
bool input_prompt(eval::Machine &m, const Value *arguments, uint32_t,
                  Value *answer)
{
    return ask(m, text_of(arguments[0]), answer);
}

// `satellite.console.input(prompt, target)` `1 5 4` -- the same ask, and the
// WRITE IS NOT HERE. words.def declares argument 1 a place, the compiler
// resolved it to a slot and refused every misuse before this could run, and
// op_place writes what this answers -- skipping the write when the answer is
// nothing, which is the interrupt contract. So the handler's arity is ONE:
// the place never reaches it, and this row and `1 5 3` differ only in what
// the machine does with what they answer.
bool input_into(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    return ask(m, text_of(arguments[0]), answer);
}

// `satellite.console.typed()` `1 5 5` -- a line, or nothing, immediately.
// THE ONE GENUINELY NEW MECHANISM (PLAN M14): v1 has no non-blocking input
// of any kind, and M12 is what made *nothing* a thing a program can ask
// about. An empty line is a line; nobody typing is nothing; a closed stdin
// is nobody forever. No drain and no wait -- the reader's queue answers from
// whatever has already arrived whole.
bool typed(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    std::string line;
    if (Reader::the().typed_line(&line))
        *answer = Value::string(encode_raw(line));
    else
        *answer = Value::nothing();
    return true;
}

// `satellite.console.width` `1 5 6` and `.height` `1 5 7` -- property-shaped
// because they are FACTS, asked fresh rather than sampled: a terminal
// resizes during a run, which is what a startup-sampled object could not
// express and why these are not `arguments.machine.*` (PLAN M14 carries the
// argument in full).
bool width(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::number(Number(Console::the().width()));
    return true;
}

bool height(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::number(Number(Console::the().height()));
    return true;
}

// `satellite.console.clear()` `1 5 8` and `.home()` `1 5 9` -- call-shaped
// because they are actions, and BOTH GO THROUGH THE PRINTER'S QUEUE: v1
// wrote them as one escape straight to the fd, which is a frame shredded
// between two queued lines, and done-when clause 8 is the tight loop that
// proves the fix. `clear()` clears AND homes -- v1's byte sequence kept
// whole, terminfo's own meaning for `clear`, and the author's word of
// 2026-09-04 -- while `home()` alone is the flicker-free frame repaint QUAD's
// draw loop wants: home and overdraw, no erase, one call.
bool clear(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    Console::the().write("\033[H\033[2J");
    *answer = Value::nothing();
    return true;
}

bool home(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    Console::the().write("\033[H");
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
        eval::Handler{display, false, 1, "M10", false, kDisplayOptions});

    // M14's eight, finishing the namespace. None binds a receiver --
    // `console` is a namespace, not a value -- and `1 5 4`'s arity is ONE
    // because the place never reaches its handler; op_place owns the write.
    const auto row = [](words::NodeId id, eval::HandlerFn fn, uint32_t arity) {
        eval::Handlers::table().install(static_cast<words::PathId>(id),
                                        eval::Handler{fn, false, arity, "M14"});
    };
    row(words::NodeId::CONSOLE_INPUT_0, input_bare, 0);
    row(words::NodeId::CONSOLE_INPUT_PROMPT, input_prompt, 1);
    row(words::NodeId::CONSOLE_INPUT_PROMPT_TARGET, input_into, 1);
    row(words::NodeId::CONSOLE_TYPED_0, typed, 0);
    row(words::NodeId::CONSOLE_WIDTH, width, 0);
    row(words::NodeId::CONSOLE_HEIGHT, height, 0);
    row(words::NodeId::CONSOLE_CLEAR_0, clear, 0);
    row(words::NodeId::CONSOLE_HOME_0, home, 0);
}

} // namespace satellite::console
