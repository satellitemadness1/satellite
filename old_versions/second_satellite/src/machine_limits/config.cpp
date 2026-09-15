// Reading a `satellite_config.ini`. See machine_limits/limits.hpp for the door
// and machine_limits/config_internal.hpp for what the file may contain.
//
// THE FILE FORMAT IS THE SMALLEST ONE THAT CAN SAY WHAT §4.5 ASKS FOR: a line
// is `NAME=VALUE`, a `#` comment, or blank. There are no sections, no
// continuations, no quoting and no includes, and every one of those absences is
// a decision rather than a corner not yet reached -- a settings file that grows
// a syntax grows a parser, and this tree already has one of those for the
// language it is actually about. `[sections]` get their own clause in S0801's
// sentence because `.ini` promises them and somebody will write one.
//
// EVERY PROBLEM IS REPORTED AND THE FILE IS THEN REFUSED WHOLE, which is the
// shape M5 built the reporter for: one pass, a vector of diagnostics, nothing
// thrown, and the caller decides what a failure is worth. A config with three
// typos in it should cost one run to fix and not three.
//
// AND A MALFORMED FILE IS REFUSED RATHER THAN IGNORED, which is the one place
// this differs from M4.5's `.satc` reader; errors.def's S08xx block note carries
// the argument. A cache can be thrown away. A statement of how much of this
// machine satl may take cannot be guessed at.

#include "machine_limits/config_internal.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite::limits {

std::vector<std::string_view> config_keys()
{
    std::vector<std::string_view> keys;
    keys.reserve(kSettingCount + kDialCount);
    for (size_t i = 0; i < kSettingCount; i++)
        keys.push_back(kSettings[i].name);
    for (size_t i = 0; i < kDialCount; i++)
        keys.push_back(dial_name(static_cast<DialId>(i)));
    return keys;
}

std::string_view origin_text(Origin origin)
{
    switch (origin) {
    case Origin::Default: return "the machine";
    case Origin::File:    return "the file";
    case Origin::Machine: return "the machine, named by the file";
    case Origin::Clamped: return "the machine, over the file";
    }
    return "?";
}

namespace {

// One line of the file, already located.
struct Line {
    size_t start = 0;      // first byte of the line
    size_t stop = 0;       // one past its last byte, the newline excluded
    unsigned number = 0;   // 1-based
};

// What the reader is carrying while it walks: where a key was first set, so a
// second one can point at it, and what it is filling in.
struct Reading {
    Held &into;
    std::vector<errors::Diagnostic> &problems;
    Line first_set[kSettingCount + kDialCount];
};

// PLAN §4.5's three settings and words.def's four dials write into different
// places, so a key's index picks which. The dials run after the settings in
// exactly DialId order, which is what makes this subtraction legal and is why
// config_internal.hpp builds the key list in that order rather than
// alphabetically.
//
// A POINTER AND NOT A COPY, WHICH IS WHAT KEEPS `fact` OUT OF THE FILE'S REACH.
// The old shape assigned a whole `Setting` over the top of the one in `Held`,
// and that was fine while a Setting was three numbers; it is not fine now that
// one of its members says which machine fact is behind the row. A file chooses
// whether the fact answers, never which fact it is, so the reader writes the
// three fields it owns and never the fourth.
Setting *machine_setting(Held &into, size_t key)
{
    switch (key) {
    case 0: return &into.thread_count;
    case 1: return &into.core_count;
    case 2: return &into.memory_max;
    default: return nullptr;
    }
}

void store_number(Held &into, size_t key, unsigned long long value, unsigned line)
{
    if (Setting *const at = machine_setting(into, key)) {
        at->written = value;
        at->origin = Origin::File;
        at->line = line;
        return;
    }
    Dial &dial = into.dials[key - kSettingCount];
    dial = Dial{value, true, Origin::File, line};
}

// `CORE_COUNT=arguments.machine.cores`: the machine answers, because the file
// said to. No value is stored -- there is nothing to store, and reading one now
// is the 0.42 ms limits.cpp exists to stop paying.
void store_fact(Held &into, size_t key, unsigned line)
{
    Setting *const at = machine_setting(into, key);
    at->origin = Origin::Machine;
    at->line = line;
}

// A whole number, bounded. False having reported.
bool count_value(Reading &reading, std::string_view text, std::string_view name,
                 size_t at, size_t stop, unsigned line, unsigned long long &into)
{
    if (!whole_number(text, at, stop, into)) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_NOT_A_NUMBER>(
            span(at, stop, line), name, text.substr(at, stop - at)));
        return false;
    }
    if (into < 1) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_TOO_SMALL>(
            span(at, stop, line), name, into, 1u));
        return false;
    }
    if (into > kMostThreads) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_TOO_LARGE>(
            span(at, stop, line), name, into, kMostThreads));
        return false;
    }
    return true;
}

