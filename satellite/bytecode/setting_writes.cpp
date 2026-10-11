// satellite/bytecode/setting_writes.cpp -- a setting changed by a line (setting_writes.hpp).

#include "setting_writes.hpp"
#include "../satellite_variable_window/console_scrolling.hpp"

#include "argument_switches.hpp"
#include "../arguments/arguments.hpp"
#include "../config/config_file.hpp"
#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../machine/satellite_log.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_variable_float/float_precision.hpp"

#include <climits>
#include <string>
#include <utility>
#include <vector>

namespace satellite004 {
namespace {

Value text_value(const std::string &text)
{
    Value made;
    std::size_t bad_offset = 0;
    // A ROW THAT IS NOT UTF-8 -- a command-line word can be any bytes -- keeps the text that could
    // be read rather than being dropped (main_arguments.cpp's reason, kept).
    if (Value::of_utf8(text, made, bad_offset) != success)
        Value::of_utf8(text.substr(0, bad_offset), made, bad_offset);
    return made;
}

// The words a string value holds, or "" -- and `read` false when it held no string.
std::string words_of(const Value &value, bool &read)
{
    satellite_string text;
    std::string unused;
    read = value.is_string() && value.to_string(text, unused) == success;
    return read ? text.to_utf8() : std::string();
}

// A count the float's precisions hold: past one limb is more places than any machine can hold, so it
// means no limit that could be reached (float_values.cpp's digits_of_row, the same rule).
unsigned long long int as_a_count(const satellite_number &number)
{
    return number.fits_one_limb() ? number.limb(0) : ULLONG_MAX;
}

} // namespace

Value the_value_of_a_row(const Argument &row)
{
    switch (row.kind) {
    case ArgumentKind::text: return text_value(row.text);
    case ArgumentKind::count: return Value::of_number(satellite_number(row.count));
    case ArgumentKind::number: return Value::of_number(row.number);
    case ArgumentKind::flag: return Value::of_bool(row.flag);
    case ArgumentKind::size: return Value::of_number(satellite_number(row.count));
    case ArgumentKind::list: {
        std::vector<Value> items;
        for (const std::string &item : row.items)
            items.push_back(text_value(item));
        return Value::of_list(make_list(std::move(items)));
    }
    }
    return Value();
}

std::string why_a_setting_does_not_take(const Setting &setting, const Value &value, const std::string &written,
                                        signed long long int &code)
{
    switch (setting.kind) {
    case SettingKind::flag:
        // TRUE OR FALSE AND NOTHING ELSE (B2's rule, kept): 1 is not quietly taken for true, or
        // `arguments.access(2)` would be a line with no meaning that ran.
        if (value.is_bool())
            return std::string();
        code = setting_is_not_a_flag;
        return written + " is true or false, and was given " + value.kind_name();
    case SettingKind::number: {
        const satellite_number *number = value.as_number();
        if (number == nullptr) {
            code = types_do_not_meet;
            return written + " is a whole number, and was given " + value.kind_name();
        }
        if (number->negative() || satellite_number::compare(*number, satellite_number(setting.least)) < 0) {
            code = setting_out_of_range;
            return written + " is a whole number of at least " + std::to_string(setting.least) + ", and was given " +
                   number->to_text();
        }
        return std::string();
    }
    case SettingKind::text: {
        bool read = false;
        const std::string words = words_of(value, read);
        if (!read) {
            code = types_do_not_meet;
            return written + " is a folder or a path, written in quotes, and was given " + value.kind_name();
        }
        if (words.empty()) {
            code = setting_out_of_range;
            return written + " is a folder or a path, and was given \"\" -- a setting of nothing is no place at all";
        }
        return std::string();
    }
    }
    return std::string();
}

std::string why_a_file_line_is_too_late(const Setting &setting, const std::string &written, bool at_the_top)
{
    const std::string key = setting.key;
    if (setting.read == SettingRead::as_satl_starts)
        return written + " is read as satl starts, before it reads any file, so a line in a file comes too late to "
                         "change it for its own run. Typed at the prompt it is saved for every run after: "
                         "arguments." + key + "(value)";
    if (setting.read == SettingRead::at_the_top_of_the_file && !at_the_top)
        return a_switch_written_too_late(written, key) + ". Typed at the prompt, arguments." + key +
               "(false) is saved for every run";
    return std::string();
}

signed long long int change_a_setting(const Setting &setting, const Value &value, Lasting lasting,
                                      const std::string &written, MachineState &state, std::string &why)
{
    signed long long int code = success;
    why = why_a_setting_does_not_take(setting, value, written, code);
    if (!why.empty())
        return code;

    const std::string key = setting.key;
    Argument row;
    // A GUARD, named as one: satl's start gives every run its rows, and only a binary that never ran
    // that start has none -- and every setting has a row once gather_config has run.
    if (state.arguments == nullptr || !state.arguments->copy_of("arguments." + key, row)) {
        why = written + " is not a row this satl holds, so it has nothing to change";
        return name_not_declared;
    }

    std::string saved;   // what config.ini is given: true, false, the digits, or the words
    bool read = false;
    switch (setting.kind) {
    case SettingKind::flag:
        row.flag = *value.as_bool();
        saved = row.flag ? "true" : "false";
        break;
    case SettingKind::number:
        row.number = *value.as_number();
        saved = row.number.to_text();
        break;
    case SettingKind::text:
        row.text = words_of(value, read);
        saved = row.text;
        break;
    }

    // FOR GOOD FIRST, AND A WRITE THAT FAILS CHANGES NOTHING: a line that said "saved" when config.ini
    // refused it would be the quiet kind of wrong, and the run's row is not changed either -- the
    // line did not do what it said.
    if (lasting == Lasting::for_good) {
        // ONE VALUE A LINE: config.ini is `key = value` a line, so words with a line's end in them could
        // not be read back as the words that were saved.
        if (saved.find_first_of("\r\n") != std::string::npos) {
            why = written + " could not be saved -- config.ini holds a value a line, and these words have a "
                            "line's end in them";
            return setting_out_of_range;
        }
        std::string refused;
        code = config_file::write_value(key, saved, refused);
        if (code != success) {
            why = written + " could not be saved in config.ini" + (refused.empty() ? "" : " -- " + refused);
            return code;
        }
    }

    if (!state.arguments->set_for_this_run(row)) {
        why = written + " is not a row this satl holds, so it has nothing to change";
        return name_not_declared;
    }

    // WHAT THE RUN COPIED OF THE ROW, COPIED AGAIN, so the change holds from this line on.
    if (key == "float.whole")
        float_precision_in_use.whole = as_a_count(row.number);
    else if (key == "float.decimal")
        float_precision_in_use.decimal = as_a_count(row.number);
    else if (key == "infinity_display")
        float_precision_in_use.shown = as_a_count(row.number);
    else if (key == "log_path")
        set_log_path(row.text);
    // THE SCROLL ROWS REACH THE CONSOLE (the author, 2026-10-07): the run's answer, and satl's own
    // console follows it at once -- a program's top line turns its scrolling off for that run.
    else if (key == "scroll.vertical" || key == "scroll.horizontal") {
        (key == "scroll.vertical" ? console_scrolls_vertically() : console_scrolls_horizontally())
            .store(row.flag, std::memory_order_relaxed);
        scrolling_rows_changed();
    }
    return success;
}

Value a_value_the_scan_kept(const Setting &setting, const std::string &kept)
{
    switch (setting.kind) {
    case SettingKind::flag:
        return Value::of_bool(kept == "true");
    case SettingKind::number: {
        satellite_number number;
        std::size_t bad_offset = 0;
        satellite_number::from_text(kept, number, bad_offset);   // the scan read the digits already
        return Value::of_number(std::move(number));
    }
    case SettingKind::text:
        return text_value(kept);
    }
    return Value();
}

} // namespace satellite004
