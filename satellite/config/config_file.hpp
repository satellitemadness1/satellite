#pragma once
// $HOME/.satl/config.ini -- the settings a PROGRAM writes, and the first file in
// 004 that outlives the run that wrote it.
//
// THIS IS NOT satellite_config.hpp AND THE TWO ARE NOT RIVALS. That file is the
// AUTHOR's configuration: it is C++, it is compiled in, and a program can no
// more write it than it can write its own binary. This one is the PERSON's, and
// the difference is who does the writing:
//
//     satellite_config.hpp   written by the author, read at start-up, compiled
//     config.ini             written by a running program, read at start-up
//
// `arguments.access = true` is the first line in the language that has to
// survive the program that said it. A satellite variable dies with its
// VariableTable; a row in satellite_config.hpp cannot be reached at run time at
// all. So there is a third place, and this is it.
//
// $HOME AND NOT BESIDE THE BINARY (the author, 2026-09-18): "let's put it into
// $(HOME)/.satl/config.ini then, and we'll have to build that into the
// installer". Beside the binary was 003's answer -- `satl reads a
// satellite_config.ini beside its own binary` -- and it loses the file on every
// `make clean`, which is exactly what a setting called LASTING must not do.
// $HOME/.satl is where satl is installed, so the setting sits with the thing it
// configures and survives a rebuild.
//
// HEADER-ONLY, AND THAT IS A BUILD FACT RATHER THAN A STYLE. Each numbered
// library under satellite-numbers/ is compiled from EXACTLY ONE .cpp
// (build_libraries.py: `<folder>/<folder>.satellite.cpp`), so a library that
// needed a second translation unit could not be built at all. Everything here is
// inline for that reason.
//
// THE FORMAT IS `key = value`, ONE PER LINE, `#` COMMENTS. Whitespace around
// either side is ignored. A line that is not `key = value` is not an error and
// is not dropped -- see write_flag(), which is the whole reason the parse is
// this forgiving.

