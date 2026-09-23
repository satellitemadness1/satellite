// satellite/bytecode/library_values.cpp -- the header says what satellite.library is and whose
// each rule is. Four pieces, in the order a program meets them: the scan's line, the names
// after the word, the check, and the expression's read.

#include "library_values.hpp"

#include "capsule_scan.hpp"
#include "container_calls.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/source_position.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace satellite004 {
namespace {

using token::Code;

const Code kLibrary = word::code_of(1, 14);         // satellite.library
const Code kFalse = word::code_of(1, 17, 1);        // satellite.bool.false
const Code kTrue = word::code_of(1, 17, 2);         // satellite.bool.true

// ONE NAME AFTER A `.`, AT `at`: a name, or a method's code read back as its first
// spelling -- `satellite.library.size` lexes `size` as a method, as dotted_names_at
// already reads a capsule named like one. Empty, `at` unmoved, for anything else.
std::string segment_at(const std::vector<std::bitset<16>> &row, std::size_t &at)
{
    const Code code = code_at(row, at);
    if (code == token::name_token)
        return text_at(row, at);
    if (token::is_method_code(code)) {
        ++at;
        return token::method_name_of(code);
    }
    return std::string();
}

std::string written_as(const std::vector<std::string> &names)
{
    std::string written = "satellite.library";
    for (const std::string &name : names) written += "." + name;
    return written;
}

// THE FILE A FIRST NAME MEANS, as the stem it was included by, or "" when it names none.
// A stem spelled like a method (`color`, `power`) lexes as that method's code, so it is
// matched by the code -- the review, 2026-09-23: matched by the first spelling, a file
// named color.satl could never have its values read.
std::string included_stem_at(const CapsuleTable &table, std::size_t r, const std::vector<std::bitset<16>> &row,
                             std::size_t at)
{
    if (r >= table.included.size())
        return std::string();
    const Code code = code_at(row, at);
    if (code == token::name_token) {
        const std::string name = text_at(row, at);
        return table.included[r].count(name) != 0 ? name : std::string();
    }
    if (token::is_method_code(code))
        for (const auto &[stem, files] : table.included[r])
            if (token::method_code_of(stem) == code)
                return stem;
    return std::string();
}

// THE WORD A LITERAL'S TYPE IS DECLARED WITH -- only the kinds a_written_value lets in.
Code type_word_of(const Value &value)
{
    switch (value.kind()) {
    case satelliteObject::number: return word::code_of(1, 6, 4);
    case satelliteObject::string: return word::code_of(1, 6, 1);
    case satelliteObject::binary: return word::code_of(1, 6, 5);
    case satelliteObject::percentage: return word::code_of(1, 6, 16);
    case satelliteObject::boolean: return word::code_of(1, 6, 6);
    case satelliteObject::floating: return word::code_of(1, 6, 10);
    case satelliteObject::hexadecimal: return word::code_of(1, 6, 11);
    case satelliteObject::fraction: return word::code_of(1, 6, 20);
    default: return 0;
    }
}

bool ends_its_line(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const Code code = code_at(row, at);
    return at >= row.size() || code == token::line_end_token || code == token::comment_token ||
           code == token::end_of_file_token;
}

// ONE LITERAL AND NOTHING AFTER IT BUT THE LINE'S END, from `at` -- what one_operand reads
// without running anything: a number (a float and a fraction are numbers with a point or a
// touching slash in them), text, a binary, a hex, a percentage, or satellite.bool.true or
// .false, a minus sign allowed in front. What each literal IS, is still one_operand's to
// say when the check reads it; this only says that nothing in it could run.
bool a_written_value(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    if (code_at(row, at) == token::tight_minus_token || code_at(row, at) == token::minus_token)
        ++at;
    const Code code = code_at(row, at);
    if (code == kTrue || code == kFalse) {
        ++at;
    } else if (code == token::number_token) {
        skip_payload(row, at);
        if (code_at(row, at) == token::fraction_token) {
            ++at;
            if (code_at(row, at) != token::number_token)
                return false;
            skip_payload(row, at);
        }
    } else if (code == token::string_token || code == token::binary_token || code == token::hexadecimal_token ||
               code == token::percentage_token) {
        skip_payload(row, at);
    } else {
        return false;
    }
    return ends_its_line(row, at);
}

} // namespace

bool is_library_word(Code code)
{
    return code == kLibrary;
}

