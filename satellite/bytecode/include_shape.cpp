// satellite/bytecode/include_shape.cpp -- the header holds the five spellings
// and whose decision each one is.

#include "include_shape.hpp"

#include "word_codes.hpp"

namespace satellite004 {
namespace {

using token::Code;

const char *const kExtension = ".satl";

Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
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
        shape.written = text_at(row, i);
    } else if (first == token::name_token) {       // include(file) or include(dir/file)
        shape.kind = IncludeShape::Kind::bare_name;
        shape.written = text_at(row, i);

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
    // reading them is the walker's job, not this one's).
    while (i < row.size() && code_at(row, i) != token::right_parenthesis_token) ++i;
    if (i < row.size()) ++i;
    at = i;

    if (shape.kind == IncludeShape::Kind::main_marker)
        return shape;                              // names no file at all

    // THE EXTENSION IS OPTIONAL, in every spelling.
    std::string path = shape.written;
    if (!ends_with_extension(path))
        path += kExtension;

    // RELATIVE TO THE FILE THAT WROTE THE INCLUDE, not to the working
    // directory. A path from the root is already where it says it is.
    if (!path.empty() && path[0] == '/') {
        shape.resolved = path;
    } else {
        const std::string directory = directory_of(including_file);
        shape.resolved = directory.empty() ? path : directory + "/" + path;
    }

    shape.name = stem_of(path);
    return shape;
}

} // namespace satellite004
