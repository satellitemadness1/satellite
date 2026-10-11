#pragma once
// satellite/bytecode/include_shape.hpp -- what a satellite.include(...) names,
// read straight out of the bytecode.
//
// FIVE SPELLINGS, AND THE TOKENS TELL THEM APART. The author wrote all five
// into test_programs/hello_world.satl (2026-09-16), and each has a different
// first code after the parenthesis, so choosing is a switch and not a parse:
//
//     satellite.include(satellite)            word code 4097   THE MAIN MARKER
//     satellite.include(test_file)            name_token       a bare name
//     satellite.include("a_file.satl")        string_token     a quoted path
//     satellite.include(test_dir/test_file)   name_token, path_separator_token
//     satellite.include("/dir/file")          string_token     from the root
//
// THE UNQUOTED PATH IS NEW IN 004. 003's shape.hpp settled the other four --
// "the same two spellings as a path, IN QUOTES" -- so `test_dir/test_file`
// without quotes is the author's own addition, 2026-09-16. It works because a
// slash with nothing touching whitespace is path_separator_token and not
// division; write `test_dir / test_file` with spaces and it IS division. The
// quoted form has no such sensitivity, which is worth knowing when choosing
// between them.
//
// THE RULES ARE 003's, PORTED AS DECISIONS AND NOT AS CODE (revision 07,
// be50b10). 003 resolves from its AST and a SatString; this resolves from
// codes in a row, so nothing transferred but the semantics -- which are the
// expensive part, and were settled in the author's own words:
//
//   - A path that does not start with `/` is relative to the directory of THE
//     FILE THAT WRITES THE INCLUDE, exactly as a bare name is. So a spaceship
//     in parts/ that includes "../shared/log" reaches shared/log.satl beside
//     parts/, whoever included it.
//   - The extension is optional either way: `ship` and `ship.satl` are one file.
//   - The spaceship is NAMED BY ITS FILE STEM. Every spelling above names the
//     spaceship `ship`, reached as ship.setup().
//
// satellite.include(satellite) IS NOT AN IMPORT (the author, 2026-09-16): "it's
// only for the main file that you run... but you can include it to make
// something else a main file, and it is optional". Nothing is pulled in; the
// file is declaring that it has a main you can launch. It is metadata, read
// once, never a step the walker runs -- which is why it is its own Kind here
// and carries no path at all.

#include "bytecode_registry.hpp"

#include <string>

namespace satellite004 {

struct IncludeShape {
    enum class Kind {
        none,          // `at` was not a satellite.include, or the form made no sense
        main_marker,   // include(satellite): this file has a main you can run
        bare_name,     // include(ship)
        bare_path,     // include(dir/ship)        -- 004's own, unquoted
        quoted_path,   // include("dir/ship.satl") -- 003's spelling
    };

    Kind kind = Kind::none;
    std::string written;   // exactly what stood between the parentheses
    std::string resolved;  // the .satl this names, relative to the including file
    std::string name;      // the spaceship's name: the file stem, no directory
};

// Reads the include whose word code is at `at`, and moves `at` past its closing
// parenthesis so a walker can carry straight on. `including_file` is the file
// this row came from -- BytecodeFilenames holds it -- because every relative
// path is relative to that file's own directory and not to the working one.
//
// Answers Kind::none and does not move `at` when `at` is not satellite.include.
IncludeShape include_at(const std::vector<std::bitset<16>> &row,
                        std::size_t &at,
                        const std::string &including_file);

// The directory part of a path, "" when there is none. Exposed because the
// resolution rule is worth being able to check on its own.
std::string directory_of(const std::string &path);

// The file stem: no directory, no .satl. What the spaceship is called.
std::string stem_of(const std::string &path);

// WHETHER THIS FILE MAY BE RUN, and the machine code saying why not.
//
// (the author, 2026-09-16) "Let's refuse to run files that do not have
// satellite.include(satellite) and satellite.main because THERE ARE NO GLOBALS
// IN SATELLITE, we begin exe inside of main, and end exe inside of main... the
// only globals are the includes, other files."
//
// So the two rules are one rule. With no globals there is nowhere for a
// statement outside main to put its result and no moment for it to run in, so a
// file with no main is not a program at all -- it is a spaceship, which is
// includable and not runnable. That is also why include(satellite) being
// "optional" and this refusal are not in conflict: a file need not carry one,
// and a file you POINT SATL AT must.
//
// Answers success, or satl_file_missing_satellite_include_satellite (10),
// satl_file_missing_satellite_main (11), or
// satl_file_missing_satellite_return_satellite (12) -- the same three codes the
// prototype's check_satl answers, so the two paths cannot disagree about what a
// runnable file is.
signed long long int file_can_run(const std::vector<std::bitset<16>> &row,
                                  const std::string &filename,
                                  MachineState &state);

} // namespace satellite004