std::string library_name_of(const std::string &written)
{
    const Code as_a_method = token::method_code_of(written);
    return as_a_method != 0 ? std::string(token::method_name_of(as_a_method)) : written;
}

namespace scan {

bool starts_a_library_line(Code code)
{
    if (code == kLibrary)
        return true;
    // A WORD THE LANGUAGE OWNS UNDER satellite.library -- satellite.library.main, and
    // satellite.library.system's rows -- lexes as that word and not as the library and a
    // name, so it is caught here to be refused by name rather than stepped over.
    return word::is_word_code(code) && std::string_view(word::spelling_of(code)).rfind("satellite.library.", 0) == 0;
}

void library_line(CapsuleTable &table, const std::vector<std::bitset<16>> &row, std::size_t r,
                  std::size_t file_scope, std::size_t &i)
{
    const std::size_t declared_at = i;
    // WHEREVER THE LINE IS REFUSED, THE SCAN GOES ON AFTER IT, so a second mistake further
    // down is still found -- check_program says the earliest.
    i = past_the_statement(row, i);

    if (code_at(row, declared_at) != kLibrary) {
        const std::string spelled(word::spelling_of(code_at(row, declared_at)));
        refuse(table, r, declared_at, satl_line_not_understood,
               spelled + " is the language's own word, and a program's satellite.library value needs a name of "
                         "its own -- satellite.library.span = 25");
        return;
    }

    std::size_t k = declared_at + 1;
    std::string name;
    if (code_at(row, k) == token::method_token) {
        ++k;
        name = segment_at(row, k);
    }
    if (name.empty()) {
        refuse(table, r, declared_at, satl_line_not_understood,
               "satellite.library needs a name after it, and then its value -- satellite.library.span = 25");
        return;
    }
    const std::string written = "satellite.library." + name;

    // ONE NAME. The second name after satellite.library is a FILE's -- satellite.library
    // .settings.span reads settings.satl's span -- and a file writes only its own values.
    if (code_at(row, k) == token::method_token) {
        std::size_t m = k + 1;
        const std::string more = segment_at(row, m);
        refuse(table, r, declared_at, satl_line_not_understood,
               written + (more.empty() ? std::string() : "." + more) +
                   " has more than one name, and a satellite.library value has one -- a file writes only its own "
                   "values, and reads another file's as satellite.library.<file>.<value>");
        return;
    }
    if (code_at(row, k) != token::assign_token || ends_its_line(row, k + 1)) {
        refuse(table, r, declared_at, satl_line_not_understood,
               written + " needs its value after an = -- " + written + " = 25");
        return;
    }
    const std::size_t value_at = k + 1;
    // A CHARACTER WITH NO CODE, which is most often 003's # colour: the author's
    // dark_mechanicum writes `satellite.library.colour_ground = #000000` (003 read #RRGGBB
    // and xRRGGBB, his decision of 2026-09-13). 004's lexer has no code for # anywhere yet,
    // and a colour variable takes the hex x000000, so that is the spelling said.
    if (code_at(row, value_at) == token::error_token) {
        refuse(table, r, value_at, satl_line_not_understood,
               written + "'s value starts with a character satl has no code for -- a colour is written x000000 "
                         "here, the hex it holds (#000000 is 003's spelling, and 004 does not read it yet)");
        return;
    }
    if (!a_written_value(row, value_at)) {
        refuse(table, r, value_at, library_value_is_fixed,
               written + " is given something to work out, and a satellite.library value is written down: one "
                         "number, float, text, binary, hex, percentage or fraction, or satellite.bool.true or .false. "
                         "Nothing runs outside a capsule, so there is no moment for it to be worked out in -- work it "
                         "out in the capsule that needs it");
        return;
    }

    CapsuleScope &file = table.scopes[file_scope];
    if (file.library.count(name) != 0) {
        refuse(table, r, declared_at, name_declared_twice,
               written + " is written twice in this file -- a value is written once, and a second would quietly "
                         "replace the first");
        return;
    }
    LibraryValue value;
    value.row = r;
    value.declared_at = declared_at;
    value.value_at = value_at;
    file.library.emplace(name, std::move(value));
}

} // namespace scan

