// satellite/bytecode/file_calls.cpp -- the header says why these words are the
// object model's and not libraries, and which refusals stop a program.
//
// LINES COUNT FROM 1 (the author, 2026-09-18). Every number going into a file
// method is a line number as a person counts it, and every number coming out --
// index_of, search, size -- is one too. 0 is never a line, so it is the answer
// for "no such line".

#include "file_calls.hpp"
#include "container_calls.hpp"

#include "word_codes.hpp"
#include "../machine/source_position.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

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
// 2026-09-16: "obviously the programmer meant convert to string"); the warning
// that ruling asks for belongs to satellite.log, which is M5 and not built.
bool text_of(const Value &value, std::string &out, const std::string &what, ExpressionContext &context)
{
    if (value.is_string()) { out = value.text_utf8(); return true; }
    if (const satellite_number *number = value.as_number()) { out = fast::to_text(*number); return true; }
    context.refuse(types_do_not_meet, what + " takes text, and was given " + value.kind_name());
    return false;
}

// A LINE NUMBER BELOW ZERO IS REFUSED, AND SAID AS IT WAS WRITTEN (the review,
// 2026-09-18): it used to reach the file as the largest number there is, so
// `f.truncate(-1)` answered true and changed nothing, and `f[-1]` was reported as
// line 18446744073709551615. Lines count from 1; -1 is not a line in any file.
bool number_of(const Value &value, std::size_t &out, const std::string &what, ExpressionContext &context)
{
    if (const satellite_number *number = value.as_number()) {
        if (number->negative()) {
            context.refuse(not_a_position, what + " was given " + fast::to_text(*number) +
                                               ", and lines count from 1");
            return false;
        }
        out = line_number_of(*number);
        return true;
    }
    context.refuse(types_do_not_meet, what + " takes a line number, and was given " + value.kind_name());
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
// goes in, not the file. At the prompt there is no program file, so a path is the
// working directory's, as it always was.
std::string resolved(const std::string &written, bool making, const std::vector<std::bitset<16>> &row,
                     ExpressionContext &context)
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
        if (making ? (stat(folder_of(under).c_str(), &about) == 0 && S_ISDIR(about.st_mode))
                   : stat(under.c_str(), &about) == 0)
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

// A READ THAT CANNOT BE ANSWERED STOPS THE PROGRAM, with the file's own code and
// reason: line_past_the_end, file_not_open or file_has_no_lines.
Value a_line_or_stop(bool read, const std::string &text, const satellite_file &file, const std::string &what,
                     ExpressionContext &context)
{
    if (read) return a_string(text, context);
    if (file.code() == line_past_the_end || file.code() == success) {
        const std::size_t lines = file.size();
        context.refuse(line_past_the_end, what + ": " + (lines == 0 ? std::string("the file has no lines")
                                                                     : "there is no such line -- the file has " +
                                                                           std::to_string(lines) +
                                                                           (lines == 1 ? " line" : " lines") +
                                                                           ", counting from 1"));
        return Value();
    }
    context.refuse(file.code(), what + ": " + file.error());
    return Value();
}

// A QUESTION ASKED OF A FILE THAT IS NOT OPEN STOPS THE PROGRAM (the review,
// 2026-09-18). It answered as though the file were empty -- `size` 0, `search`
// 0, `contains` false -- so an open that failed went on to a `replace(0, ...)`
// that answered false, and the run exited 0 having changed nothing. Those are
// answers that are wrong and do not say so. The words that CHANGE a file still
// answer false, which is true; the words about the handle itself (ok, error,
// path, exists, open, close, save) still answer.
bool open_or_stop(const satellite_file &file, const std::string &name, const std::string &what,
                  ExpressionContext &context)
{
    if (file.ok()) return true;
    context.refuse(file_not_open, what + ": " + name + " is not open" +
                                      (file.error().empty() ? std::string() : " -- " + file.error()));
    return false;
}

} // namespace

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
    const bool a_file = file_method_arity(method) >= 0;
    const bool a_container = container_arity(method) >= 0;
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
    case token::exists_token: return 0;
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
    return code == word::code_of(1, 8, 1) || code == word::code_of(1, 8, 2) || code == word::code_of(1, 8, 3) ||
           code == word::code_of(1, 8, 4) || code == word::code_of(1, 8, 5) || code == word::code_of(1, 8, 6);
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
    if (!text_of(arguments[0], written, spelling, context))
        return Value();

    const bool making = code == word::code_of(1, 8, 1) || code == word::code_of(1, 8, 4);
    const std::string path = resolved(written, making, row, context);

    if (code == word::code_of(1, 8, 5))
        return Value::of_bool(satellite_file::exists(path));
    if (code == word::code_of(1, 8, 3)) {
        std::string reason;
        return Value::of_bool(satellite_file::clear(path, reason) == success);
    }

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
    if (!number_of(index, n, name + "[...]", context))
        return Value();
    const std::string what = name + "[" + fast::to_text(*index.as_number()) + "]";
    if (!open_or_stop(file, name, what, context))
        return Value();
    std::string text;
    const bool read = file.line(n, text);
    return a_line_or_stop(read, text, file, what, context);
}

