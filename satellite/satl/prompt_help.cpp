// satellite/satl/prompt_help.cpp -- satellite.help at the prompt (prompt_help.hpp).

#include "prompt_help.hpp"

#include "../bytecode/bytecode_registry.hpp"
#include "../bytecode/program_walk.hpp"
#include "../bytecode/word_codes.hpp"
#include "../machine/machine_codes.hpp"
#include "../machine/shown.hpp"

#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite004 {
namespace {

using token::Code;

bool a_folder(const std::string &path)
{
    struct stat seen{};
    return stat(path.c_str(), &seen) == 0 && S_ISDIR(seen.st_mode);
}

// WHERE THE HELP IS: beside satl, or one folder up from it. build/satl in the tree
// finds satellite.help/ at the top of the tree; an installed satl finds the copy
// installed beside it.
std::string help_folder(std::string &looked)
{
    std::string path(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", path.data(), path.size());
    const std::string satl = length > 0 ? path.substr(0, static_cast<std::size_t>(length)) : std::string();
    const std::string beside = satl.substr(0, satl.rfind('/'));
    for (const std::string &folder : {beside + "/satellite.help", beside + "/../satellite.help"}) {
        if (a_folder(folder))
            return folder;
        looked += (looked.empty() ? "" : " and ") + folder;
    }
    return std::string();
}

std::string last_part(const std::string &name)
{
    const std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(dot + 1);
}

// WHICH TOPIC A WORD MEANS. The whole name first -- satellite.include is the folder
// satellite.include -- and then the last part of it, so satellite.help(include)
// finds the same folder the author's usage line promises. A last part that is the
// start of a topic's, or starts with it, counts only when ONE topic answers:
// percentage finds satellite.variable.percent, and `variable` finds nothing rather
// than whichever folder happened to be read first.
std::string topic_folder(const std::string &folder, const std::string &asked)
{
    if (a_folder(folder + "/" + asked))
        return asked;
    const std::string wanted = last_part(asked);
    std::string exact, prefixed;
    int exacts = 0, prefixes = 0;
    if (DIR *listing = opendir(folder.c_str())) {
        while (const dirent *entry = readdir(listing)) {
            const std::string name = entry->d_name;
            if (name == "." || name == ".." || !a_folder(folder + "/" + name))
                continue;
            const std::string part = last_part(name);
            if (part == wanted || name == wanted) { exact = name; ++exacts; }
            else if (part.rfind(wanted, 0) == 0 || wanted.rfind(part, 0) == 0) { prefixed = name; ++prefixes; }
        }
        closedir(listing);
    }
    if (exacts == 1) return exact;
    if (exacts == 0 && prefixes == 1) return prefixed;
    return std::string();
}

signed long long int print_file(const std::string &path)
{
    std::ifstream file(path);
    if (!file) {
        std::cerr << "satl(prompt): the help file " << shown(path) << " could not be read\n";
        return file_unreadable;
    }
    std::ostringstream text;
    text << file.rdbuf();
    std::cout << text.str();
    if (!text.str().empty() && text.str().back() != '\n')
        std::cout << '\n';
    return success;
}

} // namespace

bool answer_help(const std::vector<std::bitset<16>> &row, signed long long int &answer)
{
    const Code word = code_at(row, 0);
    if (word != word::code_of(1, 19) && word != word::code_of(1, 19, 0) && word != word::code_of(1, 19, 1))
        return false;
    std::size_t at = 1;
    if (code_at(row, at) != token::left_parenthesis_token)
        return false;
    ++at;

    // THE TOPIC IS A WORD, NOT A VALUE: `include` is not a declared name and
    // `satellite.include` is not a call, so neither is evaluated -- a name is read as
    // its text, a word of the language as its spelling, and a string as itself.
    std::string asked;
    const Code given = code_at(row, at);
    if (given == token::name_token || given == token::string_token)
        asked = text_at(row, at);
    else if (word::is_word_code(given)) {
        asked = word::spelling_of(given);
        asked = asked.substr(0, asked.find('('));
        ++at;
    }
    if (code_at(row, at) != token::right_parenthesis_token)
        return false;
    ++at;
    const Code after = code_at(row, at);
    if (after != token::line_end_token && after != token::end_of_file_token && after != token::comment_token)
        return false;

    std::string looked;
    const std::string folder = help_folder(looked);
    if (folder.empty()) {
        std::cerr << "satl(prompt): the help files are not beside satl -- looked in " << shown(looked) << "\n";
        answer = directory_not_found;
        return true;
    }
    if (asked.empty()) {
        answer = print_file(folder + "/help.txt");
        return true;
    }
    const std::string topic = topic_folder(folder, asked);
    if (topic.empty()) {
        std::cerr << "satl(prompt): there is no help on " << shown(asked)
                  << " yet -- satellite.help() lists the topics there are\n";
        answer = file_not_found;
        return true;
    }
    answer = print_file(folder + "/" + topic + "/help_text.txt");
    return true;
}

} // namespace satellite004
