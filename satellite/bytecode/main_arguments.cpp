// satellite/bytecode/main_arguments.cpp -- the arguments variable (main_arguments.hpp).

#include "main_arguments.hpp"

#include "argument_switches.hpp"
#include "program_walk.hpp"
#include "setting_writes.hpp"
#include "../machine/machine_state.hpp"
#include "word_codes.hpp"
#include "../arguments/argument_settings.hpp"
#include "../arguments/arguments.hpp"
#include "../satellite_object/satellite_index.hpp"
#include "../satellite_object/satellite_list.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

using token::Code;

const std::string kPrefix = "arguments.";
// WHERE THE ARGUMENTS LIVE (the author, 2026-10-03): satellite.library.arguments, not
// satellite.library.main.arguments -- "satellite.library.main is just for satellite.main".
const std::string kLibraryPrefix = "satellite.library.arguments.";

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

// A LIVE ROW: the word satellite.library.arguments.<key>, when it has a
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
        // THE COMMAND LINE FIRST (MS-1): argument1 -- satl itself -- onwards, then length, then
        // the rest of the variable, so `satellite.console.display(args)` begins with what was typed.
        // AS THEY ARE NOW (2026-10-06), copied under the lock a line takes to change a setting.
        for (const Argument &row : arguments->every_row_now()) {
            const std::string key =
                row.name.compare(0, kPrefix.size(), kPrefix) == 0 ? row.name.substr(kPrefix.size()) : row.name;
            file_under(rows, filed(key), text_value(key), the_value_of_a_row(row));
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
            file_under(rows, filed(key), text_value(key), std::move(value));
    }
    return Value::of_index(index);
}

