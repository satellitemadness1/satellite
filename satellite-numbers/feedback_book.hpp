#pragma once
// satellite-numbers/feedback_book.hpp -- WHERE `satellite.feedback(x)` PUTS WHAT
// A PERSON SAYS, AND WHY IT CANNOT BE USED TO FLOOD ANYBODY.
//
// The author, 2026-09-18: *"I do not want to spy on the users... the users code
// and what they are doing on their machine is their own business, and the
// programming language should NOT collect that sort of stuff, so we just need to
// build a system that you cannot BOMB with satellite.feedback({"something_in_a_loop"})"*.
//
// THE INTERPRETER NEVER TOUCHES THE NETWORK. THAT IS THE WHOLE DESIGN.
// `satellite.feedback(x)` writes a line to a file on this machine and stops. A
// separate, explicit `satl --feedback` is the only thing that could ever send
// anything. So a program that calls feedback a million times in a loop cannot
// reach a server AT ALL -- not slowly, not eventually, not at a rate limit. It
// fills a bounded local file and that is the end of it.
//
// A client-side cap is bypassable by anybody willing to patch satl, and that is
// fine: this is not a defence against an attacker, who can just use curl. It is
// a defence against an ACCIDENT -- the loop the author named -- and against a
// language feature quietly becoming a network client.
//
// FOUR BOUNDS, AND EVERY ONE OF THEM IS A NUMBER RATHER THAN A FEELING:
//
//   1. IDENTICAL TEXT IS COUNTED, NEVER REPEATED. feedback("x") in a loop is one
//      entry that says 40,000. So the loop the author is worried about costs one
//      line of disk, and the count is itself worth reading.
//   2. AT MOST kDifferentThings distinct messages a run. A person who is angry
//      types five different things as fast as they can; nobody types 200.
//   3. AT MOST kBytesEach of one message, and it is TRUNCATED rather than
//      refused -- somebody mid-rant should not lose the rant to a limit.
//   4. AT MOST kBytesInAll in the file, ever. Past that, the oldest go.
//
// WHAT IS WRITTEN, AND WHAT IS NOT. What the person typed, the time, and a
// count. NOT their name, NOT their file, NOT their directory, NOT their source,
// NOT their hostname -- none of which this file can even see, because
// `satellite.feedback` is handed a string and nothing else. That is not
// discipline, it is the shape of the call.

#include "number_row.hpp"
#include "../satellite/config/config_file.hpp"
#include "../satellite/machine/machine_codes.hpp"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

namespace satellite004 {
namespace feedback_book {

inline constexpr std::size_t kDifferentThings = 32;        // distinct messages a run
inline constexpr std::size_t kBytesEach = 2000;            // one message
inline constexpr std::size_t kBytesInAll = 256 * 1024;     // the whole book

inline std::string book_path()
{
    const std::string where = config_file::folder();
    if (where.empty()) return std::string();
    return where + "/feedback.txt";
}

// WHAT A PERSON TYPED, MADE SAFE TO WRITE ON ONE LINE -- and safe for whatever
// reads the file later.
//
// NEWLINES AND CONTROL BYTES BECOME SPACES, so one message is one line and
// nobody can forge a second entry by typing a newline into the first. A file
// whose format a person can inject into is a file nothing downstream can trust.
inline std::string one_line(const std::string &said)
{
    std::string out;
    out.reserve(std::min(said.size(), kBytesEach));
    for (unsigned char c : said) {
        if (out.size() >= kBytesEach) break;
        out += (c < 0x20 || c == 0x7f) ? ' ' : static_cast<char>(c);
    }
    // Trailing blanks say nothing and make two equal messages look different.
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

// The messages this RUN has already kept, so a loop is counted and not repeated.
struct Said {
    std::string text;
    unsigned long long int times = 1;
};

inline std::vector<Said> &this_run()
{
    static std::vector<Said> kept;
    return kept;
}

// Answers false when this run has already said as many DIFFERENT things as it
// may. A repeat is always accepted, because accepting it costs one increment.
inline bool remember(const std::string &text, bool &is_new)
{
    for (Said &already : this_run()) {
        if (already.text == text) {
            ++already.times;
            is_new = false;
            return true;
        }
    }
    if (this_run().size() >= kDifferentThings) return false;
    this_run().push_back(Said{text, 1});
    is_new = true;
    return true;
}

// Keep the book under its bound by dropping the OLDEST entries -- the same
// choice the statement ring makes, and for the same reason: a bound that is
// enforced by refusing new writes stops telling you what is happening now.
inline void keep_the_book_small(const std::string &path)
{
    std::ifstream in(path, std::ios::ate);
    if (!in.is_open()) return;
    const std::streamoff size = in.tellg();
    if (size <= static_cast<std::streamoff>(kBytesInAll)) return;

    in.seekg(0);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    in.close();

    std::size_t bytes = 0, from = lines.size();
    while (from > 0 && bytes + lines[from - 1].size() + 1 <= kBytesInAll) {
        --from;
        bytes += lines[from].size() + 1;
    }
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) return;
    for (std::size_t i = from; i < lines.size(); ++i) out << lines[i] << "\n";
}

// Write one message. Answers success, or why it could not.
inline signed long long int write_one(const std::string &said, std::string &why)
{
    const std::string path = book_path();
    if (path.empty()) {
        why = "$HOME is not set, so there is no ~/.satl to keep feedback in";
        return config_file_unwritable;
    }

    const std::string text = one_line(said);
    if (text.empty()) {
        why = "there is nothing in it";
        return empty_search_text;
    }

    bool is_new = false;
    if (remember(text, is_new) == false) {
        why = "this run has already said " + std::to_string(kDifferentThings) +
              " different things, which is as many as one run may";
        return setting_out_of_range;
    }
    if (is_new == false)
        return success;              // a repeat: counted in memory, not written again

    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
        why = "could not open " + path;
        return config_file_unwritable;
    }
    const std::time_t when = std::time(nullptr);
    char stamp[32] = {0};
    std::tm broken {};
    if (localtime_r(&when, &broken) != nullptr)
        std::strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", &broken);
    out << stamp << "\t" << text << "\n";
    out.close();
    keep_the_book_small(path);
    return out.good() ? success : config_file_unwritable;
}

} // namespace feedback_book
} // namespace satellite004
