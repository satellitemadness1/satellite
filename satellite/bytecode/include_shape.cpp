// satellite/bytecode/include_shape.cpp -- the header holds the five spellings
// and whose decision each one is.

#include "include_shape.hpp"

#include "../machine/s_codes.hpp"

#include "word_codes.hpp"

#include <cstdlib>

namespace satellite004 {
namespace {

using token::Code;

const char *const kExtension = ".satl";

Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
}

// The Linux user's home directory, or "" when the machine will not say. The
// author's arguments.home is meant to be this; until the config row exists this
// is where it comes from.
std::string linux_home_directory()
{
    const char *home = std::getenv("HOME");
    return home == nullptr ? std::string() : std::string(home);
}

bool ends_with_extension(const std::string &path)
{
    const std::string suffix = kExtension;
    return path.size() >= suffix.size() &&
           path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

std::string directory_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

std::string stem_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    if (ends_with_extension(name))
        name.erase(name.size() - std::string(kExtension).size());
    return name;
}

IncludeShape include_at(const std::vector<std::bitset<16>> &row,
                        std::size_t &at,
                        const std::string &including_file)
{
    IncludeShape shape;
    if (code_at(row, at) != word::code_of(1, 1))   // satellite.include
        return shape;

    std::size_t i = at + 1;
    if (code_at(row, i) != token::left_parenthesis_token)
        return shape;
    ++i;

    // ONE SWITCH ON THE FIRST CODE, which is the whole point of the tokens
    // telling the spellings apart.
    const Code first = code_at(row, i);

    if (first == word::code_of(1)) {               // include(satellite)
        shape.kind = IncludeShape::Kind::main_marker;
        shape.written = "satellite";
        ++i;
    } else if (first == token::string_token) {     // include("dir/file")
        shape.kind = IncludeShape::Kind::quoted_path;
        shape.written = string_at(row, i);
    } else if (first == token::path_separator_token || first == token::bit_not_token ||
               first == token::name_token) {
        // A BARE PATH MAY START WITH ITS MARK. include(/test) and include(~/test)
        // begin with a slash or a tilde, not a name, and the switch used to fall
        // straight through them into "a form with no meaning yet" -- found by the
        // checks, 2026-09-16.
        shape.kind = IncludeShape::Kind::bare_name;
        if (first == token::path_separator_token) { shape.written = "/"; ++i; shape.kind = IncludeShape::Kind::bare_path; }
        else if (first == token::bit_not_token)   { shape.written = "~"; ++i; shape.kind = IncludeShape::Kind::bare_path; }
        if (code_at(row, i) == token::name_token) shape.written += text_at(row, i);

        // A bare PATH is a name, then path_separator_token, then more. The
        // separator is only ever a slash with nothing touching whitespace --
        // `dir / file` with spaces is division and never reaches here.
        while (code_at(row, i) == token::path_separator_token) {
            ++i;
            shape.kind = IncludeShape::Kind::bare_path;
            shape.written += '/';
            if (code_at(row, i) == token::name_token) shape.written += text_at(row, i);
            else if (code_at(row, i) == token::method_token) { shape.written += '.'; ++i; }
            else break;
        }

        // `ship.satl` unquoted arrives as name, method_token, name -- the
        // extension written out, which 003 allows in every spelling.
        while (code_at(row, i) == token::method_token) {
            ++i;
            shape.written += '.';
            if (code_at(row, i) == token::name_token) shape.written += text_at(row, i);
            else break;
        }
    } else {
        return shape;                              // a form with no meaning yet
    }

    // Past the closing parenthesis, whatever else stood inside it (arguments to
    // the spaceship's launch capsules are 003's `include(ship(1, "two"))`, and
    // reading them is the walker's job, not this one's). A string among them is
    // skipped whole: its count or a code inside it can be 0x0203 itself -- a
    // literal of 515 codes is one -- and stopping there handed file_can_run the
    // middle of a payload to walk (found by the count review, 2026-09-17).
    while (i < row.size() && code_at(row, i) != token::right_parenthesis_token) {
        if (token::carries_a_count(code_at(row, i))) { text_at(row, i); continue; }
        ++i;
    }
    if (i < row.size()) ++i;
    at = i;

    if (shape.kind == IncludeShape::Kind::main_marker)
        return shape;                              // names no file at all

    // THE EXTENSION IS OPTIONAL, in every spelling.
    std::string path = shape.written;
    if (!ends_with_extension(path))
        path += kExtension;

    // FOUR SPELLINGS AND NO PROBING (the author, 2026-09-16). Every one of these
    // is a RULE: none of them looks at the disk to decide what it means, so a
    // program means the same thing whatever directory it is run from. The
    // rejected alternative was "try the cwd, then the root", and its cost is
    // exactly that -- include(/etc/config) would find a local etc/config on one
    // machine and the real /etc/config on another, silently.
    //
    //   include(test)       ./test.satl      beside the including file
    //   include(/test)      ./test.satl      A LEADING SLASH IS RELATIVE: the
    //                                        author, "satellite.include(/test)
    //                                        would actually just be ./test"
    //   include(~/test)     $HOME/test.satl  the user's home -- and ~ is the
    //                                        convention every Unix user knows,
    //                                        which is why it beats comparing a
    //                                        path against arguments.home after
    //                                        the fact
    //   include(/root/x)    /x.satl          the machine's root, ASKED FOR. The
    //                                        author chose this spelling knowing
    //                                        /root is itself a real directory on
    //                                        Linux; // would have been unambiguous
    //                                        but the lexer reads it as a comment.
    const std::string kRootMark = "/root/";
    const std::string home = linux_home_directory();

    if (path.size() > kRootMark.size() && path.compare(0, kRootMark.size(), kRootMark) == 0) {
        shape.resolved = path.substr(kRootMark.size() - 1);      // keep the one slash
    } else if (path.size() > 1 && path[0] == '~' && path[1] == '/') {
        shape.resolved = home.empty() ? path.substr(2) : home + path.substr(1);
    } else if (!home.empty() && path.compare(0, home.size(), home) == 0) {
        shape.resolved = path;                                   // already under home
    } else {
        std::string relative = path;
        while (!relative.empty() && relative[0] == '/')
            relative.erase(0, 1);
        const std::string directory = directory_of(including_file);
        shape.resolved = directory.empty() ? relative : directory + "/" + relative;
    }

    shape.name = stem_of(path);
    return shape;
}

namespace {

// One report for a file whose SHAPE is wrong. The sentence the check already
// built stays as the description; the S-code's own text says what to type.
//
// THE DESCRIPTION SAYS "this file" AND NOT THE PATH, because the path is already
// the `directory:` row directly under it. Naming it twice made a long path wrap
// across the report twice over and pushed the sentence that matters off the top.
signed long long int raise_file_shape(const std::string &filename, const std::string &said,
                                      signed long long int machine_code)
{
    const SCode named = s_code_for(machine_code);
    CriticalReport report;
    report.code = named.code;
    report.name = named.name;
    report.description = said;
    report.directory = filename;
    if (named.means[0] != '\0')
        report.notes.push_back(named.means);
    report.notes.push_back("machine code " + std::to_string(machine_code) + " " +
                           machine_code_name(machine_code) + " -- satl exits with this.");
    return raise(report, machine_code);
}

} // namespace

signed long long int file_can_run(const std::vector<std::bitset<16>> &row,
                                  const std::string &filename,
                                  MachineState &state)
{
    bool marker = false, main = false, returns = false;

    for (std::size_t i = 0; i < row.size(); ) {
        const Code code = code_at(row, i);

        // A payload's codes are skipped, never classified -- so a STRING that
        // says "satellite.main" cannot make a file look runnable.
        if (token::carries_a_count(code)) { std::size_t k = i; text_at(row, k); i = k; continue; }

        if (code == word::code_of(1, 1)) {          // satellite.include
            std::size_t k = i;
            const IncludeShape shape = include_at(row, k, filename);
            if (shape.kind == IncludeShape::Kind::main_marker) marker = true;
            i = (k > i) ? k : i + 1;
            continue;
        }
        if (code == word::code_of(1, 3)) main = true;      // satellite.main
        if (code == word::code_of(1, 15)) returns = true;  // satellite.return
        ++i;
    }

    // THE THREE FILE-SHAPE REFUSALS, AS REPORTS. SATELLITE_ERROR S101-S103.
    //
    // NO CARET, AND THAT IS NOT A GAP. These are about a whole file -- a line
    // that is MISSING has no position to point at -- so raise() is the right
    // call and raise_at() is not. What a report gives them that the old one line
    // could not is room to print the exact line to type.
    //
    // THE FIRST ONE IS THE COMMONEST MISTAKE IN THE LANGUAGE: a file with no
    // satellite.include(satellite) is a spaceship, and the old message said so
    // without ever saying what to do about it.
    if (!marker)
        return raise_file_shape(filename,
                                "satl.file(check): this file has no satellite.include(satellite), so it "
                                "is a spaceship and not a program",
                                satl_file_missing_satellite_include_satellite);
    if (!main)
        return raise_file_shape(filename,
                                "satl.file(check): this file has no satellite.main -- there are no "
                                "globals, so there is nowhere else to begin",
                                satl_file_missing_satellite_main);
    if (!returns)
        return raise_file_shape(filename,
                                "satl.file(check): this file has no satellite.return -- execution ends "
                                "inside main",
                                satl_file_missing_satellite_return_satellite);

    state.set("satl.file(runnable): " + filename, success);
    return success;
}

} // namespace satellite004