std::string arguments_row_written_as_a_word(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            bool after_system)
{
    std::vector<std::string> names;
    for (std::size_t k = at; code_at(row, k) == token::name_token;) {
        names.push_back(text_at(row, k));
        if (code_at(row, k) != token::method_token) break;
        ++k;
    }
    // satellite.system.hostname is arguments.system.hostname first, and satellite.system.cores
    // is arguments.cores; satellite.machine.cores is arguments.machine.cores.
    const char *const system_first[] = {"system.", ""};
    const char *const plain[] = {""};
    for (const char *under : after_system ? std::vector<const char *>(system_first, system_first + 2)
                                          : std::vector<const char *>(plain, plain + 1)) {
        std::string path, found, written = after_system ? "satellite.system" : "satellite";
        std::size_t used = 0;
        for (std::size_t n = 0; n < names.size(); ++n) {
            path += (n == 0 ? "" : ".") + names[n];
            if (word::code_of_spelling(kLibraryPrefix + std::string(under) + path) != 0) {
                found = under + path;
                used = n + 1;
            }
        }
        if (found.empty()) continue;
        for (std::size_t n = 0; n < used; ++n) written += "." + names[n];
        return written + " is not a word -- in 004 it is a row of main's arguments: arguments." + found +
               " in a satellite.main(satellite.variable.arguments arguments), or satellite.library.arguments." +
               found + " in any capsule";
    }
    return std::string();
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

namespace {

// THE OLD SPELLING AND THE NUMBER 0, SAID BY NAME (MS-1), wherever the key arrives -- a dotted
// read or a key handed over as text. argument_1 was the first word after the file, and that is
// argument3 now -- argument1 is satl itself, argument2 the file -- so it is refused with the
// row it means, and never quietly answered with satl's own path.
bool an_argument_name_is_refused(const std::string &key, const std::string &name, ExpressionContext &context)
{
    const std::string now = the_row_argument_underscore_is_now(key);
    if (!now.empty()) {
        context.refuse(name_not_declared, name + "." + key + " is " + name + "." + now + " now -- the arguments "
                                          "are numbered as they were typed: " + name + ".argument1 is satl itself, " +
                                          name + ".argument2 the file, and " + name + ".argument3 the first word "
                                          "after it");
        return true;
    }
    if (names_argument_zero(key)) {
        context.refuse(counts_from_one, name + "." + key + " -- the arguments count from 1, as satellite does: " +
                                        name + ".argument1 is satl itself, as it was started");
        return true;
    }
    return false;
}

} // namespace

bool an_argument_key(Value &key, const std::string &name, ExpressionContext &context)
{
    satellite_string text;
    std::string unused;
    if (key.kind() != satelliteObject::string || key.to_string(text, unused) != success)
        return true;
    const std::string spelled = text.to_utf8();
    if (an_argument_name_is_refused(spelled, name, context))
        return false;
    const std::string row = the_argument_row_spelled(spelled);
    if (row != spelled)
        key = text_value(row);
    return true;
}

Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read, std::string &read_as,
                       bool the_runs)
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
    if (an_argument_name_is_refused(runs.front().first, name, context))
        return Value();
    const IndexHandle *index = the_runs ? nullptr : arguments.as_index();
    for (std::size_t n = runs.size(); n > 0; --n) {
        // arg3, args3 AND arguments3 ARE argument3 (MS-1): one row, read under its own name.
        const std::string tried = the_argument_row_spelled(runs[n - 1].first);
        // THE ROW satl GATHERED WINS, and a library is asked only for what was not
        // gathered: memory.used is live because no start-up row holds it, while
        // machine.cores is the count gather() measured. Asked the other way round,
        // the machine.cores LIBRARY answered 24 on a machine whose row says 12 -- it
        // counts threads under the cores name, which is its own bug to fix.
        //
        // AND IT IS READ AS IT IS NOW (2026-10-06): out of the run's own rows, where a line that
        // changed a setting left it -- by this name or by any other -- and not out of the copy main
        // was handed when it began. A row the program took out of its copy (.remove) stays out,
        // unless a library answers it live, as memory.used always did -- and a SETTING's library
        // answers config.ini, so there the run's row is what is read (the fresh reader: access,
        // taken out after args.access(false), read true).
        Value answer;
        std::string why;
        signed long long int code = success;
        const Value *held = index != nullptr && *index != nullptr ? value_at(**index, filed(tried)) : nullptr;
        Argument now;
        const bool gathered = (held != nullptr || the_runs) && context.state.arguments != nullptr &&
                              context.state.arguments->copy_of(kPrefix + tried, now);
        if (gathered) {
            answer = the_value_of_a_row(now);
        } else if (live_row(tried, context.functions, answer, why, code)) {
            if (code != success) {
                context.refuse(code, name + "." + tried + " could not be read" + (why.empty() ? "" : " -- " + why));
                return Value();
            }
            if (a_setting_named(tried) != nullptr && context.state.arguments != nullptr &&
                context.state.arguments->copy_of(kPrefix + tried, now))
                answer = the_value_of_a_row(now);
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

const char *a_bare_true_or_false(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    if (code_at(row, at) != token::name_token)
        return nullptr;
    std::size_t k = at;
    const std::string name = text_at(row, k);     // and k past its letters
    const char *bare = name == "true" ? "true" : name == "false" ? "false" : nullptr;
    const Code after = code_at(row, k);
    const bool ends = k >= row.size() || after == token::line_end_token || after == token::comment_token ||
                      after == token::end_of_file_token;
    return ends ? bare : nullptr;
}

const char *a_bare_true_or_false_in_brackets(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    if (code_at(row, at) != token::name_token)
        return nullptr;
    std::size_t k = at;
    const std::string name = text_at(row, k);     // and k past its letters
    const char *bare = name == "true" ? "true" : name == "false" ? "false" : nullptr;
    if (bare == nullptr || code_at(row, k) != token::right_parenthesis_token)
        return nullptr;
    const Code after = code_at(row, k + 1);
    const bool ends = k + 1 >= row.size() || after == token::line_end_token || after == token::comment_token ||
                      after == token::end_of_file_token;
    return ends ? bare : nullptr;
}

bool an_argument_call(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &key, std::size_t &open,
                      bool any_names)
{
    // THE DOTTED NAMES TO THE `(`, each a name or a method's word -- `.display.buffer(` is a method's
    // word and then a name -- with whether the last was a plain name.
    std::vector<std::string> names;
    bool plain = false;
    std::size_t k = at;
    for (;;) {
        if (code_at(row, k) != token::method_token)
            return false;
        const Code next = code_at(row, k + 1);
        if (next == token::name_token) {
            std::size_t m = k + 1;
            names.push_back(text_at(row, m));     // and m past its letters
            k = m;
            plain = true;
        } else if (token::is_method_code(next)) {
            names.push_back(token::method_name_of(next));
            k += 2;
            plain = false;
        } else {
            return false;
        }
        if (code_at(row, k) == token::left_parenthesis_token)
            break;
    }
    key.clear();
    for (const std::string &name : names)
        key += (key.empty() ? "" : ".") + name;
    open = k;
    // A MAP'S .get AND .set ARE ITS OWN WORDS (expression.cpp), refused on the arguments where they
    // always were -- `args.get("x")` is not a row named get being written.
    if (names.size() == 1 && (names.front() == "get" || names.front() == "set"))
        return false;
    // A NAME OF THE PROGRAM'S OWN IS ONE NAME: `args.my_value(5)`. Past one, the names before the `(`
    // reach INTO a row -- `args.b.call_put(9)` runs a capsule of the object the row b holds, and
    // `args.m.set("k", 5)` is a map's own word -- and that is read as it always was (the fresh reader,
    // 2026-10-06: taken for a write, both were refused). A setting is known by its whole name.
    return a_setting_named(key) != nullptr || (plain && (names.size() == 1 || any_names));
}

bool a_word_of_the_arguments(Code word)
{
    const char *spelled = word::is_word_code(word) ? word::spelling_of(word) : nullptr;
    if (spelled == nullptr)
        return false;
    const std::string path = spelled;
    return path.compare(0, kLibraryPrefix.size(), kLibraryPrefix) == 0 ||
           path.rfind("satellite.library.main.arguments.", 0) == 0;
}

namespace {

// satl's: a library's row (memory.used) or a group of them (memory); one gathered at
// start-up (username, version); and argument7 -- or arg7, or the old argument_7 -- on a run
// given two words, which is still satl's name: written, length would say 4 while argument7
// said otherwise. A SETTING IS NOT satl's in this sense (argument_settings.hpp): it is
// asked about first, and changed.
bool satl_holds(const std::string &key, const Arguments *arguments)
{
    return word::code_of_spelling(kLibraryPrefix + key) != 0 || filled_in_by_satl(kPrefix + key) ||
           (arguments != nullptr && (arguments->find(kPrefix + key) != nullptr || arguments->find(key) != nullptr));
}

} // namespace

std::string why_an_argument_is_not_written(const std::string &key, const std::string &name, const Value *rows,
                                           const Arguments *arguments, const FunctionTable &functions,
                                           const ArgumentWrite &where)
{
    (void)functions;
    const std::string yours = name + ".my_" + key.substr(key.rfind('.') + 1) + " = ...";
    const std::string own = " A name of the program's own is written: " + yours;
    // FIRST, so `c.access = ...` on a name with no rows changes no setting before refusing.
    if (!where.the_runs && rows != nullptr && rows->as_index() == nullptr)
        return name + " holds no rows -- the arguments are handed to satellite.main's parameter only, and a "
               "satellite.variable.arguments declared anywhere else is given none, so " + name + "." + key +
               " has nowhere to be written";
    // A SETTING (the author, 2026-10-06; setting_writes.hpp): for good when it is typed at the prompt,
    // and for this run in a file -- unless a file's line is too late for it: a row satl reads as it
    // starts, or the program's switch inside a capsule (MS-2).
    if (const Setting *setting = a_setting_named(key)) {
        if (where.at_the_prompt)
            return std::string();
        return why_a_file_line_is_too_late(*setting, name + "." + key, false);
    }
    const std::string settings = "the settings a line changes are " + every_setting_named();
    // THE WARM THREADS ARE THE AUTHOR'S (argument_settings.hpp): said why, because they look like settings.
    const std::string why_not = key == "threads_startup_per_core" || key == "threads_max"
                                    ? "; the warm threads are the author's rows in satellite_config.hpp, because one "
                                      "line saved for good could make every start ask for a million threads"
                                    : std::string();
    if (satl_holds(key, arguments))
        return name + "." + key + " is a row satl holds, and a program reads it but does not write it -- the "
               "machine, the command line or satl itself says what it is" + why_not + "; " + settings + "." +
               (where.the_runs ? std::string() : own);
    // THE RUN'S OWN, BY THE BARE NAME: its rows are satl's, and a name of the program's own is written
    // where main was handed the arguments -- there is no second copy for it to land in.
    if (where.the_runs && where.at_the_prompt)
        return name + "." + key + " is not one of the settings -- " + settings + ". At the prompt the bare name "
               "arguments is satl's own rows, and a name of your own is a variable the prompt keeps until you "
               "leave: satellite.variable.number " + key.substr(key.rfind('.') + 1) + " = 5";
    if (where.the_runs)
        return name + "." + key + " is not one of the settings -- " + settings + ". The bare name arguments is "
               "the run's own rows; a name of the program's own is written on satellite.main's arguments, as "
               "args." + key + " = ... in satellite.main(satellite.variable.arguments args)";
    // A ROW'S MEMBER IS NOT A ROW (the review, 2026-09-23). `args.l.size = 99` names
    // something OF the row l; filed as a row of its own it answered every later read of
    // args.l.size while args.l said otherwise. So no part before the last may be a name
    // already -- satl's, or one the program wrote (`rows`; the checker has none).
    const IndexHandle *index = rows != nullptr ? rows->as_index() : nullptr;
    for (std::size_t dot = key.find('.'); dot != std::string::npos; dot = key.find('.', dot + 1)) {
        const std::string part = key.substr(0, dot);
        if (satl_holds(part, arguments) || a_setting_named(part) != nullptr)
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
                       ExpressionContext &context, const ArgumentWrite &where)
{
    // A SETTING, FOR GOOD OR FOR THIS RUN (setting_writes.hpp): its value judged there -- true or false
    // for a switch, so `argz.access = 2` is still a line refused rather than one with no meaning that
    // ran (B2's rule) -- and nothing is changed when it is refused.
    if (const Setting *setting = a_setting_named(key)) {
        std::string why;
        const Lasting lasting = where.at_the_prompt ? Lasting::for_good : Lasting::this_run;
        const signed long long int changed = change_a_setting(*setting, value, lasting, name + "." + key,
                                                              context.state, why);
        if (changed != success) {
            context.refuse(changed, why);
            return;
        }
    }
    // THE RUN'S OWN HAS NO COPY: the row it changed is the run's, and every name reads it there.
    if (where.the_runs)
        return;
    // THE VARIABLE'S OWN COPY, and only the handle it holds: about_to_change copies the
    // rows first when another name shares them, as every index write does.
    IndexHandle *index = rows_of(name, key, arguments, context);
    if (index != nullptr)
        file_under(about_to_change(*index), filed(key), text_value(key), std::move(value));
}

Value *an_argument_to_change(const std::string &key, const std::string &name, Value &arguments,
                             ExpressionContext &context, bool the_runs)
{
    // A SETTING IS GIVEN A VALUE WHOLE (setting_writes.hpp) -- a switch, a number, a folder: there is
    // nothing inside one to change in place.
    if (a_setting_named(key) != nullptr) {
        context.refuse(word_takes_no_assignment, name + "." + key + " is a setting, given a value whole -- " + name +
                                                     "." + key + "(value) -- and there is nothing inside it to change");
        return nullptr;
    }
    ArgumentWrite where;
    where.the_runs = the_runs;
    const std::string refused =
        why_an_argument_is_not_written(key, name, &arguments, context.state.arguments, context.functions, where);
    if (!refused.empty()) {
        context.refuse(word_takes_no_assignment, refused);
        return nullptr;
    }
    IndexHandle *index = rows_of(name, key, arguments, context);
    Value *held = index != nullptr ? value_to_change_inside(about_to_change(*index), filed(key)) : nullptr;
    if (index != nullptr && held == nullptr)
        context.refuse(name_not_declared, name + "." + key + " is not one of the arguments -- give it a value "
                                          "first: " + name + "." + key + " = satellite.container.list()");
    return held;
}

} // namespace satellite004
