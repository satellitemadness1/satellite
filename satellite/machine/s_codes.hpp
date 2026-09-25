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
//     S07xx  settings and arguments        (S010-S014 already assigned)
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

#include <cstdint>
#include <string>
#include <unordered_map>

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
        return {"S110", "LINE_NOT_UNDERSTOOD",
                "satellite has no scenario for this line -- it is spelled in a shape the language "
                "does not have a meaning for."};
    case not_built_yet:
        return {"S210", "NOT_BUILT_YET",
                "this word is numbered in the language and the library behind it does not exist "
                "yet. The word is right; there is nothing yet to run."};
    // S02xx -- READING A FILE: it is not there, not readable, not satellite.
    // These three are about a WHOLE FILE and have no position in it, so they
    // carry no caret. What they carry instead is the exact line to type, which
    // is the only thing a person wants when a file is refused before it runs.
    case satl_file_missing_satellite_include_satellite:
        return {"S101", "FILE_HAS_NO_INCLUDE",
                "a program's first line is satellite.include(satellite). Without it the file is a "
                "SPACESHIP -- something another file includes -- and satl will not start it. "
                "Add this as the first line:\n\n    satellite.include(satellite)"};
    case satl_file_missing_satellite_main:
        return {"S102", "FILE_HAS_NO_MAIN",
                "there are no globals in satellite, so a statement outside a capsule has nowhere to "
                "put its result and no moment to run in -- which means there is nowhere to begin. "
                "Add:\n\n    satellite.capsule satellite.main()\n    {\n        ...\n    }"};
    case satl_file_missing_satellite_return_satellite:
        return {"S103", "FILE_HAS_NO_RETURN",
                "execution ends inside main, and the file has to say so. "
                "Add this as the last line:\n\n    satellite.return(satellite)"};

    // S00xx -- STARTING UP.
    case error:
        return {"S910", "REFUSED_WITHOUT_A_REASON",
                "something refused and answered the general code rather than one of its own. This is "
                "the interpreter failing to say what went wrong, so the S-code is worth reporting as "
                "a bug in satellite itself."};
    // S9xx -- SATL ITSELF IS IN TROUBLE, and S999 is the top of the whole scale.
    // The author, 2026-09-18: *"999 will be... cannot load the C++ libraries,
    // that is the worst possible error, well, the worst possible error is that we
    // cannot set the memory for them"*. He corrected himself mid-sentence and the
    // correction is right: a library that will not load leaves an interpreter
    // that runs and cannot do everything. NO MEMORY leaves no interpreter.
    case out_of_memory:
        return {"S999", "OUT_OF_MEMORY",
                "the machine would not give satl the memory it asked for. This is the top of the "
                "scale because it is the one failure where nothing else can be attempted -- a report "
                "itself needs memory. Nothing was left half-written: satl stops here."};
    case libraries_not_understood:
        return {"S980", "LIBRARY_NOT_UNDERSTOOD",
                "a numbered library loaded and did not describe itself. The .so is there and is not "
                "the one this satl was built against -- a rebuild of satellite-numbers/ is what fixes "
                "it, and `satl --rebuild` says what was found."};
    case vector_loading_error:
        return {"S901", "LIBRARIES_NOT_LOADED",
                "the numbered libraries under satellite-numbers/ could not be read, so most words of "
                "the language have nothing behind them. satl --rebuild reports what it found."};

    // S01xx -- THE COMMAND LINE.
    case command_line_not_understood:
        return {"S130", "COMMAND_LINE_NOT_UNDERSTOOD",
                "satl was given words it does not take. satl --help lists every way to start it."};

    case missing_satl_file:
        return {"S140", "FILE_NOT_FOUND",
                "satl was told to run a file that is not there. Check the path, and that the name "
                "ends .satl."};

    // S03xx -- READING WHAT IS WRITTEN: a number or some text that cannot be read.
    case int_error:
        return {"S120", "NUMBER_NOT_READ",
                "this is not a number satellite can read. A number is an optional '-' then digits, "
                "with no spaces or separators, and a float has one point in it: 12.34."};
    case string_error:
        return {"S121", "TEXT_NOT_UTF8",
                "these bytes are not valid UTF-8. satellite's strings hold text, so bytes that "
                "stand for no character are refused rather than carried."};

    case name_not_declared:
        return {"S201", "NAME_NOT_DECLARED",
                "this name was used and no satellite.variable line ever declared it. A name has to "
                "be given a type before it can hold anything."};
    case name_declared_twice:
        return {"S202", "NAME_DECLARED_TWICE",
                "this name is already declared where this line stands -- a variable in its "
                "capsule, or a capsule, a satellite.namespace or an included file in its file or "
                "space. One name, one declaration -- the second would quietly replace the first."};
    // THE TWO OF 2026-09-22, in the names block: a name inside a spacesuit that the
    // line cannot reach, and a capsule that had no answer to give.
    case member_is_protected:
        return {"S230", "MEMBER_IS_PROTECTED",
                "this is inside a spacesuit, where only its own capsules can reach it -- every field "
                "is, and so is a capsule written in satellite.protected. A capsule in satellite.public "
                "that answers it is the way in from outside."};
    case capsule_gave_no_answer:
        return {"S240", "CAPSULE_GAVE_NO_ANSWER",
                "this capsule's answer was used, and the way it went reached no satellite.return(...) "
                "-- so there was nothing to hand back."};
    // AND 2026-09-23's, with satellite.library: a value nothing may change or work out.
    case library_value_is_fixed:
        return {"S250", "LIBRARY_VALUE_IS_FIXED",
                "a satellite.library value is written down at the top of its file -- one number, text or other "
                "literal -- and nothing works it out or changes it afterwards. A value every capsule could "
                "change would be a global, and satellite has none."};
    case types_do_not_meet:
        return {"S301", "TYPES_DO_NOT_MEET",
                "this operator has no scenario for the two kinds it was given. Nothing was guessed "
                "at, because a guess here is an answer that is wrong and does not say so."};
    case division_by_zero:
        return {"S401", "DIVISION_BY_ZERO",
                "a divisor worked out to 0. There is no number this could answer, so it answers "
                "nothing rather than something."};
    case answer_is_not_whole:
        return {"S402", "ANSWER_IS_NOT_WHOLE",
                "the answer exists and is not a whole number -- 2 ^ -1 is one half. "
                "satellite.variable.number holds whole numbers, so this has nowhere to go."};
    case setting_is_not_a_flag:
        return {"S310", "SETTING_IS_NOT_A_FLAG",
                "this setting is true or false and was given something that is neither."};
    case word_takes_no_assignment:
        return {"S220", "WORD_TAKES_NO_ASSIGNMENT",
                "this is not a place a program can write: a word that is not a setting and not a "
                "variable, or a row of the arguments that the machine, the command line or config.ini "
                "says -- a program reads it, and gives a name of its own a value instead."};
    case config_file_unreadable:
        return {"S013", "CONFIG_FILE_UNREADABLE",
                "config.ini is there and could not be read. Every setting still has its built-in "
                "default, so satl runs -- it is running on defaults and not on your settings."};
    case config_file_unwritable:
        return {"S015", "REGISTER_NOT_WRITTEN",
                "a setting could not be saved, so it holds for this run and is gone at the end of it."};
    case line_past_the_end:
        return {"S501", "LINE_PAST_THE_END",
                "a line was read by its number and the file has no line with that number. Lines "
                "count from 1, and the last one is the file's size."};
    case file_has_no_lines:
        return {"S502", "FILE_HAS_NO_LINES",
                "a word about lines was used on a binary file, and a binary file is bytes, not lines."};
    case file_not_open:
        return {"S503", "FILE_NOT_OPEN",
                "a question was asked of a file that is not open -- closed, or never opened because the "
                "open or new failed. Its answer would have been made up, so nothing was answered. Ask "
                "the file's ok first; its error says why it is not open."};
    case file_unwritable:
        return {"S504", "FILE_NOT_SAVED",
                "changes to a file could not be put on the disk. The file on the disk is as it was before "
                "them -- a save is whole or not at all."};
    case not_a_position:
        return {"S410", "NOT_A_POSITION",
                "a position or a line number below zero. Lines count from 1, and nothing counts below zero."};
    case config_value_not_understood:
        return {"S601", "CONFIG_VALUE_NOT_UNDERSTOOD",
                "a row in satellite_config.hpp cannot mean what its name asks."};
    case machine_fact_not_read:
        return {"S701", "MACHINE_FACT_NOT_READ",
                "this machine does not state the fact that was asked for. It is refused rather than "
                "answered as 0 or \"\", because either of those is something a program would use."};
    case setting_out_of_range:
        return {"S610", "SETTING_OUT_OF_RANGE",
                "a number satl was given is past what this machine allows."};
    case machine_conf_unwritable:
        return {"S0734", "MACHINE_CONF_NOT_WRITTEN",
                "~/.satl/machine.conf could not be written, so satl keeps answering from the /proc "
                "ceilings. That answer is still true -- it is the ceiling rather than the measurement."};

    // S09xx -- STRINGS. See the note in Part 4 about this block and files.
    case text_not_found:
        return {"S420", "TEXT_NOT_FOUND",
                "the text looked for is not in this string."};
    case position_past_the_end:
        return {"S411", "POSITION_PAST_THE_END",
                "this position is past the last character of the string."};
    case positions_backwards:
        return {"S412", "POSITIONS_BACKWARDS",
                "the start of this range is after its end."};
    case empty_search_text:
        return {"S421", "EMPTY_SEARCH_TEXT",
                "there is nothing to look for -- an empty search matches everywhere and means nothing."};

    // S10xx -- INPUT AND THE CONSOLE.
    case display_error:
        return {"S820", "DISPLAY_REFUSED",
                "the output refused the line. A satellite program writing into a pipe whose reader "
                "has gone is the usual way this happens."};
    case input_ended:
        return {"S830", "INPUT_ENDED",
                "satellite.console.input() waited for a line and the input has ended -- a file piped "
                "in ran out, or Ctrl-D was pressed. There will never be another line, so this stops "
                "rather than answering an empty one forever."};
    case interrupted:
        return {"S810", "INTERRUPTED",
                "Ctrl-C stopped this between statements. satl exits 130, which is 128 + SIGINT -- "
                "what a shell and 003 both answer."};

    // S13xx -- THE WINDOW (SATELLITE_WINDOW.md WIN-3, 2026-09-20).
    case no_display:
        return {"S730", "NO_DISPLAY",
                "there is no screen to draw on. GTK found no Wayland or X11 session -- which is "
                "true of a build server and of an ssh session without forwarding, and is not a "
                "fault in the program. The same program on a desktop opens its window."};
    case window_is_closed:
        return {"S505", "WINDOW_IS_CLOSED",
                "a window word was used on a window that is not on a screen. Either the program "
                "closed it, or the person running it did -- a window is a thing on a desktop, so "
                "the second one can happen between any two lines."};

    // S11xx -- THREADS AND MEMORY.
    case thread_start_error:
        return {"S720", "THREAD_NOT_STARTED",
                "the machine refused a thread. satl --config says how many this machine allows."};
    // A PROGRAM'S OWN THREADS (2026-09-23, bytecode/thread_calls.hpp), 003's S1401-S1405 in
    // 004's numbering.
    case thread_needs_a_capsule_call:
        return {"S721", "THREAD_NEEDS_A_CAPSULE_CALL",
                "satellite.thread.new runs a capsule of your own on a thread, so what goes inside it is a "
                "CALL -- my_capsule() or my_capsule(x). Its arguments are worked out at new; the capsule "
                "does not begin until start()."};
    case thread_already_started:
        return {"S722", "THREAD_ALREADY_STARTED",
                "this thread has already been started, and a thread runs once. satellite.thread.new is "
                "what makes another one."};
    case thread_not_started:
        return {"S723", "JOIN_BEFORE_START",
                "join() and wait() wait for a thread that is running, and this one has not been started "
                "-- start() comes first."};
    case thread_already_joined:
        return {"S724", "THREAD_ALREADY_JOINED",
                "this thread had already been joined, so this join gave back the same answer the first "
                "one did -- a thread runs once and is waited for once."};
    case thread_cannot_start:
        return {"S725", "THREAD_CANNOT_START",
                "this machine would not make another thread. satellite sets no ceiling of its own on how "
                "many a program may start, so this is the operating system's answer and not satellite's."};
    case thread_stopped:
        return {"S726", "THREAD_STOPPED",
                "this thread was asked to stop, and stopped between two statements -- stop() never ends "
                "a thread in the middle of one."};
    case wait_never_ends:
        return {"S728", "WAIT_NEVER_ENDS",
                "this line would wait forever: the object's lock it needs is held by a thread that is itself "
                "waiting -- through locks and joins -- for this one. Two threads each waiting for the other "
                "never end, so satellite stops this line instead of the program freezing (003's S1407 and "
                "S1408). A line that holds an object's lock -- writing it, or reading it -- should not wait for a "
                "thread that needs the same object's lock."};
    case thread_cannot_share_yet:
        return {"S727", "THREAD_CANNOT_SHARE_YET",
                "a window belongs to the main thread, which draws it, so a thread the program started may "
                "not be handed one, open one or build one yet (THREADS.md T3). Objects and files ARE "
                "shared with a thread; an object's .lock() makes writes to it one at a time."};

    // S12xx -- DIRECTORIES AND THE FILES A PROGRAM OPENS.
    case directory_not_found:
        return {"S520", "DIRECTORY_NOT_FOUND", "nothing is at that path."};
    case not_a_directory:
        return {"S521", "NOT_A_DIRECTORY", "something is at that path and it is not a directory."};
    case directory_unreadable:
        return {"S522", "DIRECTORY_UNREADABLE",
                "it is a directory and its entries could not be read."};
    case path_holds_a_nul:
        return {"S523", "PATH_HOLDS_A_NUL",
                "this path has a NUL in it, so the system would only ever see the part before it."};
    case file_not_found:
        return {"S510", "FILE_NOT_THERE", "no file is at that path."};
    case file_already_there:
        return {"S511", "FILE_ALREADY_THERE",
                "satellite.file.new refuses to clobber a file that is already there. Use "
                "satellite.file.open to work on it, or satellite.file.clear to empty it."};
    case not_a_file:
        return {"S512", "NOT_A_FILE",
                "something is at that path and it is not an ordinary file -- a FIFO, a device or a socket."};
    case file_unreadable:
        return {"S513", "FILE_UNREADABLE", "the file is there and could not be read."};
    case file_not_text:
        return {"S514", "FILE_NOT_TEXT",
                "this is a text file and the bytes given are not text. A lone carriage return is "
                "refused too, so a handle never writes a file it would refuse to open."};

    default:
        break;
    }
    // THE FALLBACK IS NOT A FAILURE. A refusal with no S-code yet is still worth
    // reporting with everything else this file gathers -- the file, the line and
    // the caret -- and S000 says plainly that the number is owed rather than
    // pretending the refusal is nameless.
    return {"S000", "REFUSED", ""};
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

