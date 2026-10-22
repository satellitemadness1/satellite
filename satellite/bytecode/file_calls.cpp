// satellite/bytecode/file_calls.cpp -- the header says why these words are the
// object model's and not libraries, and which refusals stop a program.
//
// A FILE'S METHODS ARE REACHED BY THEIR NUMBERS SINCE 2026-10-11 (NM-8): each
// one's code is a file under satellite-numbers/words/ named by its number --
// 1.6.2.12.cpp is append -- and this file is what they share: the arguments going
// in, the refusals made before one runs, and run_file_number, which calls the one
// a method's number names. satellite.file's own words are still all here.
//
// LINES COUNT FROM 1 (the author, 2026-09-18). Every number going into a file
// method is a line number as a person counts it, and every number coming out --
// index_of, search, size -- is one too. 0 is never a line, so it is the answer
// for "no such line".

#include "file_calls.hpp"
#include "container_calls.hpp"
#include "number_calls.hpp"
#include "string_calls.hpp"

#include "word_codes.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/source_position.hpp"
#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_variable_file/file_moves.hpp"

#include <sys/stat.h>

#include <cstddef>
#include <limits>
#include <string>
#include <utility>

namespace satellite004 {
namespace {

using token::Code;
namespace fast = number_fast_path;

// A LINE NUMBER THAT IS NOT ONE -- negative, or too large to be any line -- goes
// to the file as a number no file can hold, so the file answers "past the end"
// in its own words rather than this code inventing a second sentence for it.
constexpr std::size_t kNoSuchLine = std::numeric_limits<std::size_t>::max();

std::size_t line_number_of(const satellite_number &value)
{
    return fast::fits_a_count(value) ? static_cast<std::size_t>(fast::as_count(value)) : kNoSuchLine;
}

} // namespace

// THE PIECES A FILE'S NUMBERED METHODS SHARE (file_calls.hpp) -- with satellite.file's words and f[n] too.
// Each takes the call as a Said and spells it only when it says something.
namespace file_method_parts {

Value a_number(std::size_t n)
{
    return Value::of_number(satellite_number(static_cast<unsigned long long int>(n)));
}

// Text coming OUT of a file. It was checked as UTF-8 when the file was read, so
// this cannot fail on a line; it is still asked, because a refusal is cheaper
// than a string that is not one.
Value a_string(const std::string &utf8, ExpressionContext &context)
{
    Value out;
    std::size_t bad_offset = 0;
    if (Value::of_utf8(utf8, out, bad_offset) != success) {
        context.refuse(file_not_text, "a line held a byte at " + std::to_string(bad_offset) +
                                          " that is not part of any character");
        return Value();
    }
    return out;
}

// Text going IN. A NUMBER WHERE TEXT IS EXPECTED IS ITS DIGITS (the author,
// 2026-09-16: "obviously the programmer meant convert to string, but record the
// warning in satellite.log") -- and the warning goes there, S020 (M5).
bool text_of(const Value &value, std::string &out, const Said &said, ExpressionContext &context)
{
    if (value.is_string()) { out = value.text_utf8(); return true; }
    if (const satellite_number *number = value.as_number()) {
        out = fast::to_text(*number);
        warn_number_taken_as_text(context.state, said.text(), out);
        return true;
    }
    context.refuse(types_do_not_meet, said.text() + " takes text, and was given " + value.kind_name());
    return false;
}

// A LINE NUMBER BELOW ZERO IS REFUSED, AND SAID AS IT WAS WRITTEN (the review,
// 2026-09-18): it used to reach the file as the largest number there is, so
// `f.truncate(-1)` answered true and changed nothing, and `f[-1]` was reported as
// line 18446744073709551615. Lines count from 1; -1 is not a line in any file.
bool number_of(const Value &value, std::size_t &out, const Said &said, ExpressionContext &context)
{
    if (const satellite_number *number = value.as_number()) {
        if (number->negative()) {
            context.refuse(not_a_position, said.text() + " was given " + fast::to_text(*number) +
                                               ", and lines count from 1");
            return false;
        }
        out = line_number_of(*number);
        return true;
    }
    context.refuse(types_do_not_meet, said.text() + " takes a line number, and was given " + value.kind_name());
    return false;
}

// A FILE'S OWN WORDS ABOUT ITSELF -- its path and its last error -- ARE NEVER A
// REASON TO STOP (the review: a folder whose name is not UTF-8 made `f.error`
// stop the program as if a LINE were bad). Bytes that are not UTF-8 are shown as
// \xHH so the words still reach the person.
Value a_string_always(const std::string &bytes)
{
    Value out;
    std::size_t bad_offset = 0;
    if (Value::of_utf8(bytes, out, bad_offset) == success)
        return out;
    static const char digits[] = "0123456789ABCDEF";
    std::string shown;
    for (const unsigned char c : bytes) {
        if (c < 0x80) { shown += static_cast<char>(c); continue; }
        shown += "\\x";
        shown += digits[c >> 4];
        shown += digits[c & 15];
    }
    Value::of_utf8(shown, out, bad_offset);
    return out;
}

// A READ THAT CANNOT BE ANSWERED STOPS THE PROGRAM, with the file's own code and
// reason: line_past_the_end, file_not_open or file_has_no_lines.
Value a_line_or_stop(bool read, const std::string &text, const satellite_file &file, const Said &said,
                     ExpressionContext &context)
{
    if (read) return a_string(text, context);
    if (file.code() == line_past_the_end || file.code() == success) {
        const std::size_t lines = file.size();
        context.refuse(line_past_the_end, said.text() + ": " + (lines == 0 ? std::string("the file has no lines")
                                                                            : "there is no such line -- the file has " +
                                                                                  std::to_string(lines) +
                                                                                  (lines == 1 ? " line" : " lines") +
                                                                                  ", counting from 1"));
        return Value();
    }
    context.refuse(file.code(), said.text() + ": " + file.error());
    return Value();
}

// A QUESTION ASKED OF A FILE THAT IS NOT OPEN STOPS THE PROGRAM (the review,
// 2026-09-18). It answered as though the file were empty -- `size` 0, `search`
// 0, `contains` false -- so an open that failed went on to a `replace(0, ...)`
// that answered false, and the run exited 0 having changed nothing. Those are
// answers that are wrong and do not say so. The words that CHANGE a file still
// answer false, which is true; the words about the handle itself (ok, error,
// path, exists, open, close, save) still answer.
bool open_or_stop(const satellite_file &file, const std::string &name, const Said &said, ExpressionContext &context)
{
    if (file.ok()) return true;
    context.refuse(file_not_open, said.text() + ": " + name + " is not open" +
                                      (file.error().empty() ? std::string() : " -- " + file.error()));
    return false;
}

} // namespace file_method_parts

using namespace file_method_parts;

namespace {

std::string folder_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    if (slash == std::string::npos) return ".";
    if (slash == 0) return "/";
    return path.substr(0, slash);
}

std::string joined(const std::string &folder, const std::string &rest)
{
    if (rest.empty()) return folder;
    if (folder == ".") return rest;
    return folder.back() == '/' ? folder + rest : folder + "/" + rest;
}

// WHERE A PATH A PROGRAM WROTE POINTS -- the author's include rule, used for files
// (SATELLITE_FILE_OPERATIONS R2; PROGRESS §4: "program root first, then filesystem
// root when it's not found ... the files directory becomes the cwd for each file"):
//
//   no leading /   relative to the folder of the .satl the call is WRITTEN in;
//   a leading /    under the program's own folder first, then the filesystem's.
//
// `making` is true for new(): what must already be there is the folder the file
// goes in, not the file. `link_itself` is true for delete, which takes a link and
// never what it points to: a dangling link under the program's folder is there
// (FD-2's reader: it was passed over for the filesystem's file of that name).
// `folders` is true for satellite.directory.create, which
// makes every missing folder: only the path's FIRST folder must be under the
// program's own (FILE_AND_DRIVE_PLAN.md FD-1's reader: create("/data/two/deeper")
// with only P/data there went to the filesystem's /data). At the prompt there is no
// program file, so a path is the working directory's, as it always was.
std::string resolved(const std::string &written, bool making, const std::vector<std::bitset<16>> &row,
                     ExpressionContext &context, bool folders = false, bool link_itself = false)
{
    const std::string *calling = nullptr;
    const std::string *main_file = nullptr;
    if (context.state.program != nullptr && context.state.program_files != nullptr) {
        const BytecodeFilenames &files = *context.state.program_files;
        const std::size_t which = row_index_of(context.state.program, row);
        if (which < files.size()) calling = &files[which];
        if (!files.empty()) main_file = &files.front();
    }
    // THE FOLDERS ARE THE PROGRAM'S AS IT WAS LOADED (the review, 2026-09-18): a
    // file name in the registry is as it was written on the command line, often
    // relative, so after satellite.directory.change it named a different folder.
    // Joined to the directory satl started in, it names the same one all run.
    const auto anchored = [](const std::string &folder) {
        if (!folder.empty() && folder.front() == '/') return folder;
        const std::string &start = program_start_directory();
        if (start.empty()) return folder;
        return folder == "." ? start : joined(start, folder);
    };
    if (!written.empty() && written.front() == '/') {
        if (main_file == nullptr) return written;
        const std::string root = anchored(folder_of(*main_file));
        if (root == "/") return written;
        const std::string under = joined(root, written.substr(1));
        struct stat about;
        if (folders) {
            const std::size_t first = written.find_first_not_of('/');
            const std::string top = first == std::string::npos ? std::string()
                                                                : written.substr(first, written.find('/', first) - first);
            if (!top.empty() && stat(joined(root, top).c_str(), &about) == 0 && S_ISDIR(about.st_mode))
                return under;
            return written;
        }
        if (making ? (stat(folder_of(under).c_str(), &about) == 0 && S_ISDIR(about.st_mode))
                   : (link_itself ? lstat(under.c_str(), &about) : stat(under.c_str(), &about)) == 0)
            return under;
        return written;
    }
    if (calling == nullptr || written.empty()) return written;
    return joined(anchored(folder_of(*calling)), written);
}

bool kind_of(const Value &value, satellite_file::Kind &kind, const std::string &what, ExpressionContext &context)
{
    std::string word;
    if (!value.is_string()) {
        context.refuse(types_do_not_meet, what + "'s second argument is \"text\" or \"binary\", and it was given " +
                                              value.kind_name());
        return false;
    }
    word = value.text_utf8();
    if (word == "text") { kind = satellite_file::Kind::text; return true; }
    if (word == "binary") { kind = satellite_file::Kind::binary; return true; }
    context.refuse(types_do_not_meet, what + "'s second argument is \"text\" or \"binary\", and it was given \"" +
                                          word + "\"");
    return false;
}

} // namespace