// A dial's value against the range its meaning implies, when it has one. False
// having reported.
//
// THE SPAN IS HANDED IN RATHER THAN REBUILT, because this runs after
// `whole_number` has already succeeded over exactly those offsets -- one span,
// two possible sentences about it -- where every other check in this file is
// still deciding where the caret goes.
bool dial_in_range(Reading &reading, std::string_view name, size_t dial,
                   unsigned long long value, errors::Span where)
{
    const DialRange &range = kDialRanges[dial];
    if (!range.checked)
        return true;
    if (value < range.least) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_TOO_SMALL>(
            where, name, value, range.least));
        return false;
    }
    if (value > range.most) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_TOO_LARGE>(
            where, name, value, range.most));
        return false;
    }
    return true;
}

// A whole number of units, with the unit written down. False having reported.
//
// THE AMOUNT AND THE SUFFIX GET DIFFERENT CARETS, which is the whole reason
// this is three checks rather than one regular expression. `61.9GiB` is refused
// with the caret under `61.9` and `48 gigs` with the caret under `gigs`, and a
// person reading either one knows which half to edit without being told.
//
// FRACTIONS ARE REFUSED, AND THAT IS A DECISION. `61.9GiB` is 66,461,528,883.2
// bytes, and a ceiling that has been rounded to a whole byte behind the user's
// back is a ceiling nobody can predict the behaviour of -- so satl asks for the
// next unit down instead, where the number they meant is exact. It costs one
// error message once, and it is the same argument the unit itself is required
// for.
bool size_value(Reading &reading, std::string_view text, std::string_view name,
                size_t at, size_t stop, unsigned line, unsigned long long &into)
{
    size_t digits = at;
    while (digits < stop && text[digits] >= '0' && text[digits] <= '9')
        digits++;

    // A FRACTION IS CAUGHT HERE RATHER THAN FALLING THROUGH TO THE UNIT, and
    // the first version of this function did fall through: `61.9GiB` parsed
    // `61`, then asked what unit `.9GiB` is and answered "`.9GiB` is not a
    // unit". True, useless, and pointing at the wrong half of the value. The
    // caret belongs under `61.9`.
    if (digits < stop && (text[digits] == '.' || text[digits] == ',')) {
        size_t shown = digits + 1;
        while (shown < stop && text[shown] >= '0' && text[shown] <= '9')
            shown++;
        reading.problems.push_back(
            errors::make<errors::Code::CONFIG_NOT_A_WHOLE_AMOUNT>(
                span(at, shown, line), name, text.substr(at, shown - at)));
        return false;
    }

    unsigned long long amount = 0;
    if (digits == at || !whole_number(text, at, digits, amount)) {
        // Everything up to a blank or the end, so `61.9` is quoted whole
        // rather than as the `61` that parsed.
        size_t shown = at;
        while (shown < stop && !blank(text[shown]))
            shown++;
        reading.problems.push_back(errors::make<errors::Code::CONFIG_NOT_A_NUMBER>(
            span(at, shown, line), name, text.substr(at, shown - at)));
        return false;
    }

    size_t suffix = digits;
    while (suffix < stop && blank(text[suffix]))
        suffix++;
    if (suffix >= stop) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_NEEDS_A_UNIT>(
            span(at, digits, line), name, text.substr(at, digits - at)));
        return false;
    }

    const std::string_view written = text.substr(suffix, stop - suffix);
    const unsigned long long multiplier = unit_multiplier(written);
    if (multiplier == 0) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_NO_SUCH_UNIT>(
            span(suffix, stop, line), written));
        return false;
    }

    if (amount < 1) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_TOO_SMALL>(
            span(at, digits, line), name, amount, 1u));
        return false;
    }
    // Refused rather than wrapped, for the reason whole_number() gives about
    // its own overflow: a ceiling that silently became a small number is a
    // watchdog that fires on a healthy process.
    if (amount > ~0ULL / multiplier) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_TOO_LARGE>(
            span(at, stop, line), name, text.substr(at, stop - at),
            std::to_string(~0ULL / multiplier) + std::string(written)));
        return false;
    }

    into = amount * multiplier;
    return true;
}

