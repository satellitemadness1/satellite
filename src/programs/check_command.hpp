#pragma once

// `satl --check <file>`, and the two things every arm that names a file needs.
//
// THE REPORTER'S CONSUMER, IN THE MILESTONE THAT WROTE IT. PLAN M2 made that a
// rule -- the first satellite shipped three commits where its word registry had
// no reader and four defects accumulated behind a guarantee nothing was
// checking -- and `--words`, `--tokens`, `--unparse` and `--satc` are the four
// arms it has produced since. This is M5's, and it is the first one whose
// answer is the DIAGNOSTICS: the other four print a table, a stream, a program
// or a cache file and mention what went wrong on the way, while this one prints
// nothing at all when a file is fine.
//
// WHICH IS WHY IT IS NOT `--unparse` WITH THE OUTPUT THROWN AWAY. A tool
// wants a command whose stdout is empty on success and whose exit status is the
// answer -- an editor, a Makefile, a pre-commit hook. `satl --unparse f.satl >
// /dev/null` is that command spelled as an accident, and it also pays for a
// printer it does not want.
//
// THE TWO HELPERS MOVED TO programs/source_report.hpp AT M26.5, and this
// include keeps every arm that used to get them from here working. They left
// because THIS header now pulls the whole language: --check runs all four
// passes, so a test binary that wanted open_source() started needing an
// evaluator. source_report.hpp pulls the reporter and stops.
#include "programs/source_report.hpp"

#include "error_reporter/report.hpp"

#include <string>
#include <vector>

namespace satellite {

// Read it, parse it, resolve it, compile it, say everything wrong with it, and
// print nothing else.
//
// ALL FOUR PASSES SINCE M26.5. It ran one until then, which made the arm whose
// job is to answer before a run the only one that could not see what the
// compiler had already decided -- including a line that parses and resolves and
// is refused the moment it is reached.
int check_command(const std::string &path);

} // namespace satellite
