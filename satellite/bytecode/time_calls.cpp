// satellite/bytecode/time_calls.cpp -- satellite.time's words: three that make a time, now, which reads
// the clock, and the sleep that waits one out -- and the unit now and a time's .satellite.time.convert
// take, read in one place. time_calls.hpp says why they have no library and how a sleep is stopped.

#include "time_calls.hpp"

#include "program_walk.hpp"
#include "word_codes.hpp"
#include "../machine/stop_flag.hpp"
#include "../machine/thread_stop.hpp"
#include "../satellite_object/object_lock.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <ctime>

namespace satellite004 {
namespace {

using token::Code;

enum class TimeWord { now, sleep, sleep_with_a_unit, second, millisecond, nanosecond, other };

TimeWord time_word_of(Code code)
{
    if (code == word::fixed_code<1, 9, 1>) return TimeWord::now;
    if (code == word::fixed_code<1, 9, 3>) return TimeWord::sleep;
    if (code == word::fixed_code<1, 9, 4>) return TimeWord::sleep_with_a_unit;
    if (code == word::fixed_code<1, 9, 5>) return TimeWord::second;
    if (code == word::fixed_code<1, 9, 6>) return TimeWord::millisecond;
    if (code == word::fixed_code<1, 9, 7>) return TimeWord::nanosecond;
    return TimeWord::other;
}

satellite_time::Unit unit_of(TimeWord word)
{
    return word == TimeWord::millisecond  ? satellite_time::Unit::millisecond
           : word == TimeWord::nanosecond ? satellite_time::Unit::nanosecond
                                          : satellite_time::Unit::second;
}

// THE UNIT now AND convert TAKE (TIME_PLAN.md Section 2; the author, 2026-10-26: "you can type "nanoseconds" or
// "milliseconds" or "s" or "ns" or "ms" but if nothing is given just default to nanoseconds"): six spellings, each
// written exactly and in small letters. False for anything else, and `unit` untouched.
bool unit_named(const std::string &written, satellite_time::Unit &unit)
{
    if (written == "seconds" || written == "s") { unit = satellite_time::Unit::second; return true; }
    if (written == "milliseconds" || written == "ms") { unit = satellite_time::Unit::millisecond; return true; }
    if (written == "nanoseconds" || written == "ns") { unit = satellite_time::Unit::nanosecond; return true; }
    return false;
}

// WHAT A UNIT THAT IS NOT ONE OF THE SIX IS TOLD -- the walker's sentence and the checker's, so the two never part.
// `receiver` and `word` are the call (time_calls.hpp), and `given` the string in its quotes, "minutes", or what was
// given in its place, "a number".
std::string not_a_unit(const std::string &receiver, const char *word, const std::string &given)
{
    return receiver + word + " takes a unit -- \"seconds\" or \"s\", \"milliseconds\" or \"ms\", \"nanoseconds\" or "
                             "\"ns\" -- and was given " + given;
}

// HOW OFTEN A LONG SLEEP LOOKS UP: a tenth of a second, program join()'s own (kJoinLooksUp), so a
// stop is seen as soon in one as in the other.
constexpr long long int kLooksUpEvery = 100000000;   // nanoseconds

// THE LONGEST STEP THE DEADLINE MOVES IN -- 2^62 nanoseconds, 146 years -- so a time no 64-bit count
// can hold is still waited out a step at a time: a satellite_number has no ceiling, and neither has
// a sleep (the language has no limits). Every step is added to the same deadline, so none drifts.
constexpr long long int kLongestStep = 1ll << 62;

// WHY THE SLEEP MUST END NOW, or success: its thread was asked to stop, the program was told to end
// on another thread -- the two a join() asks -- or Ctrl-C was pressed at the prompt (stop_flag.hpp),
// which a loop asks. The flag's place is read ONCE: the session nulls it when it ends, and a thread
// it started may still be asleep then (the review, 2026-10-10).
signed long long int asked_to_stop()
{
    if (stop_of_this_thread != nullptr && stop_of_this_thread->load(std::memory_order_relaxed))
        return thread_stopped;
    if (program_quit().load(std::memory_order_relaxed))
        return program_returned;
    const volatile sig_atomic_t *flag = stop_flag();
    if (flag != nullptr && *flag != 0)
        return interrupted;
    return success;
}

timespec monotonic_now()
{
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now;
}

void move_on(timespec &when, long long int nanoseconds)   // 0 <= nanoseconds <= kLongestStep
{
    when.tv_sec += static_cast<time_t>(nanoseconds / 1000000000);
    when.tv_nsec += static_cast<long>(nanoseconds % 1000000000);
    if (when.tv_nsec >= 1000000000) {
        when.tv_nsec -= 1000000000;
        when.tv_sec += 1;
    }
}

bool earlier(const timespec &left, const timespec &right)
{
    return left.tv_sec < right.tv_sec || (left.tv_sec == right.tv_sec && left.tv_nsec < right.tv_nsec);
}

// UNTIL `deadline` ON THE MONOTONIC CLOCK, looking up every tenth of a second. An absolute deadline
// is 003's way (clock_nanosleep with TIMER_ABSTIME): a wait woken early -- by a signal, or to look
// up -- carries on toward the same moment, with no arithmetic and no drift.
signed long long int wait_until(const timespec &deadline)
{
    for (;;) {
        const signed long long int stop = asked_to_stop();
        if (stop != success)
            return stop;
        timespec wake = monotonic_now();
        if (!earlier(wake, deadline))
            return success;
        move_on(wake, kLooksUpEvery);
        if (earlier(deadline, wake))
            wake = deadline;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, nullptr);   // early or not, look up again
    }
}

signed long long int sleep_for(const satellite_number &length)
{
    static const satellite_number longest_step(static_cast<unsigned long long int>(kLongestStep));
    timespec deadline = monotonic_now();
    satellite_number left = length;
    do {
        long long int step = kLongestStep;
        if (satellite_number::compare(left, longest_step) > 0) {
            left = left - longest_step;
        } else {
            step = static_cast<long long int>(number_fast_path::as_count(left));
            left = satellite_number();
        }
        move_on(deadline, step);
        const signed long long int stop = wait_until(deadline);
        if (stop != success)
            return stop;
    } while (!left.is_zero());
    return success;
}

// A FLOAT'S OWN DIGITS -- "1.5", "-0.25", "2" -- exactly, from its parts: the fraction is a whole
// number over 10^places (satellite_float.hpp), so .05 is 5 at 2 places and is written 05.
std::string digits_of(const satellite_float &real)
{
    std::string written = (real.negative ? "-" : "") + real.whole.to_text();
    if (real.places == 0)
        return written;
    std::string fraction = real.fraction.to_text();
    if (fraction.size() < real.places)
        fraction.insert(0, real.places - fraction.size(), '0');
    return written + "." + fraction;
}

// satellite.time.second(1), .millisecond(250), .nanosecond(30): a number, whole or not.
Value make_a_time(TimeWord word, const Value &given, ExpressionContext &context)
{
    const satellite_time::Unit unit = unit_of(word);
    const std::string spelled = std::string("satellite.time.") + satellite_time::named(unit);
    if (const satellite_number *count = given.as_number())
        return Value::of_time(satellite_time::of_count(*count, unit));
    if (const satellite_float *real = given.as_float()) {
        satellite_time length;
        const std::string digits = digits_of(*real);
        if (satellite_time::from_digits(digits, unit, length) != success) {
            context.refuse(int_error, spelled + " could not read " + digits + " as a number");
            return Value();
        }
        return Value::of_time(std::move(length));
    }
    context.refuse(types_do_not_meet, spelled + " takes a number -- " + spelled + "(1) or " + spelled +
                                          "(1.5) -- and was given " + given.kind_name());
    return Value();
}

// satellite.time.now(unit): THE COMPUTER'S CLOCK, the time since 1970-01-01 00:00:00 UTC (the author, 2026-10-26, as he
// ruled for 003 on 2026-09-04: the value now answers is the clock, and sleep waits on the monotonic one). Read from
// CLOCK_REALTIME fresh at every call, as 003's now_nanoseconds() read system_clock, and held exactly, as every time
// is: whole nanoseconds, every one of them kept ("keep every nanosecond"), and the unit only how it is shown -- so
// now("s") shows 1793036700.123456789s, and now() 1793036700123456789ns.
Value now(const std::vector<Value> &arguments, ExpressionContext &context)
{
    satellite_time read;
    if (!unit_given(arguments, std::string(), "satellite.time.now", read.unit, context))
        return Value();
    timespec clock{};
    clock_gettime(CLOCK_REALTIME, &clock);
    read.nanoseconds = satellite_number::from_signed(clock.tv_sec) * satellite_time::one(satellite_time::Unit::second) +
                       satellite_number(static_cast<unsigned long long int>(clock.tv_nsec));
    return Value::of_time(std::move(read));
}

// WHAT sleep SAYS OF AN ARGUMENT THAT IS NOT A TIME -- the walker's sentence and the checker's, so
// the two never part. A BARE NUMBER NAMES NO UNIT, and three are possible: 5 seconds and 5
// nanoseconds differ a billion times over, so it is refused with the three ways to say it (DESIGN
// 1.1: the unit is seen at the call site), in the number's own digits -- 1.5 as 1.5. A number below
// zero is told that a wait is 0 or longer, not shown three spellings that would each be S440. Text
// or anything else (`shown` empty) is shown 5.
std::string not_a_time(const std::string &kind, const std::string &shown)
{
    if (!shown.empty() && shown.front() == '-')
        return "satellite.time.sleep takes a time, written with its unit, and a wait is 0 or longer -- and was "
               "given " + kind + ", " + shown;
    const std::string digits = shown.empty() ? std::string("5") : shown;
    return "satellite.time.sleep takes a time, written with its unit -- " + digits + "s, " + digits + "ms or " +
           digits + "ns -- and was given " + kind;
}

// satellite.time.sleep(1s): this thread waits that long, and answers nothing.
Value sleep(const Value &given, ExpressionContext &context)
{
    const satellite_time *length = given.as_time();
    if (length == nullptr) {
        std::string shown;
        if (const satellite_number *count = given.as_number())
            shown = count->to_text();
        else if (const satellite_float *real = given.as_float())
            shown = digits_of(*real);
        context.refuse(types_do_not_meet, not_a_time(given.kind_name(), shown));
        return Value();
    }
    if (length->negative()) {
        context.refuse(time_below_zero, "satellite.time.sleep was given " + length->written() +
                                            ", and a wait is 0 or longer");
        return Value();
    }
    // IT LETS GO OF THIS LINE'S OBJECT LOCKS WHILE IT WAITS, as join() does (object_lock.hpp): its
    // argument was read under them and is its own copy now, and the wait reads nothing -- so
    // `satellite.time.sleep(pause)` in a capsule of a .lock()ed object no longer makes every writer
    // of it wait out the pause (the review, 2026-10-10). Taken back once it has waited; a sleep that
    // was stopped ends its statement, and the holds give back only what they hold.
    let_go_while_waiting();
    const signed long long int stopped = sleep_for(length->nanoseconds);
    switch (stopped) {
    case success:
        if (const signed long long int back = take_back_after_waiting(); back != success)
            context.refuse(back, "satellite.time.sleep(" + length->written() + ") ended, and taking this line's "
                                 "object locks back would never end");
        return Value();
    // A STOP OR THE PROGRAM'S END IS QUIET, as it is for a join() (program_calls.cpp): the walk ends
    // there, and nothing went wrong to say.
    case thread_stopped:
        context.refuse(thread_stopped, "a satellite.time.sleep on a thread asked to stop");
        context.reported = true;
        return Value();
    case program_returned:
        context.refuse(program_returned, "a satellite.time.sleep when satellite.return(satellite) was reached");
        context.reported = true;
        return Value();
    default:
        context.refuse(stopped, "satellite.time.sleep(" + length->written() + ") was stopped with Ctrl-C");
        return Value();
    }
}

// ONE LITERAL STANDING ALONE IN A CALL'S BRACKETS -- a minus allowed in front of a number, and the bracket
// straight after it -- and what it is: a number, a float, a fraction, a string, a binary, a hex, a percentage, a
// bool or a time. `shown` is a number's own digits, sign and all, and a string's text. how_many_kinds for anything
// worked out -- 5 * 1s, a name, a call -- which is the walker's to judge. `at` is on the argument. sleep's literal
// and the unit now and convert are given are read by it, so the checker reads all three one way.
satelliteObject::Kind a_literal_alone(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &shown)
{
    using O = satelliteObject;
    std::size_t k = at;
    const bool below_zero = code_at(row, k) == token::tight_minus_token || code_at(row, k) == token::minus_token;
    if (below_zero)
        ++k;
    const Code literal = code_at(row, k);
    O::Kind kind = O::how_many_kinds;
    if (literal == token::number_token) {
        shown = text_at(row, k);
        kind = shown.find('.') == std::string::npos ? O::number : O::floating;
        if (code_at(row, k) == token::fraction_token) {        // 1/2, touching, is one fraction
            ++k;
            if (code_at(row, k) != token::number_token)
                return O::how_many_kinds;
            text_at(row, k);
            kind = O::fraction;
            shown.clear();
        } else if (below_zero) {
            shown.insert(0, "-");
        }
    } else if (below_zero) {
        return O::how_many_kinds;                              // a minus on text is the walker's to say
    } else if (literal == token::string_token) {
        shown = string_at(row, k);
        kind = O::string;
    } else if (literal == token::binary_token || literal == token::hexadecimal_token ||
               literal == token::percentage_token || literal == token::time_token) {
        text_at(row, k);
        kind = literal == token::binary_token        ? O::binary
               : literal == token::hexadecimal_token ? O::hexadecimal
               : literal == token::percentage_token  ? O::percentage
                                                     : O::time;
    } else if (literal == word::code_of(1, 17, 1) || literal == word::code_of(1, 17, 2)) {
        ++k;                                                   // satellite.bool.false, .true
        kind = O::boolean;
    } else {
        return O::how_many_kinds;
    }
    if (code_at(row, k) != token::right_parenthesis_token)
        return O::how_many_kinds;                              // worked out: 5 * 1s is the walker's
    return kind;
}

} // namespace

