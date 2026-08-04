// The satellite command-line tool.
//
// Version 001 is preliminary: there is no parser and no VM yet, so a .satl
// file cannot be executed.  What this tool can honestly do today is lex a file
// and report on it, and execute the satellite.cxx blocks inside it -- that
// path is complete end to end.
//
//   satellite version
//   satellite check   file.satl     lex it, report errors, exit non-zero on any
//   satellite lex     file.satl     dump the token stream
//   satellite run     file.satl     execute every satellite.cxx block in it
#include "satellite/gui.hpp"
#include "satellite/jit.hpp"
#include <chrono>
#include "satellite/container.hpp"
#include "satellite/cxx.hpp"
#include "satellite/lexer.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace satellite;

static const char* kVersion = "001";

static bool read_file(const std::string& path, std::string* out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    *out = ss.str();
    return true;
}

static void report_errors(const Lexer& lx, const std::string& path) {
    for (const auto& e : lx.errors())
        std::fprintf(stderr, "%s:%d:%d: error: %s\n",
                     path.c_str(), e.line, e.col, e.message.c_str());
}

// ---------------------------------------------------------------------------
static int cmd_check(const std::string& path) {
    std::string src;
    if (!read_file(path, &src)) {
        std::fprintf(stderr, "cannot read %s\n", path.c_str());
        return 2;
    }
    Lexer lx(src);
    auto ts = lx.scan();
    report_errors(lx, path);

    int sat = 0, ident = 0, str = 0, num = 0, blocks = 0, stmts = 0;
    for (const auto& t : ts) {
        switch (t.kind) {
            case Tok::Satellite: ++sat;    break;
            case Tok::Ident:     ++ident;  break;
            case Tok::Str:       ++str;    break;
            case Tok::Int:
            case Tok::Real:      ++num;    break;
            case Tok::CxxBlock:  ++blocks; break;
            case Tok::Term:      ++stmts;  break;
            default: break;
        }
    }

    std::printf("%s: %s\n", path.c_str(), lx.ok() ? "ok" : "FAILED");
    std::printf("  %zu tokens, %d statements\n", ts.size(), stmts);
    std::printf("  %d satellite keywords, %d identifiers\n", sat, ident);
    std::printf("  %d strings, %d numbers, %d cxx blocks\n", str, num, blocks);
    return lx.ok() ? 0 : 1;
}

// ---------------------------------------------------------------------------
static int cmd_lex(const std::string& path) {
    std::string src;
    if (!read_file(path, &src)) {
        std::fprintf(stderr, "cannot read %s\n", path.c_str());
        return 2;
    }
    Lexer lx(src);
    auto ts = lx.scan();

    for (const auto& t : ts) {
        if (t.kind == Tok::End) break;
        std::printf("%4d:%-3d  %-10s", t.line, t.col, tok_name(t.kind));
        switch (t.kind) {
            case Tok::Ident:
            case Tok::Int:
            case Tok::Real:
                std::printf("  %s", t.text.c_str());
                break;
            case Tok::Str:
                std::printf("  \"%s\"", t.text.c_str());
                break;
            case Tok::CxxBlock:
                std::printf("  <%zu bytes of C++>", t.text.size());
                break;
            default: break;
        }
        std::printf("\n");
    }
    report_errors(lx, path);
    return lx.ok() ? 0 : 1;
}

// ---------------------------------------------------------------------------
static int cmd_run(const std::string& path) {
    std::string src;
    if (!read_file(path, &src)) {
        std::fprintf(stderr, "cannot read %s\n", path.c_str());
        return 2;
    }
    Lexer lx(src);
    auto ts = lx.scan();
    if (!lx.ok()) {
        report_errors(lx, path);
        return 1;
    }

    std::vector<const Token*> blocks;
    for (const auto& t : ts)
        if (t.kind == Tok::CxxBlock) blocks.push_back(&t);

    if (blocks.empty()) {
        std::printf("%s lexes cleanly, but contains no satellite.cxx blocks.\n",
                    path.c_str());
        std::printf("Version %s has no parser or VM yet, so there is nothing "
                    "else to execute.\n", kVersion);
        return 0;
    }

    cxx::Bridge bridge;
    int failures = 0;
    for (size_t k = 0; k < blocks.size(); ++k) {
        std::printf("--- satellite.cxx block %zu (line %d) ---\n",
                    k + 1, blocks[k]->line);
        // stderr is unbuffered and stdout is not when either is a pipe, so
        // without this an error overtakes the header it belongs under and ends
        // up filed against the previous block.
        std::fflush(stdout);
        std::string err;

        std::vector<cxx::CxxArg> args;
        if (!blocks[k]->args.empty() &&
            !cxx::parse_args(blocks[k]->args, &args, &err)) {
            std::fprintf(stderr, "  arguments: %s\n", err.c_str());
            ++failures;
            continue;
        }

        // No capture here on purpose: a command line HAS a terminal, so the
        // block's own output belongs on it directly, interleaved as it happens
        // rather than replayed afterwards.
        cxx::Timing tm;
        Container r = bridge.run(blocks[k]->text, args, &err, nullptr, &tm);
        if (!err.empty()) {
            std::fprintf(stderr, "%s\n", err.c_str());
            ++failures;
            continue;
        }
        std::printf("  => %s   [%s]\n", r.to_string().c_str(), type_name(r.type));
        std::printf("     %s in %.1f ms, ran in %.3f ms\n",
                    tm.cached ? "cached" : "compiled", tm.compile_ms, tm.run_ms);
    }
    std::printf("\n%zu block(s), %zu compiled, %zu from cache, %d failed\n",
                blocks.size(), bridge.compiles(), bridge.cache_hits(), failures);
    return failures ? 1 : 0;
}

