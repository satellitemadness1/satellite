// satellite/bytecode/capsule_calls.cpp -- the header says what these are for.

#include "capsule_calls.hpp"

#include "program_walk.hpp"
#include "../machine/source_position.hpp"

#include <utility>

namespace satellite004 {
namespace {

token::Code code_at_here(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<token::Code>(row[at].to_ulong()) : 0;
}

// THE ARGUMENTS BETWEEN `(` AND `)`, worked out in the caller's frame -- `at` on the
// `(` and left past the `)`. False, with the refusal in `context`, when one could not
// be worked out or the brackets do not close.
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
        context.refuse(satl_line_not_understood, written + "(...) was given something it could not read to the end of",
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
    const signed long long int ran = run_capsule_for(*program, *table, context.functions, site, std::move(arguments),
                                                     self, context.state, dropped ? nullptr : &answer);
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
        const UserDefinedHandle *handle = object ? object.value->as_user_defined() : nullptr;
        if (handle != nullptr && *handle != nullptr && (*handle)->layout != nullptr) {
            signed long long int refused = success;
            std::string why;
            const CapsuleSite *site =
                table->member((*handle)->layout->suit, names.back(), table->scope_at(which, started), refused, why);
            if (site == nullptr) {
                context.refuse(refused, why, started);
                return false;
            }
            at = open;
            if (!arguments_at(row, at, out.written, out.arguments, context))
                return false;
            out.site = site;
            out.self = *handle;
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
    const std::string written = receiver + "." + name;

    // THE OBJECT IS HELD HERE, BY A HANDLE OF ITS OWN, for the whole call. `object` is
    // whatever the chain was standing on -- a variable, or a field of this body's object
    // -- and the arguments below, or the capsule itself, may give that name another
    // object while this one is still being run on.
    const UserDefinedHandle *named = object.as_user_defined();
    const UserDefinedHandle held = named != nullptr ? *named : UserDefinedHandle();
    const UserDefinedHandle *handle = &held;
    if (named == nullptr || held == nullptr || held->layout == nullptr) {
        context.refuse(satl_line_not_understood, receiver + " holds no object yet, so it has no " + name +
                                                     " -- a field of a spacesuit's type starts empty until something "
                                                     "gives it one", dot);
        return Value();
    }
    // THE AUTHOR'S LOCK (satellite_object/object_lock.hpp): `.lock()` turns it on and
    // `.unlock()` off. Neither locks anything itself; a statement that writes the object does.
    if (next == token::lock_token || next == token::unlock_token) {
        if (code_at_here(row, at) != token::left_parenthesis_token ||
            code_at_here(row, at + 1) != token::right_parenthesis_token) {
            context.refuse(satl_line_not_understood, written + "() takes nothing, in its brackets", dot);
            return Value();
        }
        at += 2;
        (*handle)->lock.on.store(next == token::lock_token, std::memory_order_release);
        return Value();
    }
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
