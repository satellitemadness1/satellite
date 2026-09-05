// The machine's expression arms. See evaluator/machine.hpp for the contract
// every one of them is written against, and closure.hpp for what an Op holds.
//
// EVERY ARM HERE LEAVES EXACTLY ONE VALUE ON THE VALUE STACK, and that is the
// whole type system of this machine. operations_control.cpp holds the arms that
// leave none. Two files rather than one because that is the line -- an
// expression answers and a statement acts -- and it is the same seam DESIGN §6
// draws between its two halves of the grammar.
//
// AN ARM NEVER CALLS ANOTHER ARM. That is the rule the explicit stack exists to
// keep: `a + b` does not evaluate `a`, it PUSHES `a` and asks to be resumed.
// The one thing that looks like an exception is a refusal, which does not
// return to its caller because nothing runs after it.
//
// AND done() COMES AFTER THE LAST THING THAT CAN REFUSE, in every arm, which is
// not a style choice. Machine::here() reads the top of the work stack, so an
// arm that pops itself before raising a diagnostic draws its caret under its
// PARENT. The order below is what puts the caret under the operator.

#include "evaluator/evaluator_internal.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_value/render.hpp"

#include <utility>

namespace satellite {
namespace eval {

namespace {

// The two operands of a binary op, in the order they were written.
//
// READ AND NOT POPPED, and both halves of that matter. The stack gives them
// back in reverse -- the arm below pushed left on top so it would run first --
// so getting the ORDER wrong is invisible for `+` and wrong for `-`, which is
// why it is one function. And they stay on the stack until the answer exists,
// because Machine::fold() then turns the two slots into one without a push:
// pop, pop, push is three vector operations where fold is an assignment and a
// pop, and the push is the one that has to ask about the ceiling.
struct Pair {
    const Value &left;
    const Value &right;
};

Pair operands(Machine &m)
{
    return {m.value_from_top(1), m.value_from_top(0)};
}

// NUMERIC MEANS THE TWO NUMERIC ARMS, number and float, and since M15 every
// arithmetic and ordering arm below accepts either on either side. Mixing is
// not a truthiness ladder sneaking in: DESIGN §8.6's conversion paragraph is
// the licence -- "a number becomes a float as (n, 0), which is exact and
// always succeeds" -- so nothing is invented on the way across, and the
// program wrote the float into the expression somewhere for the mix to occur
// at all (there is no float literal; only a declaration or a class-3 method
// produces one).
bool is_numeric(const Value &value)
{
    return value.is_number() || value.is_float();
}

// The exact promotion. A float answers itself; a number splits at the point,
// losing nothing. (A null float handle has no producer -- Value::floating
// always allocates -- and reads as zero here rather than as undefined.)
Float as_float(const Value &value)
{
    if (const Flo *held = std::get_if<Flo>(&value))
        return *held ? **held : Float();
    return Float::from_number(std::get<Number>(value));
}

// Both operands are numeric, or the arm refuses naming the one that is not.
bool both_numeric(Machine &m, BinaryOp op, const Pair &pair, errors::Code code)
{
    const Value *wrong = nullptr;
    if (!is_numeric(pair.left))
        wrong = &pair.left;
    else if (!is_numeric(pair.right))
        wrong = &pair.right;
    if (wrong == nullptr)
        return true;

    // THE SENTENCE NAMES THE OPERATOR AND THE TYPE, NOT THE VALUE. A number
    // printed into an error message can be fifty thousand digits long (M8
    // deleted the clamp that stopped it), so what a person needs is "this one
    // is a string" rather than the string.
    errors::Diagnostic problem =
        code == errors::Code::EVAL_NOT_A_NUMBER
            ? errors::make<errors::Code::EVAL_NOT_A_NUMBER>(
                  m.span_of(m.here()), text_of(op), type_name(*wrong))
            : errors::make<errors::Code::EVAL_NOT_COMPARABLE>(
                  m.span_of(m.here()), text_of(op), type_name(*wrong));
    m.refuse(std::move(problem));
    return false;
}

} // namespace

void op_no_op(Machine &m, const Op &, uint32_t)
{
    // OP ZERO, AND IT IS THE EMPTY STATEMENT. closure.hpp keeps index 0 as a
    // no-op so kNoOp is safe to push, the way ast.hpp keeps node 0 a None node.
    // An EXPRESSION position is never kNoOp -- the compiler emits op_refuse
    // rather than nothing -- so this arm leaving no value cannot starve one.
    m.done();
}

void op_constant(Machine &m, const Op &op, uint32_t)
{
    m.done();
    m.push_value(m.program().constant(op.a));
}

void op_local(Machine &m, const Op &op, uint32_t)
{
    // DESIGN §7.2 IN ONE LINE: an integer index into this activation's own
    // storage, decided before anything ran. §7.1's receipt for what the first
    // satellite did instead is that a recursive `fact` returned 1 for every
    // input and eight threads produced 1585 wrong results out of 1600.
    m.done();
    m.push_value(m.local(op.a));
}

void op_global(Machine &m, const Op &op, uint32_t)
{
    m.done();
    m.push_value(m.global(op.a));
}

void op_unary(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }

