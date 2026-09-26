#include "interpreter/interp.hpp"

#include "lexical_analyzer/lexer.hpp"

#include <cctype>
#include <string>
#include <vector>

// What the prompt does to a raw line BEFORE deciding to evaluate it, out of
// interp.cpp: reading a `run <file>` command off it (parse_run_command, with
// split_words), and counting whether it left a block open (scan_block).
//
// Neither half touches the retained-image machinery interp.cpp keeps, and
// neither runs anything -- which is why this is the piece that carries no
// evaluator include at all.

namespace satellite {
namespace {

// Splits the tail of a `run` line into words. Quotes group; a backslash is an
// ordinary character, because the word most likely to follow the verb is a
// path. Returns false on an unterminated quote rather than silently running
// whatever the truncated word happens to name.
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

    // The verb is read off the raw line before any quote handling, so that a
    // line with a bad quote still gets the run-command error rather than being
    // handed to the parser as source.
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


// ---------------------------------------------------------------------------
// Multi-line entry at the prompt.
//
// A brace-depth counter over tokens is a COMPLETE trigger for every multi-line
// form the language has: a capsule body, a spacesuit body, an access block, a
// constructor, a method, and every satellite.statement.if / else / while / for
// block. They all bottom out in the same braces, so none of them needs a rule
// of its own here.
//
// What the prompt SUPPLIES a brace for is every head whose body is a block:
// satellite.capsule, satellite.spacesuit, satellite.protected, satellite.public
// and every satellite.statement form. All five are segment-1 dispatch keys in
// §5's table, which is why one test over segment 1 covers them and why adding a
// sixth would be one word here rather than a rule.
//
// The test is `!saw_brace` first. Somebody who typed the whole construct on one
// line, brace and all, has said exactly what they meant, and a prompt that
// added to that would be rewriting working input.
// ---------------------------------------------------------------------------

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

    // A declaration head is three tokens, and this is the parser's own test --
    // Parser::at_language_path is `Word(satellite) Punct(.) Word(<segment>)`.
    // Repeating its SHAPE here rather than calling it keeps the prompt out of
    // the parser, and the shape is stable because §1 fixes it: a language-owned
    // name is a dotted path rooted at the one reserved word.
    //
    // `saw_brace` is what keeps this from firing on a one-line capsule that the
    // user closed themselves. Somebody who types the whole thing on one line
    // has said what they meant, and the prompt must not add to it.
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