// `arguments.machine.cores` -- the machine's own answer, named. False having
// reported.
//
// ONE PATH PER SETTING AND NOT "ANY PATH OF THE RIGHT SHAPE", which is DESIGN
// §7.7's pairing enforced rather than merely documented. `CORE_COUNT` takes
// `arguments.machine.cores` and nothing else, so `CORE_COUNT=arguments.memory.total`
// is refused with the right one named -- and it has to be, because both walk to
// a real node and a shape check could not tell them apart.
bool fact_value(Reading &reading, std::string_view text, std::string_view name,
                size_t key, size_t at, size_t stop, unsigned line)
{
    const std::string_view written = text.substr(at, stop - at);
    const words::NodeId wants = kSettings[key].spells;
    if (fact_named(written) == static_cast<words::PathId>(wants))
        return true;

    const std::string wanted = fact_spelling(wants);
    errors::Diagnostic wrong = errors::make<errors::Code::CONFIG_NOT_A_FACT>(
        span(at, stop, line), name, wanted, written);

    // THE FULL PATH IS A DIFFERENT MISTAKE FROM A TYPO AND IS ANSWERED FIRST.
    // WORD_NUMBERS.md §2.2 writes this fact down as
    // `satellite.library.main.arguments.machine.cores`, and `satl --words`
    // answers to that spelling, so somebody who looked the name up in the
    // authority will copy the rooted form -- which is correct everywhere except
    // here. It walks, it walks to the RIGHT node, and it is still not what a
    // program writes (DESIGN §7.7), so the answer is the short spelling rather
    // than the sentence about what this setting takes.
    const words::Walk rooted = words::walk(written);
    if (rooted.error == words::WalkError::NONE &&
        rooted.id == static_cast<words::PathId>(wants)) {
        wrong.suggestion = wanted;
    } else {
        // Otherwise M5's distance, over one candidate rather than a list --
        // `arguments.machine.core` is a typo and gets an answer, `twelve` is
        // not close to anything and gets the sentence on its own.
        //
        // BOTH TESTS, AND close_enough() ALONE IS NOT ONE OF THEM. distance()
        // is CAPPED: it returns kTooFar for anything at or past it rather than
        // the real figure, "so the loop stops paying for a word it has already
        // ruled out". close_enough(4, 23) is true -- one edit plus one per four
        // characters -- so a candidate this long would accept the cap itself as
        // a score and offer `arguments.machine.cores` for `twelve`, which is
        // what the first version of this did. config_internal.hpp's
        // nearest_key() has the same pair of tests one function up, where the
        // `edits < closest` half hides it inside the search.
        const size_t edits = errors::distance(written, wanted);
        const size_t longest = std::max(written.size(), wanted.size());
        if (edits < errors::kTooFar && errors::close_enough(edits, longest))
            wrong.suggestion = wanted;
    }

    reading.problems.push_back(std::move(wrong));
    return false;
}

