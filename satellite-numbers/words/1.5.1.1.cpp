// satellite-numbers/words/1.5.1.1.cpp -- satellite.console.display.center, built into satl
//
// satellite.console.display.center   1 5 1 1
// The display's line in the middle of the console (the author, 2026-09-25: satellite.console.display(
// "something").center() "renders the text in the middle of the console window"), for the width the
// console has as the line is shown. Its second spelling is centre, 1 5 1 2.
//
// RUN BY ITS NUMBER SINCE 2026-10-11 (the author: "we have to give everything a number and then make it
// run those numbers"; "everything builtin needs the word satellite still"):
// `satellite.console.display("x").satellite.center()` is taken inside the display word's call, before
// the line is printed (bytecode/expression.cpp), and bytecode/console_calls.cpp's display_with_options
// hands the line to the function below through the function table. The code in it is console_calls.cpp's
// centred_text until that day, moved here whole. What it is handed and answers is the line as text: it
// follows a word of the language, not a value, so no other type has it.

#include "../number_row.hpp"
#include "../../satellite/bytecode/console_calls.hpp"
#include "../../satellite/machine/machine_codes.hpp"

#include <string>
#include <unistd.h>

namespace {

using namespace satellite004;

// EACH LINE IN THE MIDDLE OF THE CONSOLE (.center(), the author 2026-09-25: "it doesn't need
// to change with a resized console, just for that console at that time"). Padded with half
// of what is left of the width the terminal has NOW, counted in characters; a line as wide
// as the console or wider is left as it is. NOT A TERMINAL -- a file, a pipe -- has no width
// to be in the middle of, and the text goes out as written, as colour does there.
std::string centred_text(const std::string &text)
{
    if (!isatty(STDOUT_FILENO))
        return text;
    const std::size_t width = static_cast<std::size_t>(console_parts::terminal_size(true));
    std::string out;
    std::size_t start = 0;
    for (;;) {
        const std::size_t end = text.find('\n', start);
        const std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::size_t shown = 0;
        for (const char c : line)
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) ++shown;
        if (shown < width)
            out.append((width - shown) / 2, ' ');
        out += line;
        if (end == std::string::npos)
            break;
        out += '\n';
        start = end + 1;
    }
    return out;
}

Value method(const Value &receiver, Value *, const TypeShape *, const std::vector<Value> &, bool,
             const std::string &name, ExpressionContext &context)
{
    Value centred;
    std::size_t bad = 0;
    const signed long long int made = Value::of_utf8(centred_text(receiver.text_utf8()), centred, bad);
    if (made != success) {
        context.refuse(made, name + ".satellite.center was handed a line whose bytes are not text, at byte " +
                                 std::to_string(bad + 1));
        return Value();
    }
    return centred;
}

} // namespace

namespace satellite004::built_in {

signed long long int describe_1_5_1_1(LibraryRow *row)
{
    row->name = "satellite.console.display.center";
    row->numbers[0] = 1;
    row->numbers[1] = 5;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->depth = 4;
    row->scenarios.method_call = method;
    return satellite004::success;
}

} // namespace satellite004::built_in
