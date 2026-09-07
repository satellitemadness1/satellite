// One prompt session. See satellite_prompt/session.hpp.

#include "satellite_prompt/session.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/machine.hpp"
#include "satellite_console/console.hpp"
#include "satellite_prompt/block.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/interrupt.hpp"

#include <cstdio>
#include <vector>

namespace satellite::prompt {

namespace {

// The name a diagnostic from a typed line is headed with. NOT A PATH, and
// deliberately in angle brackets: `errors::Source::path` is printed as though
// it were a file, and a bare word there would read as a file in the working
// directory that the user could go and look at.
constexpr const char *kPromptName = "<prompt>";

int find_main(const Built &built)
{
    return built.program.find(static_cast<words::PathId>(words::NodeId::MAIN));
}

// Move a diagnostic back onto the line the person typed.
//
// `above` IS COUNTED AND NOT ASSUMED, which is the correction that matters once
// a session has declared anything. The prologue is three lines only while no
// top-level form has been kept; each form that has been adds its own lines
// between the include and the capsule, so a constant here would drift by
// exactly as much as the user had declared -- and the drift would grow, which
// is the worst shape for a wrong number because it looks right at first.
//
// CLAMPED AT 1 AND NOT AT 0, because Span::somewhere() IS `line != 0` -- a
// rebase that produced 0 would not move a caret, it would delete one, and the
// diagnostic would print with no place at all.
void rebase(errors::Span &at, int above)
{
    if (at.line == 0)
        return;
    const int moved = static_cast<int>(at.line) - above;
    at.line = static_cast<uint32_t>(moved > 0 ? moved : 1);
}

void rebase_all(std::vector<errors::Diagnostic> &problems, int above)
{
    for (errors::Diagnostic &problem : problems) {
        rebase(problem.at, above);
        for (errors::Note &note : problem.notes)
            rebase(note.at, above);
    }
}

// Lines in a block of text, counting a final line with no newline on it.
int lines_in(const std::string &text)
{
    if (text.empty())
        return 0;
    int lines = 1;
    for (const char c : text)
        if (c == '\n')
            lines++;
    return lines;
}

} // namespace

std::string Session::wrap(const std::string &body) const
{
    // THE PROLOGUE IS EXACTLY kPrologueLines LINES AND THAT IS LOAD-BEARING.
    // rebase() subtracts that number from every diagnostic, so a line added
    // here without changing it moves every caret the prompt prints.
    std::string out = "satellite.include(satellite)\n";
    for (const std::string &form : top_level_) {
        out += form;
        out += '\n';
    }
    out += "satellite.capsule satellite.main()\n{\n";
    out += body;
    out += "\n}\n";

    // The accumulated forms sit BETWEEN the include and the capsule, so they do
    // not shift the body when there are none -- and they do when there are, by
    // exactly as many lines as they occupy. That is why the prologue count is a
    // constant only while `top_level_` is empty, and why the rebase below is
    // corrected by its size.
    return out;
}

void Session::report(Built &built, const std::string &name, int above) const
{
    const errors::Source against{name, built.text, &built.words};

    rebase_all(built.parsed.errors, above);
    rebase_all(built.resolved.problems, above);
    rebase_all(built.program.problems, above);

    if (!built.parsed.errors.empty())
        fputs(errors::render(built.parsed.errors, against).c_str(), stderr);
    else if (!built.resolved.problems.empty())
        fputs(errors::render(built.resolved.problems, against).c_str(), stderr);
    else if (!built.program.problems.empty())
        fputs(errors::render(built.program.problems, against).c_str(), stderr);
}

bool Session::run(const std::string &entry)
{
    const Scan scanned = scan(entry);
    if (scanned.empty)
        return true;

    // A TOP-LEVEL FORM IS KEPT AND RUN, NOT KEPT AND SKIPPED. Running it is what
    // reports a mistake in it now rather than on the next line, when the user
    // has moved on and the caret would point at something they are no longer
    // looking at.
    const bool top = scanned.placement == Placement::TopLevel;

    // HOW MANY LINES SIT ABOVE WHAT WAS TYPED, computed before the entry joins
    // them so that both halves are measured the same way. A top-level form goes
    // after the include and after every form kept before it; a statement goes
    // after all of those AND after the capsule's two lines.
    int kept = 0;
    for (const std::string &form : top_level_)
        kept += lines_in(form);
    const int above = top ? 1 + kept : kPrologueLines + kept;

    if (top)
        top_level_.push_back(entry);

    Built built;
    built.text = top ? wrap(std::string()) : wrap(entry);

    if (!build_source(kPromptName, built, false)) {
        report(built, kPromptName, above);
        // A FORM THAT DID NOT BUILD DOES NOT STAY. Keeping it would poison every
        // later line with the same error, and the user would have no way to take
        // it back -- there is no editor here, only a prompt.
        if (top)
            top_level_.pop_back();
        return true;
    }

    const int which = find_main(built);
    if (which < 0)
        return true;

    clear_interrupt();

    eval::Machine machine(built.program.closures, built.parsed.ast,
                          policy_from_the_limits());
    machine.run_top_level();
    if (machine.ok())
        machine.call(static_cast<uint32_t>(which), std::vector<Value>{});

    // DRAIN AND NOT SHUTDOWN, WHICH IS THE WHOLE DIFFERENCE BETWEEN A RUN AND A
    // SESSION. `satl file.satl` shuts the console down because the process is
    // about to end; here another line is coming, and shutdown would join the
    // printer thread and start a new one for every line typed. Draining gives
    // the same guarantee that matters -- everything printed is on the terminal
    // before the next prompt is drawn -- at no cost.
    console::Console::the().drain();

    if (!machine.ok())
        fputs(errors::render(machine.problems(),
                             errors::Source{kPromptName, built.text,
                                            &built.words})
                  .c_str(),
              stderr);
    return true;
}

bool Session::run_file(const std::string &path)
{
    Built built;
    if (!build_program(path, built))
        return true;

    const int which = find_main(built);
    if (which < 0) {
        fputs(errors::render(
                  errors::make<errors::Code::FILE_NO_MAIN>(errors::kNowhere),
                  errors::Source{path, built.text, &built.words})
                  .c_str(),
              stderr);
        return true;
    }

    clear_interrupt();

    const eval::Capsule &main = built.program.closures.capsules()[which];
    std::vector<Value> arguments;
    if (main.parameters != 0)
        arguments.push_back(Value::list(List{}));

    eval::Machine machine(built.program.closures, built.parsed.ast,
                          policy_from_the_limits());
    machine.run_top_level();
    if (machine.ok())
        machine.call(static_cast<uint32_t>(which), arguments);

    console::Console::the().drain();

    if (!machine.ok())
        fputs(errors::render(machine.problems(),
                             errors::Source{path, built.text, &built.words})
                  .c_str(),
              stderr);
    return true;
}

} // namespace satellite::prompt