bool is_time_word(Code code)
{
    return time_word_of(code) != TimeWord::other;
}

signed long long int time_word_refused(Code code, std::size_t given, std::string &why)
{
    const TimeWord word = time_word_of(code);
    if (word == TimeWord::other)
        return success;
    // now TAKES A UNIT OR NOTHING (TIME-1): now() is the nanosecond's, so 0 is as right as 1.
    if (word == TimeWord::now)
        return unit_call_refused(true, given, std::string(), "satellite.time.now", why);
    if (word == TimeWord::sleep || word == TimeWord::sleep_with_a_unit) {
        // sleep(n, unit) IS 003's SPELLING -- sleep(90, "ms") -- and the unit is written on the number
        // now (the author, 2026-10-10: "satellite.time.sleep(1s or 1ms or 1ns)"). Two arguments of any
        // kind lex to that row, so this is also what sleep(1s, 2s) is told.
        if (given != 1) {
            why = "satellite.time.sleep takes one time, with its unit written on the number -- "
                  "satellite.time.sleep(90ms) -- and was given " + std::to_string(given) +
                  (given == 1 ? " argument" : " arguments");
            return satl_line_not_understood;
        }
        return success;
    }
    if (given != 1) {
        const std::string spelled = std::string("satellite.time.") + satellite_time::named(unit_of(word));
        why = spelled + " takes one number -- " + spelled + "(1) -- and was given " + std::to_string(given) +
              (given == 1 ? " argument" : " arguments");
        return satl_line_not_understood;
    }
    return success;
}

