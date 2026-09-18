// satellite.feedback(x)  `1 25 1` -- what a person wants to tell us, kept on
// THEIR machine until they choose to send it.
//
// The author, 2026-09-18: *"I do not want to spy on the users... the users code
// and what they are doing on their machine is their own business, and the
// programming language should NOT collect that sort of stuff"*.
//
// SO THIS WORD SENDS NOTHING. It writes a line to ~/.satl/feedback.txt and
// stops. `satl --feedback` shows what is there; sending is a thing a PERSON does
// with a command, never a thing a PROGRAM does while running.
//
// THAT IS ALSO THE WHOLE ANSWER TO THE FLOOD. The author asked for "a system
// that you cannot BOMB with satellite.feedback({"something_in_a_loop"})", and a
// word that cannot reach a network cannot bomb anybody through one. A loop
// calling this a million times writes ONE line with a count of a million --
// see feedback_book.hpp for the four bounds and the number behind each.
//
// IT TAKES A STRING. The author also asked for a list of strings, which is the
// other shape a person would write. `satellite.container.list` is numbered and
// unbuilt (words.tsv `1 4 2`), so the list arm is owed and is one `case` here
// when that container runs.
//
// WHAT IT NEVER SEES: a name, a path, a directory, a hostname, the program's own
// source. Not by discipline -- by the shape of the call. It is handed a string.

#include "../feedback_book.hpp"

namespace {

satellite004::SettingReply feedback_from_text(const std::string &said)
{
    satellite004::SettingReply reply;
    std::string why;
    reply.code = satellite004::feedback_book::write_one(said, why);
    reply.flag = (reply.code == satellite004::success);
    reply.reason = why;
    return reply;
}

signed long long int feedback_text(const std::string &text, bool)
{
    const satellite004::SettingReply said = feedback_from_text(text);
    return said.code;
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.feedback(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 25;
    row->numbers[2] = 1;
    row->depth = 3;
    row->scenarios.text = &feedback_text;
    return satellite004::success;
}
