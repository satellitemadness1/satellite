// One prompt session. See satellite_prompt/session.hpp.

#include "satellite_prompt/session.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/machine.hpp"
#include "satellite_console/console.hpp"
#include "satellite_prompt/block.hpp"
#include "name_resolver/resolve.hpp"
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
    out += "satellite.capsule satellite.main(" + parameters() + ")\n{\n";
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

std::string Session::parameters() const
{
    std::string out;
    for (const Kept &one : kept_) {
        if (!out.empty())
            out += ", ";
        out += one.type;
        out += ' ';
        out += one.name;
    }
    return out;
}

std::vector<Value> Session::arguments() const
{
    std::vector<Value> out;
    out.reserve(kept_.size());
    for (const Kept &one : kept_)
        out.push_back(one.value);
    return out;
}

void Session::keep_what_ran(const Built &built, const eval::Machine &machine,
                            int capsule)
{
    const std::vector<Value> &slots = machine.last_frame();
    if (slots.empty()) {
        // A CAPSULE WITH NO SLOTS AT ALL leaves nothing to keep and must not
        // clear what the session already had -- `satellite.console.display("x")`
        // on its own declares nothing, and forgetting every variable because a
        // line happened not to declare one would be the worst of both.
        return;
    }

    // The Frame that belongs to the capsule that ran. Found by its node rather
    // than by position, because resolve numbers frames in the order it MEETS
    // capsules and the wrapper is not always the first.
    const eval::Capsule &ran = built.program.closures.capsules()[capsule];
    const resolve::Frame *frame = nullptr;
    for (const resolve::Frame &one : built.resolved.frames) {
        if (one.node == ran.node) {
            frame = &one;
            break;
        }
    }
    if (frame == nullptr)
        return;

    std::vector<Kept> next;
    for (size_t slot = 0; slot < frame->names.size() && slot < slots.size();
         slot++) {
        // DESIGN §7.7's OBJECT IS NOT A VARIABLE AND IS SKIPPED. `arguments` is
        // the machine's answer rather than the program's storage, and writing
        // it into the next line's parameter list would declare a name the
        // resolver already routes somewhere else.
        if (frame->arguments == static_cast<resolve::Slot>(slot))
            continue;

        const words::PathId type =
            built.resolved.at(frame->types[slot]).path;
        // A SLOT WHOSE TYPE DID NOT RESOLVE CANNOT BE RE-DECLARED, so it is
        // dropped rather than guessed at -- there is no text to write in a
        // parameter list for it.
        if (type == words::kNoPath)
            continue;

        Kept one;
        one.name = std::string(frame->names[slot]);
        one.type = std::string(
            words::path_text(static_cast<words::NodeId>(type)));
        one.value = slots[slot];
        next.push_back(std::move(one));
    }
    kept_ = std::move(next);
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
    // A GLOBAL ALREADY DECLARED IS AN ASSIGNMENT AND NOT A SECOND DECLARATION.
    // `satellite.library.counter = 5` then `= 9` is two top-level declarations
    // of one name, which is S0291 -- and inside a capsule the second line is a
    // perfectly ordinary assignment, which is what the user meant. The scanner
    // cannot tell (block.hpp says why); this can, because it is the thing that
    // remembers what has been declared.
    bool top = scanned.placement == Placement::TopLevel;
    if (top && !scanned.library_name.empty()) {
        for (const std::string &name : globals_) {
            if (name == scanned.library_name) {
                top = false;
                break;
            }
        }
    }

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

    // REMEMBERED ONLY AFTER IT BUILT, so a declaration that was rejected does
    // not make the next mention of the name an assignment to something that
    // does not exist.
    if (top && !scanned.library_name.empty())
        globals_.push_back(scanned.library_name);

    const int which = find_main(built);
    if (which < 0)
        return true;

    clear_interrupt();

    eval::Machine machine(built.program.closures, built.parsed.ast,
                          policy_from_the_limits());
    machine.run_top_level();
    if (machine.ok())
        machine.call(static_cast<uint32_t>(which), arguments());

    // WHAT THE LINE LEFT BEHIND, TAKEN BEFORE ANYTHING ELSE CAN DISTURB IT.
    // Only from a run that finished: a line that refused halfway has a frame
    // whose later slots were never assigned, and keeping those would hand the
    // next line variables holding nothing under a type that says otherwise.
    if (machine.ok())
        keep_what_ran(built, machine, which);

    // DRAIN AND NOT SHUTDOWN, WHICH IS THE WHOLE DIFFERENCE BETWEEN A RUN AND A
    // SESSION. `satl file.satl` shuts the console down because the process is
    // about to end; here another line is coming, and shutdown would join the
    // printer thread and start a new one for every line typed. Draining gives
    // the same guarantee that matters -- everything printed is on the terminal
    // before the next prompt is drawn -- at no cost.
    console::Console::the().drain();

    // A RUN-TIME DIAGNOSTIC NEEDS REBASING TOO, AND THIS WAS MISSED THE FIRST
    // TIME. `report()` above covers what the four passes found; S0721 -- a path
    // the language has a number for and nothing behind yet -- is raised by
    // op_dispatch while the program RUNS, so it arrives here instead and went
    // out with the wrapper's line number on it. `satellite.help` at the prompt
    // said "line 4". Found by typing it, which is the only way this one shows.
    if (!machine.ok()) {
        std::vector<errors::Diagnostic> problems = machine.problems();
        rebase_all(problems, above);
        fputs(errors::render(problems, errors::Source{kPromptName, built.text,
                                                      &built.words})
                  .c_str(),
              stderr);
    }
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

    // THE FILE'S OWN VARIABLES BECOME THE SESSION'S, which is the whole of
    // `satl -i` and of `run <file>` at the prompt: the program finishes and
    // what it was holding is still there to be asked about. Nothing in the
    // user's file is rewritten to make this work -- the machine kept its
    // outermost frame and resolve already knew the names.
    if (machine.ok())
        keep_what_ran(built, machine, which);

    console::Console::the().drain();

    if (!machine.ok())
        fputs(errors::render(machine.problems(),
                             errors::Source{path, built.text, &built.words})
                  .c_str(),
              stderr);
    return true;
}

} // namespace satellite::prompt
