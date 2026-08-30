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
// THE TWO HELPERS ARE HERE RATHER THAN IN main.cpp because three arms share
// them and main.cpp is where they were three copies. `satl --tokens`,
// `--unparse` and `--satc` each opened a file and each wrote its own sentence
// about not being able to -- which is the same fact in three places, and one of
// the three had already drifted into a different stream.

#include "error_reporter/report.hpp"

#include <string>
#include <vector>

namespace satellite {

// Read a source, or say why not and answer false.
//
// THE SENTENCE IS S0401 AND IT IS ONE ROW, which is the difference from the
// three literals this replaces. programs/source_file.hpp gets the bytes and
// says in its own header that "what to say when a file will not open is the
// caller's"; this is that caller, for every arm.
bool open_source(const std::string &path, std::string &into);

// Every diagnostic about a file, on stderr, through the one renderer.
//
// STDERR AND NOT STDOUT, ALWAYS, and the reason is a defect --tokens shipped
// with for one day: `satl --unparse f.satl > out.satl` must write the program
// to the file and the complaints to the terminal, or a person redirecting the
// output gets an empty file and no idea why.
void report(const std::string &path, const std::string &source,
            const std::vector<errors::Diagnostic> &problems);

// Lex it, parse it, say everything wrong with it, and print nothing else.
int check_command(const std::string &path);

} // namespace satellite