bool library_names_at(const CapsuleTable &table, std::size_t r, const std::vector<std::bitset<16>> &row,
                      std::size_t &at, std::vector<std::string> &names)
{
    std::size_t k = at + 1;
    if (code_at(row, k) != token::method_token)
        return false;
    ++k;
    const std::string stem = included_stem_at(table, r, row, k);
    const std::string first = segment_at(row, k);
    if (first.empty())
        return false;
    // A FILE THIS ONE INCLUDES, BY THE STEM IT WAS INCLUDED BY, and then a second name:
    // `settings.span`. Anything else after the first name is a method on the value --
    // `satellite.library.name.reverse()` -- and join_includes refuses a value named like an
    // included file, so the two never meet.
    names.assign(1, stem.empty() ? first : stem);
    if (!stem.empty() && code_at(row, k) == token::method_token) {
        std::size_t m = k + 1;
        const std::string second = segment_at(row, m);
        if (!second.empty()) {
            names.push_back(second);
            k = m;
        }
    }
    at = k;
    return true;
}

const LibraryValue *library_value_named(const CapsuleTable &table, std::size_t r,
                                        const std::vector<std::string> &names, std::string &why)
{
    const std::string written = written_as(names);
    if (r >= table.file_scope.size()) {
        why = written + " is written at the top of a file, and a line typed at the prompt has no file behind it";
        return nullptr;
    }
    if (names.size() == 1) {
        const CapsuleScope &own = table.scopes[table.file_scope[r]];
        const auto found = own.library.find(names.front());
        if (found != own.library.end())
            return &found->second;
        if (table.included[r].count(names.front()) != 0) {
            why = names.front() + " is a file this file includes, and " + written +
                  " names the file and no value in it -- write " + written + ".<a value it writes>";
            return nullptr;
        }
        // ANOTHER FILE'S, WRITTEN BARE: the spelling that reaches it is said, as a capsule
        // called bare across files is told (capsule_reach.cpp).
        for (const auto &[stem, file] : files_included_by(table, r))
            if (table.scopes[file].library.count(names.front()) != 0) {
                why = written + " is written in " + table.scopes[file].file +
                      ", and another file's values are reached through its name -- write satellite.library." + stem +
                      "." + names.front();
                return nullptr;
            }
        // A FILE THAT ONE OF ITS INCLUDES INCLUDES: not reached from here, by the rule
        // capsules keep (capsule_reach.cpp) -- a file reaches only the files it includes itself.
        for (const auto &[stem, file] : files_included_by(table, r)) {
            const std::size_t theirs = table.scopes[file].row;
            if (theirs < table.included.size() && table.included[theirs].count(names.front()) != 0) {
                why = names.front() + " is a file that " + table.scopes[file].file + " includes, and a file reaches "
                      "only the files it includes itself -- add satellite.include(" + names.front() + ") to this file";
                return nullptr;
            }
        }
        why = "this file writes no " + written + " -- a satellite.library value is written at the top of its file, "
              "outside every capsule: " + written + " = 25";
        return nullptr;
    }

    // THROUGH A FILE: library_names_at reads a second name only after a file this one
    // includes. Two files of one name may both be included; only a value BOTH write is
    // the one case the line could mean either.
    std::vector<std::size_t> holding;
    std::string files;
    const auto included = table.included[r].find(names.front());
    if (included != table.included[r].end())
        for (const std::size_t file : included->second) {
            files += (files.empty() ? "" : " and ") + table.scopes[file].file;
            if (table.scopes[file].library.count(names[1]) != 0)
                holding.push_back(file);
        }
    if (holding.size() == 1)
        return &table.scopes[holding.front()].library.at(names[1]);
    if (holding.empty()) {
        why = files + " writes no satellite.library." + names[1];
        return nullptr;
    }
    why = written + " could be either of two files this file includes -- " + files + " are both named " +
          names.front() + " and both write " + names[1] + ", so rename one of the files";
    return nullptr;
}

signed long long int library_read_is_right(const CapsuleTable &table, const BytecodeRegistry &registry,
                                           const std::vector<std::bitset<16>> &row, std::size_t &at,
                                           std::string &written, Code &type, std::string &why)
{
    const std::size_t r = row_index_of(&registry, row);
    std::vector<std::string> names;
    std::size_t k = at;
    if (!library_names_at(table, r, row, k, names)) {
        why = "satellite.library needs a value's name after its dot -- satellite.library.span";
        return satl_line_not_understood;
    }
    const LibraryValue *found = library_value_named(table, r, names, why);
    if (found == nullptr)
        return name_not_declared;
    written = written_as(names);
    type = type_word_of(found->value);
    // WHAT MAY FOLLOW THE NAMES, judged before anything runs as it is after a variable's
    // name (the review, 2026-09-23: each of these printed first and was refused running).
    if (code_at(row, k) == token::left_parenthesis_token) {
        why = written + " is a value, and a value is not called -- nothing in brackets goes after it";
        return satl_line_not_understood;
    }
    if (code_at(row, k) == token::method_token && code_at(row, k + 1) == token::name_token) {
        std::size_t m = k + 1;
        const std::string member = text_at(row, m);
        why = written + " is " + std::string(word::spelling_of(type)) + ", and " + member +
              " is not one of its methods -- only an object of a satellite.spacesuit has capsules to call";
        return satl_line_not_understood;
    }
    at = k;
    return success;
}

