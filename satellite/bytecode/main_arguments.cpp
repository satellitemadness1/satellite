// satellite/bytecode/main_arguments.cpp -- the arguments variable (main_arguments.hpp).

#include "main_arguments.hpp"

#include "program_walk.hpp"
#include "../machine/machine_state.hpp"
#include "word_codes.hpp"
#include "../arguments/arguments.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

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

// THE FILING NAME OF A ROW, as every index files a key (satellite_index.hpp's key_name_of).
// The rows ARE an index, and `args["username"]`, `.remove` and `.contains` look them up
// by this. Filed under the bare name, a row `.remove` took out left that name pointing
// past the end, and the next `args.row = x` wrote into the destroyed slot (the review,
// 2026-09-23: a list freed while another name still held it).
std::string filed(const std::string &key)
{
    std::string name;
    key_name_of(text_value(key), name);     // a string is always a key
    return name;
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
            case ArgumentKind::list: {
                std::vector<Value> items;
                for (const std::string &item : row.items)
                    items.push_back(text_value(item));
                value = Value::of_list(make_list(std::move(items)));
                break;
            }
            }
            value_for_writing(rows, filed(key), text_value(key)) = std::move(value);
        }
    }
    // AND EVERY ROW A LIBRARY ANSWERS, as it stood when main began -- a program that
    // displays `arguments` sees them all. Read by name they are asked again.
    for (std::size_t at = 0; at < word::kSpelledWordCount; ++at) {
        const std::string path = word::kSpelledWords[at].path;
        if (path.compare(0, kLibraryPrefix.size(), kLibraryPrefix) != 0 || path.find('(') != std::string::npos)
            continue;
        const std::string key = path.substr(kLibraryPrefix.size());
        if (value_at(rows, filed(key)) != nullptr)
            continue;
        Value value;
        std::string why;
        signed long long int code = success;
        if (live_row(key, functions, value, why, code) && code == success)
            value_for_writing(rows, filed(key), text_value(key)) = std::move(value);
    }
    return Value::of_index(index);
}

std::size_t past_the_argument_names(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    std::string key;
    return past_the_argument_names(row, at, key);
}

std::size_t past_the_argument_names(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &key)
{
    std::string segment;
    key.clear();
    while (one_segment(row, at, segment))
        key += (key.empty() ? "" : ".") + segment;
    return at;
}

Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read, std::string &read_as)
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
        const Value *held = index != nullptr && *index != nullptr ? value_at(**index, filed(tried)) : nullptr;
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
        read_as = tried;
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

namespace {

// THE LIBRARY OF A SETTING a program may write through -- `access` -- or nullptr.
const NumberRow *an_argument_setting(const std::string &key, const FunctionTable &functions)
{
    const Code word = word::code_of_spelling(kLibraryPrefix + key);
    const NumberRow *library = word != 0 ? functions[word] : nullptr;
    return library != nullptr && library->scenarios.flag_setting != nullptr ? library : nullptr;
}

} // namespace

namespace {

// satl's: a library's row (memory.used) or a group of them (memory); one gathered at
// start-up (username, infinity); and argument_7 on a run given two words, which is still
// satl's name -- written, length would say 3 while argument_7 said otherwise.
bool satl_holds(const std::string &key, const Arguments *arguments)
{
    return word::code_of_spelling(kLibraryPrefix + key) != 0 || filled_in_by_satl(kPrefix + key) ||
           (arguments != nullptr && (arguments->find(kPrefix + key) != nullptr || arguments->find(key) != nullptr));
}

} // namespace

