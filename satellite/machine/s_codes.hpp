#pragma once
// satellite/machine/s_codes.hpp -- THE NUMBER A PERSON IS SHOWN, AND THE ONE
// CALL THAT SHOWS IT. SATELLITE_ERROR E5-E12.
//
// THE TWO NUMBERING SCHEMES ARE NOT RIVALS, and this file is the bridge:
//
//   a MACHINE CODE is what a function ANSWERS, and becomes the exit status.
//                  machine_codes.hpp. 0-254.
//   an S-CODE      is what a PERSON is shown in a report. SATELLITE_ERROR Part 4.
//
// They are MANY-TO-MANY on purpose. satl_line_not_understood (13) is a hundred
// different things a person needs a hundred different sentences for, and one
// S-code may be reachable from more than one machine code. A report carries
// both: the S-code in its header, the machine code in the exit status.
//
// BLOCKS, SO A NUMBER NEVER HAS TO MOVE. Add at the END of a block, never
// renumber -- the same rule as the word table, for the same reason: a number a
// person has written down, searched for or put in a bug report is a number that
// has to keep meaning what it meant.
//
//     S04xx  the checker: shapes refused before anything runs
//     S05xx  names: declared twice, never declared, out of scope
//     S06xx  types: two kinds that do not meet
//     S07xx  settings and arguments        (S0721-S0727 already assigned)
//     S08xx  numbers: division by zero, an answer that is not whole
//     S09xx  files: the two file refusals that stop a program (2026-09-18)
//
// 003'S NUMBERS ARE NOT PORTED WHOLESALE. Where a 004 refusal IS 003's refusal
// it takes 003's number so a person moving between them reads one number; where
// it is new it takes the next free one in its block. **003's string refusals are
// deliberately absent here**: 003 numbered them in S07xx, which is 004's settings
// block, and picking between 003's number and 004's block is the author's call
// and not one to make quietly in a header.

#include "critical_report.hpp"
#include "machine_codes.hpp"
#include "machine_state.hpp"
#include "source_position.hpp"

#include <string>