    const UnaryOp which = static_cast<UnaryOp>(op.b);
    const Value &value = m.value_from_top(0);

    if (which == UnaryOp::Negate) {
        if (const Flo *held = std::get_if<Flo>(&value)) {
            // One bool, and invariant 3 keeps zero positive -- §8.6's
            // "negate: flip `positive`, unless the value is zero".
            Value negated = Value::floating(*held ? (*held)->negated()
                                                  : Float());
            m.done();
            m.set_top(std::move(negated));
            return;
        }
        if (!value.is_number()) {
            m.refuse(errors::make<errors::Code::EVAL_NOT_A_NUMBER>(
                m.span_of(m.here()), text_of(which), type_name(value)));
            return;
        }
        Number negated = std::get<Number>(value).negated();
        m.done();
        m.set_top(Value::number(std::move(negated)));
        return;
    }

    bool flag = false;
    if (!truth_of(value, &flag)) {
        m.refuse(errors::make<errors::Code::EVAL_NOT_A_CONDITION>(
            m.span_of(m.here()), type_name(value)));
        return;
    }
    m.done();
    m.set_top(Value::boolean(!flag));
}

void op_binary(Machine &m, const Op &op, uint32_t step)
{
    if (step == 0) {
        // LEFT ON TOP SO IT RUNS FIRST. DESIGN §6.6 makes every level
        // left-associative and satellite has no operator with a side effect
        // yet, but evaluation ORDER is a thing a language either decides or
        // discovers later -- and C++17's indeterminate sequencing is exactly
        // what M8.5 found in the `.satc` writer's comment column.
        m.again(1);
        m.push(op.b);
        m.push(op.a);
        return;
    }

    const BinaryOp which = static_cast<BinaryOp>(op.c);
    const Pair pair = operands(m);

    // EQUALITY WORKS ON ANYTHING AND ORDERING DOES NOT, which is DESIGN §8's
    // table read honestly: two values of different types are not equal, and
    // there is no answer at all to whether a string is less than a bool.
    if (which == BinaryOp::Equal || which == BinaryOp::NotEqual) {
        const bool equal = same(pair.left, pair.right);
        m.done();
        m.fold(Value::boolean(which == BinaryOp::Equal ? equal : !equal));
        return;
    }

    if (which == BinaryOp::Less || which == BinaryOp::Greater ||
        which == BinaryOp::LessEqual || which == BinaryOp::GreaterEqual) {
        if (!both_numeric(m, which, pair, errors::Code::EVAL_NOT_COMPARABLE))
            return;
        // The all-number path stays on Number::compare untouched; a float on
        // either side promotes the other exactly and compares totally --
        // equality (same(), above) and this ordering MUST agree across the
        // two arms, or QUAD.md §3.3's `if (a != b) return a > b` comparators
        // stop being a strict weak order. value.cpp says it from its side.
        const int order =
            pair.left.is_number() && pair.right.is_number()
                ? Number::compare(std::get<Number>(pair.left),
                                  std::get<Number>(pair.right))
                : Float::compare(as_float(pair.left), as_float(pair.right));
        bool answer = false;
        switch (which) {
        case BinaryOp::Less:         answer = order < 0; break;
        case BinaryOp::Greater:      answer = order > 0; break;
        case BinaryOp::LessEqual:    answer = order <= 0; break;
        default:                     answer = order >= 0; break;
        }
        m.done();
        m.fold(Value::boolean(answer));
        return;
    }

    if (!both_numeric(m, which, pair, errors::Code::EVAL_NOT_A_NUMBER))
        return;

    if (pair.left.is_number() && pair.right.is_number()) {
        const Number &left = std::get<Number>(pair.left);
        const Number &right = std::get<Number>(pair.right);

        // DIVISION BY ZERO IS S0601 AND NOT A ROW OF ITS OWN, which is
        // FORMAT/CXX.md §1's second rule about the one place a fact lives.
        // `satl --number 1 / 0` already says this sentence and programs/
        // number_command.cpp already checks this way; a second row would be a
        // second sentence about one thing.
        if ((which == BinaryOp::Divide || which == BinaryOp::Modulo) &&
            right.is_zero()) {
            m.refuse(errors::make<errors::Code::NUMBER_DIVIDE_BY_ZERO>(
                m.span_of(m.here()), left.to_string()));
            return;
        }

        Number answer;
        switch (which) {
        case BinaryOp::Add:      answer = Number::add(left, right); break;
        case BinaryOp::Subtract: answer = Number::sub(left, right); break;
        case BinaryOp::Multiply: answer = Number::mul(left, right); break;
        case BinaryOp::Divide:
            answer = Number::divide(left, right, m.policy().division_digits);
            break;
        default:                 answer = Number::modulo(left, right); break;
        }

        m.done();
        m.fold(Value::number(std::move(answer)));
        return;
    }

    // A FLOAT ON EITHER SIDE MAKES THE OPERATION A FLOAT OPERATION, and the
    // answer is a float: the type that carries a precision wins, because the
    // program asked for it by putting a float in the expression. DESIGN
    // §8.6's classes place each operator -- `+` and `-` exact, `%` exact by
    // composition, `*` and `/` rounding the right half to the result's
    // precision, which is where policy().float_digits earns its Policy row.
    const Float left = as_float(pair.left);
    const Float right = as_float(pair.right);

    if ((which == BinaryOp::Divide || which == BinaryOp::Modulo) &&
        right.is_zero()) {
        m.refuse(errors::make<errors::Code::NUMBER_DIVIDE_BY_ZERO>(
            m.span_of(m.here()), left.to_string()));
        return;
    }

    Float answer;
    switch (which) {
    case BinaryOp::Add:
        answer = Float::add(left, right);
        break;
    case BinaryOp::Subtract:
        answer = Float::sub(left, right);
        break;
    case BinaryOp::Multiply:
        answer = Float::mul(left, right, m.policy().float_digits);
        break;
    case BinaryOp::Divide:
        answer = Float::divide(left, right, m.policy().float_digits);
        break;
    default:
        // `a % b` is `a - b x trunc(a/b)` and NEVER ROUNDS -- §8.6 files it
        // in class 1, M8 built it on Number, and the exact join reaches it.
        answer = Float::from_number(
            Number::modulo(left.to_number(), right.to_number()));
        break;
    }

    m.done();
    m.fold(Value::floating(std::move(answer)));
}

void op_to_float(Machine &m, const Op &op, uint32_t step)
{
    // THE ONE NAMED CONVERSION RUNNING. DESIGN §8.6: "A number becomes a
    // float as (n, 0), which is exact and always succeeds" -- and the NAME is
    // the declared type at the target, which is why the compiler emits this
    // only under `satellite.variable.float` declarations and assignments.
    // §1.1 forbids a silent conversion; a store into a float-declared name is
    // not silent, the program wrote the type. The other direction stays
    // named too -- trunc, floor, ceil or round, never this op backwards.
    if (step == 0) {
        m.again(1);
        m.push(op.a);
        return;
    }

    const Value &value = m.value_from_top(0);
    if (value.is_float()) {
        m.done();
        return;
    }
    if (const Number *number = std::get_if<Number>(&value)) {
        Value converted = Value::floating(Float::from_number(*number));
        m.done();
        m.set_top(std::move(converted));
        return;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), "satellite.variable.float",
        "a number or a float", type_name(value)));
}

