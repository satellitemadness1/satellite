#pragma once
// satellite/machine/input_source.hpp -- where satellite.console.input() reads a line
// from while the prompt's session is running (the review, 2026-09-23).
//
// WHY A HOOK AND NOT std::cin. The session reads standard input through its own
// LineReader (prompt/line_reader.hpp: read(2) into a buffer of its own, "never
// std::cin"), so a line a person typed or piped after `input("q? ")` may already be
// in THAT buffer when input() asks -- and std::cin, reading the same descriptor
// behind it, finds the input ended. A session sets this to a function that reads
// through its reader, which also draws the prompt and answers Ctrl-C as the prompt
// does. Null in every other run of satl, where input() reads std::cin itself.
//
// stop_flag.hpp's shape and stop_flag.hpp's reason: one pointer set once when a
// session starts, instead of a parameter threaded through five signatures. It holds
// nothing a program wrote -- there are still no globals in the language.

#include <string>

namespace satellite004 {

enum class InputAnswer { line, ended, interrupted };

// `text` is the prompt as it is counted; `drawn` is the same with its colour, or empty.
using InputSource = InputAnswer (*)(const std::string &text, const std::string &drawn, std::string &line);

inline InputSource &input_source()
{
    static InputSource source = nullptr;
    return source;
}

} // namespace satellite004