// One `NAME=VALUE`, with `at`..`stop` already trimmed of blanks.
void setting(Reading &reading, std::string_view text, const Line &at)
{
    size_t start = at.start;
    size_t stop = at.stop;
    trim(text, start, stop);

    const size_t equals = text.find('=', start);
    if (equals == std::string_view::npos || equals >= stop || equals == start) {
        reading.problems.push_back(errors::make<errors::Code::CONFIG_NOT_A_SETTING>(
            span(start, stop, at.number), text.substr(start, stop - start)));
        return;
    }

    size_t name_start = start;
    size_t name_stop = equals;
    trim(text, name_start, name_stop);
    const std::string_view name = text.substr(name_start, name_stop - name_start);

    const size_t key = key_index(name);
    if (key == kSettingCount + kDialCount) {
        errors::Diagnostic unknown =
            errors::make<errors::Code::CONFIG_NO_SUCH_SETTING>(
                span(name_start, name_stop, at.number), name);
        unknown.suggestion = std::string(nearest_key(name));
        reading.problems.push_back(std::move(unknown));
        return;
    }

    if (reading.first_set[key].number != 0) {
        const Line &first = reading.first_set[key];
        errors::Diagnostic twice = errors::make<errors::Code::CONFIG_SET_TWICE>(
            span(name_start, name_stop, at.number), name);
        size_t was_start = first.start;
        size_t was_stop = first.stop;
        trim(text, was_start, was_stop);
        twice.notes.push_back(errors::note<errors::Code::NOTE_CONFIG_SET_HERE>(
            span(was_start, was_stop, first.number), name));
        reading.problems.push_back(std::move(twice));
        return;
    }
    reading.first_set[key] = at;

    size_t value_start = equals + 1;
    size_t value_stop = stop;
    trim(text, value_start, value_stop);

    // A DIAL HAS A KIND OF ITS OWN NOW, and config_internal.hpp's kDialKinds
    // says why: `max_depth` is bytes and every other dial is a bare count.
    const Kind kind = key < kSettingCount ? kSettings[key].kind
                                          : kDialKinds[key - kSettingCount];

    // WHETHER A FACT MAY BE WRITTEN IS A SEPARATE QUESTION FROM THE SYNTAX, and
    // the two used to be one test. `arguments.memory.total` is legal for
    // MEMORY_MAX because DESIGN §7.7 pairs that setting with that path; no dial
    // has such a pairing, whatever its kind.
    const bool takes_a_fact = key < kSettingCount;

    // NOTHING AFTER THE `=` IS ANSWERED BY WHICHEVER SENTENCE LISTS EVERYTHING
    // THE VALUE COULD HAVE BEEN, which is why this is under the kind rather
    // than above it. A dial takes a number and is told so; a machine setting
    // takes a number OR the machine's own answer and is told both, because
    // half an answer to `CORE_COUNT=` sends somebody looking up a core count
    // they never needed to write down.
    if (value_start == value_stop) {
        if (takes_a_fact)
            fact_value(reading, text, name, key, value_start, value_stop,
                       at.number);
        else
            reading.problems.push_back(
                errors::make<errors::Code::CONFIG_NOT_A_NUMBER>(
                    span(equals, stop, at.number), name, ""));
        return;
    }

    // A MACHINE SETTING'S VALUE IS A NUMBER IF IT STARTS WITH A DIGIT AND THE
    // MACHINE'S OWN ANSWER OTHERWISE, and that one rule is the whole of the
    // dispatch. It is a leading digit and not a search for a `.`, because the
    // three things a number can be wrong about all begin with one -- `48` with
    // no unit is S0805, `61.9GiB` is S0809, `99999` is S0808 -- and each of
    // those sentences is better than "is not a path". Nothing satl accepts as a
    // fact begins with a digit: §1's generating rule makes every one of them a
    // word.
    if (takes_a_fact &&
        !(text[value_start] >= '0' && text[value_start] <= '9')) {
        if (fact_value(reading, text, name, key, value_start, value_stop,
                       at.number))
            store_fact(reading.into, key, at.number);
        return;
    }

    unsigned long long value = 0;
    bool ok = false;
    switch (kind) {
    case Kind::Count:
        ok = count_value(reading, text, name, value_start, value_stop,
                         at.number, value);
        break;
    case Kind::Size:
        ok = size_value(reading, text, name, value_start, value_stop,
                        at.number, value);
        break;
    // A DIAL IS BOUNDED ONLY ONCE ITS MEANING EXISTS, which is PLAN M6's rule
    // about the dials whose meaning belongs to M8, M9 and M15: a range check is
    // a claim about what the value MEANS.
    //
    // `max_depth` HAS A MEANING AND A READER SINCE M9, AND STILL HAS NO RANGE --
    // WHICH IS NOT THE SAME GAP THIS PARAGRAPH USED TO DESCRIBE. It said the
    // range was waiting on the reader ("the row and the reader land together"),
    // which is what PLAN §8's M9 entry asked for. The reader landed;
    // limits::max_depth_bytes() is it, and there is nothing to bound. A ceiling
    // in bytes on the control stack can be any number of bytes: zero refuses the
    // first push and says so, and a number wider than any machine means the
    // machine. `min_free_mb` is the row that was already in that position, and
    // config_internal.hpp says so beside both.
    //
    // M8 IS THE FIRST MILESTONE TO ANSWER ONE, so `division_digits` is the first
    // dial this arm can say anything about -- as a row of config_internal.hpp's
    // kDialRanges rather than a branch here, so the second dial to get a meaning
    // is a line in a table. The bound is on what the FILE may say and never on
    // what a division may spend: Number::divide has no ceiling at all.
    case Kind::Dial:
        ok = whole_number(text, value_start, value_stop, value);
        if (!ok)
            reading.problems.push_back(
                errors::make<errors::Code::CONFIG_NOT_A_NUMBER>(
                    span(value_start, value_stop, at.number), name,
                    text.substr(value_start, value_stop - value_start)));
        else
            ok = dial_in_range(reading, name, key - kSettingCount, value,
                               span(value_start, value_stop, at.number));
        break;
    }
    if (ok)
        store_number(reading.into, key, value, at.number);
}