std::string why_an_argument_is_not_written(const std::string &key, const std::string &name, const Value *rows,
                                           const Arguments *arguments, const FunctionTable &functions)
{
    const std::string yours = name + ".my_" + key.substr(key.rfind('.') + 1) + " = ...";
    const std::string own = " A name of the program's own is written: " + yours;
    // FIRST, so `c.access = ...` on a name with no rows writes no setting before refusing.
    if (rows != nullptr && rows->as_index() == nullptr)
        return name + " holds no rows -- the arguments are handed to satellite.main's parameter only, and a "
               "satellite.variable.arguments declared anywhere else is given none, so " + name + "." + key +
               " has nowhere to be written";
    if (an_argument_setting(key, functions) != nullptr)
        return "";
    if (satl_holds(key, arguments))
        return name + "." + key + " is a row satl holds, and a program reads it but does not write it -- the "
               "machine, the command line or config.ini says what it is." + own;
    // A ROW'S MEMBER IS NOT A ROW (the review, 2026-09-23). `args.l.size = 99` names
    // something OF the row l; filed as a row of its own it answered every later read of
    // args.l.size while args.l said otherwise. So no part before the last may be a name
    // already -- satl's, or one the program wrote (`rows`; the checker has none).
    const IndexHandle *index = rows != nullptr ? rows->as_index() : nullptr;
    for (std::size_t dot = key.find('.'); dot != std::string::npos; dot = key.find('.', dot + 1)) {
        const std::string part = key.substr(0, dot);
        if (satl_holds(part, arguments))
            return name + "." + key + " is inside " + name + "." + part + ", a name satl holds, and a program "
                   "reads it but does not write it." + own;
        if (index != nullptr && *index != nullptr && value_at(**index, filed(part)) != nullptr)
            return name + "." + key + " is inside " + name + "." + part + ", a row of the program's own, so it "
                   "names something of that row and not a row -- give " + name + "." + part + " a new value, or "
                   "write a name of the program's own: " + yours;
    }
    return "";
}

namespace {

// THE ROWS A NAME HOLDS, or nullptr and a refusal. A GUARD: every caller has asked
// why_an_argument_is_not_written first, which refuses a name with no rows -- the
// `satellite.variable.arguments c` declared in a body, which is handed none (the review,
// 2026-09-23: an earlier guard here was called unreachable, and that declaration reached it).
IndexHandle *rows_of(const std::string &name, const std::string &key, Value &arguments, ExpressionContext &context)
{
    IndexHandle *index = arguments.as_index();
    if (index == nullptr)
        context.refuse(types_do_not_meet, name + " holds no rows -- the arguments are handed to satellite.main's "
                                          "parameter only, so " + name + "." + key + " has nowhere to be written");
    return index;
}

} // namespace

void write_an_argument(const std::string &key, Value value, const std::string &name, Value &arguments,
                       ExpressionContext &context)
{
    if (const NumberRow *setting = an_argument_setting(key, context.functions)) {
        // B2's rule, kept: a true/false setting takes true or false, and 1 is not quietly
        // taken for true -- `argz.access = 2` would be a line with no meaning that ran.
        if (!value.is_bool()) {
            context.refuse(setting_is_not_a_flag,
                           name + "." + key + " is true or false, and was given " + value.kind_name());
            return;
        }
        const SettingReply said = setting->scenarios.flag_setting(true, *value.as_bool());
        if (said.code != success) {
            context.refuse(said.code, name + "." + key + " could not be written" +
                                          (said.reason.empty() ? "" : " -- " + said.reason));
            return;
        }
    }
    // THE VARIABLE'S OWN COPY, and only the handle it holds: about_to_change copies the
    // rows first when another name shares them, as every index write does.
    IndexHandle *index = rows_of(name, key, arguments, context);
    if (index != nullptr)
        value_for_writing(about_to_change(*index), filed(key), text_value(key)) = std::move(value);
}

Value *an_argument_to_change(const std::string &key, const std::string &name, Value &arguments,
                             ExpressionContext &context)
{
    const std::string refused =
        why_an_argument_is_not_written(key, name, &arguments, context.state.arguments, context.functions);
    if (!refused.empty()) {
        context.refuse(word_takes_no_assignment, refused);
        return nullptr;
    }
    IndexHandle *index = rows_of(name, key, arguments, context);
    Value *held = index != nullptr ? value_at(about_to_change(*index), filed(key)) : nullptr;
    if (index != nullptr && held == nullptr)
        context.refuse(name_not_declared, name + "." + key + " is not one of the arguments -- give it a value "
                                          "first: " + name + "." + key + " = satellite.container.list()");
    return held;
}

} // namespace satellite004
