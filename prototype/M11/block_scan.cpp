// Multi-line block scanner and command line parser implementation.
// Milestone 11 Prototype in prototype/M11.

#include "block_scan.hpp"
#include "lexer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace satellite {

namespace {

bool split_words(const std::string &line, std::vector<std::string> &out)
{
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && isspace(static_cast<unsigned char>(line[i])))
            i++;
        if (i >= line.size())
            break;

        std::string word;
        char quote = 0;
        for (; i < line.size(); i++) {
            char c = line[i];
            if (quote) {
                if (c == quote)
                    quote = 0;
                else
                    word += c;
            } else if (c == '"' || c == '\'') {
                quote = c;
            } else if (isspace(static_cast<unsigned char>(c))) {
                break;
            } else {
                word += c;
            }
        }
        if (quote)
            return false;
        out.push_back(word);
    }
    return true;
}

} // namespace

RunCommand parse_run_command(const std::string &line)
{
    RunCommand command;

    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos)
        return command;
    size_t end = line.find_first_of(" \t", start);
    std::string verb = line.substr(start, end == std::string::npos
                                              ? std::string::npos
                                              : end - start);
    if (verb != "run" && verb != "interpret" && verb != "--run")
        return command;
    command.matched = true;

    std::vector<std::string> words;
    if (end != std::string::npos && !split_words(line.substr(end), words)) {
        command.error = "satellite: unterminated quote in " + verb + "\n";
        return command;
    }
    if (words.empty()) {
        command.error = "usage: " + verb + " <file> [args]\n";
        return command;
    }

    command.path = words.front();
    command.args.assign(words.begin() + 1, words.end());
    return command;
}

bool exit_command(const std::string &line, int &status)
{
    size_t a = line.find_first_not_of(" \t");
    if (a == std::string::npos)
        return false;
    size_t b = line.find_last_not_of(" \t");
    const std::string s = line.substr(a, b - a + 1);

    if (s == "exit" || s == "quit" || s == "exit()" || s == "quit()") {
        status = 0;
        return true;
    }

    const std::string keyword = "satellite.return";
    if (s.compare(0, keyword.size(), keyword) != 0)
        return false;

    std::string rest = s.substr(keyword.size());
    if (rest.empty()) {
        status = 0;
        return true;
    }
    if (rest.front() != '(' || rest.back() != ')')
        return false;
    rest = rest.substr(1, rest.size() - 2);

    std::string arg;
    for (char c : rest)
        if (c != ' ' && c != '\t')
            arg += c;

    status = 0;
    if (!arg.empty() && arg != "satellite" && arg.find_first_not_of("0123456789") == std::string::npos) {
        status = static_cast<int>(strtol(arg.c_str(), nullptr, 10) & 0xff);
    }
    return true;
}

void abandon_block(std::string &block, int &depth)
{
    const long lines = std::count(block.begin(), block.end(), '\n');
    std::printf("satellite: multi-line entry abandoned, %ld line%s discarded\n",
                lines, lines == 1 ? "" : "s");
    std::fflush(stdout);
    block.clear();
    depth = 0;
}

BlockScan scan_block(const std::string &line)
{
    BlockScan scan;

    const std::vector<Token> tokens = lex(line);

    bool saw_brace = false;
    for (const Token &token : tokens) {
        if (token.kind == TokenKind::Error) {
            scan.lex_error = true;
            return scan;
        }
        if (token.kind != TokenKind::Punct)
            continue;
        if (token.text == "{") {
            scan.depth++;
            saw_brace = true;
        } else if (token.text == "}") {
            scan.depth--;
            saw_brace = true;
        }
    }

    if (!saw_brace && tokens.size() >= 3 &&
        tokens[0].kind == TokenKind::Word && tokens[0].text == "satellite" &&
        tokens[1].kind == TokenKind::Punct && tokens[1].text == "." &&
        tokens[2].kind == TokenKind::Word &&
        (tokens[2].text == "capsule" || tokens[2].text == "spacesuit" ||
         tokens[2].text == "statement" || tokens[2].text == "protected" ||
         tokens[2].text == "public")) {
        scan.opens_body = true;
        scan.depth = 1;
    }

    return scan;
}

} // namespace satellite

