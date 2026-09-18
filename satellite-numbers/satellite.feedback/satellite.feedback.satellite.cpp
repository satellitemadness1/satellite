// satellite.feedback  `1 25` -- what a person wants to tell us, kept on
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
// THIS IS THE BARE SPELLING, `1 25`, and it exists because the lexer matches the
// longest PLAIN path before it tries a shaped one: `satellite.feedback("x")`
// resolves to this row, not to `satellite.feedback(x)`. Both are built and both
// do the same thing, which is what `arguments.memory` does for the same reason.
//
// IT TAKES A STRING. The author also asked for a list of strings, which is the
// other shape a person would write. `satellite.container.list` is numbered and
// unbuilt (words.tsv `1 4 2`), so the list arm is owed and is one `case` here
// when that container runs.
//
// WHAT IT NEVER SEES: a name, a path, a directory, a hostname, the program's own
// source. Not by discipline -- by the shape of the call. It is handed a string.

#include "../feedback_book.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.feedback";
    row->numbers[0] = 1;
    row->numbers[1] = 25;
    row->depth = 2;
    row->scenarios.text = &satellite004::feedback_book::feedback_text;
    row->scenarios.list = &satellite004::feedback_book::feedback_list;
    return satellite004::success;
}
