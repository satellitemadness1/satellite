// Tests for the interpreter front door.
//
// The point of most of these is to pin down the HONESTY of the result: a file
// that validates must not claim to have run, and the only thing that really
// executes is a satellite.cxx block.
#include "satellite/interpret.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using namespace satellite;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

static void eqs(const std::string& got, const std::string& want,
                const std::string& what) {
    ++checks;
    if (got != want) {
        ++failures;
        printf("  FAIL  %s\n        want [%s]\n        got  [%s]\n",
               what.c_str(), want.c_str(), got.c_str());
    }
}

// ---------------------------------------------------------------------------
static std::string tmp_dir() {
    const char* t = std::getenv("TMPDIR");
    std::string d = std::string(t ? t : "/tmp") + "/satellite-interpret-test";
    ::mkdir(d.c_str(), 0755);
    return d;
}

static void write_file(const std::string& path, const std::string& body) {
    std::ofstream f(path, std::ios::binary);
    f << body;
}

static bool has_error_containing(const InterpretResult& r, const std::string& s) {
    for (const auto& d : r.diagnostics)
        if (d.is_error && d.message.find(s) != std::string::npos) return true;
    return false;
}

static bool has_note_containing(const InterpretResult& r, const std::string& s) {
    for (const auto& d : r.diagnostics)
        if (!d.is_error && d.message.find(s) != std::string::npos) return true;
    return false;
}

// a minimal file that satisfies every structural requirement
static const char* kGood =
    "satellite.include(satellite)\n"
    "\n"
    "satellite.capsule satellite.main("
    "satellite.container.list<satellite.variable.string> arguments)\n"
    "{\n"
    "    satellite.console.display(arguments[0])\n"
    "    satellite.return(satellite)\n"
    "}\n";

