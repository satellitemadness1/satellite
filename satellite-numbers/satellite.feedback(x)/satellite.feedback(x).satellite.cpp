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
// IT TAKES A STRING, OR A BRACED LIST OF THEM -- `satellite.feedback({"a","b"})`,
// which is the shape the author asked for and which now exists (2026-09-18).
//
// THE WHOLE LIST ARRIVES IN ONE CALL, and that is what keeps the cap honest.
// feedback_book.hpp bounds how many DIFFERENT things one run may say, and if the
// interpreter sent each item as its own call, a list of forty would spend forty
// of that allowance while looking to the person like one report. One call of
// three items is one thing said, in three lines.
//
// AN EMPTY LIST IS REFUSED, not silently accepted: `satellite.feedback({})` is
// somebody who meant to say something. Saying so is better than a success that
// stored nothing.
//
// WHAT IT NEVER SEES: a name, a path, a directory, a hostname, the program's own
// source. Not by discipline -- by the shape of the call. It is handed a string.

#include "../feedback_book.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.feedback(x)";
    row->numbers[0] = 1;
    row->numbers[1] = 25;
    row->numbers[2] = 1;
    row->depth = 3;
    row->scenarios.text = &satellite004::feedback_book::feedback_text;
    row->scenarios.list = &satellite004::feedback_book::feedback_list;
    return satellite004::success;
}
