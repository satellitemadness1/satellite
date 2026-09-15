// The two helpers every arm that names a file shares. See
// programs/source_report.hpp for why they are not in check_command.cpp.

#include "programs/source_report.hpp"

#include "error_reporter/report.hpp"
#include "programs/source_file.hpp"

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

} // namespace satellite
