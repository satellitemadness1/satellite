#pragma once
// full list of satellite machine codes (do not use a machine code without adding it to this list...)
//
// Every satellite-004 function answers one of these. 0 is success; anything
// else says what went wrong, or which stage of loading was reached.
//
// The author's list, 2026-09-14. 13 was added the same day, for a line the
// runner has no scenario for yet. 14-19 were added the same night for the
// satellite_string methods, each matching a refusal 003 06 already makes
// (satellite_scalars/string_methods.cpp). 20 and 21 were added 2026-09-15, when
// the interpreter first read satellite_config.hpp and started its threads.
//
// 24-27 were added 2026-09-16, when the arithmetic tokens were wired to
// satellite_number's fast paths. 24 is the one to read twice: `2 ^ -1` is a real
// answer that a WHOLE number cannot hold, and saying so is not the same as
// calling it an error. It is the seam satellite.variable.float (003 DESIGN §8.6,
// a bool and two satellite_numbers) arrives at, and the seam a fraction would.

// 255 IS NEVER GIVEN TO A CODE (PLAN D0.5.2). An exit status holds 0 to 255, so a
// code below 0 or above 254 -- 256, -1, 4294967298 -- exits 255 with the whole
// code on stderr (exit_status.hpp). A code cut to 8 bits would let 256 exit 0.

namespace satellite004 {

enum MachineCode : signed long long int {
    success = 0,
    error = 1,
    display_error = 2,
    int_error = 3,
    string_error = 4,
    vector_loading_error = 5,
    number_vector_defined = 6,          // the number index is built
    satellite_loading_successful = 7,   // end of all loading
    missing_satl_file = 8,              // cannot run, obviously
    successfully_loaded_satl_file = 9,  // can run the file
    satl_file_missing_satellite_include_satellite = 10,
    satl_file_missing_satellite_main = 11,
    satl_file_missing_satellite_return_satellite = 12,
    satl_line_not_understood = 13,      // no scenario for this line yet
    not_built_yet = 14,                 // the word is numbered, but what it needs does not exist yet
    text_not_found = 15,                // find: the text is not in the string (003's S0716)
    position_past_the_end = 16,         // substring / at: past the last character (003's S07xx past-the-end)
    positions_backwards = 17,           // substring: start is after end (003's backwards refusal)
    empty_search_text = 18,             // replace: nothing to replace (003 refuses an empty needle)
    not_a_position = 19,                // a position that is negative
    config_value_not_understood = 20,   // satellite_config.hpp: a row that cannot mean what its name asks
    thread_start_error = 21,            // the machine refused a start-up thread (the rest stay warm)
    division_by_zero = 22,              // satellite_number: a divisor of 0
    command_line_not_understood = 23,   // satl was given words it does not take (PLAN M0.5)
    answer_is_not_whole = 24,           // the answer exists but is not a whole number: 2 ^ -1 is 1/2
    name_not_declared = 25,             // a name used before any satellite.variable line declared it
    name_declared_twice = 26,           // a second satellite.variable line for a name already in this capsule
    types_do_not_meet = 27,             // an operator given two kinds it has no scenario for: "a" - "b"
    directory_not_found = 28,           // change / list: nothing is at that path (PLAN M0.6)
    not_a_directory = 29,               // something is there, and it is not a directory
    directory_unreadable = 30,          // it IS a directory and its entries cannot be read; the reason is said
    path_holds_a_nul = 31,              // a path with a NUL in it: c_str() would act on the part before it

    // 32-35 WERE ADDED 2026-09-18, for the first setting a program can WRITE.
    // `arguments.access = true` is not a call and not a variable: it is a word
    // that remembers, so it needs the two ways writing fails (the file, and the
    // kind) and the two ways the word is wrong.
    config_file_unwritable = 32,        // $HOME/.satl/config.ini could not be written; the reason is said
    config_file_unreadable = 33,        // it exists and could not be read
    setting_is_not_a_flag = 34,         // a true/false setting was given something that is not one
    word_takes_no_assignment = 35,      // `<word> = ...` for a word that is not a setting

    // 36-38 WERE ADDED 2026-09-18 for `satl --config`, which is the first thing
    // satl does that ASKS THE MACHINE A QUESTION and can be told nothing.
    machine_fact_not_read = 36,         // /proc or sysconf states nothing where a fact was expected
    setting_out_of_range = 37,          // a number satl was given is past what the machine allows
    machine_conf_unwritable = 38,       // $HOME/.satl/machine.conf could not be written

    // 130 AND NOT 32, ON PURPOSE (PLAN M0.6): 128 + SIGINT is what a shell and 003
    // both answer for Ctrl-C, and exit_status_of passes a code under 255 through as
    // itself -- so a session stopped by Ctrl-C exits the status everything already reads.
    interrupted = 130,                  // Ctrl-C stopped a line between entries
};

inline const char *machine_code_name(signed long long int code)
{
    switch (code) {
    case success: return "success";
    case error: return "error";
    case display_error: return "display_error";
    case int_error: return "int_error";
    case string_error: return "string_error";
    case vector_loading_error: return "vector_loading_error";
    case number_vector_defined: return "number_vector_defined";
    case satellite_loading_successful: return "satellite_loading_successful";
    case missing_satl_file: return "missing_satl_file";
    case successfully_loaded_satl_file: return "successfully_loaded_satl_file";
    case satl_file_missing_satellite_include_satellite: return "satl_file_missing_satellite_include_satellite";
    case satl_file_missing_satellite_main: return "satl_file_missing_satellite_main";
    case satl_file_missing_satellite_return_satellite: return "satl_file_missing_satellite_return_satellite";
    case satl_line_not_understood: return "satl_line_not_understood";
    case not_built_yet: return "not_built_yet";
    case text_not_found: return "text_not_found";
    case position_past_the_end: return "position_past_the_end";
    case positions_backwards: return "positions_backwards";
    case empty_search_text: return "empty_search_text";
    case not_a_position: return "not_a_position";
    case config_value_not_understood: return "config_value_not_understood";
    case thread_start_error: return "thread_start_error";
    case division_by_zero: return "division_by_zero";
    case command_line_not_understood: return "command_line_not_understood";
    case answer_is_not_whole: return "answer_is_not_whole";
    case name_not_declared: return "name_not_declared";
    case name_declared_twice: return "name_declared_twice";
    case types_do_not_meet: return "types_do_not_meet";
    case directory_not_found: return "directory_not_found";
    case not_a_directory: return "not_a_directory";
    case directory_unreadable: return "directory_unreadable";
    case path_holds_a_nul: return "path_holds_a_nul";
    case config_file_unwritable: return "config_file_unwritable";
    case config_file_unreadable: return "config_file_unreadable";
    case setting_is_not_a_flag: return "setting_is_not_a_flag";
    case word_takes_no_assignment: return "word_takes_no_assignment";
    case machine_fact_not_read: return "machine_fact_not_read";
    case setting_out_of_range: return "setting_out_of_range";
    case machine_conf_unwritable: return "machine_conf_unwritable";
    case interrupted: return "interrupted";
    }
    return "not_on_the_list";
}

// A code that means the program cannot go on. The loading stages (6, 7, 9)
// are reports, not failures.
inline bool stops_the_program(signed long long int code)
{
    return code != success && code != number_vector_defined &&
           code != satellite_loading_successful && code != successfully_loaded_satl_file;
}

} // namespace satellite004
