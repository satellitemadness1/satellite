// satellite.<module>.* — the module function surface.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

#include <iostream>

#include "console_output/console.hpp"
#include "random_numbers/random.hpp"

namespace satellite {

// call_module was 771 lines in one function. It is now a dispatcher, and each
// arm lives in its own file beside this one:
//
//   modules_file.cpp        satellite.time.now, satellite.file.*
//   modules_help.cpp        satellite.help, bare module constants, satellite.analyze
//   modules_system.cpp      satellite.system.*
//   modules_directory.cpp   satellite.directory.*
//   modules_random.cpp      satellite.random.*
//   modules_console.cpp     satellite.console.display / .input
//
// ORDER IS PRESERVED EXACTLY. The arms are tried in the order the branches ran
// in before the split, which matters because the arms are not disjoint: the
// bare-module-constant arm in modules_help.cpp matches `path.size() == 2` and
// would swallow paths a later arm wants if it were tried earlier.
ValuePtr Evaluator::call_module(const std::vector<std::string> &path,
                                const std::vector<ValuePtr> &argv, Span span)
{
    const std::string full = join_path(path);

    if (auto r = module_time_and_file(full, path, argv, span))   return *r;
    if (auto r = module_help_and_analyze(full, path, argv, span)) return *r;
    if (auto r = module_system(full, path, argv, span))           return *r;
    if (auto r = module_directory(full, path, argv, span))        return *r;
    if (auto r = module_random(full, path, argv, span))           return *r;
    if (auto r = module_console(full, path, argv, span))          return *r;

    fail(span, "no such module function: " + full);
    return nullptr;
}

// Reads one line from standard input, after making sure everything already
// displayed has actually reached the terminal.
//
// THE DRAIN IS THE WHOLE REASON THIS IS A FUNCTION. Output goes through the
// Console's printer thread, so a prompt written with
// satellite.console.display(">>", end="") is QUEUED rather than printed, and a
// read that did not wait for it would block on an empty-looking terminal while
// the prompt sat behind it. drain() is exactly the "everything queued is
// written AND flushed" barrier console.hpp already provides for the error
// report, and this is the second caller with the same need.
//
// Returns false at end of input, which is a real condition and not a failure of
// this function: a program run with its stdin closed reaches it immediately, and
// the caller decides what that means.
bool Evaluator::read_input_line(std::string &line)
{
    if (console_)
        console_->drain();
    else
        std::cout.flush();

    return static_cast<bool>(std::getline(std::cin, line));
}

// satellite.console.input's value, for both of its shapes.
//
// The prompt is displayed with NO trailing newline, because a prompt whose
// cursor sits on the line below it is not a prompt. That makes this the second
// caller of the same unnewlined write display(text, end="") uses, and both go
// through emit() as ONE piece so a prompt cannot be torn in half.
ValuePtr Evaluator::console_input(const std::string &prompt, Span span)
{
    if (!prompt.empty())
        emit(prompt);

    std::string line;
    if (!read_input_line(line)) {
        // Loud rather than an empty string. An empty line and no line at all
        // are different answers — the first is the user pressing return, the
        // second is there being nobody there — and a program that cannot tell
        // them apart loops forever on a closed stdin.
        fail(span, "satellite.console.input reached the end of input");
        return nullptr;
    }

    return make_value(encode_raw(line));
}

// satellite.console.input(prompt, target) — read a line, write it into
// `target`.
//
// THE ONLY OUT PARAMETER IN THE LANGUAGE, and it is deliberately not the
// beginning of a general facility: no user capsule can declare one, because
// nothing in a capsule's parameter list can say "this one is written back".
// This exists because it is the shape a program reaches for when it asks for a
// line, and because the value form alone would make
// `satellite.console.input("", answer)` a mystery rather than a mistake.
//
// The value form is the one to prefer and this is written in terms of it, so
// there is one place that prompts, one place that drains and one place that
// reads.
ValuePtr Evaluator::console_input_into(const Expr &prompt, const Expr &target,
                                       Span span)
{
    // The PLACE first, before the prompt is printed and before anything is
    // read: a target that cannot be written to is a mistake in the program, and
    // discovering it after the user has already typed an answer would throw
    // that answer away.
    Slot slot = slot_of(target);
    if (!slot.valid) {
        fail(target.span, "satellite.console.input writes its answer into its "
                          "second argument, so that argument has to be a "
                          "variable");
        return nullptr;
    }

    ValuePtr text = eval(prompt);
    if (failed() || !text)
        return nullptr;

    ValuePtr line = console_input(to_string(*text), span);
    if (failed() || !line)
        return nullptr;

    // The declared type is checked the same way an assignment's is, and with
    // the same message, because this IS an assignment — it just gets its value
    // from the terminal. A line is always a string, so this fires whenever the
    // target is not one.
    const Type *declared = declared_type(slot);
    if (declared && !matches(*declared, *line)) {
        fail(target.span, "cannot assign " + to_string(*line) + " to " +
                          unparse(*declared) + " " + slot.name);
        return nullptr;
    }

    if (!write_slot(slot, line, target.span))
        return nullptr;

    // Nil, not the line. The out-parameter form is used as a statement, and
    // handing back the value as well would give one call two ways to be read.
    return make_value(std::monostate{});
}

// One message for every wrong use of a named argument, so the reader learns the
// whole rule from any of them. Named after duration_misuse, which exists for
// exactly the same reason and is written the same way: name the form that
// works rather than only refusing the one that does not.
std::string named_arg_misuse(const std::string &name)
{
    return "named arguments are not part of the language; " + name +
           "= is understood only as satellite.console.display(text, end=\"\")"
           ", which displays text with the ending given instead of a newline";
}

// satellite.console.display(text, end=<expr>).
//
// The ending is a VALUE and not a flag, so end="" is a prompt, end=" " puts two
// displays on one line separated by a space, and end="\n" is the default
// spelled out. A boolean `newline=false` could not do the middle one.
ValuePtr Evaluator::display_with_end(const Expr &text, const Expr &ending,
                                     Span span)
{
    (void)span;

    // Left to right, because both can be calls that display something of their
    // own and the order is observable.
    ValuePtr value = eval(text);
    if (failed() || !value)
        return nullptr;
    ValuePtr tail = eval(ending);
    if (failed() || !tail)
        return nullptr;

    // ONE emit, for the reason the plain display arm gives: the unit handed to
    // the Console is the unit another thread cannot tear in half. This one is
    // not a whole line, and that is the caller's choice — but it is still one
    // piece, so a prompt cannot arrive split around another thread's output.
    emit(to_string(*value) + to_string(*tail));
    return make_value(std::monostate{});
}

// satellite.console.display(100ms) — a setting, not a display.
//
// What it buys is stated in console.hpp: the pause is taken by the printer
// thread, so the program does not wait for it. The walk carries on building the
// next line while the previous one is still being spaced out on the terminal,
// and the queue between them is what absorbs the difference.
ValuePtr Evaluator::set_display_pace(const DurationLit &node, Span span)
{
    // floor() first: the pace is nanoseconds and a fraction of one is not a
    // wait anybody can observe, so 0.0000001ms is zero rather than an error.
    long long ns = 0;
    if (!node.nanoseconds.floor().to_integer(ns) || ns < 0) {
        fail(span, "a pace of " + node.text +
                   " is not a length of time this can wait");
        return nullptr;
    }

    // Null with no Console attached, which is every test that reads output()
    // back: there is no printer thread there, and so nothing between two lines
    // to pause. Accepted rather than refused, because the same source has to
    // run under `satl --run` where it does pace.
    if (!console_)
        return make_value(std::monostate{});

    // At the PROMPT, say what changed. A setting whose entire effect is a delay
    // between two future lines is invisible at the moment it is made, and the
    // report it earned was "I entered satellite.console.display(100ms) and it
    // didn't work" — from a session where it had worked and had nothing to show
    // for itself.
    //
    // `echo_` is the REPL's own flag, the same one that makes `x` print 1, so a
    // program run with --run stays silent.
    //
    // AFTER the new pace is applied, which is only safe because the pace is a
    // minimum gap: the seconds spent typing the command count toward it, so the
    // acknowledgement of a 1500ms pace still appears at once. Emitting it first
    // instead would stamp it with the pace being replaced, and the one line
    // that must never be slow — `display(0ms)`, the way out — would be the one
    // line still paying the old pace.
    console_->pace(ns);

    if (echo_)
        emit(ns > 0 ? "display paced: " + node.text + " between lines\n"
                    : "display paced: off\n");

    // Nil, like the display it is spelled as. `0ms` is how a program turns the
    // pacing back off, and it needs no second word to do it.
    return make_value(std::monostate{});
}

// ---------------------------------------------------------------------------
// Capsule calls
// ---------------------------------------------------------------------------

} // namespace satellite