// The parser's own cap, one registry on. errors.def's S0891 says why.
constexpr size_t kMostProblems = 20;

} // namespace

bool read_config(std::string_view text, Held &into,
                 std::vector<errors::Diagnostic> &problems)
{
    Reading reading{into, problems, {}};

    unsigned number = 0;
    size_t at = 0;
    while (at <= text.size()) {
        const size_t newline = text.find('\n', at);
        const size_t stop = newline == std::string_view::npos ? text.size() : newline;
        number++;

        size_t start = at;
        size_t end = stop;
        trim(text, start, end);

        // A COMMENT IS `#` OR `;` AND BOTH ARE HERE. `.ini` files in the wild
        // use either, an installer writing this file will use `#` because every
        // other configuration file on the machine does, and refusing `;` would
        // be a rule with nothing behind it.
        const bool comment = start < end && (text[start] == '#' || text[start] == ';');
        if (start < end && !comment)
            setting(reading, text, Line{at, stop, number});

        if (problems.size() >= kMostProblems) {
            problems.back().notes.push_back(
                errors::note<errors::Code::NOTE_CONFIG_TOO_MANY>(
                    errors::kNowhere, problems.size()));
            break;
        }

        if (newline == std::string_view::npos)
            break;
        at = newline + 1;
    }

    return !errors::any_error(problems);
}

} // namespace satellite::limits