std::string resolved_path(const std::string &written, bool making, const std::vector<std::bitset<16>> &row,
                          ExpressionContext &context, bool folders)
{
    return resolved(written, making, row, context, folders);
}

const char *method_spelling(Code method)
{
    // THE REGISTRY NAMES EVERY METHOD (INF-1): token::method_name_of is generated
    // from its [METHOD] rows -- the first spelling of each, which is the one the
    // language thinks in (`number/as_number/to_number` is "number"). This used to be
    // a hand-written switch of 36 cases, and two more copies lived in
    // container_calls.cpp and expression.cpp; a method added to the registry fell
    // through all three to "that method". One table now, and it cannot go stale.
    const char *named = token::method_name_of(method);
    return named[0] != '\0' ? named : "that method";
}

const char *so_far_whose(token::Code method)
{
    // A COLOUR IS TEXT'S (console_style.hpp), and .background is a window piece's too.
    if (method == token::foreground_token)
        return "so far it is a string's: \"text\".satellite.foreground(xFF8800)";
    if (method == token::background_token)
        return "so far a string and a window piece have it";
    // .ok() IS A FILE'S, A PROGRAM'S AND EVERY OBJECT'S (pointer_calls.hpp) -- it said a file's alone.
    if (method == token::ok_token)
        return "so far a file, a program and an object have it";
    // A HEX'S .width (a window's too), A COLOUR'S .transparency AND A FRACTION'S TWO NUMBERS (scalar_calls.hpp)
    // -- each said "no type has it" since the four types of 2026-09-22 gave them one (2026-10-11).
    if (method == token::width_token)
        return "so far a hex and a window have it";
    if (method == token::transparency_token)
        return "so far it is a color's";
    if (method == token::numerator_token || method == token::denominator_token)
        return "so far it is a fraction's";
    const bool a_file = file_method_arity(method) >= 0;
    const bool a_container = container_arity(method) >= 0;
    // A NUMBER'S OWN (NM-8, 2026-10-11, number_calls.hpp) -- the built ones: .sqrt is numbered and refused. A
    // number's words and a string's share none, so a number is named on lines of its own: `f.floor()` on a
    // float is told a number has it, and `x.max(1)` on an infinity that a number and a container do.
    const bool a_number = number_number_of(method) != 0 && number_method_not_built(method).empty();
    if (a_number && a_file && a_container)
        return "so far a number, a file and a container have it";
    if (a_number && a_container)
        return "so far a number and a container have it";
    if (a_number && a_file)
        return "so far a number and a file have it";
    if (a_number)
        return "so far it is a number's";
    // A STRING'S OWN (M16, string_calls.hpp) -- named first, as the one a person is most
    // likely to have meant: `n.contains("4")` on a number wants n.satellite.string.satellite.contains("4").
    const bool a_string = string_method_arity(method) >= 0;
    if (a_string && a_file && a_container)
        return "so far a string, a file and a container have it";
    if (a_string && a_file)
        return "so far a string and a file have it";
    if (a_string && a_container)
        return "so far a string and a container have it";
    if (a_string)
        return "so far it is a string's";
    if (a_file && a_container)
        return "so far a file and a container have it";
    if (a_container)
        return "so far it is a container's";
    return a_file ? "so far it is a file's" : "so far no type has it";
}

