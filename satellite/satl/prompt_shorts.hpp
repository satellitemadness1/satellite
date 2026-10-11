#pragma once
// satellite/satl/prompt_shorts.hpp -- THE PROMPT'S SHORT SPELLINGS of the folder words.
//
// THE AUTHOR, 2026-10-10: "I need "directory("some/dir/") to work on the prompt, and it changes
// the directory, and as an alias dir("/some/dir") to ONLY work when entered at the prompt, and for
// "list" to give me the same thing as satellite.directory.list()" -- and then, the same evening:
// "alias dir("some_dir") and list() as an alias to satellite.directory.list() and
// directory("some/dir") is just an alias of satellite.directory.change("some/dir")". So:
//
//     directory("some/dir")    is   satellite.directory.change("some/dir")
//     dir("some/dir")          is   satellite.directory.change("some/dir")
//     list()                   is   satellite.directory.list()
//     list("some/dir")         is   satellite.directory.list("some/dir")     (the same word's other shape)
//
// ONE WHOLE STATEMENT TYPED AT THE PROMPT THAT BEGINS WITH ONE OF THEM -- the name, then its
// bracket -- is read as the word it spells, before the lexer sees it (session.cpp calls this only
// where it runs a statement of its own, with no block open). Everything after is the line the full
// spelling makes: the lexer picks the word's shape by the call's arguments, the checker judges
// it, list() draws the same table satellite.directory.list() draws (session.cpp's
// is_one_listing), directory(...) moves the folder the prompt names, and a refusal names the
// word. The person's history keeps what was typed.
//
// ONLY THERE: a program never gets them, and neither does a line of a block typed at the prompt
// -- an if's body, a capsule's, a spacesuit's -- nor the next line of a string or a bracket that
// runs on; there they are names like any other, and a missing capsule of that name is told what
// to write (capsule_reach.cpp). A capsule the session itself declared under one of these names is
// the one called: the declared names are passed in, and a line naming one is left as it is.

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace satellite004 {

inline std::string with_the_prompts_short_spellings(const std::string &line,
                                                    const std::vector<std::string> &declared_names)
{
    static const std::pair<const char *, const char *> shorts[] = {
        {"directory", "satellite.directory.change"},
        {"dir", "satellite.directory.change"},
        {"list", "satellite.directory.list"},
    };
    std::size_t from = 0;
    while (from < line.size() && (line[from] == ' ' || line[from] == '\t')) ++from;
    std::size_t to = from;
    while (to < line.size() && (std::isalnum(static_cast<unsigned char>(line[to])) || line[to] == '_')) ++to;
    std::size_t bracket = to;
    while (bracket < line.size() && (line[bracket] == ' ' || line[bracket] == '\t')) ++bracket;
    if (to == from || bracket >= line.size() || line[bracket] != '(')
        return line;
    const std::string name = line.substr(from, to - from);
    for (const auto &[short_name, word] : shorts) {
        if (name != short_name)
            continue;
        for (const std::string &declared : declared_names)
            if (declared == name)
                return line;
        return line.substr(0, from) + word + line.substr(to);
    }
    return line;
}

} // namespace satellite004