// A WARNING FOR satellite.log, PLACED WHERE THE WALKER IS (M5). S0xx is the author's band
// for "the run carries on" (SATELLITE_ERROR Part 4), so a warning has no machine code and
// no exit status: `code`, `name` and `means` are the whole of it. The line is the statement
// being walked (MachineState::statement_row), and there is no caret -- the walker knows
// which statement, not which part of it.
inline void place_here(CriticalReport &report, const MachineState &state)
{
    if (state.program == nullptr || state.program_files == nullptr || state.statement_row == nullptr)
        return;
    report_at(report, *state.program, *state.program_files, row_index_of(state.program, *state.statement_row),
              state.statement_at);
    report.caret_at = std::string::npos;
    // THE LINE GOES IN BESIDE ITS PLACE: a notice is one line, and "(prog.satl:12)" alone
    // makes a person open the file to learn which of three appends it was.
    std::string line = report.syntax;
    for (char &c : line)
        if (c == '\t')
            c = ' ';                    // one column, as render() shows a tab
    if (!line.empty())
        report.directory += " " + line;
}

// A PLACE ALREADY WARNED FROM IS COUNTED BY ITS ADDRESS, BEFORE ANYTHING IS READ (the
// review, 2026-09-25): place_here quotes the line by re-reading the program's file, and a
// conversion in a loop asked it every turn -- 100,000 turns at line 5010 went from 0.16 s
// to 4.9 s. Now the file is read once a place. The key is the statement's row and position
// and the code; the value is where the tally keeps its count.
inline std::unordered_map<std::string, std::size_t> &warned_places()
{
    static std::unordered_map<std::string, std::size_t> one;
    return one;
}

