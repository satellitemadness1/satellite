#include "interp.hpp"

#include "env.hpp"
#include "eval.hpp"
#include "parser.hpp"
#include "satellite_string.hpp"

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
struct Image {
    ParseResult parsed;
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
void retain(std::shared_ptr<Image> image)
{
    static std::mutex lock;
    static std::vector<std::shared_ptr<Image>> images;
    std::lock_guard<std::mutex> guard(lock);
    images.push_back(std::move(image));
}

// Collects a parse or eval failure into the same text channel, so a caller
// never has to ask which stage broke.
void report(std::string &out, const std::vector<ParseError> &errors,
            const std::string &source)
{
    for (const ParseError &error : errors)
        out += format_error(error, source);
}

void report(std::string &out, const std::vector<EvalError> &errors,
            const std::string &source)
{
    for (const EvalError &error : errors)
        out += format_error(error, source);
}

void report(std::string &out, const std::vector<ResolveError> &errors,
            const std::string &source)
{
    for (const ResolveError &error : errors)
        out += format_error(error, source);
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
                        bool echo)
{
    InterpResult result;

    std::shared_ptr<Image> image = std::make_shared<Image>();
    image->parsed = parse(source);
    if (!image->parsed.ok()) {
        // Every syntax error, not just the first: the parser already recovers
        // and keeps going, so burying the rest here would waste that.
        report(result.output, image->parsed.errors, source);
        result.status = 1;
        return result;
    }

    // Resolution is a whole pass of its own, between parsing and the walk:
    // every capsule local becomes a frame slot index here, once, rather than a
    // name looked up on every read (§6), and every spacesuit gets its field
    // layout and its method table. It is also where an unknown variable inside
    // a capsule is caught — before any of the program runs, instead of partway
    // through its output.
    image->resolved = resolve(image->parsed.program);
    if (!image->resolved.ok()) {
        report(result.output, image->resolved.errors, source);
        result.status = 1;
        return result;
    }

    Evaluator evaluator(image->resolved, ns, echo);
    evaluator.run(image->parsed.program);

    result.output = evaluator.output();
    report(result.output, evaluator.errors(), source);
    result.ok = evaluator.ok();
    result.status = status_of(evaluator.returned(), result.ok);

    if (!image->resolved.suits.empty())
        retain(std::move(image));
    return result;
}

InterpResult run_program(const std::string &source,
                         const std::vector<std::string> &args)
{
    InterpResult result;

    std::shared_ptr<Image> image = std::make_shared<Image>();
    image->parsed = parse(source);
    if (!image->parsed.ok()) {
        report(result.output, image->parsed.errors, source);
        result.status = 1;
        return result;
    }

    image->resolved = resolve(image->parsed.program);
    if (!image->resolved.ok()) {
        report(result.output, image->resolved.errors, source);
        result.status = 1;
        return result;
    }

    Evaluator evaluator(image->resolved, "main", false);
    evaluator.run_entry(image->parsed.program, args_to_list(args));

    result.output = evaluator.output();
    report(result.output, evaluator.errors(), source);
    result.ok = evaluator.ok();
    result.status = status_of(evaluator.returned(), result.ok);

    if (!image->resolved.suits.empty())
        retain(std::move(image));
    return result;
}

InterpResult run_file(const std::string &path,
                      const std::vector<std::string> &args)
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

    return run_program(buffer.str(), full);
}

std::string eval_line(const std::string &line)
{
    return run_source(line, "main", true).output;
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

} // namespace satellite
