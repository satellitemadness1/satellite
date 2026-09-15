// display's style options -- M30. See style.hpp for what is written and when.

#include "satellite_console/style.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/machine.hpp"
#include "satellite_bits/bits.hpp"
#include "satellite_value/render.hpp"
#include "satellite_value/value.hpp"

#include <cstdlib>
#include <string>

#include <unistd.h>

namespace satellite::console {

namespace {

// `foreground=` or `background=`, as the three decimal numbers SGR's 24-bit
// form takes -- "255;136;0" -- or false after refusing. A hex run holds its
// bits most significant first, so bits 0-7 are red, 8-15 green, 16-23 blue.
bool colour_of(eval::Machine &m, const char *name, const Value &given,
               std::string *out)
{
    const bits::HexRun *run = given.is_hex() ? as_hex(given) : nullptr;
    if (run == nullptr || run->digits() != 6) {
        std::string what;
        if (run != nullptr)
            what = text_of(given);
        else if (given.is_string())
            what = "\"" + text_of(given) + "\", a string,";
        else
            what = std::string("a ") + type_name(given);
        m.refuse(errors::make<errors::Code::CONSOLE_NOT_A_COLOUR>(
            m.span_of(m.here()), std::string(name) + "=", what));
        return false;
    }
    for (size_t channel = 0; channel < 3; channel++) {
        unsigned level = 0;
        for (size_t i = 0; i < 8; i++)
            level = (level << 1) | (run->bits.bits[channel * 8 + i] ? 1u : 0u);
        if (channel > 0)
            out->push_back(';');
        *out += std::to_string(level);
    }
    return true;
}

// `bold=` or `italic=` -- true, false, or refused.
bool switch_of(eval::Machine &m, const char *name, const Value &given, bool *on)
{
    if (!given.is_bool()) {
        m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
            m.span_of(m.here()), std::string(name) + "=",
            "`satellite.bool.true` or `satellite.bool.false`",
            type_name(given)));
        return false;
    }
    *on = std::get<bool>(given);
    return true;
}

} // namespace

bool style_of(eval::Machine &m, std::string *open, std::string *close)
{
    std::string foreground, background;
    bool bold = false, italic = false;

    if (const Value *given = m.option("foreground"))
        if (!colour_of(m, "foreground", *given, &foreground))
            return false;
    if (const Value *given = m.option("background"))
        if (!colour_of(m, "background", *given, &background))
            return false;
    if (const Value *given = m.option("bold"))
        if (!switch_of(m, "bold", *given, &bold))
            return false;
    if (const Value *given = m.option("italic"))
        if (!switch_of(m, "italic", *given, &italic))
            return false;

    // Checked above whatever happens below; see style.hpp.
    if (isatty(STDOUT_FILENO) == 0)
        return true;
    const char *no_color = std::getenv("NO_COLOR");
    if (no_color != nullptr && *no_color != '\0')
        foreground.clear(), background.clear();

    // ONE SGR SEQUENCE, parameters joined by `;`, so the style cannot be half
    // written. The reset goes BEFORE the newline or end=: a background still
    // set when the terminal scrolls paints the whole new row in some terminals.
    std::string parameters;
    const auto add = [&](const std::string &part) {
        if (!parameters.empty())
            parameters.push_back(';');
        parameters += part;
    };
    if (bold)
        add("1");
    if (italic)
        add("3");
    if (!foreground.empty())
        add("38;2;" + foreground);
    if (!background.empty())
        add("48;2;" + background);
    if (parameters.empty())
        return true;

    *open = "\033[" + parameters + "m";
    *close = "\033[0m";
    return true;
}

} // namespace satellite::console