int main() {
    init_tables();
    printf("interpreter tests\n");

    // =======================================================================
    // stage names
    // =======================================================================
    eqs(stage_name(Stage::Lex),      "lex",      "stage name: lex");
    eqs(stage_name(Stage::Validate), "validate", "stage name: validate");
    eqs(stage_name(Stage::Parse),    "parse",    "stage name: parse");
    eqs(stage_name(Stage::Compile),  "compile",  "stage name: compile");
    eqs(stage_name(Stage::Execute),  "execute",  "stage name: execute");

    // =======================================================================
    // diagnostic formatting
    // =======================================================================
    {
        Diagnostic d{"bad thing", 12, 4, true};
        eqs(d.format("hello.satl"), "hello.satl:12:4: error: bad thing",
            "an error formats like a compiler message");
        Diagnostic w{"just saying", 1, 1, false};
        eqs(w.format("hello.satl"), "hello.satl:1:1: warning: just saying",
            "a non-error formats as a warning");
    }

    // =======================================================================
    // has_errors / error_count
    // =======================================================================
    {
        InterpretResult r;
        check(!r.has_errors(), "an empty result has no errors");
        check(r.error_count() == 0, "and counts zero");
        r.diagnostics.push_back({"note", 1, 1, false});
        check(!r.has_errors(), "a note is not an error");
        r.diagnostics.push_back({"boom", 2, 1, true});
        r.diagnostics.push_back({"bang", 3, 1, true});
        check(r.has_errors(), "an error is an error");
        check(r.error_count() == 2, "notes are not counted");
    }

    // =======================================================================
    // path resolution
    // =======================================================================
    std::string dir = tmp_dir();
    {
        std::string exact = dir + "/exact.satl";
        write_file(exact, kGood);

        eqs(resolve_satl_path(exact), exact,
            "a path that exists resolves to itself");

        // `interpret hello` finds hello.satl
        std::string stem = dir + "/exact";
        eqs(resolve_satl_path(stem), exact,
            "a bare stem gets .satl appended");

        eqs(resolve_satl_path(dir + "/nope"), "",
            "an unresolvable path is empty");
        eqs(resolve_satl_path(""), "", "an empty path resolves to nothing");
        eqs(resolve_satl_path(dir), "",
            "a directory is not a satellite source file");

        // an extensionless file wins over the .satl one, being tried first
        std::string bare = dir + "/bare";
        write_file(bare, kGood);
        write_file(bare + ".satl", kGood);
        eqs(resolve_satl_path(bare), bare,
            "the path exactly as typed is tried first");
    }

    // =======================================================================
    // structural checks against the real lexer
    // =======================================================================
    {
        Lexer lx("satellite.include(satellite)\n");
        auto ts = lx.scan();
        check(lx.ok(), "the include line lexes cleanly");
        check(declares_satellite_include(ts), "the include is recognised");
        check(!declares_main(ts), "and it is not a main capsule");

        // the inner `satellite` follows '(' not '.', so it stays the keyword
        int keywords = 0, idents = 0;
        for (const auto& t : ts) {
            if (t.kind == Tok::Satellite) ++keywords;
            if (t.kind == Tok::Ident)     ++idents;
        }
        check(keywords == 2,
              "both occurrences of satellite are the reserved word");
        check(idents == 1, "only `include` is an identifier -- it follows a dot");
    }
    {
        Lexer lx("satellite.include(other)\n");
        auto ts = lx.scan();
        check(!declares_satellite_include(ts),
              "including something else is not the required include");
    }
    {
        Lexer lx("x.include(satellite)\n");
        auto ts = lx.scan();
        check(!declares_satellite_include(ts),
              "the keyword must lead the sequence");
    }
    {
        Lexer lx(kGood);
        auto ts = lx.scan();
        check(lx.ok(), "the sample program lexes cleanly");
        check(declares_satellite_include(ts), "sample declares the include");
        check(declares_main(ts), "sample declares main");
    }
    {
        Lexer lx("satellite.capsule satellite.helper()\n");
        auto ts = lx.scan();
        check(!declares_main(ts), "a non-main capsule is not main");
    }
    {
        std::vector<Token> none;
        check(!declares_satellite_include(none), "an empty stream declares nothing");
        check(!declares_main(none), "an empty stream has no main");
    }

    // =======================================================================
    // interpret_source: the happy path stops honestly at validate
    // =======================================================================
    {
        InterpretOptions o;
        o.require_main = true;
        InterpretResult r = interpret_source(kGood, "good.satl", o);
        check(r.ok, "a well formed file validates: " + r.summary);
        check(r.reached == Stage::Validate, "and reaches validate");
        check(!r.has_errors(), "with no errors");
        check(r.error_count() == 0, "error_count agrees");
        eqs(r.file, "good.satl", "the name is carried through");
        check(has_note_containing(r, "no parser"),
              "a note admits there is no parser or VM");
        check(has_note_containing(r, "not executed"),
              "the note says the statements were not executed");
        check(r.summary.find("stopped at validate") != std::string::npos,
              "the summary says where it stopped: " + r.summary);
        check(r.summary.find("ok") == 0, "and starts with ok");
        check(r.output.empty(), "nothing ran, so there is no output");
    }

    // =======================================================================
    // the required include
    // =======================================================================
    {
        InterpretResult r = interpret_source(
            "satellite.capsule satellite.main()\n{\n}\n", "noinc.satl");
        check(!r.ok, "a main file without the include fails");
        check(r.reached == Stage::Validate, "it fails at validate, not lex");
        check(has_error_containing(r, "satellite.include(satellite)"),
              "the error names the missing include");
        check(r.error_count() == 1, "exactly one error");
        check(r.diagnostics[0].line == 1, "pointed at line 1");
    }
    {
        InterpretOptions o;
        o.require_main_include = false;
        InterpretResult r = interpret_source(
            "satellite.capsule satellite.main()\n{\n}\n", "noinc.satl", o);
        check(r.ok, "the include check can be turned off: " + r.summary);
    }

    // =======================================================================
    // the required main capsule
    // =======================================================================
    {
        InterpretOptions o;
        o.require_main = true;
        InterpretResult r =
            interpret_source("satellite.include(satellite)\n", "nomain.satl", o);
        check(!r.ok, "requiring main, a file without one fails");
        check(r.reached == Stage::Validate, "at validate");
        check(has_error_containing(r, "main"), "the error names main");
    }
    {
        // main is NOT required by default -- a library file is legal
        InterpretResult r =
            interpret_source("satellite.include(satellite)\n", "lib.satl");
        check(r.ok, "main is not required by default: " + r.summary);
    }

    // =======================================================================
    // lex errors surface
    // =======================================================================
    {
        InterpretResult r = interpret_source(
            "satellite.include(satellite)\nx = \"unterminated\n", "bad.satl");
        check(!r.ok, "a lex error fails the file");
        check(r.reached == Stage::Lex, "and stops at lex");
        check(r.has_errors(), "with errors reported");
        check(r.diagnostics[0].line == 2, "the error keeps its line number");
        check(r.diagnostics[0].is_error, "it is an error, not a note");
        check(!has_note_containing(r, "no parser"),
              "no validate note when we never got to validate");
        check(r.summary.find("lex") != std::string::npos,
              "the summary mentions lex: " + r.summary);
    }

    // =======================================================================
    // satellite.cxx blocks actually execute -- this is the one real path
    // =======================================================================
    {
        InterpretResult r = interpret_source(
            "satellite.include(satellite)\n"
            "satellite.cxx {\n"
            "    int total = 0;\n"
            "    for (int k = 1; k <= 100; ++k) total += k;\n"
            "    satellite.return(total)\n"
            "}\n",
            "one.satl");
        check(r.ok, "a file with a cxx block validates: " + r.summary);
        check(r.output.size() == 1, "one output line for one block");
        if (r.output.size() == 1) {
            const std::string& line = r.output[0];
            check(line.find("cxx block 1") == 0,
                  "the line is numbered from 1: " + line);
            check(line.find("(line 2)") != std::string::npos,
                  "and carries the source line: " + line);
            check(line.find("=> 5050") != std::string::npos,
                  "the block really ran: " + line);
            check(line.find("[int]") != std::string::npos,
                  "and the value's type is shown: " + line);
        }
        check(r.summary.find("1 cxx block") != std::string::npos,
              "the summary counts the block: " + r.summary);
    }
    {
        // two blocks, numbered in order
        InterpretResult r = interpret_source(
            "satellite.include(satellite)\n"
            "satellite.cxx { satellite.return(1) }\n"
            "satellite.cxx { #include <string>\n"
            "std::string s = \"two\";\n"
            "satellite.return(s) }\n",
            "two.satl");
        check(r.ok, "two blocks both run: " + r.summary);
        check(r.output.size() == 2, "two output lines");
        if (r.output.size() == 2) {
            check(r.output[0].find("cxx block 1") == 0, "first is block 1");
            check(r.output[1].find("cxx block 2") == 0, "second is block 2");
            check(r.output[1].find("=> two") != std::string::npos,
                  "the second block's string comes back: " + r.output[1]);
        }
    }
    {
        // blocks can be switched off without touching the file
        InterpretOptions o;
        o.run_cxx_blocks = false;
        InterpretResult r = interpret_source(
            "satellite.include(satellite)\n"
            "satellite.cxx { satellite.return(1) }\n",
            "off.satl", o);
        check(r.ok, "validation still passes with blocks off");
        check(r.output.empty(), "but nothing ran");
        check(r.summary.find("1 cxx block") != std::string::npos,
              "the block is still counted: " + r.summary);
    }
    {
        // a block that does not compile is an error diagnostic, not a crash
        InterpretResult r = interpret_source(
            "satellite.include(satellite)\n"
            "satellite.cxx { this is not valid C++ at all !!! }\n",
            "boom.satl");
        check(!r.ok, "a failing block fails the file");
        check(has_error_containing(r, "cxx block 1"),
              "the error names the block");
        check(r.output.empty(), "and produces no output line");
    }

    // =======================================================================
    // interpret_file
    // =======================================================================
    {
        std::string p = dir + "/prog.satl";
        write_file(p, kGood);

        InterpretResult r = interpret_file(p);
        check(r.ok, "a real file interprets: " + r.summary);
        eqs(r.file, p, "the resolved path is reported");

        // the same file, typed without its extension
        InterpretResult s = interpret_file(dir + "/prog");
        check(s.ok, "the same file found by stem: " + s.summary);
        eqs(s.file, p, "and reported under its real name");
    }
    {
        std::string missing = dir + "/absent";
        InterpretResult r = interpret_file(missing);
        check(!r.ok, "a missing file fails");
        check(has_error_containing(r, "no such file: " + missing),
              "the error names the file");
        check(has_error_containing(r, "tried " + missing + " and " + missing +
                                          ".satl"),
              "and says both paths it tried");
        eqs(r.file, missing, "the result remembers what was typed");
    }

    // -----------------------------------------------------------------------
    // The entry point belongs to the FIRST file.
    // -----------------------------------------------------------------------
    {
        const std::string prog =
            "satellite.include(satellite)\n"
            "satellite.capsule satellite.main()\n{\n}\n";

        InterpretOptions main_file;                 // the first file
        main_file.require_main = true;
        InterpretResult r = interpret_source(prog, "prog.satl", main_file);
        check(!r.has_errors(), "the first file may declare satellite.main");

        InterpretOptions included;                  // an include
        included.require_main_include = false;
        included.forbid_main          = true;
        r = interpret_source(prog, "lib.satl", included);
        check(!r.ok, "an INCLUDE that declares satellite.main is rejected");
        check(has_error_containing(r, "belongs in the first file"),
              "and the error says where main belongs");
        check(r.diagnostics[0].line == 2, "pointing at the line that declared it");

        // an include with no main is fine
        r = interpret_source("satellite.capsule helper()\n{\n}\n", "lib.satl",
                             included);
        check(!r.has_errors(), "an include with no main is fine");

        // require_main with nothing to find
        InterpretOptions needs_main;
        needs_main.require_main = true;
        r = interpret_source("satellite.include(satellite)\n", "prog.satl",
                             needs_main);
        check(!r.ok, "require_main fails when there is no main");
        check(has_error_containing(r, "satellite.capsule satellite.main"),
              "and names what it wanted");
    }
    {
        InterpretResult r = interpret_source(
            "satellite.include(satellite)\nsatellite.capsule satellite.main\n",
            "prog.satl", {});
        check(!r.ok, "satellite.main without a parameter list is rejected");
        check(has_error_containing(r, "needs a parameter list"),
              "and the message says exactly what is missing");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