int file_method_arity(Code method)
{
    switch (method) {
    case token::insert_token:
    case token::replace_token: return 2;
    case token::append_token:
    case token::index_of_token:
    case token::search_token:
    case token::contains_token:
    case token::remove_at_token:
    case token::remove_token:
    case token::truncate_token: return 1;
    case token::remove_first_token:
    case token::remove_last_token:
    case token::clear_token:
    case token::size_token:
    case token::empty_token:
    case token::first_token:
    case token::last_token:
    case token::save_token:
    case token::read_all_token:
    case token::close_token:
    case token::open_token:
    case token::ok_token:
    case token::error_text_token:
    case token::path_token:
    case token::exists_token:
    case token::lock_token:       // the author's lock (satellite_object/object_lock.hpp)
    case token::unlock_token: return 0;
    default: return -1;
    }
}

std::size_t file_word_arity(Code code)
{
    return (code == word::code_of(1, 8, 2) || code == word::code_of(1, 8, 4)) ? 2 : 1;
}

std::string file_word_takes(Code code)
{
    const std::string spelling(word::spelling_of(code));
    const std::string bare = spelling.substr(0, spelling.find('('));
    const bool with_kind = code == word::code_of(1, 8, 1) || code == word::code_of(1, 8, 2) ||
                           code == word::code_of(1, 8, 4) || code == word::code_of(1, 8, 6);
    return bare + (with_kind ? " takes a path, or a path and \"text\" or \"binary\"" : " takes a path");
}