Value call_file_method(Code method, satellite_file &file, const std::vector<Value> &arguments,
                       bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string what = name + "." + method_spelling(method);

    // HOW MANY ARGUMENTS EACH TAKES (file_method_arity, which the checker reads
    // too). Zero-argument methods may leave the brackets off (`f.size`, as
    // words.tsv spells them); the rest may not.
    const int arity = file_method_arity(method);
    if (arity < 0) {
        context.refuse(types_do_not_meet, what + " -- a file has no " + method_spelling(method) +
                                              " (SATELLITE_FILE_OPERATIONS Part 3 lists what a file does)");
        return Value();
    }
    const std::size_t wanted = static_cast<std::size_t>(arity);
    if (arguments.size() != wanted) {
        context.refuse(satl_line_not_understood, what + " takes " + std::to_string(wanted) + " argument" +
                                                     (wanted == 1 ? "" : "s") + ", and was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }
    if (wanted > 0 && !had_parentheses) {
        context.refuse(satl_line_not_understood, what + " takes an argument and needs a ( after it");
        return Value();
    }

    // THE QUESTIONS ABOUT LINES NEED AN OPEN FILE (open_or_stop says why).
    switch (method) {
    case token::index_of_token:
    case token::search_token:
    case token::contains_token:
    case token::size_token:
    case token::empty_token:
    case token::first_token:
    case token::last_token:
    case token::read_all_token:
        if (!open_or_stop(file, name, what, context)) return Value();
        break;
    default: break;
    }

    std::string text, other;
    std::size_t n = 0;
    switch (method) {
    case token::append_token:
        if (!text_of(arguments[0], text, what, context)) return Value();
        return Value::of_bool(file.append(text));
    case token::insert_token:
        if (!number_of(arguments[0], n, what, context) || !text_of(arguments[1], text, what, context)) return Value();
        return Value::of_bool(file.insert(n, text));
    // TWO SHAPES, ONE WORD (the author, 2026-09-18: "we will do .replace(line_number,
    // "string") or .replace will optionally take this: .replace("string", "with
    // string") so it will have two syntaxes"). The first argument's KIND chooses.
    case token::replace_token:
        if (arguments[0].is_number()) {
            if (!number_of(arguments[0], n, what, context) || !text_of(arguments[1], text, what, context))
                return Value();
            return Value::of_bool(file.replace_line(n, text));
        }
        if (!text_of(arguments[0], text, what, context) || !text_of(arguments[1], other, what, context))
            return Value();
        return Value::of_bool(file.replace_text(text, other));
    case token::index_of_token:
        if (!text_of(arguments[0], text, what, context)) return Value();
        return a_number(file.index_of(text));
    case token::search_token:
        if (!text_of(arguments[0], text, what, context)) return Value();
        return a_number(file.search(text));
    case token::contains_token:
        if (!text_of(arguments[0], text, what, context)) return Value();
        return Value::of_bool(file.contains(text));
    case token::remove_at_token:
        if (!number_of(arguments[0], n, what, context)) return Value();
        return Value::of_bool(file.remove_at(n));
    case token::remove_token:
        if (!text_of(arguments[0], text, what, context)) return Value();
        return Value::of_bool(file.remove(text));
    case token::truncate_token:
        if (!number_of(arguments[0], n, what, context)) return Value();
        return Value::of_bool(file.truncate(n));
    case token::remove_first_token: return Value::of_bool(file.remove_first());
    case token::remove_last_token: return Value::of_bool(file.remove_last());
    case token::clear_token: return Value::of_bool(file.clear_lines());
    case token::size_token: return a_number(file.size());
    case token::empty_token: return Value::of_bool(file.empty());
    case token::first_token: {
        const bool read = file.first(text);
        return a_line_or_stop(read, text, file, what, context);
    }
    case token::last_token: {
        const bool read = file.last(text);
        return a_line_or_stop(read, text, file, what, context);
    }
    case token::save_token: return Value::of_bool(file.save());
    case token::read_all_token: return a_string(file.read_all(), context);
    case token::close_token: return Value::of_bool(file.close());
    case token::open_token: return Value::of_bool(file.reopen());
    case token::ok_token: return Value::of_bool(file.ok());
    case token::error_text_token: return a_string_always(file.error());
    case token::path_token: return a_string_always(file.path());
    case token::exists_token: return Value::of_bool(file.path_exists());
    default: break;
    }
    context.refuse(types_do_not_meet, what + " -- a file has no " + method_spelling(method));
    return Value();
}

} // namespace satellite004
