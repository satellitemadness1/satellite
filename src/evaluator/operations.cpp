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

// WHAT A STRING WILL JOIN WITH -- the four types words.def gives a
// `to_string` row, and no others. Written as a list rather than as "not a
// container" so that a type added later is refused until somebody decides it
// has a text form, which is the direction DESIGN §1.1 asks a default to lean.
bool joins_into_a_string(const Value &value)
{
    return value.is_number() || value.is_float() || value.is_binary() ||
           value.is_hex();
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

    // `+` ON TWO STRINGS JOINS THEM -- the author, 2026-09-08, at M19.
    //
    // IT IS ONE OPERATOR OVER TWO TYPES AND NOT A SECOND MEANING FOR `+`.
    // Addition and joining are the same shape: take two of a thing, answer one
    // of that thing, change neither. What separates this from the truthiness
    // ladder DESIGN §1.1 refuses is that NOTHING IS CONVERTED -- a string and a
    // number is still S0711, so `"n = " + 4` is refused and the program says
    // `4.to_string()` out loud. The only pair that joins is two strings.
    //
    // WHY IT WAS MISSING UNTIL NOW, which is worth recording because it looks
    // like an omission and was one: `append(x)` `1 6 1 14` MUTATES its receiver
    // under DESIGN §6.4's storage-slot rule, so every string built out of
    // pieces needed a variable to build it in. That makes
    // `display("the reason: " + f.error())` -- one string, used once, named
    // nowhere -- unwritable, and M19's own file diagnostics are exactly that
    // shape. Two example programs in the tree were already written this way and
    // could not run.
    //
    // BEFORE both_numeric, DELIBERATELY. That check's refusal names a number,
    // which is the right sentence for `"a" - "b"` and the wrong one for a join
    // it would have refused on the way past.
    //
    // AND SINCE 2026-09-12 THE TWO SIDES NEED NOT BE THE SAME TYPE -- the
    // author, reversing the paragraph above in the words the motto is usually
    // said in: "just do it all for them! That is our motto -- if we can do it
    // for the user, then we do it for them."
    //
    // THE LEFT SIDE DECIDES WHAT IS TRIED FIRST, which is the author's rule
    // ("it gives them the first variable type") and is the rule the float arm
    // below has followed since M15 -- "the type that carries a precision wins,
    // because the program asked for it by putting a float in the expression."
    // A string on the left joins; a number on the left adds when the other side
    // is a number and joins when it is not. `"n = " + 4` is "n = 4", `4 + "2"`
    // is 6, and `4 + "abc"` is "4abc" -- three readings, and each is the only
    // one its operands allow.
    //
    // THIS IS NOT THE TRUTHINESS LADDER DESIGN §1.1 REFUSES, and the
    // difference is worth stating because the paragraph above used to say this
    // conversion WAS that. Truthiness is a value standing in for a TEST it is
    // not -- a number pretending to be a condition, where the program never
    // wrote what it meant. Here the program wrote both types out and wrote the
    // operator between them; what the language supplies is the `to_string()`
    // or the `to_number()` the user would otherwise type on the next line, in
    // the one position where there is nothing else `+` could have meant.
    //
    // WHAT CONVERTS IS EXACTLY WHAT `.to_string()` ALREADY CONVERTS, which is
    // the boundary and is not an arbitrary list: number `1 6 4 6`, binary
    // `1 6 5 3`, float `1 6 10 1` and hex `1 6 11 3` are the four rows
    // words.def has, so `+` grants no type a text form the language did not
    // already give it. A list, a map, a file, a thread and a SPACESUIT are
    // left refused -- the last on purpose, because DESIGN §12 defers "a
    // spacesuit `to_string` the printer consults", and joining one into a
    // string here would be that feature arriving through the back door.
    if (which == BinaryOp::Add &&
        (pair.left.is_string() || pair.right.is_string())) {
        static const SatString empty;

        // A STRING ON THE LEFT JOINS, AND THE RIGHT IS RENDERED AS THE
        // CHARACTERS `display` WOULD HAVE PRINTED. text_of() is the printer's
        // own function, so `display(x)` and `display("" + x)` cannot disagree
        // -- one of them being the other's definition.
        if (pair.left.is_string()) {
            if (!pair.right.is_string() && !joins_into_a_string(pair.right)) {
                m.refuse(errors::make<errors::Code::EVAL_NOT_A_NUMBER>(
                    m.span_of(m.here()), text_of(which),
                    type_name(pair.right)));
                return;
            }
            const Str &left = std::get<Str>(pair.left);
            SatString joined = left ? *left : empty;
            if (const Str *right = std::get_if<Str>(&pair.right)) {
                if (*right)
                    joined.append(**right);
            } else {
                joined.append(encode_raw(satellite::text_of(pair.right)));
            }
            m.done();
            m.fold(Value::string(std::move(joined)));
            return;
        }

        // A NUMBER ON THE LEFT READS THE STRING AS A NUMBER WHEN IT IS ONE,
        // and JOINS WHEN IT IS NOT. The author's rule is both halves: "we just
        // need to always check if it's a number WHEN we are given a number to
        // add" -- and, of the other half, "well if the user types in 4 + "abc"
        // don't you think they mean "4abc"?"
        //
        // THEY DO, AND THERE IS NOTHING ELSE IT COULD MEAN. `4 + "abc"` is not
        // an arithmetic line with a typo in it; "abc" is not a number and no
        // amount of squinting makes one. So a refusal here tells a person
        // something they already knew and refuses the only reading available.
        //
        // THIS ARM REFUSED WITH S0610 FIRST, ON AN ARGUMENT THAT DOES NOT HOLD
        // IN THIS LANGUAGE, and the argument is written out because it is a
        // good one about a different language. It was: joining makes the TYPE
        // of `n + s` depend on the CONTENTS of `s`, so `total + reading` is a
        // number all week and a string the day a sensor writes "n/a", and the
        // arithmetic downstream changes meaning without changing. What kills it
        // is that satellite does not enforce a declared type on a store at all
        // -- `satellite.variable.number n = "abc"` is accepted today and prints
        // `abc`, and so does `n = s` from a string variable. A content-shaped
        // type is something this language permits in every other line it has;
        // refusing it HERE would have been one operator defending an invariant
        // nothing else in the language keeps. Measured, not reasoned about.
        if (is_numeric(pair.left)) {
            const Str &right = std::get<Str>(pair.right);
            const std::string text = right ? decode(*right) : std::string();
            Number read;
            if (Number::parse(text, read)) {
                if (pair.left.is_number()) {
                    Number answer =
                        Number::add(std::get<Number>(pair.left), read);
                    m.done();
                    m.fold(Value::number(std::move(answer)));
                    return;
                }
                Float answer =
                    Float::add(as_float(pair.left), Float::from_number(read));
                m.done();
                m.fold(Value::floating(std::move(answer)));
                return;
            }

            // falls through to the join below
        }

        // ANYTHING ELSE THAT HAS A TEXT FORM JOINS, which is the arm above's
        // last line and a bit run's or a hex run's only one. `is_numeric` is
        // number and float alone -- bits do not do arithmetic in this language
        // -- so without this `b1010 + "abc"` reached both_numeric and was
        // refused as "works on two numbers", which is a sentence about
        // arithmetic nobody was attempting. The left is rendered by the
        // printer's own function, so `display(x + s)` and `display("" + x + s)`
        // cannot disagree.
        if (joins_into_a_string(pair.left)) {
            const Str &right = std::get<Str>(pair.right);
            SatString joined = encode_raw(satellite::text_of(pair.left));
            if (right)
                joined.append(*right);
            m.done();
            m.fold(Value::string(std::move(joined)));
            return;
        }

        // NEITHER SIDE CAN LEAD -- `my_list + "x"`. Left to both_numeric
        // below, whose sentence names the operator and the offending type,
        // which is the right one here and was already written.
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
    // for the static_assert is the contract that a code this op carries fills
    // EXACTLY the holes its row declares, and tests/eval_test raises each row
    // so a sentence grown a hole fails a fixture rather than printing one.
    //
    // THE COUNT IS AN OPERAND SINCE M23, AND IT USED TO BE THE SENTENCE "every
    // code this op carries has exactly one {1}". S1401 has two -- what was
    // asked, and what arrived instead -- so the contract that was a comment is
    // now `d`, and the two M14 rows pass 1 where they used to pass nothing.
    //
    // A COUNT AND NOT A `c != 0` TEST, because 0 is a perfectly good text index
    // (Compiled::add_text returns the position, and the first text is at zero),
    // so a sentinel here would have been a bug that only appeared in the first
    // program in a run to raise one of these.
    //
    // RAISED AT RUN TIME for op_refuse's reason, told from the other side:
    // the misuse was DETECTED at compile, but `input(">", 3)` in a branch
    // that never runs is a program that runs.
    errors::Diagnostic problem;
    problem.code = static_cast<errors::Code>(op.a);
    problem.at = m.span_of(m.here());
    problem.arguments = {std::string(m.program().text(op.b))};
    if (op.d == 2)
        problem.arguments.push_back(std::string(m.program().text(op.c)));
    m.refuse(std::move(problem));
}

// op_dispatch LIVED HERE FROM M9 TO M11 AND MOVED WHEN IT STOPPED BEING ALONE.
// M11's method ops share its whole body except where a changed receiver goes,
// so the three arms and their one core are operations_dispatch.cpp -- the same
// split this file already has with operations_control.cpp, made on the same
// grounds: arms that share a contract live where the contract is written once.

} // namespace eval
} // namespace satellite
