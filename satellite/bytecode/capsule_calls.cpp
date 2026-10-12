// satellite/bytecode/capsule_calls.cpp -- the header says what these are for.

#include "capsule_calls.hpp"

#include "method_periods.hpp"
#include "pointer_calls.hpp"
#include "program_walk.hpp"
#include "../machine/source_position.hpp"

#include <utility>

namespace satellite004 {
namespace {

token::Code code_at_here(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<token::Code>(row[at].to_ulong()) : 0;
}

// IS `name` A CAPSULE OF THE SPACESUIT THIS IS AN OBJECT OF, or of one it extends (CapsuleTable::declares)?
// `held` is object_behind(object), already asked. A pointer carries its spacesuit even when its object
// is gone (satellite_pointer.hpp); a field never given an object, and a line typed at the prompt with no
// program around it, have no spacesuit to ask, and so no capsule of his. `from` is the scope the line stands
// in: given, only a capsule that line reaches counts (declares says why); kNoScope, any he declares.
bool his_own_capsule(const Value &object, const UserDefinedHandle &held, const std::string &name,
                     const ExpressionContext &context, std::size_t from = kNoScope)
{
    const CapsuleTable *table = context.state.capsules;
    if (table == nullptr)
        return false;
    const ObjectPointer *pointer = object.as_pointer();
    const satelliteSuitLayout *layout = pointer != nullptr ? pointer->layout.get()
                                        : held != nullptr  ? held->layout.get()
                                                           : nullptr;
    return layout != nullptr && table->declares(layout->suit, name, from);
}

// THE SCOPE A LINE STANDS IN, as member() is asked from it -- kNoScope when no program is around the line.
std::size_t scope_of_the_line(const std::vector<std::bitset<16>> &row, std::size_t at, const ExpressionContext &context)
{
    const CapsuleTable *table = context.state.capsules;
    const BytecodeRegistry *program = context.state.program;
    return table == nullptr || program == nullptr ? kNoScope : table->scope_at(row_index_of(program, row), at);
}

// THE ARGUMENTS BETWEEN `(` AND `)`, worked out in the caller's frame -- `at` on the
// `(` and left past the `)`. False, with the refusal in `context`, when one could not
// be worked out or the brackets do not close.
bool ends_the_line(const std::vector<std::bitset<16>> &row, std::size_t at);

bool arguments_at(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &written,
                  std::vector<Value> &arguments, ExpressionContext &context)
{
    const std::size_t open = at;
    ++at;
    if (code_at_here(row, at) != token::right_parenthesis_token) {
        for (;;) {
            arguments.push_back(evaluate_expression(row, at, context));
            if (context.code != success || code_at_here(row, at) != token::comma_token)
                break;
            ++at;
        }
    }
    if (context.code != success)
        return false;
    if (code_at_here(row, at) != token::right_parenthesis_token) {
        // THE LINE ENDED FIRST: the ) is missing, or a string with no closing " swallowed it
        // (the error sweep, 2026-09-25) -- said as that rather than as something unreadable.
        context.refuse(satl_line_not_understood,
                       ends_the_line(row, at) ? written + "(...)'s ( is never closed -- the line ends before its ), "
                                                          "or a string on it has no closing \""
                                              : written + "(...) was given something it could not read to the end of",
                       open);
        return false;
    }
    ++at;
    return true;
}

// IS WHAT FOLLOWS THE `)` THE LINE'S END -- so, in a statement, nothing uses the answer.
bool ends_the_line(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const token::Code code = code_at_here(row, at);
    return code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token;
}

// RUN IT, and hand back its answer -- or, when the line lets the answer go, ask for none.
Value run_for_answer(const CapsuleSite &site, std::vector<Value> arguments, const UserDefinedHandle &self,
                     const std::vector<std::bitset<16>> &row, std::size_t after, std::size_t where,
                     const std::string &written, ExpressionContext &context)
{
    const CapsuleTable *table = context.state.capsules;
    const BytecodeRegistry *program = context.state.program;
    Value answer;
    const bool dropped = context.statement && ends_the_line(row, after);
    // an argument the capsule refuses is shown at `where`, the call, and not at its statement's start
    const signed long long int ran = run_capsule_for(*program, *table, context.functions, site, std::move(arguments),
                                                     self, context.state, dropped ? nullptr : &answer, &row, where);
    // THE CAPSULE HAS ALREADY SAID WHAT WENT WRONG, with its own line and caret -- this
    // only stops the expression it was part of.
    if (stops_the_program(ran)) {
        context.refuse(ran, written + "(...) stopped", where);
        context.reported = true;
        return Value();
    }
    return answer;
}

} // namespace

Value call_capsule_for_its_answer(const std::vector<std::bitset<16>> &row, std::size_t &at,
                                  const std::vector<std::string> &names, std::size_t open,
                                  ExpressionContext &context)
{
    const std::size_t started = at;
    std::string written;
    for (const std::string &each : names) written += (written.empty() ? "" : ".") + each;
    const CapsuleTable *table = context.state.capsules;
    const BytecodeRegistry *program = context.state.program;
    if (table == nullptr || program == nullptr) {
        context.refuse(satl_line_not_understood, "no capsule named " + written + " -- a line typed at the prompt has "
                                                                                 "no capsules around it", started);
        return Value();
    }
    const std::size_t which = row_index_of(program, row);
    const Reached reached = table->reach(table->scope_at(which, started), names);
    if (reached.site == nullptr) {
        context.refuse(reached.code, reached.why, started);
        return Value();
    }
    at = open;
    std::vector<Value> arguments;
    if (!arguments_at(row, at, written, arguments, context))
        return Value();
    // A SPACESUIT'S CAPSULE BY ITS BARE NAME runs on this body's object: the checker
    // has proved the line stands in a capsule of the same spacesuit.
    const UserDefinedHandle none;
    const UserDefinedHandle &self = reached.site->suit != kNoScope ? context.variables.self : none;
    return run_for_answer(table->on_the_object(*reached.site, self), std::move(arguments), self, row, at, started,
                          written, context);
}

bool package_capsule_call(const std::vector<std::bitset<16>> &row, std::size_t &at,
                          const std::vector<std::string> &names, std::size_t open,
                          ExpressionContext &context, PackagedCall &out)
{
    const std::size_t started = at;
    out.written.clear();
    for (const std::string &each : names) out.written += (out.written.empty() ? "" : ".") + each;
    const CapsuleTable *table = context.state.capsules;
    const BytecodeRegistry *program = context.state.program;
    if (table == nullptr || program == nullptr) {
        context.refuse(satl_line_not_understood, "no capsule named " + out.written + " -- a line typed at the "
                                                 "prompt has no capsules around it to run on a thread", started);
        return false;
    }
    const std::size_t which = row_index_of(program, row);
    // `obj.call_x(...)` -- A CAPSULE OF AN OBJECT, run on a thread on that object, which the
    // thread SHARES with this one (THREADS.md T2: sharing is allowed; the object's .lock()
    // is what makes its writes one at a time).
    if (names.size() == 2) {
        const Seen object = context.variables.seen(names.front());
        // A POINTER'S OBJECT TOO (pointer_calls.hpp), held by the thread as any object it is
        // handed is -- for as long as the thread itself is kept (thread_calls.cpp keeps `self`).
        const UserDefinedHandle held = object ? object_behind(*object.value) : UserDefinedHandle();
        // ONE OF THE FIVE IS NO CAPSULE TO RUN -- unless his spacesuit declares a capsule of that name,
        // which a bare `.` calls everywhere (pointer_calls.hpp; the author's C of 2026-10-10).
        if (held != nullptr && every_object_word(names.back()) &&
            !his_own_capsule(*object.value, held, names.back(), context, table->scope_at(which, started))) {
            context.refuse(thread_needs_a_capsule_call, not_a_capsule_for_a_thread(out.written, names.back()),
                           started);
            return false;
        }
        if (object && object.value->is_pointer() && held == nullptr) {
            context.refuse(object_is_gone, object_is_gone_because(out.written, names.front()), started);
            return false;
        }
        if (held != nullptr && held->layout != nullptr) {
            signed long long int refused = success;
            std::string why;
            const CapsuleSite *site =
                table->member(held->layout->suit, names.back(), table->scope_at(which, started), refused, why);
            if (site == nullptr) {
                context.refuse(refused, why, started);
                return false;
            }
            at = open;
            if (!arguments_at(row, at, out.written, out.arguments, context))
                return false;
            out.site = site;
            out.self = held;
            return true;
        }
    }
    const Reached reached = table->reach(table->scope_at(which, started), names);
    if (reached.site == nullptr) {
        context.refuse(reached.code, reached.why, started);
        return false;
    }
    at = open;
    if (!arguments_at(row, at, out.written, out.arguments, context))
        return false;
    // A SPACESUIT'S CAPSULE BY ITS BARE NAME runs on this body's object, as a call would --
    // and as the OBJECT's own capsule of that name: a dog's call_speak, not the animal's it
    // overrides (the second review, 2026-09-24: it ran the animal's).
    if (reached.site->suit != kNoScope) {
        out.self = context.variables.self;
        out.site = &table->on_the_object(*reached.site, out.self);
    } else {
        out.site = reached.site;
    }
    return true;
}

Value call_member(const std::vector<std::bitset<16>> &row, std::size_t &at, const Value &object,
                  const std::string &receiver, ExpressionContext &context)
{
    const std::size_t dot = at;
    // WHICH PERIOD WAS WRITTEN (method_periods.hpp): `.satellite.` before the five words every object
    // has, and `.` before a capsule of his own (pointer_calls.hpp says both rulings).
    const bool under_satellite = code_at_here(row, dot) == token::satellite_method_token;
    std::size_t k = at + 1;
    const token::Code next = code_at_here(row, k);
    std::string name;
    if (next == token::name_token) {
        name = text_at(row, k);
    } else {
        name = token::method_name_of(next);
        ++k;
    }
    at = k;
    const std::string written = receiver + (under_satellite ? ".satellite." : ".") + name;

    // THE FIVE WORDS EVERY OBJECT HAS RUN THEIR NUMBERS (pointer_calls.hpp), 1 10 1 to 1 10 5 -- and a
    // pointer is asked ok(), pointer() and reference() BEFORE its object is held, which
    // run_object_number keeps: a pointer whose object is gone still answers ok() false.
    const token::Code number = next == token::name_token ? 0 : object_number_of(next);
    if (under_satellite && number != 0)
        return run_object_number(next, number, row, at, object, receiver, dot, context);

    // THE OBJECT IS HELD HERE, BY A HANDLE OF ITS OWN, for the whole call. `object` is
    // whatever the chain was standing on -- a variable, or a field of this body's object
    // -- and the arguments below, or the capsule itself, may give that name another
    // object while this one is still being run on. A POINTER'S OBJECT is held the same way,
    // so the object it points at cannot go while its capsule runs.
    const UserDefinedHandle held = object_behind(object);
    const UserDefinedHandle *handle = &held;

    // THE OTHER PERIOD, JUDGED BEFORE ANYTHING RUNS: `.satellite.` before a capsule his spacesuit
    // declares, or a bare `.` before one of the five that it does not (the author's A of 2026-10-11, and
    // his C of 2026-10-10: his own spacesuit's methods stay as he writes them). So `d.lock()` runs his
    // own lock() when his spacesuit has one -- ERRORS4 #005 -- and is told to write d.satellite.lock()
    // when it has none. A pointer's spacesuit is known even when its object is gone; a field never
    // given an object has none to ask, and is told that below.
    if (under_satellite || number != 0) {
        // A BARE ONE OF THE FIVE IS HIS ONLY WHERE THIS LINE REACHES HIS CAPSULE OF THAT NAME (declares): from
        // outside, a protected lock() of his leaves `d.lock()` the language's word written the old way.
        const bool his = his_own_capsule(object, held, name, context,
                                         under_satellite ? kNoScope : scope_of_the_line(row, dot, context));
        if (under_satellite ? his : !his && (object.is_pointer() || held != nullptr)) {
            context.refuse(satl_line_not_understood,
                           period_written_wrong(receiver, name, object.kind(), under_satellite), dot);
            return Value();
        }
    }
    if (held == nullptr || held->layout == nullptr) {
        if (object.is_pointer()) {
            context.refuse(object_is_gone, object_is_gone_because(written, receiver), dot);
            return Value();
        }
        context.refuse(satl_line_not_understood, receiver + " holds no object yet, so it has no " + name +
                                                     " -- a field of a spacesuit's type starts empty until something "
                                                     "gives it one", dot);
        return Value();
    }
    // ANY OTHER NAME IS A CAPSULE OF HIS SPACESUIT -- one of the five too, when his spacesuit declares
    // it (above) -- and the language's own words were run by their numbers before this.
    const CapsuleTable *table = context.state.capsules;
    const BytecodeRegistry *program = context.state.program;
    if (table == nullptr || program == nullptr) {
        context.refuse(satl_line_not_understood, written + " -- there is no program around this line", dot);
        return Value();
    }
    const std::size_t which = row_index_of(program, row);
    signed long long int refused = success;
    std::string why;
    const CapsuleSite *site = table->member((*handle)->layout->suit, name, table->scope_at(which, dot), refused, why);
    if (site == nullptr) {
        context.refuse(refused, why, dot);
        return Value();
    }
    // A CAPSULE IS CALLED, NEVER READ: `obj.call_name` with no brackets would be the
    // capsule itself as a value, which 003 deferred too (its S0720).
    if (code_at_here(row, at) != token::left_parenthesis_token) {
        context.refuse(satl_line_not_understood, written + " is a capsule of " + (*handle)->layout->shown +
                                                     ", and a capsule is called with its brackets: " + written + "()",
                       dot);
        return Value();
    }
    std::vector<Value> arguments;
    if (!arguments_at(row, at, written, arguments, context))
        return Value();
    return run_for_answer(*site, std::move(arguments), *handle, row, at, dot, written, context);
}

} // namespace satellite004