signed long long int library_statement(const CapsuleTable &table, const BytecodeRegistry &registry,
                                       const std::vector<std::bitset<16>> &row, std::size_t &at,
                                       std::string &why)
{
    const std::size_t r = row_index_of(&registry, row);
    std::vector<std::string> names;
    std::size_t k = at;
    const bool named = library_names_at(table, r, row, k, names);
    at = past_the_statement(row, at);
    if (!named) {
        why = "satellite.library needs a value's name after its dot -- satellite.library.span";
        return satl_line_not_understood;
    }
    const std::string written = written_as(names);

    // A LINE THAT ONLY READS ONE -- nothing after its names but the line's end, or a method
    // that answers something new. Its answer goes nowhere, so the line does nothing.
    const Code next = code_at(row, k);
    const bool only_reads = ends_its_line(row, k) || (next == token::method_token &&
                                                      token::is_method_code(code_at(row, k + 1)) &&
                                                      !changes_a_container(code_at(row, k + 1)));
    // ANYTHING ELSE IS A CHANGE -- `=`, `+=`, `.append(x)` -- and refused whether or not the
    // value exists: a satellite.library line inside a capsule is a global either way.
    if (!only_reads) {
        why = written + " is written at the top of its file and never changes -- a value every capsule could change "
                        "would be a global, and satellite has none. Copy it into a variable of your own and change "
                        "that";
        return library_value_is_fixed;
    }
    if (library_value_named(table, r, names, why) == nullptr)
        return name_not_declared;
    why = written + " is a value, and a line that only reads one does nothing -- use it where a value goes, like "
                    "satellite.console.display(" + written + ")";
    return satl_line_not_understood;
}

signed long long int read_library_values(const BytecodeRegistry &registry, const CapsuleTable &table,
                                         const FunctionTable &functions, MachineState &state)
{
    for (const CapsuleScope &scope : table.scopes) {
        if (scope.library.empty())
            continue;
        // IN THE ORDER THEY ARE WRITTEN, so the first refused is the first in the file --
        // `library` is keyed by name, and a hash is not an order.
        std::vector<std::pair<const std::string *, const LibraryValue *>> written;
        for (const auto &[name, value] : scope.library) written.emplace_back(&name, &value);
        std::sort(written.begin(), written.end(),
                  [](const auto &a, const auto &b) { return a.second->value_at < b.second->value_at; });
        for (const auto &[name, value] : written) {
            const std::vector<std::bitset<16>> &row = registry[value->row];
            // NO VARIABLES AND NO FILE: a literal needs neither, and the scan let through
            // nothing else (a_written_value).
            VariableTable none;
            ExpressionContext context(none, functions, state);
            std::size_t at = value->value_at;
            Value read = evaluate_expression(row, at, context);
            if (context.code != success)
                return raise_at(context.code, "satellite.library." + *name + "'s value -- " + context.why,
                                std::string(), state, row, context.placed ? context.refused_at : value->value_at,
                                "satl(check)");
            value->value = std::move(read);
        }
    }
    return success;
}

Value library_value_at(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    // A TYPED LINE HAS NO TABLE, and an empty one answers it: no file, so no values.
    static const CapsuleTable no_file;
    const CapsuleTable &table = context.state.capsules != nullptr ? *context.state.capsules : no_file;
    const std::size_t word_at = at;
    const std::size_t r = row_index_of(context.state.program, row);
    std::vector<std::string> names;
    if (!library_names_at(table, r, row, at, names)) {
        context.refuse(satl_line_not_understood,
                       "satellite.library needs a value's name after its dot -- satellite.library.span", word_at);
        ++at;
        return Value();
    }
    std::string why;
    const LibraryValue *found = library_value_named(table, r, names, why);
    if (found == nullptr) {
        context.refuse(name_not_declared, why, word_at);
        return Value();
    }
    return found->value;
}

} // namespace satellite004
