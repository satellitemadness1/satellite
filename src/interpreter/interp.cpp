#include "interpreter/interp.hpp"

#include "console_output/console.hpp"
#include "environment/env.hpp"
#include "evaluator/eval.hpp"
#include "spaceship_loader/loader.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "syntax_parser/parser.hpp"
#include "satellite_string/satellite_string.hpp"

#include <cctype>
#include <cmath>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

namespace satellite {
namespace {

// The tree and its resolution have ONE lifetime: resolve() records a pointer
// into the Program for every capsule and every method, and an Object records a
// pointer into the ResolveResult for its spacesuit.
//
// The LoadResult is what the parse used to be, and it holds three things that
// have to stay put: the merged Program those pointers point INTO, the
// SourceMap every error is rendered against, and the errors themselves. It is
// stored by value in an Image that is never moved after resolve() runs, which
// is what makes the pointers safe — the same contract as before §16, with a
// merged Program in place of a single file's.
struct Image {
    LoadResult loaded;
    ResolveResult resolved;
};

// Everything the evaluator produced used to die with the call it was produced
// in. An instance of a spacesuit is the first VALUE that can outlive it: a
// top-level declaration writes it into the process-global satellite.library,
// and the REPL reads it back on the next line — by which time its class, and
// the tree its methods are, would be freed memory. One line is enough to reach
// it, because a spacesuit and a declaration of its type fit on one:
//
//     satellite.spacesuit c() { satellite.public { } } c x
//     x                      -> reads x's spacesuit, already destroyed
//
// So a program that declared a spacesuit keeps its image. This is a retention
// rather than a leak — it is exactly what the live objects point at — but a
// blunt one: it retains whether or not an instance actually escaped. The sharp
// version is the ProgramPtr ast.hpp already anticipates, where an Object holds
// a shared_ptr<const Program> and keeps alive only what is reachable.
std::mutex &image_lock()
{
    static std::mutex lock;
    return lock;
}

std::vector<std::shared_ptr<Image>> &images()
{
    static std::vector<std::shared_ptr<Image>> kept;
    return kept;
}

void retain(std::shared_ptr<Image> image)
{
    std::lock_guard<std::mutex> guard(image_lock());
    images().push_back(std::move(image));
}

// A capsule typed at the prompt survives to the next line.
//
// §16 left this open on purpose -- "Making the REPL accumulate a program across
// lines is its own design question" -- and this is the answer, which turns out
// to be smaller than the question sounds because retain() had already done the
// hard half for spacesuits. What was missing was never lifetime; it was that
// nothing ever READ the retained images back.
//
// It merges the resolved TABLES and not the source text, and that is the
// decision worth recording. Re-parsing an accumulated prelude on every line was
// the obvious alternative and it is wrong in a way the user sees: every error
// on the line they just typed would be reported at a line number counting the
// whole invisible prelude above it. Merging tables leaves the fresh program one
// line long, so `line 1` still means the line under the cursor.
//
// It is safe because a CapsuleInfo's `const Capsule *` points into an Image
// that retain() is holding, and because resolve() stamped its slots into those
// AST nodes when the definition was typed -- so the body walks exactly as it
// did on the line it was written, with a frame sized by its own slot_count.
//
// NEWEST WINS. The fresh resolve is authoritative for every name it defines, so
// re-typing a capsule replaces it; only names the new program does NOT define
// are inherited, and they are searched newest-first so the most recent
// definition of a name shadows every earlier one. Without that, redefining a
// capsule at the prompt would silently keep running the first version.
ResolveResult inherited_tables()
{
    ResolveResult table;
    std::lock_guard<std::mutex> guard(image_lock());
    for (auto it = images().rbegin(); it != images().rend(); ++it) {
        for (const auto &entry : (*it)->resolved.capsules)
            table.capsules.emplace(entry.first, entry.second);
        for (const auto &entry : (*it)->resolved.suits)
            table.suits.emplace(entry.first, entry.second);
    }
    return table;
}

// Collects a parse or eval failure into the same text channel, so a caller
// never has to ask which stage broke.
//
// One template rather than three near-identical overloads, now that all three
// error types render through the same two-argument call. The three format_error
// implementations stay separate — they draw different things — but the loop
// over them never had a reason to be written out three times.
template <typename Error>
void report(std::string &out, const std::vector<Error> &errors,
            const SourceMap &sources)
{
    for (const Error &error : errors)
        out += format_error(error, sources);
}

// satellite.return(satellite) is success and yields 0. A number becomes the
// status.
//
// CLAMPED, not masked. A process exit status carries 8 bits, and `n & 0xff`
// turns return(256) into 0 — reporting success for what the program said was
// failure. Clamping keeps a failure a failure. A non-finite or negative status
// is 1 for the same reason.
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

int status_of(const ValuePtr &returned, bool ok)
{
    if (!ok)
        return 1;
    if (!returned)
        return 0;
    if (const Number *n = std::get_if<Number>(returned.get())) {
        // Truncated toward zero first, so satellite.return(2.7) is a status of
        // 2 rather than a refusal; an exit status is an integer and the
        // program said what it meant.
        long long status = 0;
        if (!n->floor().to_integer(status) || status < 0)
            return 1;
        return status > 255 ? 255 : static_cast<int>(status);
    }
    return 0;
}

} // namespace

List args_to_list(const std::vector<std::string> &args)
{
    List out;
    out.reserve(args.size());
    for (const std::string &arg : args)
        out.push_back(std::make_shared<const Value>(make_string(encode_raw(arg))));
    return out;
}

InterpResult run_source(const std::string &source, const std::string &ns,
                        bool echo, Console *console, bool session)
{
    InterpResult result;

    std::shared_ptr<Image> image = std::make_shared<Image>();
    // A REPL line has no path, so its errors say "line 3" and name no file,
    // and an include in it is resolved from the working directory.
    image->loaded = load(source);
    if (!image->loaded.ok()) {
        // Every syntax error, not just the first: the parser already recovers
        // and keeps going, so burying the rest here would waste that — and the
        // loader keeps going across spaceships for the same reason.
        report(result.output, image->loaded.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    // Resolution is a whole pass of its own, between parsing and the walk:
    // every capsule local becomes a frame slot index here, once, rather than a
    // name looked up on every read (§6), and every spacesuit gets its field
    // layout and its method table. It is also where an unknown variable inside
    // a capsule is caught — before any of the program runs, instead of partway
    // through its output.
    // Built BEFORE resolve() and handed to it, not merged afterwards. A
    // spacesuit type is checked while resolve() runs -- `box b` asks the table
    // for `box` in pass 3 -- so a table filled in after resolve() returned
    // would answer a question that had already been asked and failed.
    const ResolveResult session_tables = session ? inherited_tables()
                                                 : ResolveResult();

    image->resolved = resolve(image->loaded.program,
                              session ? &session_tables : nullptr);
    if (!image->resolved.ok()) {
        report(result.output, image->resolved.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    Evaluator evaluator(image->resolved, ns, echo);
    evaluator.set_console(console);
    evaluator.run(image->loaded.program);

    // Before the report is built, for the reason spelled out in run_program:
    // "after whatever output preceded it" is only true if that output has
    // actually left.
    if (console)
        console->drain();

    result.output = evaluator.output();
    report(result.output, evaluator.errors(), image->loaded.sources);
    result.ok = evaluator.ok();
    result.status = status_of(evaluator.returned(), result.ok);

    // Capsules join spacesuits in earning a retention, and only in a session.
    // A file run keeps the old rule: it is a whole program, and there is no
    // next line for it to be visible from.
    if (!image->resolved.suits.empty() ||
        (session && !image->resolved.capsules.empty()))
        retain(std::move(image));
    return result;
}

InterpResult run_program(const std::string &source,
                         const std::vector<std::string> &args,
                         const std::string &path, Console *console)
{
    InterpResult result;

    std::shared_ptr<Image> image = std::make_shared<Image>();
    image->loaded = load(source, path);
    if (!image->loaded.ok()) {
        report(result.output, image->loaded.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    image->resolved = resolve(image->loaded.program);
    if (!image->resolved.ok()) {
        report(result.output, image->resolved.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    Evaluator evaluator(image->resolved, "main", false);
    evaluator.set_console(console);
    evaluator.run_entry(image->loaded.program, args_to_list(args));

    // Drained BEFORE the error report is built, and that order is the whole
    // reason drain() exists as a separate call rather than being folded into
    // the Console's destructor. A runtime error is reported "after whatever
    // output preceded it" — which is only true if the output has actually left
    // by the time the caller prints the report.
    if (console)
        console->drain();

    result.output = evaluator.output();
    report(result.output, evaluator.errors(), image->loaded.sources);
    result.ok = evaluator.ok();
    result.status = status_of(evaluator.returned(), result.ok);

    if (!image->resolved.suits.empty())
        retain(std::move(image));
    return result;
}

InterpResult run_file(const std::string &path,
                      const std::vector<std::string> &args, Console *console)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        InterpResult result;
        result.output = "satellite: cannot read " + path + "\n";
        result.status = 2;
        return result;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();

    // argv[0] is the program, so argz[0] is the script.
    std::vector<std::string> full;
    full.reserve(args.size() + 1);
    full.push_back(path);
    full.insert(full.end(), args.begin(), args.end());

    // The path goes to run_program as well as into argz, so an error names the
    // file it happened in instead of a bare line number. That is §16's payoff
    // arriving early: it needs no loader, only a Span that can carry a file id.
    return run_program(buffer.str(), full, path, console);
}

std::string eval_line(const std::string &line, Console *console)
{
    return run_source(line, "main", true, console, true).output;
}

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
