// `satl --number <a> <op> <b>`. See programs/number_command.hpp.
//
// THE COMMAND LINE IS TURNED INTO A SOURCE LINE, WHICH IS WHY THIS ARM CAN
// DRAW A CARET AT ALL. M5's reporter renders a Diagnostic against an
// errors::Source -- a path and the text it names -- and a span is a byte range
// with a line number in that text. A command line has neither. Joining the
// three operands with single spaces produces exactly one line whose byte
// offsets are known while it is being built, so `satl --number 1 / 0` underlines
// the `0` the same way `satl --check` underlines a token.
//
// The `path` it renders under is the flag itself. cache_command.cpp is the
// other arm that renders against something the user did not type as a file, and
// it made the same call: a name that says where the text came from beats an
// empty one, and beats inventing a filename that does not exist.

#include "programs/number_command.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"
#include "programs/opening.hpp"
#include "satellite_number/bignum.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace satellite {

namespace {

// The three operands, joined, with a span for each. Built together because the
// spans are offsets into the join and computing them separately would be two
// places one arithmetic lives.
struct Written {
    std::string line;
    errors::Span left;
    errors::Span operation;
    errors::Span right;
};

errors::Span put(std::string &line, const std::string &piece)
{
    if (!line.empty())
        line += ' ';
    const errors::Span at = { static_cast<uint32_t>(line.size()),
                              static_cast<uint32_t>(line.size() + piece.size()),
                              1 };
    line += piece;
    return at;
}

Written write_out(const std::string &a, const std::string &op,
                  const std::string &b)
{
    Written out;
    out.left = put(out.line, a);
    out.operation = put(out.line, op);
    out.right = put(out.line, b);
    return out;
}

int report(const Written &written, const errors::Diagnostic &problem, int status)
{
    fputs(errors::render(problem, errors::Source{"--number", written.line})
              .c_str(),
          stderr);
    return status;
}

// `12345678901234567890` rather than `12345678901234567890`, which is the same
// string -- there is no thousands separator here on purpose. DESIGN §8.1.1's
// rule is to print the VALUE, and a separator is a rendering of it that no
// satellite program will ever produce, so this command showing one would be
// showing something the language does not say.
void row(const char *label, const std::string &value)
{
    printf("    %-18s%s\n", label, value.c_str());
}

// Where the digit count came from, in the words `satl --limits` already uses
// for a setting's origin.
std::string digits_origin()
{
    const limits::Dial &dial = limits::held().dial(limits::DialId::DivisionDigits);
    if (!dial.set)
        return "default";

    // NO THIRD ANSWER, AND THERE WAS ONE UNTIL 2026-08-31: `division_digits=0`
    // used to reach here as "default -- the file set it to 0, which keeps no
    // digits", because limits::division_digits() substituted 34 for it. The
    // file is refused now, with a caret under the `0`, so a dial that is set is
    // a dial whose number this ran on.
    return "satellite_config.ini line " + std::to_string(dial.line);
}

} // namespace

int number_command(const std::vector<std::string> &args)
{
    if (args.size() < 5)
        return EXIT_USAGE;   // main.cpp says what is missing

    const std::string &left_text = args[2];
    const std::string &operation = args[3];
    const std::string &right_text = args[4];
    const Written written = write_out(left_text, operation, right_text);

    Number left;
    Number right;
    if (!Number::parse(left_text, left))
        return report(written,
                      errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
                          written.left, left_text),
                      EXIT_MALFORMED);
    if (!Number::parse(right_text, right))
        return report(written,
                      errors::make<errors::Code::NUMBER_NOT_A_NUMBER>(
                          written.right, right_text),
                      EXIT_MALFORMED);

    // THE COUNT IS READ BEFORE THE OPERATION AND PRINTED WHETHER OR NOT A
    // DIVISION HAPPENS, because the point of showing it is that a person can
    // see what their config file did. limits::division_digits() is the dial's
    // meaning and this milestone is what gave it one.
    const unsigned digits = limits::division_digits();

    Number answer;
    bool exact = true;
    if (operation == "+") {
        answer = Number::add(left, right);
    } else if (operation == "-") {
        answer = Number::sub(left, right);
    } else if (operation == "*") {
        answer = Number::mul(left, right);
    } else if (operation == "/") {
        // THE CARET GOES UNDER THE ZERO AND THE SENTENCE NAMES THE DIVIDEND,
        // which is the pair rather than a choice between them: the sentence
        // says what could not be done and the caret says where the problem is.
        // Pointing at the dividend was the first version and it reads as
        // though `7` were the thing that is wrong.
        if (right.is_zero())
            return report(written,
                          errors::make<errors::Code::NUMBER_DIVIDE_BY_ZERO>(
                              written.right, left.to_string()),
                          EXIT_MALFORMED);
        answer = Number::divide(left, right, digits);

        // EXACTNESS IS MEASURED AND NOT ASSUMED, by multiplying the answer back
        // and comparing. divide() does not report whether it rounded, and it
        // should not have to: the caller already has everything needed to ask,
        // and asking is one multiplication against a division that has just
        // done up to `digits` of long division. It matters because DESIGN §8.1
        // promises that a division which TERMINATES is exact -- 1/2^100 has 69
        // places and comes back whole -- and a person cannot tell that from
        // 1/3 by looking at the digits.
        exact = Number::mul(answer, right) == left;
    } else if (operation == "%") {
        // THE FIFTH OPERATOR, AND IT ANSWERED S0612 UNTIL 2026-08-31 -- "`%` is
        // not built yet ... PLAN.md §8 puts it at M15 with the rounding rule it
        // needs". It does not need one. DESIGN §8.6 specifies modulus in full
        // and puts `%` in the class it calls "exact and bounded, never rounds":
        // the quotient is only ever wanted as an integer, so the inexact tail of
        // a division is discarded before it can matter. What M15 owns is the
        // rounding rule, and this is the one of the six deferred methods that
        // was never waiting on it.
        //
        // A zero divisor is the same refusal `/` gets, for the same reason: 7 %
        // 0 has no answer either, and S0601's sentence covers both.
        if (right.is_zero())
            return report(written,
                          errors::make<errors::Code::NUMBER_DIVIDE_BY_ZERO>(
                              written.right, left.to_string()),
                          EXIT_MALFORMED);
        answer = Number::modulo(left, right);
    } else {
        return report(written,
                      errors::make<errors::Code::NUMBER_UNKNOWN_OPERATOR>(
                          written.operation, operation),
                      EXIT_MALFORMED);
    }

    printf("  %s\n\n", written.line.c_str());
    printf("  = %s\n\n", answer.to_string().c_str());
    row("digits", std::to_string(answer.digit_count()));
    row("exact", exact ? "yes"
                       : "no -- rounded to " + std::to_string(digits) +
                             " significant digits");
    row("division_digits", std::to_string(digits) + "  (" + digits_origin() + ")");

    // WHAT IT COSTS TO HOLD, which is DESIGN §8.2's argument made visible.
    // A number that never left the small form allocated nothing and says so;
    // one that boxed says how many bytes it took beyond the value node. This is
    // the only place in satl a person can watch the promotion happen.
    const size_t payload = answer.payload_bytes();
    row("storage", payload == 0
                       ? "inline -- no allocation, " +
                             std::to_string(sizeof(Number)) + " bytes"
                       : "boxed -- " + std::to_string(payload) +
                             " bytes beyond the " +
                             std::to_string(sizeof(Number)) + "-byte value");
    return EXIT_FINE;
}

} // namespace satellite