void op_refuse(Machine &m, const Op &op, uint32_t)
{
    // A COMPILE-TIME DECISION RAISED AT RUN TIME, and the delay is the point.
    // A program with a `satellite.include` in a branch that never runs is a
    // program that runs; refusing it at compile time would make M9 narrower
    // than M4's parser for no reason a user could act on.
    m.refuse(errors::make<errors::Code::EVAL_NOT_BUILT>(
        m.span_of(m.here()), m.program().text(op.a), m.program().text(op.b)));
}

void op_no_question(Machine &m, const Op &op, uint32_t)
{
    // op_refuse's twin for a method a declared type does not have -- S0723's
    // block note in errors.def says why it exists and why it is raised at run
    // time. `a` is the selector, `b` the receiver's declared type as a path,
    // `c` the advice the compiler chose for that type.
    m.refuse(errors::make<errors::Code::EVAL_NO_SUCH_QUESTION>(
        m.span_of(m.here()), m.program().text(op.a), m.program().text(op.b),
        m.program().text(op.c)));
}

void op_misuse(Machine &m, const Op &op, uint32_t)
{
    // THE THIRD TWIN, AND THE FIRST WHOSE CODE IS AN OPERAND -- M14's two
    // place-parameter refusals (S1002, S1003) share one shape, one hole, and
    // one op. errors::make's template checks the hole count against the code
    // at compile time and cannot here, where the code is data; what stands in
    // for the static_assert is the contract that EVERY code this op carries
    // has exactly one {1}, and tests/eval_test raises each row so a sentence
    // grown a second hole fails a fixture rather than printing a hole.
    //
    // RAISED AT RUN TIME for op_refuse's reason, told from the other side:
    // the misuse was DETECTED at compile, but `input(">", 3)` in a branch
    // that never runs is a program that runs.
    errors::Diagnostic problem;
    problem.code = static_cast<errors::Code>(op.a);
    problem.at = m.span_of(m.here());
    problem.arguments = {std::string(m.program().text(op.b))};
    m.refuse(std::move(problem));
}

// op_dispatch LIVED HERE FROM M9 TO M11 AND MOVED WHEN IT STOPPED BEING ALONE.
// M11's method ops share its whole body except where a changed receiver goes,
// so the three arms and their one core are operations_dispatch.cpp -- the same
// split this file already has with operations_control.cpp, made on the same
// grounds: arms that share a contract live where the contract is written once.

} // namespace eval
} // namespace satellite
