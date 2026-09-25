// satellite/bytecode/suit_run.cpp -- the header says what making an object is.

#include "suit_run.hpp"

#include "expression.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"
#include "../machine/s_codes.hpp"

#include <utility>

namespace satellite004 {
namespace {

using token::Code;

const Code kSpacesuit = word::code_of(1, 10);

bool the_line_ends(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const Code code = code_at(row, at);
    return code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token;
}

} // namespace

bool names_a_suit(const TypeShape &shape)
{
    if (shape.is_a_suit())
        return true;
    for (const TypeShape &inner : shape.parameters)
        if (names_a_suit(inner))
            return true;
    return false;
}

bool an_object_declaration_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    if (code_at(row, at) != token::name_token)
        return false;
    std::size_t k = at;
    skip_payload(row, k);
    while (code_at(row, k) == token::method_token) {
        const Code next = code_at(row, k + 1);
        if (next == token::name_token) {
            ++k;
            skip_payload(row, k);
        } else if (token::is_method_code(next)) {
            k += 2;
        } else {
            return false;
        }
    }
    return code_at(row, k) == token::name_token;
}

signed long long int run_object_declaration(const BytecodeRegistry &registry, const CapsuleTable &capsules,
                                            const FunctionTable &functions, std::size_t which_row, std::size_t &at,
                                            VariableTable &variables, MachineState &state, bool as_a_field)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];
    const std::size_t started = at;
    std::size_t k = at;
    TypeShape shape;
    shape.word = kSpacesuit;
    dotted_names_at(row, k, shape.suit_names);
    const std::string name = text_at(row, k);

    // THE SPACESUIT IT NAMES, from where the line stands. The checker has proved it
    // reaches one; this is the walker saying so again rather than trusting a name.
    std::string why;
    if (!resolve_shape(capsules, capsules.scope_at(which_row, started), shape, why)) {
        at = past_the_statement(row, started);
        return raise_at(name_not_declared, why, name, state, row, started);
    }

    Value value;
    if (code_at(row, k) == token::assign_token) {
        // `Type x = y` SHARES, AND MAKES NOTHING (003): whatever y holds, x holds too.
        ++k;
        ExpressionContext context{variables, functions, state};
        value = evaluate_expression(row, k, context);
        if (context.code != success) {
            at = past_the_statement(row, started);
            if (context.reported)
                return context.code;
            return raise_at(context.code, context.why, name + " = ...", state, row,
                            context.placed ? context.refused_at : started);
        }
        if (!the_line_ends(row, k)) {
            at = past_the_statement(row, started);
            return raise_at(satl_line_not_understood, name + " = ... could not be read to the end", std::string(),
                            state, row, k);
        }
        if (!value_fits(shape, value, why)) {
            at = past_the_statement(row, started);
            return raise_at(types_do_not_meet, name + " was declared " + suit_written(shape) + ", and " + why,
                            std::string(), state, row, started);
        }
    } else {
        std::vector<Value> arguments;
        const bool bracketed = code_at(row, k) == token::left_parenthesis_token;
        if (bracketed) {
            ExpressionContext context{variables, functions, state};
            std::size_t a = k + 1;
            if (code_at(row, a) != token::right_parenthesis_token) {
                for (;;) {
                    arguments.push_back(evaluate_expression(row, a, context));
                    if (context.code != success || code_at(row, a) != token::comma_token)
                        break;
                    ++a;
                }
            }
            if (context.code != success) {
                at = past_the_statement(row, started);
                if (context.reported)
                    return context.code;
                return raise_at(context.code, context.why, name + "(...)", state, row,
                                context.placed ? context.refused_at : started);
            }
            if (code_at(row, a) != token::right_parenthesis_token || !the_line_ends(row, a + 1)) {
                at = past_the_statement(row, started);
                return raise_at(satl_line_not_understood,
                                name + "(...) could not be read to the end -- an object's arguments go between one "
                                       "( and its ), and nothing follows them",
                                std::string(), state, row, a);
            }
            k = a + 1;
        }
        // A FIELD WITH NOTHING AFTER ITS NAME IS AN EMPTY SLOT (the header says why).
        if (!(as_a_field && !bracketed)) {
            UserDefinedHandle made;
            const signed long long int code =
                make_an_object(registry, capsules, functions, shape.suit, std::move(arguments), state, made);
            if (stops_the_program(code)) {
                at = past_the_statement(row, started);
                return code;
            }
            value = Value::of_user_defined(std::move(made));
        }
    }
    variables[name] = Variable{kSpacesuit, std::move(shape), std::move(value)};
    at = past_the_statement(row, k);
    return success;
}

signed long long int make_an_object(const BytecodeRegistry &registry, const CapsuleTable &capsules,
                                    const FunctionTable &functions, std::size_t suit, std::vector<Value> arguments,
                                    MachineState &state, UserDefinedHandle &made)
{
    const CapsuleScope &of = capsules.scopes[suit];
    made = make_object(of.layout);
    const satelliteSuitLayout &layout = *of.layout;

    // ITS FIELDS, EACH BY ITS OWN DECLARING STATEMENT, in a frame with no object in it --
    // its supertypes' first, where THEY are declared, which may be another file's row.
    if (!layout.fields.empty()) {
        VariableTable making;
        // A FIELD'S WARNING NAMES THE FIELD'S LINE (the review, 2026-09-25): these lines do
        // not pass through run_statements, so without this an S020 in a field's value named
        // the line that declared the object -- and two objects, two entries for one line.
        const StatementPlaceKept declaring_line(state);
        for (std::size_t slot = 0; slot < layout.fields.size(); ++slot) {
            std::size_t at = layout.fields[slot].at;
            state.statement_row = &registry[layout.fields[slot].row];
            state.statement_at = at;
            bool was_one = false;
            const signed long long int code = run_declaration(registry, capsules, functions, layout.fields[slot].row,
                                                              at, making, state, true, was_one);
            if (stops_the_program(code))
                return code;
            const VariableTable::iterator field = making.find(layout.fields[slot].name);
            if (field != making.end())
                made->fields[slot] = std::move(field->second.value);
        }
    }

    // AND THEN ITS CONSTRUCTORS, ON THE OBJECT, from the furthest supertype down (003's
    // order): what the declaration handed over goes to the nearest one, and every one
    // above it runs with nothing -- the checker has proved those take nothing.
    const std::size_t receiving = capsules.constructor_of(suit);
    if (receiving == kNoSite && !arguments.empty())
        return report_error("satl(run): " + layout.shown + " was given " + std::to_string(arguments.size()) +
                                (arguments.size() == 1 ? " argument" : " arguments") +
                                ", and has no satellite.constructor to take them",
                            satl_line_not_understood);
    const std::vector<std::size_t> own{suit};
    const std::vector<std::size_t> &lineage = layout.lineage.empty() ? own : layout.lineage;
    for (std::size_t n = lineage.size(); n > 0; --n) {
        const std::size_t constructor = capsules.scopes[lineage[n - 1]].constructor;
        if (constructor == kNoSite)
            continue;
        const signed long long int code =
            run_capsule_for(registry, capsules, functions, capsules.sites[constructor],
                            constructor == receiving ? std::move(arguments) : std::vector<Value>(), made, state, nullptr);
        if (stops_the_program(code))
            return code;
        if (constructor == receiving)
            break;
    }
    return success;
}

} // namespace satellite004