namespace satellite004 {

// WHAT A PERSON IS SHOWN FOR A MACHINE CODE: the S-code, the shouted name, and
// the sentence that says what it MEANS rather than what it is called.
struct SCode {
    const char *code;
    const char *name;
    const char *means;
};

// ONE ROW A MACHINE CODE THAT A PERSON CAN REACH. A code missing from here is
// not a mistake -- it gets the fallback below, which still says the machine
// code's own name, so nothing is ever reported as a bare number.
inline SCode s_code_for(signed long long int machine_code)
{
    switch (machine_code) {
    case satl_line_not_understood:
        return {"S0401", "LINE_NOT_UNDERSTOOD",
                "satellite has no scenario for this line -- it is spelled in a shape the language "
                "does not have a meaning for."};
    case not_built_yet:
        return {"S0402", "NOT_BUILT_YET",
                "this word is numbered in the language and the library behind it does not exist "
                "yet. The word is right; there is nothing yet to run."};
    case name_not_declared:
        return {"S0501", "NAME_NOT_DECLARED",
                "this name was used and no satellite.variable line ever declared it. A name has to "
                "be given a type before it can hold anything."};
    case name_declared_twice:
        return {"S0502", "NAME_DECLARED_TWICE",
                "this name already has a satellite.variable line in this capsule. One name, one "
                "declaration -- the second would quietly replace the first."};
    case types_do_not_meet:
        return {"S0601", "TYPES_DO_NOT_MEET",
                "this operator has no scenario for the two kinds it was given. Nothing was guessed "
                "at, because a guess here is an answer that is wrong and does not say so."};
    case division_by_zero:
        return {"S0801", "DIVISION_BY_ZERO",
                "a divisor worked out to 0. There is no number this could answer, so it answers "
                "nothing rather than something."};
    case answer_is_not_whole:
        return {"S0802", "ANSWER_IS_NOT_WHOLE",
                "the answer exists and is not a whole number -- 2 ^ -1 is one half. "
                "satellite.variable.number holds whole numbers, so this has nowhere to go."};
    case setting_is_not_a_flag:
        return {"S0728", "SETTING_IS_NOT_A_FLAG",
                "this setting is true or false and was given something that is neither."};
    case word_takes_no_assignment:
        return {"S0729", "WORD_TAKES_NO_ASSIGNMENT",
                "this word is not a setting and not a variable, so there is nothing for an `=` to "
                "write to."};
    case config_file_unreadable:
        return {"S0730", "CONFIG_FILE_UNREADABLE",
                "config.ini is there and could not be read. Every setting still has its built-in "
                "default, so satl runs -- it is running on defaults and not on your settings."};
    case config_file_unwritable:
        return {"S0722", "REGISTER_NOT_WRITTEN",
                "a setting could not be saved, so it holds for this run and is gone at the end of it."};
    case line_past_the_end:
        return {"S0901", "LINE_PAST_THE_END",
                "a line was read by its number and the file has no line with that number. Lines "
                "count from 1, and the last one is the file's size."};
    case file_has_no_lines:
        return {"S0902", "FILE_HAS_NO_LINES",
                "a word about lines was used on a binary file, and a binary file is bytes, not lines."};
    case file_not_open:
        return {"S0903", "FILE_NOT_OPEN",
                "a question was asked of a file that is not open -- closed, or never opened because the "
                "open or new failed. Its answer would have been made up, so nothing was answered. Ask "
                "the file's ok first; its error says why it is not open."};
    case file_unwritable:
        return {"S0904", "FILE_NOT_SAVED",
                "changes to a file could not be put on the disk. The file on the disk is as it was before "
                "them -- a save is whole or not at all."};
    case not_a_position:
        return {"S0905", "NOT_A_POSITION",
                "a position or a line number below zero. Lines count from 1, and nothing counts below zero."};
    default:
        break;
    }
    // THE FALLBACK IS NOT A FAILURE. A refusal with no S-code yet is still worth
    // reporting with everything else this file gathers -- the file, the line and
    // the caret -- and S0000 says plainly that the number is owed rather than
    // pretending the refusal is nameless.
    return {"S0000", "REFUSED", ""};
}

// E5 -- ONE CALL THAT RENDERS, PRINTS AND ANSWERS THE MACHINE CODE.
//
// `return raise(report, code);` is the whole shape, so a caller never writes the
// print and the return as two statements that could disagree about which code
// the run stopped on.
inline signed long long int raise(const CriticalReport &report, signed long long int machine_code)
{
    print_critical(report);
    return machine_code;
}

// E5/E6 TOGETHER -- A REFUSAL, A PLACE, AND NOTHING ELSE FROM THE CALLER.
//
// `why` is the sentence the refusal already built ("a minus sign was put in
// front of a satellite.variable.string"), and it is kept: it says what happened
// on THIS line, where the S-code's `means` says what the refusal IS. Both are
// worth having and they are not the same sentence.
//
// `doing` is what the interpreter was in the middle of -- "satellite.statement.if",
// "my_name = ..." -- which is the one thing the position cannot say.
//
// `stage` IS WHICH HALF OF THE INTERPRETER REFUSED, and it carries more than it
// looks like. "satl(check)" means NOTHING RAN: every capsule was walked and
// judged before main was entered, so a program that cannot finish did not half
// print first. "satl(run)" means it got as far as this line and stopped there.
// A person reading a report needs that before they need anything else -- it is
// the difference between "my program is wrong" and "my program did half a job"
// -- and check.sh asserts on it in a dozen places for the same reason.
inline signed long long int raise_at(signed long long int machine_code,
                                     const std::string &why,
                                     const std::string &doing,
                                     const MachineState &state,
                                     const std::vector<std::bitset<16>> &row,
                                     std::size_t at,
                                     const char *stage = "satl(run)")
{
    const SCode named = s_code_for(machine_code);
    CriticalReport report;
    report.code = named.code;
    report.name = named.name;

    // WHAT HAPPENED, ON ITS OWN LINE. `why` is the sentence the refusal already
    // built -- "the / of 10 and 0 is a division by zero" -- and it is the first
    // thing a person reads.
    report.description = why;
    if (!doing.empty())
        report.description = "in " + doing + ", " + report.description;
    if (stage != nullptr && stage[0] != '\0')
        report.description = std::string(stage) + ": " + report.description;

    // THE PROGRAM COMES OFF MachineState AND THE ROW NAMES ITSELF. A caller
    // passes what it has in its hand -- the row it is walking and the position
    // in it -- and never a file, a line, a column or an index it would have had
    // to be handed for this one path.
    if (state.program != nullptr && state.program_files != nullptr)
        report_at(report, *state.program, *state.program_files,
                  row_index_of(state.program, row), at);

    // THE MACHINE CODE IS IN THE REPORT AS WELL AS IN THE EXIT STATUS, because a
    // person reading a report and a person reading `echo $?` are often the same
    // person an hour apart, and they should not have to map one to the other.
    // WHAT THE REFUSAL IS, AS A NOTE AND NOT GLUED TO THE DESCRIPTION. The
    // renderer wraps each of these on its own, so a sentence that carried its own
    // newlines into `description` came out broken mid-word. Two sentences that
    // say different things -- what happened here, and what this refusal means --
    // are two entries.
    if (named.means[0] != '\0')
        report.notes.push_back(named.means);
    report.notes.push_back("machine code " + std::to_string(machine_code) + " " +
                           machine_code_name(machine_code) + " -- satl exits with this.");
    return raise(report, machine_code);
}

} // namespace satellite004
