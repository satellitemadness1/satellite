// The containers' shared checks, the two constructors, the search dials, and
// the one install that sums the halves. See satellite_containers/handlers.hpp.

#include "satellite_containers/handlers.hpp"

#include "error_reporter/report.hpp"
#include "satellite_containers/methods_internal.hpp"
#include "satellite_containers/search.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::containers {

std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

namespace {

const char *spelled(const Value &value)
{
    return type_name(value);
}

// The nothing-or-wrong-type tail every receiver check shares -- scalars'
// string_at, restated once for both containers.
bool wrong_receiver(eval::Machine &m, const Value &value, uint32_t who,
                    const char *wanted)
{
    if (value.is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), wanted, spelled(value)));
    return false;
}

} // namespace

bool list_at(eval::Machine &m, const Value *arguments, uint32_t who,
             const List **out)
{
    const Value &value = arguments[who];
    if (value.is_list()) {
        *out = as_list(value);
        return true;
    }
    return wrong_receiver(m, value, who,
                          "a `satellite.container.list` -- constructed with "
                          "`satellite.container.list()`");
}

bool map_at(eval::Machine &m, const Value *arguments, uint32_t who,
            const MapBody **out)
{
    const Value &value = arguments[who];
    if (value.is_map()) {
        *out = as_map(value);
        return true;
    }
    return wrong_receiver(m, value, who,
                          "a `satellite.container.map` -- constructed with "
                          "`satellite.container.map()`");
}

bool position_at(eval::Machine &m, const Value *arguments, uint32_t who,
                 unsigned long long *out)
{
    const Value &value = arguments[who];
    const Number *number = std::get_if<Number>(&value);
    long long narrow = 0;
    // A POSITION IS A WHOLE NUMBER NO LESS THAN ZERO -- scalars' rule, held
    // to the same sentence: refused rather than rounded or wrapped, because a
    // refusal can loosen later and a convention silently adopted is forever.
    if (number == nullptr || !number->is_integer() || number->is_negative() ||
        !number->to_integer(narrow)) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m),
            "a whole number no less than 0 that a machine word can hold",
            number ? number->to_string() : std::string(spelled(value))));
        return false;
    }
    *out = static_cast<unsigned long long>(narrow);
    return true;
}

bool key_at(eval::Machine &m, const Value *arguments, uint32_t who,
            std::string *canonical)
{
    if (map_key_of(arguments[who], *canonical))
        return true;
    m.refuse(errors::make<errors::Code::EVAL_NOT_A_KEY>(
        m.span_of(m.here()), spelled(arguments[who])));
    return false;
}

namespace {

// --- the constructors -------------------------------------------------------
//
// `satellite.container.list()` `1 4 2 0` and `satellite.container.map()`
// `1 4 1 0` -- the bare call shapes the numbering has carried since the
// transcription, given their meaning by the author on 2026-09-05: DESIGN
// §8.7's "a declared variable of any type holds nothing until a value is
// assigned to it" stands as written, so a declaration does NOT construct
// (v1 default-constructed; this tree does not), and these two rows are how a
// program obtains an empty container to append or set into.

bool construct_list(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::list(List{});
    return true;
}

bool construct_map(eval::Machine &, const Value *, uint32_t, Value *answer)
{
    *answer = Value::map(MapBody{});
    return true;
}

// --- the dials --------------------------------------------------------------

bool read_threshold(eval::Machine &m, const Value *, uint32_t, Value *answer)
{
    // No argument READS it, so a program can ask what the dial is set to
    // without a second word for the question -- v1's shape, kept.
    *answer = Value::number(Number(
        static_cast<long long>(m.search_threshold())));
    return true;
}

bool set_threshold(eval::Machine &m, const Value *arguments, uint32_t,
                   Value *answer)
{
    const std::string range =
        "a whole satellite.variable.number from " +
        std::to_string(SEARCH_TIGHTEST) + " to " +
        std::to_string(SEARCH_LOOSEST) + " -- " +
        std::to_string(SEARCH_TIGHTEST) + " is an exact match and " +
        std::to_string(SEARCH_LOOSEST) + " anything remotely alike";

    const Number *level = std::get_if<Number>(&arguments[0]);
    long long asked_for = 0;
    // OUT OF RANGE IS AN ERROR, NOT A CLAMP -- v1's DECISION 5b, kept: a
    // clamp would make threshold(11) silently mean 10, and the program would
    // never learn it had asked for something the language does not have.
    if (level == nullptr || !level->is_integer() ||
        !level->to_integer(asked_for) || asked_for < SEARCH_TIGHTEST ||
        asked_for > SEARCH_LOOSEST) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), asked(m), range.c_str(),
            text_of(arguments[0])));
        return false;
    }

    m.set_search_threshold(static_cast<int>(asked_for));
    // Answers nothing, the way a mutator's statement form reads: the dial is
    // set, and echoing it back would make every threshold(n) line print.
    *answer = Value::nothing();
    return true;
}

} // namespace

void install_handlers()
{
    eval::Handlers &table = eval::Handlers::table();

    // NOT RECEIVERS, any of these four: a constructor is called on the TYPE
    // PATH and a dial on a module path, and neither is a value.
    table.install(static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_0),
                  {construct_list, false, 0, "M16"});
    table.install(static_cast<words::PathId>(words::NodeId::CONTAINER_MAP_0),
                  {construct_map, false, 0, "M16"});
    table.install(
        static_cast<words::PathId>(words::NodeId::SYSTEM_THRESHOLD_0),
        {read_threshold, false, 0, "M16"});
    table.install(
        static_cast<words::PathId>(words::NodeId::SYSTEM_THRESHOLD_N),
        {set_threshold, false, 1, "M16"});

    install_list_methods();
    install_list_sorting();
    install_map_methods();
}

} // namespace satellite::containers
