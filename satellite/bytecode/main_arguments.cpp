// satellite/bytecode/main_arguments.cpp -- the arguments variable (main_arguments.hpp).

#include "main_arguments.hpp"

#include "program_walk.hpp"
#include "../machine/machine_state.hpp"
#include "word_codes.hpp"
#include "../arguments/arguments.hpp"
#include "../satellite_object/satellite_index.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

using token::Code;

const std::string kPrefix = "arguments.";
const std::string kLibraryPrefix = "satellite.library.main.arguments.";

Value text_value(const std::string &text)
{
    Value made;
    std::size_t bad_offset = 0;
    // A row that is not UTF-8 -- a command-line word can be any bytes -- keeps the
    // text that could be read rather than being dropped: `arguments` holds every
    // row, and a missing one would be a hole nobody was told about.
    if (Value::of_utf8(text, made, bad_offset) != success)
        Value::of_utf8(text.substr(0, bad_offset), made, bad_offset);
    return made;
}

// A LIVE ROW: the library of satellite.library.main.arguments.<key>, when it has a
// fact or a setting to answer. memory.used is what the machine uses NOW, so it is
// asked when it is read, never kept from when main began.
bool live_row(const std::string &key, const FunctionTable &functions, Value &out, std::string &why,
              signed long long int &code)
{
    const Code word = word::code_of_spelling(kLibraryPrefix + key);
    const NumberRow *library = word != 0 ? functions[word] : nullptr;
    if (library == nullptr)
        return false;
    if (library->scenarios.fact != nullptr) {
        const FactReply said = library->scenarios.fact();
        code = said.code;
        if (said.code != success) why = said.reason;
        else out = said.is_text ? text_value(said.text) : Value::of_number(satellite_number(said.count));
        return true;
    }
    if (library->scenarios.flag_setting != nullptr) {
        const SettingReply said = library->scenarios.flag_setting(false, false);
        code = said.code;
        if (said.code != success) why = said.reason;
        else out = Value::of_bool(said.flag);
        return true;
    }
    return false;
}

// One `.segment` of a run: a name, or a method's word with no `(` after it -- `.path`
// is a row as well as a file's method. Answers false at anything else.
bool one_segment(const std::vector<std::bitset<16>> &row, std::size_t &at, std::string &segment)
{
    if (code_at(row, at) != token::method_token)
        return false;
    const Code next = code_at(row, at + 1);
    if (next == token::name_token) {
        std::size_t k = at + 1;
        segment = text_at(row, k);
        if (code_at(row, k) == token::left_parenthesis_token)
            return false;                 // `.upper(` is a call on the row, not part of its name
        at = k;
        return true;
    }
    if (token::is_method_code(next) && code_at(row, at + 2) != token::left_parenthesis_token) {
        segment = token::method_name_of(next);
        at += 2;
        return true;
    }
    return false;
}

} // namespace

Value the_arguments_value(const Arguments *arguments, const FunctionTable &functions)
{
    IndexHandle index = make_index();
    satelliteIndex &rows = about_to_change(index);
    if (arguments != nullptr) {
        for (const Argument &row : arguments->all()) {
            const std::string key =
                row.name.compare(0, kPrefix.size(), kPrefix) == 0 ? row.name.substr(kPrefix.size()) : row.name;
            Value value;
            switch (row.kind) {
            case ArgumentKind::text: value = text_value(row.text); break;
            case ArgumentKind::count: value = Value::of_number(satellite_number(row.count)); break;
            case ArgumentKind::number: value = Value::of_number(row.number); break;
            case ArgumentKind::flag: value = Value::of_bool(row.flag); break;
            case ArgumentKind::size: value = Value::of_number(satellite_number(row.count)); break;
            }
            value_for_writing(rows, key, text_value(key)) = std::move(value);
        }
    }
    // AND EVERY ROW A LIBRARY ANSWERS, as it stood when main began -- a program that
    // displays `arguments` sees them all. Read by name they are asked again.
    for (std::size_t at = 0; at < word::kSpelledWordCount; ++at) {
        const std::string path = word::kSpelledWords[at].path;
        if (path.compare(0, kLibraryPrefix.size(), kLibraryPrefix) != 0 || path.find('(') != std::string::npos)
            continue;
        const std::string key = path.substr(kLibraryPrefix.size());
        if (value_at(rows, key) != nullptr)
            continue;
        Value value;
        std::string why;
        signed long long int code = success;
        if (live_row(key, functions, value, why, code) && code == success)
            value_for_writing(rows, key, text_value(key)) = std::move(value);
    }
    return Value::of_index(index);
}

std::size_t past_the_argument_names(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    std::string segment;
    while (one_segment(row, at, segment)) {}
    return at;
}

Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read)
{
    read = false;
    std::vector<std::pair<std::string, std::size_t>> runs;   // "memory", "memory.total" ... and where each ends
    std::string key, segment;
    std::size_t k = at;
    while (one_segment(row, k, segment)) {
        key += (key.empty() ? "" : ".") + segment;
        runs.emplace_back(key, k);
    }
    if (runs.empty())
        return Value();
    const IndexHandle *index = arguments.as_index();
    for (std::size_t n = runs.size(); n > 0; --n) {
        const std::string &tried = runs[n - 1].first;
        // THE ROW satl GATHERED WINS, and a library is asked only for what was not
        // gathered: memory.used is live because no start-up row holds it, while
        // machine.cores is the count gather() measured. Asked the other way round,
        // the machine.cores LIBRARY answered 24 on a machine whose row says 12 -- it
        // counts threads under the cores name, which is its own bug to fix.
        Value answer;
        std::string why;
        signed long long int code = success;
        const Value *held = index != nullptr && *index != nullptr ? value_at(**index, tried) : nullptr;
        const bool gathered = held != nullptr && context.state.arguments != nullptr &&
                              context.state.arguments->find(kPrefix + tried) != nullptr;
        if (gathered) {
            answer = *held;
        } else if (live_row(tried, context.functions, answer, why, code)) {
            if (code != success) {
                context.refuse(code, name + "." + tried + " could not be read" + (why.empty() ? "" : " -- " + why));
                return Value();
            }
        } else if (held != nullptr) {
            answer = *held;
        } else {
            continue;
        }
        read = true;
        at = runs[n - 1].second;
        return answer;
    }
    // A METHOD MAY FOLLOW THE NAME ITSELF -- `arguments.size()` -- so a single
    // segment that names no row is left to the methods; a longer run names a row
    // or nothing, and nothing is said as that.
    if (runs.size() == 1 && token::method_code_of(runs.front().first) != 0)
        return Value();
    context.refuse(name_not_declared, name + "." + runs.back().first + " is not one of the arguments -- " +
                                          "satellite.console.display(" + name + ") shows every one");
    return Value();
}

} // namespace satellite004
