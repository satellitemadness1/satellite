// satellite.analyze(path) — what is IN a spaceship, counted rather than
// estimated.
//
// A file of its own, for two reasons. modules.cpp is dispatch and this is a
// walk, and modules.cpp had already gone past the 400-line mark this tree sets
// for itself.
//
// It LEXES the file rather than scanning its text, and that is the whole
// reason the counts can be believed. `satellite.spacesuit` inside a string
// literal is one String token, and the same words in a comment are not tokens
// at all, so neither is counted -- while a grep over the same file counts
// both, and would report a spacesuit in any program that merely prints the
// word. The lexer is also what makes `satellite . spacesuit` spread across a
// line break count once, since whitespace stops existing at this level.
//
// What it does NOT do is parse. A count of what a file declares does not need
// a tree, and needing one would mean a file with a syntax error could not be
// analysed at all -- which is the file most worth asking about.

#include "evaluator/eval_internal.hpp"

#include "lexical_analyzer/lexer.hpp"

namespace satellite {
namespace {

bool word_is(const std::vector<Token> &tokens, size_t i, const char *text)
{
    return i < tokens.size() && tokens[i].kind == TokenKind::Word &&
           tokens[i].text == text;
}

bool punct_is(const std::vector<Token> &tokens, size_t i, const char *text)
{
    return i < tokens.size() && tokens[i].kind == TokenKind::Punct &&
           tokens[i].text == text;
}

// A satellite-rooted path at i: `satellite . <second>`. Answers the index just
// past what matched, or i itself for no match -- so a caller tests `j != i`
// and never has to invent a sentinel index that a real match could collide
// with at the top of a file.
size_t after_path(const std::vector<Token> &tokens, size_t i, const char *second)
{
    if (!word_is(tokens, i, "satellite") || !punct_is(tokens, i + 1, ".") ||
        !word_is(tokens, i + 2, second))
        return i;
    return i + 3;
}

// The same with the third segment left open: `satellite . <second> . <word>`.
// Open on purpose -- satellite.statement.else is as much a statement form as
// .if is, and a list of the four spelled out here would be a list that goes
// stale the day a fifth lands.
size_t after_path_any(const std::vector<Token> &tokens, size_t i,
                      const char *second)
{
    if (!word_is(tokens, i, "satellite") || !punct_is(tokens, i + 1, ".") ||
        !word_is(tokens, i + 2, second) || !punct_is(tokens, i + 3, ".") ||
        i + 4 >= tokens.size() || tokens[i + 4].kind != TokenKind::Word)
        return i;
    return i + 5;
}

// Past a generic argument list, if one starts here: the `<...>` on a
// satellite.container.list<satellite.variable.string>. `>>` closes two,
// because the lexer hands the two-character punctuation back whole and the
// parser is what splits it later (lexer.hpp's split_punct) -- so a list of
// lists would otherwise never close and the name after it would go uncounted.
size_t after_generics(const std::vector<Token> &tokens, size_t i)
{
    if (!punct_is(tokens, i, "<"))
        return i;
    int depth = 0;
    for (size_t j = i; j < tokens.size(); j++) {
        if (tokens[j].kind != TokenKind::Punct)
            continue;
        const std::string &p = tokens[j].text;
        if (p == "<")
            depth++;
        else if (p == ">")
            depth--;
        else if (p == ">>")
            depth -= 2;
        if (depth <= 0)
            return j + 1;
    }
    return i;
}

// A type followed by a NAME is a declaration; a type followed by anything else
// is a type being mentioned. That one rule is what keeps the inner
// satellite.variable.string of a
// satellite.container.list<satellite.variable.string> out of the count: it is
// followed by `>`, not by a name.
bool declares_a_name(const std::vector<Token> &tokens, size_t after_type)
{
    const size_t k = after_generics(tokens, after_type);
    return k < tokens.size() && tokens[k].kind == TokenKind::Word;
}

const char *yes_no(bool yes)
{
    return yes ? "TRUE" : "FALSE";
}

} // namespace

std::string analyze_spaceship(const std::string &path, std::string &error)
{
    error.clear();

    std::string source;
    if (FILE *f = fopen(path.c_str(), "rb")) {
        char buf[65536];
        size_t got;
        while ((got = fread(buf, 1, sizeof buf, f)) > 0)
            source.append(buf, got);
        fclose(f);
    } else {
        error = "cannot read";
        return std::string();
    }

    const std::vector<Token> tokens = lex(source);

    bool has_include = false;
    bool has_main = false;
    bool has_return = false;
    long spacesuits = 0;
    long statements = 0;
    long variables = 0;
    std::string note;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].kind == TokenKind::Error) {
            // Reported, and the counts are kept. A file that does not lex is
            // still a file someone wants the shape of, and saying how far it
            // got is more use than refusing to answer.
            note = "lexing stopped at line " + std::to_string(tokens[i].line) +
                   " -- " + tokens[i].text;
            break;
        }

        // satellite.include(satellite): the runtime included, which is the
        // ceremony every program opens with. An include of a user spaceship is
        // deliberately not this question.
        size_t j = after_path(tokens, i, "include");
        if (j != i && punct_is(tokens, j, "(") &&
            word_is(tokens, j + 1, "satellite") && punct_is(tokens, j + 2, ")"))
            has_include = true;

        // satellite.main, wherever it is spelled -- the declaration is
        // `satellite.capsule satellite.main(...)`, so the name is what is
        // looked for and not the capsule in front of it.
        if (after_path(tokens, i, "main") != i)
            has_main = true;

        j = after_path(tokens, i, "return");
        if (j != i && punct_is(tokens, j, "(") &&
            word_is(tokens, j + 1, "satellite") && punct_is(tokens, j + 2, ")"))
            has_return = true;

        if (after_path(tokens, i, "spacesuit") != i)
            spacesuits++;

        if (after_path_any(tokens, i, "statement") != i)
            statements++;

        j = after_path_any(tokens, i, "variable");
        if (j != i && declares_a_name(tokens, j))
            variables++;

        // A container counts as two. It is one name and two things to hold in
        // your head -- the container and what is in it -- and a file of twenty
        // lists is not the same size of thing as a file of twenty numbers.
        j = after_path_any(tokens, i, "container");
        if (j != i && declares_a_name(tokens, j))
            variables += 2;
    }

    std::string out;
    out += "file: " + path + "\n";
    out += "has_satellite_include_satellite: ";
    out += yes_no(has_include);
    out += "\nhas_satellite_main: ";
    out += yes_no(has_main);
    out += "\nhas_satellite_return_satellite: ";
    out += yes_no(has_return);
    out += "\nspacesuits: " + std::to_string(spacesuits);
    out += "\nstatements: " + std::to_string(statements);
    out += "\nvariables: " + std::to_string(variables);
    out += "\n";
    if (!note.empty())
        out += "note: " + note + "\n";
    return out;
}

} // namespace satellite
