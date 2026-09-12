#pragma once

// The two things every arm that names a file needs: getting the bytes, and
// saying what was wrong with them.
//
// SPLIT OUT OF programs/check_command.hpp AT M26.5, AND THE SPLIT IS A LINK
// EDGE RATHER THAN A TIDY-UP. These helpers sit under `--tokens`, `--unparse`,
// `--satc`, `--resolve`, `--limits` and `--number`; `--check` is the one arm
// that also needs all four passes. While the two lived in one translation unit,
// every test binary that wanted `open_source` got the `--check` arm with it --
// and when that arm started calling build_program(), three suites that link no
// evaluator at all stopped linking. make_support/065-tests.mk says the rule
// they broke: "linking only what is called is how a test starts failing on an
// unrelated edit."
//
// SO THE SEAM IS WHAT EACH ONE DRAGS IN. This file pulls the reporter and
// nothing else. check_command.hpp pulls the whole language.
//
// THE SENTENCE IS S0401 AND IT IS ONE ROW, which is the difference from the
// three literals this replaced when it was written. programs/source_file.hpp
// gets the bytes and says in its own header that "what to say when a file will
// not open is the caller's"; this is that caller, for every arm.

#include "error_reporter/report.hpp"

#include <string>
#include <vector>

namespace satellite {

// Read a source, or say why not and answer false.
bool open_source(const std::string &path, std::string &into);

// Every diagnostic about a file, on stderr, through the one renderer.
//
// STDERR AND NOT STDOUT, ALWAYS, and the reason is a defect --tokens shipped
// with for one day: `satl --unparse f.satl > out.satl` must write the program
// to the file and the complaints to the terminal, or a person redirecting the
// output gets an empty file and no idea why.
void report(const std::string &path, const std::string &source,
            const std::vector<errors::Diagnostic> &problems);

} // namespace satellite