#include "../machine/machine_codes.hpp"

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace satellite004::config_file {

// Trim the spaces and tabs off both ends. Not a general trimmer -- it is what
// `  access = true  ` needs and nothing more.
inline std::string trimmed(const std::string &text)
{
    const std::string spaces = " \t\r";
    const std::string::size_type first = text.find_first_not_of(spaces);
    if (first == std::string::npos)
        return std::string();
    const std::string::size_type last = text.find_last_not_of(spaces);
    return text.substr(first, last - first + 1);
}

// $HOME/.satl -- the FOLDER. Empty when HOME is unset, which is a real state:
// a daemon started with a scrubbed environment has no home, and the caller must
// say "could not" rather than write to a path beginning with "/.satl".
inline std::string folder()
{
    const char *home = std::getenv("HOME");
    if (home == nullptr || *home == '\0')
        return std::string();
    return std::string(home) + "/.satl";
}

inline std::string path()
{
    const std::string where = folder();
    if (where.empty())
        return std::string();
    return where + "/config.ini";
}

// THE FILE IS READ ONCE (the author, 2026-10-03: "reading config.ini a single time"). Every
// reader here used to open it again: a start-up opened it 22 times -- once for each feature
// switch, the register, the text rows, the float precisions and the display buffer. Now the
// first reader reads every line into one copy, every reader after it reads the copy, and a
// write puts what it wrote into the copy as well as the file, so a value read after a write
// is the value written.
//
// WHAT IT GIVES UP: a change another program makes to the file while this one runs is not
// seen until this one starts again. Every setting here is read at start-up anyway, and the
// one a program reads while it runs -- access -- is one it writes itself.
//
// UNDER ONE LOCK, because a thread may read a setting while another writes one.
struct Copy {
    bool read = false;      // the file has been looked for once
    bool found = false;     // and it was there
    std::vector<std::string> lines;
};

inline std::mutex &copy_lock()
{
    static std::mutex lock;
    return lock;
}

// The copy, read from the file the first time anything asks. Call it holding copy_lock().
inline Copy &the_copy()
{
    static Copy copy;
    if (!copy.read) {
        copy.read = true;
        const std::string where = path();
        if (!where.empty()) {
            std::ifstream in(where);
            if (in) {
                copy.found = true;
                for (std::string line; std::getline(in, line);)
                    copy.lines.push_back(line);
            }
        }
    }
    return copy;
}

// The key and value a `key = value` line holds -- the value is everything after the first
// `=` -- or false for a blank line, a comment, or a line that is not one.
inline bool key_and_value(const std::string &line, std::string &key, std::string &value)
{
    const std::string clean = trimmed(line);
    if (clean.empty() || clean[0] == '#')
        return false;
    const std::string::size_type equals = clean.find('=');
    if (equals == std::string::npos)
        return false;
    key = trimmed(clean.substr(0, equals));
    value = trimmed(clean.substr(equals + 1));
    return true;
}

// Whether the file is there at all. Asked at start-up so satl can SAY it is
// missing -- the author, 2026-09-18: *"don't run the interpreter without
// /home_dir/.satl/config.ini say 'cannot find ...' please reinstall satellite,
// or create a config.ini there"*.
//
// AND IT IS A NOTICE RATHER THAN A REFUSAL, on the same instruction: *"let's
// build default config values so the interpreter still runs without the
// config.ini file"*. A missing file is a thing to tell somebody about, not a
// reason to take their interpreter away -- every setting carries its own default
// (see each library's kDefault), so there is nothing satl cannot do without it.
//
// NOTHING IS WRITTEN HERE TO FIX IT. satl could create the file and be quiet,
// and that is exactly what would make a broken install invisible: the person
// asked to be told, and told is what a person can act on.
inline bool exists()
{
    std::lock_guard<std::mutex> hold(copy_lock());
    return the_copy().found;
}

// Whether the file names this key, and what it says. `found` is false both when
// the file is absent and when it is there without this key -- the caller wants a
// default in either case, and telling them apart buys nothing.
//
// THE LAST ONE WINS. A file with two `access =` rows is not rejected: it is read
// the way a person reading it top to bottom would read it, and write_flag()
// leaves exactly one behind anyway.
// The general reader: the text on the right of `key =`, whatever it is.
// read_flag() is this with the two words understood.
inline bool read_value(const std::string &key, std::string &value)
{
    std::lock_guard<std::mutex> hold(copy_lock());
    bool found = false;
    std::string said_key, said;
    for (const std::string &line : the_copy().lines) {
        if (!key_and_value(line, said_key, said) || said_key != key)
            continue;
        value = said;
        found = true;   // the LAST one wins; see above
    }
    return found;
}

// EVERY LINE THAT IS NOT BLANK OR A COMMENT, with its line number, read the way read_value
// reads one -- the key and value of a `key = value`, and an empty key for a line that is not
// one. For start-up's S016, which names a row satl does not know (the author, 2026-09-25).
struct Row {
    std::size_t line = 0;
    std::string key;
    std::string written;
};

inline std::vector<Row> rows()
{
    std::vector<Row> found;
    std::lock_guard<std::mutex> hold(copy_lock());
    const std::vector<std::string> &lines = the_copy().lines;
    for (std::size_t at = 0; at < lines.size(); ++at) {
        const std::string clean = trimmed(lines[at]);
        if (clean.empty() || clean[0] == '#')
            continue;
        const std::string::size_type equals = clean.find('=');
        found.push_back(Row{at + 1, equals == std::string::npos ? std::string() : trimmed(clean.substr(0, equals)), clean});
    }
    return found;
}

inline bool read_flag(const std::string &key, bool &value)
{
    std::string said;
    if (read_value(key, said)) {
        // `true` AND `false` AND NOTHING ELSE IS TRUE. A row saying `yes` or `1`
        // is a row somebody meant as true, and reading it as false would be the
        // quiet kind of wrong -- so anything that is not literally `false` and
        // not empty reads as true, and the writer only ever writes the two words.
        value = !(said == "false" || said.empty());
        return true;
    }
    return false;
}

// THE WHOLE FILE REWRITTEN: every line as it is ON DISK now -- not the copy, because a person
// or another program may have changed the file since this one started, and a writer must keep
// what they wrote -- then `change`, then written back, and the copy becomes what was written.
// No file yet is no lines yet: what `change` adds is the file.
template <typename Change>
inline signed long long int rewrite(Change change, std::string &why)
{
    std::lock_guard<std::mutex> hold(copy_lock());
    const std::string where = path();
    if (where.empty()) {
        why = "HOME is not set, so there is no $HOME/.satl to write to";
        return config_file_unwritable;
    }

    // The folder may not exist -- a first run before the installer, or a home
    // satl has never been installed into. 0755 is what the installer makes.
    ::mkdir(folder().c_str(), 0755);   // EEXIST is the ordinary answer and is not read

    std::vector<std::string> lines;
    {
        std::ifstream in(where);
        for (std::string line; in && std::getline(in, line);)
            lines.push_back(line);
    }
    change(lines);

    // WRITTEN TO A TEMPORARY AND RENAMED, so a config.ini is never half a file.
    // rename(2) within one directory is atomic: a reader either sees all of the
    // old one or all of the new one, and a program killed mid-write leaves the
    // setting it had rather than a truncated row nothing can parse.
    //
    // THE TEMPORARY IS THIS PROCESS'S OWN (2026-10-03): two satl started together on a first
    // start both write the machine's facts, and with one shared name the second could rename
    // the first one's half-written file into place.
    const std::string temporary = where + ".writing." + std::to_string(::getpid());
    {
        std::ofstream out(temporary, std::ios::trunc);
        if (!out) {
            why = "could not write " + temporary;
            return config_file_unwritable;
        }
        for (const std::string &line : lines)
            out << line << '\n';
        out.flush();
        if (!out) {
            why = "could not finish writing " + temporary;
            return config_file_unwritable;
        }
    }
    if (std::rename(temporary.c_str(), where.c_str()) != 0) {
        std::remove(temporary.c_str());
        why = "could not put " + temporary + " in place as " + where;
        return config_file_unwritable;
    }
    Copy &copy = the_copy();
    copy.read = true;
    copy.found = true;
    copy.lines = std::move(lines);
    return success;
}

// Write `key = value`, keeping every other line of the file exactly as it was.
//
// REWRITTEN IN PLACE AND NOT APPENDED, because appending makes the file grow by
// one row every time a program assigns -- which for a setting written in a loop
// is the console queue's bug with a disk behind it instead of memory. The whole
// file is read, the matching rows are replaced, and a key that was never there
// is added once at the end.
//
// EVERY OTHER LINE SURVIVES, COMMENTS INCLUDED. The file is a person's, and a
// program that rewrote it into a canonical form would throw away notes the
// person left. That is the reason the parser above ignores what it cannot read
// rather than refusing it.
inline signed long long int write_value(const std::string &key, const std::string &value, std::string &why)
{
    return rewrite(
        [&](std::vector<std::string> &lines) {
            bool replaced = false;
            std::string said_key, said;
            for (std::string &line : lines) {
                if (!key_and_value(line, said_key, said) || said_key != key)
                    continue;
                line = key + " = " + value;
                replaced = true;
            }
            if (!replaced)
                lines.push_back(key + " = " + value);
        },
        why);
}

// The two words understood. `true` and `false` are the only things written, so
// the forgiving read above never has to guess about a row this wrote.
inline signed long long int write_flag(const std::string &key, bool value, std::string &why)
{
    return write_value(key, value ? "true" : "false", why);
}

} // namespace satellite004::config_file