bool is_file_word(Code code)
{
    return code == word::fixed_code<1, 8, 1> || code == word::fixed_code<1, 8, 2> ||
           code == word::fixed_code<1, 8, 3> || code == word::fixed_code<1, 8, 4> ||
           code == word::fixed_code<1, 8, 5> || code == word::fixed_code<1, 8, 6> ||
           code == word::fixed_code<1, 8, 7>;
}

Value call_file_word(Code code, const std::vector<Value> &arguments, const std::vector<std::bitset<16>> &row,
                     ExpressionContext &context)
{
    const std::string written_as(word::spelling_of(code));
    const std::string spelling = written_as.substr(0, written_as.find('('));
    const bool two = code == word::code_of(1, 8, 2) || code == word::code_of(1, 8, 4);
    const std::size_t wanted = file_word_arity(code);
    if (arguments.size() != wanted) {
        context.refuse(satl_line_not_understood,
                       file_word_takes(code) + ", and was given " + std::to_string(arguments.size()) + " arguments");
        return Value();
    }

    std::string written;
    if (!text_of(arguments[0], written, Said{spelling}, context))
        return Value();

    const bool making = code == word::code_of(1, 8, 1) || code == word::code_of(1, 8, 4);
    const std::string path = resolved(written, making, row, context, false, code == word::code_of(1, 8, 7));

    if (code == word::code_of(1, 8, 5))
        return Value::of_bool(satellite_file::exists(path));
    if (code == word::code_of(1, 8, 3)) {
        std::string reason;
        return Value::of_bool(satellite_file::clear(path, reason) == success);
    }
    // DELETE GOES THROUGH OR ASKS (file_moves.hpp, FILE_AND_DRIVE_PLAN.md FD-2): true when the file is gone.
    if (code == word::code_of(1, 8, 7))
        return Value::of_bool(delete_a_file(path).done);

    satellite_file::Kind kind = satellite_file::Kind::text;
    if (two && !kind_of(arguments[1], kind, spelling, context))
        return Value();
    FileHandle handle = making ? satellite_file::make_new(path, kind) : satellite_file::open_existing(path, kind);
    // NULL ONLY WHEN THE HANDLE ITSELF COULD NOT BE ALLOCATED, which is the one
    // failure a handle cannot carry (satellite_file.cpp). A null handle must never
    // become a value: every method would dereference it.
    if (handle == nullptr) {
        context.refuse(error, spelling + " could not make a handle -- the machine is out of memory");
        return Value();
    }
    return Value::of_file(std::move(handle));
}