signed long long int time_literal_refused(Code code, const std::vector<std::bitset<16>> &row, std::size_t at,
                                          std::string &why)
{
    const TimeWord word = time_word_of(code);
    if (word == TimeWord::now)
        return unit_literal_refused(row, at, std::string(), "satellite.time.now", why);
    if (word != TimeWord::sleep && word != TimeWord::sleep_with_a_unit)
        return success;
    std::string shown;
    const satelliteObject::Kind kind = a_literal_alone(row, at, shown);
    if (kind == satelliteObject::how_many_kinds || kind == satelliteObject::time)
        return success;
    // A NUMBER IS SHOWN IN ITS OWN DIGITS, and anything else as 5 (not_a_time).
    why = not_a_time(satelliteObject::kind_name_of(kind),
                     kind == satelliteObject::number || kind == satelliteObject::floating ? shown : std::string());
    return types_do_not_meet;
}

signed long long int unit_call_refused(bool bracketed, std::size_t given, const std::string &receiver, const char *word,
                                       std::string &why)
{
    if (bracketed && given <= 1)
        return success;
    const std::string spelled = receiver + word;
    const std::string both = spelled + "() or " + spelled + "(\"s\")";
    if (!bracketed)
        why = spelled + " is called with brackets -- " + both;
    else
        why = spelled + " takes one unit or none -- " + both + " -- and was given " + std::to_string(given) +
              " arguments";
    return satl_line_not_understood;
}

