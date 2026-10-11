// satellite/bytecode/argument_switches.cpp -- a file's own switches (argument_switches.hpp): the
// scan's line, and the run's rows set from the program's file.

#include "argument_switches.hpp"

#include "capsule_scan.hpp"
#include "main_arguments.hpp"
#include "program_walk.hpp"
#include "setting_writes.hpp"
#include "word_codes.hpp"
#include "../arguments/argument_settings.hpp"
#include "../arguments/arguments.hpp"
#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../machine/source_position.hpp"

#include <string>

namespace satellite004 {
namespace {

using token::Code;

const Code kFalse = word::code_of(1, 17, 1);   // satellite.bool.false
const Code kTrue = word::code_of(1, 17, 2);    // satellite.bool.true
const std::string kPlace = "satellite.library.arguments";
const std::string kOldPlace = "satellite.library.main.arguments";   // where they lived until 2026-10-03

// HOW A LINE AT `at` BEGINS AS THE ARGUMENTS, or "" when it does not: "arguments" for the bare
// name, or the place a word is spelled under -- satellite.library.arguments itself, a word under
// it (satellite.library.arguments.access lexes as ONE code), or the same at the old place.
// `rest` is what a word under the place spells after it ("access"), `next` the code after the head.
std::string head_of(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &rest, std::size_t &next)
{
    rest.clear();
    next = at;
    const Code code = code_at(row, at);
    if (code == token::name_token) {
        const std::string name = text_at(row, next);   // past the name and its letters
        return name == "arguments" && code_at(row, next) == token::method_token ? name : std::string();
    }
    const char *spelled = word::is_word_code(code) ? word::spelling_of(code) : nullptr;
    if (spelled == nullptr)
        return std::string();
    const std::string word = spelled;
    for (const std::string &place : {kPlace, kOldPlace}) {
        if (word == place && code_at(row, at + 1) == token::method_token) {
            next = at + 1;
            return place;
        }
        if (word.size() > place.size() + 1 && word.compare(0, place.size() + 1, place + ".") == 0 &&
            word.find('(') == std::string::npos) {
            rest = word.substr(place.size() + 1);
            next = at + 1;
            return place;
        }
    }
    return std::string();
}

// ONE LINE OF THE ARGUMENTS AT A FILE'S TOP, AS IT IS WRITTEN: how it began (`head`, "" when the line
// is not one of these), the row it names after `arguments.` ("missing"), whether it is the call --
// `arguments.missing(false)` -- or the `=`, and where its `=` or its `(` is.
struct SwitchLine {
    std::string head;
    std::string key;
    bool call = false;
    std::size_t sign = 0;
};

SwitchLine read_the_line(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    SwitchLine line;
    std::string rest;
    std::size_t next = at;
    line.head = head_of(row, at, rest, next);
    if (line.head.empty())
        return line;
    const auto joined = [&rest](const std::string &more) {
        return rest + (rest.empty() || more.empty() ? "" : ".") + more;
    };
    // THE CALL (2026-10-06): `arguments.missing(false)`, `satellite.library.arguments.missing(false)`,
    // and a word under the place touching its `(` -- satellite.library.arguments.access(false).
    std::string called;
    std::size_t open = next;
    if (an_argument_call(row, next, called, open, true)) {
        line.key = joined(called);
        line.call = true;
        line.sign = open;
        return line;
    }
    if (!rest.empty() && code_at(row, next) == token::left_parenthesis_token) {
        line.key = rest;
        line.call = true;
        line.sign = next;
        return line;
    }
    // THE `=`: the dotted names.
    std::string more;
    line.sign = past_the_argument_names(row, next, more);
    line.key = joined(more);
    return line;
}

// WHETHER THE `=` AT `at` ENDS ITS LINE AS IT WAS WRITTEN. A line that ends in = goes on into the
// next (bytecode_registry.hpp's join), so `arguments.missing =` with its value forgotten is handed
// the next line written as its value, and a refusal of that value would point at a line nobody
// meant as one (the fresh reader, 2026-10-04). So the line is read back as it was written, a //
// comment left off. The join leaves every line_end_token where its line is, so counting them
// finds the line the `=` was written on.
bool the_equals_ends_its_line(const std::vector<std::bitset<16>> &row, std::size_t at, const std::string &file)
{
    std::size_t line = 1;
    for (std::size_t k = 0; k < at && k < row.size();) {
        if (token::carries_a_count(code_at(row, k))) { skip_payload(row, k); continue; }
        if (code_at(row, k) == token::line_end_token) ++line;
        ++k;
    }
    std::string text = source_line(file, line);
    const std::size_t comment = text.find("//");
    if (comment != std::string::npos) text.erase(comment);
    // A LINE THAT CANNOT BE READ BACK is not said to be missing anything: the codes are judged.
    const std::size_t last = text.find_last_not_of(" \t\r");
    return last != std::string::npos && text[last] == '=';
}

// WHETHER THE LINE ENDS AT `at` -- for the call, its `)` and then the line's end -- with `at` left on
// the `)` when there is one.
bool ends_here(const std::vector<std::bitset<16>> &row, std::size_t at, bool call)
{
    if (call) {
        if (code_at(row, at) != token::right_parenthesis_token)
            return false;
        ++at;
    }
    const Code after = code_at(row, at);
    return at >= row.size() || after == token::line_end_token || after == token::comment_token ||
           after == token::end_of_file_token;
}

// HOW THE LINE IS WRITTEN WITH A VALUE IN IT, for a sentence that shows one.
std::string shown_with(const std::string &written, bool call, const std::string &value)
{
    return call ? written + "(" + value + ")" : written + " = " + value;
}

std::string an_example_of(const Setting &setting)
{
    switch (setting.kind) {
    case SettingKind::flag: return "false";
    case SettingKind::number: return std::to_string(setting.least > 0 ? setting.least : 1);
    case SettingKind::text: return "\"~/folder\"";
    }
    return "false";
}

// THE VALUE WRITTEN AT `at`, OF THE SETTING'S KIND, KEPT AS TEXT -- or "" in `kept` and the refusal,
// with its code, in the answer. Written, not worked out: nothing outside a capsule runs, so only a
// literal is a value there, as a satellite.library value's is (library_values.cpp).
std::string the_value_written(const std::vector<std::bitset<16>> &row, std::size_t at, const SwitchLine &line,
                              const Setting &setting, const std::string &written, std::string &kept,
                              signed long long int &code)
{
    const std::string example = shown_with(written, line.call, an_example_of(setting));
    switch (setting.kind) {
    case SettingKind::flag: {
        // BARE OR satellite.bool's, the same here (D22).
        const char *bare = line.call ? a_bare_true_or_false_in_brackets(row, at) : a_bare_true_or_false(row, at);
        if (bare != nullptr) {
            kept = bare;
            return std::string();
        }
        const Code value = code_at(row, at);
        if ((value == kTrue || value == kFalse) && ends_here(row, at + 1, line.call)) {
            kept = value == kTrue ? "true" : "false";
            return std::string();
        }
        code = setting_is_not_a_flag;
        return written + " is true or false, and nothing else -- " + example + " (a bare false and satellite.bool.false "
               "are the same here), with nothing after it " + (line.call ? "in its brackets" : "on its line");
    }
    case SettingKind::number: {
        std::size_t k = at;
        const std::string digits = code_at(row, k) == token::number_token ? text_at(row, k) : std::string();
        satellite_number number;
        std::size_t bad_offset = 0;
        const bool whole = !digits.empty() && digits.find_first_not_of("0123456789") == std::string::npos &&
                           code_at(row, k) != token::fraction_token &&
                           satellite_number::from_text(digits, number, bad_offset) == success;
        if (!whole || !ends_here(row, k, line.call)) {
            code = types_do_not_meet;
            return written + " is a whole number, written as its digits -- " + example + ", with nothing after it " +
                   (line.call ? "in its brackets" : "on its line");
        }
        if (satellite_number::compare(number, satellite_number(setting.least)) < 0) {
            code = setting_out_of_range;
            return written + " is a whole number of at least " + std::to_string(setting.least) + ", and was given " +
                   digits;
        }
        kept = number.to_text();
        return std::string();
    }
    case SettingKind::text: {
        std::size_t k = at;
        const std::string words = code_at(row, k) == token::string_token ? string_at(row, k) : std::string();
        if (code_at(row, at) != token::string_token || !ends_here(row, k, line.call)) {
            code = types_do_not_meet;
            return written + " is a folder or a path, written in quotes -- " + example + ", with nothing after it " +
                   (line.call ? "in its brackets" : "on its line");
        }
        if (words.empty()) {
            code = setting_out_of_range;
            return written + " is a folder or a path, and was given \"\" -- a setting of nothing is no place at all";
        }
        kept = words;
        return std::string();
    }
    }
    return std::string();
}

} // namespace

bool a_switch_a_file_writes(const std::string &key)
{
    return a_setting_named(key) != nullptr;
}

std::string a_switch_written_too_late(const std::string &written, const std::string &key)
{
    // SOME, NOT ALL: S031 and the S15x are said before main begins, S032-S034 when the run ends (the
    // fresh reader, 2026-10-04) -- and one value has to hold for all of them.
    return written + " is the program's switch, written once at the top of its file, outside every capsule: "
                     "arguments." + key + " = false. It holds for the whole run, and some of what it turns off is "
                     "said before satellite.main begins, so a line inside a capsule cannot be what sets it";
}

namespace scan {

bool starts_a_switch_line(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &key)
{
    const SwitchLine line = read_the_line(row, at);
    key = line.key;
    return !line.head.empty();
}

void switch_line(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r, std::size_t file_scope,
                 std::size_t &i)
{
    const std::size_t declared_at = i;
    // WHEREVER THE LINE IS REFUSED, THE SCAN GOES ON AFTER IT, as it does after a satellite.library
    // line, so a second mistake further down is still found.
    i = past_the_statement(row, i);

    const SwitchLine line = read_the_line(row, declared_at);
    const std::string &key = line.key;
    const std::string written = line.head + "." + key;
    const Setting *setting = a_setting_named(key);

    // NO NAME AFTER THE DOT, OR NO `=` AFTER A NAME THAT IS NO SETTING -- `arguments.update()`: a line
    // like any other at a file's top, refused in the words every other one is.
    if (key.empty() || (setting == nullptr && !line.call && code_at(row, line.sign) != token::assign_token)) {
        refuse(table, r, declared_at, satl_line_not_understood, kOutsideEveryCapsule);
        return;
    }
    // ANY OTHER ROW: a line outside every capsule, as it was before switches -- a row is read in a capsule,
    // and a name of the program's own written there (the fresh reader: "the machine says what they are"
    // was untrue of the program's own).
    if (setting == nullptr) {
        refuse(table, r, declared_at, satl_line_not_understood,
               written + " is outside every capsule, and nothing outside a capsule ever runs -- a row of the "
                         "arguments is read inside one, and a name of the program's own is written there, as "
                         "args.my_value = 5 is in satellite.main(satellite.variable.arguments args). The lines of "
                         "the arguments a file writes at its top are its settings: " + every_setting_named());
        return;
    }
    // THE OLD PLACE: its spelling now is a line that runs.
    if (line.head == kOldPlace) {
        refuse(table, r, declared_at, satl_line_not_understood,
               written + " is where the arguments lived until 2026-10-03 -- a file's line is written arguments." +
                   key + "(" + an_example_of(*setting) + "), or satellite.library.arguments." + key + " = " +
                   an_example_of(*setting));
        return;
    }
    // TOO LATE: a row satl reads as it starts, before it reads this file.
    if (const std::string late = why_a_file_line_is_too_late(*setting, written, true); !late.empty()) {
        refuse(table, r, declared_at, word_takes_no_assignment, late);
        return;
    }
    if (!line.call && code_at(row, line.sign) != token::assign_token) {
        refuse(table, r, declared_at, satl_line_not_understood,
               written + " needs its value after an = -- " + shown_with(written, false, an_example_of(*setting)));
        return;
    }
    // NO VALUE ON ITS LINE: the caret on the `=`, and never under the line the join handed it.
    if (!line.call && the_equals_ends_its_line(row, line.sign, table.scopes[file_scope].file)) {
        refuse(table, r, line.sign, satl_line_not_understood,
               written + " needs its value after the =, on its own line -- " +
                   shown_with(written, false, an_example_of(*setting)) +
                   ". A line that ends in = goes on into the next, so the next line written was read as its value");
        return;
    }
    std::string kept;
    signed long long int code = success;
    const std::string refused = the_value_written(row, line.sign + 1, line, *setting, written, kept, code);
    if (!refused.empty()) {
        refuse(table, r, line.sign + 1, code, refused);
        return;
    }
    CapsuleScope &file = table.scopes[file_scope];
    if (file.switches.count(key) != 0) {
        refuse(table, r, declared_at, name_declared_twice,
               "arguments." + key + " is written twice in this file -- a switch is written once, and a second "
                                   "would quietly replace the first");
        return;
    }
    ArgumentSwitch switched;
    switched.row = r;
    switched.declared_at = declared_at;
    switched.value = kept == "true";
    switched.kept = kept;
    file.switches.emplace(key, switched);
}

} // namespace scan

void set_the_program_s_switches(const CapsuleTable &table, Arguments &arguments, MachineState &state)
{
    (void)arguments;   // the run's rows, which state.arguments is, and change_a_setting changes them there
    // THE PROGRAM'S FILE IS THE FIRST ROW: load_program reads it first, and its includes after it.
    if (table.file_scope.empty())
        return;
    for (const auto &[key, line] : table.scopes[table.file_scope.front()].switches) {
        const Setting *setting = a_setting_named(key);
        if (setting == nullptr)
            continue;                        // the scan lets through nothing else
        // FOR THIS RUN (setting_writes.hpp): a file's line, and the scan has judged its value already.
        std::string why;
        change_a_setting(*setting, a_value_the_scan_kept(*setting, line.kept), Lasting::this_run, "arguments." + key,
                         state, why);
        if (state.debug_mode)
            state.set("arguments." + key + " = " + line.kept + " (the program's own line, at the top of its file)",
                      success);
    }
}

} // namespace satellite004