Value read_file_line(satellite_file &file, const Value &index, const std::string &name, ExpressionContext &context)
{
    std::size_t n = 0;
    const std::string any_line = name + "[...]";
    if (!number_of(index, n, Said{any_line}, context))
        return Value();
    const std::string what = name + "[" + fast::to_text(*index.as_number()) + "]";
    if (!open_or_stop(file, name, Said{what}, context))
        return Value();
    std::string text;
    const bool read = file.line(n, text);
    return a_line_or_stop(read, text, file, Said{what}, context);
}

// ---------------------------------------------------------------------------
// A FILE'S METHODS RUN BY THEIR NUMBERS (NM-8; the author, 2026-10-11: "we still need to get the word
// satellite onto everything ... everything builtin needs the word satellite still", and of how, 2026-10-10:
// "we have to give everything a number and then make it run those numbers"). Each is
// satellite.variable.file.<it>, 1 6 2 n, whose code is a file under satellite-numbers/words/.
// ---------------------------------------------------------------------------

// EVERY NUMBER BELOW IS IN words/words.tsv, OR THIS FILE DOES NOT COMPILE: a fixed_code of 0
// would quietly send that method to call_file_method's refusal instead of to its number.
template <int... Last>
inline constexpr bool every_file_word_numbered = ((word::fixed_code<1, 6, 2, Last> != 0) && ...);
static_assert(every_file_word_numbered<2, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                                       26, 27, 28, 29, 30>,
              "a file method's number is missing from words/words.tsv");

token::Code file_number_of(token::Code method)
{
    // ONE METHOD, ONE NUMBER, as a string's are (string_number_of) -- replace too, whose two shapes,
    // replace(n, x) and replace(a, b), are one method that looks at its first argument (1.6.2.14.cpp).
    // `contain` and `has` are contains by the time anything here sees them: the lexer folds a second
    // spelling into the first. 1 6 2 1 new, 3 read_line, 4 write_line(s) and 11 write(x) are 003's words,
    // and no method of 004 is spelled that way -- they stay numbered and empty, as a list's 1 4 2 4 to 7 do
    // (SATELLITE_FILE_OPERATIONS.md 3.4: "A number is never reused").
    switch (method) {
    case token::open_token: return word::fixed_code<1, 6, 2, 2>;
    case token::read_all_token: return word::fixed_code<1, 6, 2, 5>;
    case token::close_token: return word::fixed_code<1, 6, 2, 6>;
    case token::exists_token: return word::fixed_code<1, 6, 2, 7>;
    case token::ok_token: return word::fixed_code<1, 6, 2, 8>;
    case token::path_token: return word::fixed_code<1, 6, 2, 9>;
    case token::error_text_token: return word::fixed_code<1, 6, 2, 10>;
    case token::append_token: return word::fixed_code<1, 6, 2, 12>;
    case token::insert_token: return word::fixed_code<1, 6, 2, 13>;
    case token::replace_token: return word::fixed_code<1, 6, 2, 14>;
    case token::index_of_token: return word::fixed_code<1, 6, 2, 15>;
    case token::search_token: return word::fixed_code<1, 6, 2, 16>;
    case token::contains_token: return word::fixed_code<1, 6, 2, 17>;
    case token::remove_at_token: return word::fixed_code<1, 6, 2, 18>;
    case token::remove_token: return word::fixed_code<1, 6, 2, 19>;
    case token::remove_first_token: return word::fixed_code<1, 6, 2, 20>;
    case token::remove_last_token: return word::fixed_code<1, 6, 2, 21>;
    case token::truncate_token: return word::fixed_code<1, 6, 2, 22>;
    case token::clear_token: return word::fixed_code<1, 6, 2, 23>;
    case token::size_token: return word::fixed_code<1, 6, 2, 24>;
    case token::empty_token: return word::fixed_code<1, 6, 2, 25>;
    case token::first_token: return word::fixed_code<1, 6, 2, 26>;
    case token::last_token: return word::fixed_code<1, 6, 2, 27>;
    case token::save_token: return word::fixed_code<1, 6, 2, 28>;
    // THE AUTHOR'S LOCK ON A FILE (satellite_object/object_lock.hpp) is a file's word as much as any: numbered
    // with the rest, and run by the walker BEFORE the hold it turns on (expression.cpp).
    case token::lock_token: return word::fixed_code<1, 6, 2, 29>;
    case token::unlock_token: return word::fixed_code<1, 6, 2, 30>;
    default: return 0;
    }
}