signed long long int unit_literal_refused(const std::vector<std::bitset<16>> &row, std::size_t at,
                                          const std::string &receiver, const char *word, std::string &why)
{
    std::string shown;
    satellite_time::Unit unit = satellite_time::Unit::nanosecond;
    satelliteObject::Kind kind = a_literal_alone(row, at, shown);
    // A MINUS ON A TIME, A PERCENTAGE, A HEX OR A BINARY -- now(-1s), convert(-50%) -- is still one literal and no
    // unit, so it is refused here as now(1s) is (the fresh read, 2026-10-26). a_literal_alone is sleep's reader too,
    // and leaves a minus on them to the walker, so this asks again past the sign and sleep's answers stay as they
    // were. A minus on text or a bool keeps the walker's own sentence.
    const Code sign = code_at(row, at), after = code_at(row, at + 1);
    if (kind == satelliteObject::how_many_kinds && (sign == token::tight_minus_token || sign == token::minus_token) &&
        (after == token::time_token || after == token::percentage_token || after == token::hexadecimal_token ||
         after == token::binary_token))
        kind = a_literal_alone(row, at + 1, shown);    // still none when it is worked out: -1s + 2s
    if (kind == satelliteObject::how_many_kinds || (kind == satelliteObject::string && unit_named(shown, unit)))
        return success;
    why = not_a_unit(receiver, word, kind == satelliteObject::string ? "\"" + shown + "\""
                                                                     : std::string(satelliteObject::kind_name_of(kind)));
    return types_do_not_meet;
}