// ---------------------------------------------------------------------------
static void usage() {
    std::printf(
        "satellite %s -- preliminary build\n"
        "\n"
        "  satellite version\n"
        "  satellite check <file.satl>   lex the file and report on it\n"
        "  satellite lex   <file.satl>   dump the token stream\n"
        "  satellite run   <file.satl>   execute the satellite.cxx blocks\n"
        "  satellite jit   \"<c++>\"       compile and run C++ in-process\n"
        "  satellite cxx-config          show the satellite.cxx settings\n"
        "\n"
        "What works in %s: the value type (satellite_container), strings with\n"
        "the satellite charmap, big integers, the lexer, synthetic keyboard\n"
        "input, and inline C++ via satellite.cxx { }.\n"
        "\n"
        "What does NOT work yet: there is no parser and no virtual machine, so\n"
        "ordinary satellite code cannot be executed.  `run` executes only the\n"
        "satellite.cxx blocks in a file.\n",
        kVersion, kVersion);
}

int main(int argc, char** argv) {
    init_tables();

    // No arguments means the console.  `satellite check foo.satl` stays a
    // command-line tool, so one binary serves both and every shipped example
    // keeps working.
    if (argc < 2) {
        if (gui_available()) return gui_main(argc, argv);
        usage();
        return 1;
    }
    std::string cmd = argv[1];

    // ...unless the console is asked for by name, which is how you get it on a
    // build where you also want to pass arguments.
    if (cmd == "console" || cmd == "gui") {
        if (gui_available()) return gui_main(argc - 1, argv + 1);
        std::fprintf(stderr, "this build has no GUI console (GTK4 was not found)\n");
        return 1;
    }

    if (cmd == "version" || cmd == "--version" || cmd == "-v") {
        std::printf("%s\n", kVersion);
        return 0;
    }
    if (cmd == "help" || cmd == "--help" || cmd == "-h") { usage(); return 0; }

    // Every tunable, its value, and the variable that overrides it.  Running C++
    // is a large part of what satellite does, so the settings that govern it
    // should be inspectable without reading the source.
    if (cmd == "cxx-config") {
        std::printf("%s", cxx::default_config().describe().c_str());
        return 0;
    }

    // `satellite jit "return 6*7;"` -- the embedded compiler from the command
    // line, so the timing is scriptable and does not need a window.
    if (cmd == "jit") {
        if (!jit::available()) {
            std::fprintf(stderr, "this build has no embedded compiler\n");
            return 1;
        }
        if (argc < 3) {
            std::fprintf(stderr, "usage: satellite jit \"<c++ body>\"\n");
            return 1;
        }
        std::string body;
        for (int i = 2; i < argc; ++i) {
            if (i > 2) body += " ";
            body += argv[i];
        }
        std::string  err, printed;
        jit::Timing  t;
        Container    v = jit::run(body, &err, &printed, &t);
        if (!err.empty()) {
            std::fprintf(stderr, "%s\n", err.c_str());
            v.release();
            return 1;
        }
        std::fputs(printed.c_str(), stdout);
        std::printf("=> %s [%s]\n", v.to_string().c_str(), type_name(v.type));
        std::printf("compiled in %.2f ms, ran in %.3f ms (total %.2f ms)\n",
                    t.compile_ms, t.run_ms, t.total_ms());
        v.release();
        return 0;
    }

    if (argc < 3) {
        std::fprintf(stderr, "%s needs a file\n", cmd.c_str());
        return 1;
    }
    std::string path = argv[2];

    if (cmd == "check") return cmd_check(path);
    if (cmd == "lex")   return cmd_lex(path);
    if (cmd == "run")   return cmd_run(path);

    std::fprintf(stderr, "unknown command: %s\n", cmd.c_str());
    usage();
    return 1;
}
