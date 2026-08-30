// `satl --check <file>` and the two helpers it shares. See
// programs/check_command.hpp for why the reporter gets an arm of its own.

#include "programs/check_command.hpp"

#include "error_reporter/report.hpp"
#include "parser/parser.hpp"
#include "programs/opening.hpp"
#include "programs/source_file.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace satellite {

bool open_source(const std::string &path, std::string &into)
{
    if (read_file(path, into))
        return true;
    // A DIAGNOSTIC WITH NO SPAN, WHICH IS A SHAPE THE RENDERER HAS ON PURPOSE.
    // There is no place inside a file that could not be opened, and there is a
    // path -- so the header line names the file and stops, rather than
    // inventing a line 1 nobody can look at.
    const errors::Diagnostic problem =
        errors::make<errors::Code::FILE_UNREADABLE>(errors::kNowhere);
    fputs(errors::render(problem, errors::Source{path, {}}).c_str(), stderr);
    return false;
}

void report(const std::string &path, const std::string &source,
            const std::vector<errors::Diagnostic> &problems)
{
    if (problems.empty())
        return;
    fputs(errors::render(problems, errors::Source{path, source}).c_str(), stderr);
}

int check_command(const std::string &path)
{
    std::string source;
    if (!open_source(path, source))
        return EXIT_USAGE;

    // A RUN'S NAMES END WITH THE RUN, which is why this is a local and not a
    // global: words_runtime.hpp makes the point that M22 runs many programs
    // in one process and each needs its own numbering.
    words::Words words;
    const Parse parsed = parse(source, words);
    report(path, source, parsed.errors);

    // NOTHING ON STDOUT, EITHER WAY, and that is the whole of what this arm is
    // for. A command whose silence means yes is one a script can use without
    // parsing anything, and a person who wants to SEE the file understood has
    // `--tokens`, `--unparse` and `--satc` for that.
    //
    // THE STATUS IS THE ANSWER, and it is the code three arms have been waiting
    // for since M3 -- programs/opening.hpp carries the argument for why a
    // malformed program is 1 and not EXIT_USAGE's 2.
    return parsed.ok() ? EXIT_FINE : EXIT_MALFORMED;
}

} // namespace satellite