bool unit_given(const std::vector<Value> &arguments, const std::string &receiver, const char *word,
                satellite_time::Unit &unit, ExpressionContext &context)
{
    if (arguments.empty()) {
        unit = satellite_time::Unit::nanosecond;
        return true;
    }
    const Value &given = arguments.front();
    if (!given.is_string()) {
        context.refuse(types_do_not_meet, not_a_unit(receiver, word, given.kind_name()));
        return false;
    }
    const std::string written = given.text_utf8();
    if (unit_named(written, unit))
        return true;
    context.refuse(types_do_not_meet, not_a_unit(receiver, word, "\"" + written + "\""));
    return false;
}

bool a_time_part(const std::vector<std::bitset<16>> &row, std::size_t dot)
{
    return code_at(row, dot) == token::satellite_method_token && code_at(row, dot + 1) == token::time_piece_token;
}

std::string time_part_written_alone(const std::string &receiver)
{
    return receiver + ".satellite.time -- time is the start of a word: " + receiver + ".satellite.time.convert(...)";
}

Value call_time_word(Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    std::string why;
    if (const signed long long int refused = time_word_refused(code, arguments.size(), why); refused != success) {
        context.refuse(refused, why);
        return Value();
    }
    const TimeWord word = time_word_of(code);
    // now IS THE ONE WORD GIVEN NOTHING AS OFTEN AS SOMETHING, so it is asked before arguments.front() is read.
    if (word == TimeWord::now)
        return now(arguments, context);
    if (word == TimeWord::sleep || word == TimeWord::sleep_with_a_unit)
        return sleep(arguments.front(), context);
    return make_a_time(word, arguments.front(), context);
}

} // namespace satellite004