Value run_file_number(token::Code method, token::Code number, const Value &receiver,
                      const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                      ExpressionContext &context)
{
    // HOW MANY ARGUMENTS EACH TAKES (file_method_arity, which the checker reads
    // too). Zero-argument methods may leave the brackets off (`f.satellite.size`, as
    // words.tsv spells them); the rest may not. Every method with a number has a count.
    // LOCK AND UNLOCK ARE NOT COUNTED HERE, AS THEY NEVER WERE: BUILD 0032 turned the lock on or off and read
    // nothing in their brackets. The checker refuses `f.satellite.lock(5)` on a name it can type; on one it
    // cannot -- a list's item, a word's answer -- the line runs as it always ran. Not about the spelling, so
    // not changed with it (the fresh reader's H2, 2026-10-11; asked of the author).
    const std::size_t wanted = static_cast<std::size_t>(file_method_arity(method));
    const bool counted = method != token::lock_token && method != token::unlock_token;
    if (counted && arguments.size() != wanted) {
        context.refuse(satl_line_not_understood, name + ".satellite." + method_spelling(method) + " takes " +
                                                     std::to_string(wanted) + " argument" + (wanted == 1 ? "" : "s") +
                                                     ", and was given " + std::to_string(arguments.size()));
        return Value();
    }
    if (wanted > 0 && !had_parentheses) {
        context.refuse(satl_line_not_understood,
                       name + ".satellite." + method_spelling(method) + " takes an argument and needs a ( after it");
        return Value();
    }

    // THE QUESTIONS ABOUT LINES NEED AN OPEN FILE (open_or_stop says why) -- asked before the number runs,
    // so no question's own code ever sees a file that is not open.
    switch (method) {
    case token::index_of_token:
    case token::search_token:
    case token::contains_token:
    case token::size_token:
    case token::empty_token:
    case token::first_token:
    case token::last_token:
    case token::read_all_token:
        if (!open_or_stop(*receiver.as_file(), name, Said{name, method_spelling(method)}, context)) return Value();
        break;
    default: break;
    }

    // THE NUMBER RUNS: table[code], one subtraction and one load (function_table.hpp), and the
    // function its file under satellite-numbers/words/ put in the row at start-up.
    const NumberRow *row = context.functions.method(number);
    if (row == nullptr || row->scenarios.method_call == nullptr) {
        context.refuse(not_built_yet, name + ".satellite." + method_spelling(method) + " is not built for a file yet");
        return Value();
    }
    return row->scenarios.method_call(receiver, nullptr, nullptr, arguments, had_parentheses, name, context);
}

Value call_file_method(Code method, const std::string &name, ExpressionContext &context)
{
    // A WORD A FILE DOES NOT HAVE, written with a `.` -- every word it has runs its number above, and is
    // written .satellite. (method_periods.hpp).
    const std::string what = name + "." + method_spelling(method);
    context.refuse(types_do_not_meet, what + " -- a file has no " + method_spelling(method) +
                                          " (SATELLITE_FILE_OPERATIONS Part 3 lists what a file does)");
    return Value();
}

} // namespace satellite004