inline void log_warning_here(const MachineState &state, const char *code, const char *name, const std::string &means)
{
    // THE TALLIES' LOCK (critical_report.hpp), held across the look-up, the first placing
    // and the count, so two threads at one place count once. Recursive, and nothing at all
    // until a program starts a thread.
    const ConsoleHold one_warning;
    const bool placed = state.program != nullptr && state.program_files != nullptr && state.statement_row != nullptr;
    std::string place;
    if (placed) {
        place = std::string(code) + "@" + std::to_string(reinterpret_cast<std::uintptr_t>(state.statement_row)) +
                ":" + std::to_string(state.statement_at);
        const std::unordered_map<std::string, std::size_t>::const_iterator seen = warned_places().find(place);
        if (seen != warned_places().end()) {
            ++logged_tally().counts[seen->second];
            return;
        }
    }
    CriticalReport report;
    report.code = code;
    report.name = name;
    report.description = means;
    place_here(report, state);
    // AT THE PROMPT THERE IS NO FILE, SO NO PLACE, and every warning is its own entry: a
    // typed line cannot loop -- a block has nowhere to live there -- so this is bounded by
    // what a person types, and keeping the first alone lost the rest (the review).
    if (!placed) {
        log_only_warning(report, false);
        return;
    }
    const std::size_t before = logged_tally().keys.size();
    log_only_warning(report);
    if (logged_tally().keys.size() > before)
        warned_places()[place] = before;
}

// S020 -- A NUMBER WHERE TEXT IS EXPECTED WAS TAKEN AS ITS DIGITS (the author, 2026-09-16:
// "just convert the number to the string and run that piece, obviously the programmer
// meant convert to string, but record the warning in satellite.log"). The digits are
// said up to 40; a number of a million digits is a line of a million characters, which
// helps nobody reading a log.
inline void warn_number_taken_as_text(const MachineState &state, const std::string &what, const std::string &digits)
{
    const std::string said = digits.size() <= 40 ? digits : "of " + std::to_string(digits.size()) + " digits";
    log_warning_here(state, "S020", "NUMBER_TAKEN_AS_TEXT",
                     what + " takes text and was given the number " + said +
                         ", so it used its digits as the text -- satellite.variable.number.to_string says so "
                         "out loud, and then there is nothing to warn about");
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
