#pragma once
// satellite/satl/prompt_help.hpp -- satellite.help() and satellite.help(topic) at the
// prompt, read from the text files in satellite.help/.
//
// (the author, 2026-09-22) "004 prompt doesn't even take satellite.help()!", then
// "you just need to write text files ... then a function that reads them". So the
// help IS the files: satellite.help/help.txt is the topic list, and
// satellite.help/<topic>/help_text.txt is one topic -- his own layout, begun with
// satellite.help/satellite.include/help_text.txt. They are read when asked, never
// compiled in, so a topic edited is a topic changed the next time it is asked for.

#include <bitset>
#include <vector>

namespace satellite004 {

// When the typed line is satellite.help() or satellite.help(topic) and nothing
// else, prints the help and answers true with its code in `answer`. Answers false,
// touching nothing, for any other line.
bool answer_help(const std::vector<std::bitset<16>> &row, signed long long int &answer);

} // namespace satellite004
