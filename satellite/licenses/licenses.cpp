#include "licenses.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace satellite004 {

namespace {

// The first line worth showing beside a name -- "MIT License (Expat)", "GNU LESSER
// GENERAL PUBLIC LICENSE". Some texts open with something plainer ("Copyright
// notice:" is zlib's), which is not wrong, only less useful; the name carries it.
std::string first_meaningful_line(const std::string &text)
{
    std::string::size_type at = 0;
    while (at < text.size()) {
        const std::string::size_type end = text.find('\n', at);
        std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t'))
            line.erase(line.begin());
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r'))
            line.pop_back();
        if (!line.empty()) {
            if (line.size() > 46)
                line = line.substr(0, 45) + "-";
            return line;
        }
        if (end == std::string::npos)
            break;
        at = end + 1;
    }
    return "";
}

std::string padded(const std::string &word, std::string::size_type width)
{
    return word.size() >= width ? word : word + std::string(width - word.size(), ' ');
}

// ONE LICENCE PER LINE, numbered from 1, as the author asked. The number is the
// argument to --license and the position in THIRD-PARTY-NOTICES.txt, which is why
// license_data.cpp keeps satellite first rather than sorting purely alphabetically:
// a number has to mean the same thing everywhere or it means nothing.
std::string menu_lines()
{
    const std::vector<licence_row> &rows = licence_rows();
    std::string out;
    for (std::vector<licence_row>::size_type i = 0; i < rows.size(); ++i) {
        std::string number = std::to_string(i + 1);
        out += "  " + padded(number, 3) + " " + padded(rows[i].name, 18) + " " +
               first_meaningful_line(rows[i].text) + "\n";
    }
    return out;
}

bool all_digits(const std::string &word)
{
    return !word.empty() && word.find_first_not_of("0123456789") == std::string::npos;
}

// A name, case-insensitively, so --license GTK and --license gtk are the same ask.
const licence_row *by_name(const std::string &want)
{
    std::string lowered;
    for (char c : want)
        lowered += static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    for (const licence_row &row : licence_rows())
        if (row.name == lowered)
            return &row;
    return nullptr;
}

void print_one(const licence_row &row)
{
    std::cout << "\n" << row.name << "\n" << std::string(row.name.size(), '=') << "\n\n" << row.text;
    if (!row.text.empty() && row.text.back() != '\n')
        std::cout << "\n";
}

// A READER WHO LEFT IS NOT A FAILURE, AND A FULL DISK IS.
//
// `satl --license all | head -50` is an ordinary thing to type, and head closing
// the pipe after fifty lines is what head is FOR. The write fails with EPIPE, the
// stream goes bad, and reporting that as an error would be scolding somebody for
// using a pipe correctly. --version and --help never hit it only because they are
// small enough to fit the 64 KB pipe buffer; 275 KB of licences is not.
//
// EPIPE IS THE ONLY ONE FORGIVEN. check.sh has a row that writes to /dev/full and
// must answer 2 -- that is ENOSPC, a real failure, and it still reports. Treating
// every bad stream as success to quieten the pipe would silently un-do the one
// test that catches a lost write (DESIGN 3.4, and the reason -static-libstdc++
// was abandoned: two std::cout, and a failed write reported as a run that
// succeeded).
signed long long int output_state()
{
    std::cout.flush();
    if (std::cout)
        return success;
    if (errno == EPIPE)
        return success;
    return report_error("satl(licence): the output refused the lines", display_error);
}

signed long long int print_all()
{
    const std::vector<licence_row> &rows = licence_rows();
    std::cout << "Every licence in this binary, " << rows.size() << " of them.\n";
    for (const licence_row &row : rows)
        print_one(row);
    return output_state();
}

// THE MENU ONLY PROMPTS WHEN SOMEBODY IS THERE TO ANSWER. `satl --license | less`
// and `satl --license > notices.txt` must not sit waiting on a question nobody can
// see, so a pipe or a file gets the list and nothing else. isatty is the whole test
// and it is the same reasoning the prompt makes about raw mode.
bool someone_is_watching()
{
    return isatty(STDIN_FILENO) == 1 && isatty(STDOUT_FILENO) == 1;
}

signed long long int ask_and_answer()
{
    std::cout << "\nType a number for that licence, or Enter to stop: " << std::flush;
    std::string answer;
    if (!std::getline(std::cin, answer))
        return success;   // end of input is not a failure; it is somebody leaving

    while (!answer.empty() && (answer.back() == ' ' || answer.back() == '\r'))
        answer.pop_back();
    while (!answer.empty() && answer.front() == ' ')
        answer.erase(answer.begin());
    if (answer.empty())
        return success;

    const std::vector<licence_row> &rows = licence_rows();
    if (all_digits(answer)) {
        const unsigned long long int pick = std::strtoull(answer.c_str(), nullptr, 10);
        if (pick >= 1 && pick <= rows.size()) {
            print_one(rows[static_cast<std::vector<licence_row>::size_type>(pick - 1)]);
            std::cout.flush();
            return success;
        }
        std::cout << "\nThere is no licence " << answer << ". They are numbered 1 to " << rows.size()
                  << ".\n";
        return success;
    }
    if (const licence_row *row = by_name(answer)) {
        print_one(*row);
        std::cout.flush();
        return success;
    }
    std::cout << "\nNothing here is called \"" << answer << "\". The names are in the list above.\n";
    return success;
}

} // namespace

signed long long int run_licence(const std::string &which)
{
    const std::vector<licence_row> &rows = licence_rows();
    if (rows.empty())
        return report_error("satl(licence): this binary carries no licence text, which is a build "
                            "fault -- satellite/licenses/make_license_data.py did not run",
                            display_error);

    if (which == "all")
        return print_all();

    if (!which.empty()) {
        if (all_digits(which)) {
            const unsigned long long int pick = std::strtoull(which.c_str(), nullptr, 10);
            if (pick < 1 || pick > rows.size())
                return report_error("satl(licence): there is no licence " + which + " -- they are "
                                    "numbered 1 to " + std::to_string(rows.size()) +
                                    ", and satl --license lists them",
                                    command_line_not_understood);
            print_one(rows[static_cast<std::vector<licence_row>::size_type>(pick - 1)]);
        } else {
            const licence_row *row = by_name(which);
            if (row == nullptr)
                return report_error("satl(licence): nothing here is called \"" + which +
                                        "\" -- satl --license lists every name",
                                    command_line_not_understood);
            print_one(*row);
        }
        return output_state();
    }

    // `satl --license` with nothing after it: SATELLITE'S OWN LICENCE FIRST. That is
    // the one somebody asking about satl's licence means, and making them pick it out
    // of a list of twenty-six would be answering a question with a question.
    print_one(rows.front());
    std::cout << "\n"
              << "satl also carries " << rows.size() - 1 << " other projects, compiled in, each\n"
              << "under its own licence. All of them are in this binary:\n\n"
              << menu_lines();
    std::cout << "\n"
              << "    satl --license " << (rows.size() > 1 ? rows[1].name : std::string("name")) << "\n"
              << "    satl --license 2\n"
              << "    satl --license all\n";
    if (const signed long long int state = output_state(); state != success)
        return state;

    if (!someone_is_watching())
        return success;
    return ask_and_answer();
}

} // namespace satellite004
