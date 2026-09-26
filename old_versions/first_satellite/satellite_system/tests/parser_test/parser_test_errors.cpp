// Errors: that a malformed program is reported rather than thrown or silently
// accepted, that the report points somewhere usable, that it is rendered
// against its OWN spaceship, and that one bad statement does not swallow the
// good ones after it.
//
// Part of satellite_system/tests/parser_test/, split from a 442-line
// parser_test.cpp.

#include "parser_test.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

using namespace satellite;

void parser_test_error_reporting()
{
    // ---- errors do not throw and do point somewhere ----------------------
    check_fails("satellite.variable.time\n", "a type with no name");
    check_fails("satellite.capsule\n", "a capsule with no name");
    check_fails("satellite.capsule f(\n{\n}\n", "an unclosed parameter list");
    check_fails("satellite.return(\n", "an unclosed return");
    check_fails("satellite.statement.if(a)\n", "an if with no block");
    check_fails("satellite.container.list<satellite.variable.string ns\n",
                "an unclosed generic");
    check_fails("\"unterminated\n", "a lexer error surfaces as a parse error");
    check_fails("satellite.nonsense.thing x = 1\n", "a bad type namespace");
    check_fails("satellite.variable.number satellite = 1\n",
                "'satellite' cannot name a variable");
    check_fails("x = \n", "an assignment with no value");
}

// The three cases where an error has to render, not merely exist.
void parser_test_error_rendering()
{
    {
        // An error must carry a usable position and render with a caret.
        const std::string src = "satellite.variable.number x = 1\n"
                                "satellite.return(\n";
        ParseResult r = parse(src);
        check(!r.ok(), "the bad line is reported");
        if (!r.errors.empty()) {
            check(r.errors[0].span.line == 2, "the error points at line 2");
            const std::string rendered = format_error(r.errors[0], SourceMap(src));
            check(rendered.find("line 2") != std::string::npos,
                  "the rendered error names the line");
            check(rendered.find("^") != std::string::npos,
                  "the rendered error draws a caret");
            check(rendered.find("satellite.return(") != std::string::npos,
                  "the rendered error shows the source line");

            // The same error, from a source that HAS a name: §16's file id at
            // work. A span carries the id, format_error looks the path up, and
            // the header stops saying "line 2" about a file it cannot name.
            const std::string named =
                format_error(r.errors[0], SourceMap(src, "orbit.satl"));
            check(named.find("orbit.satl:2") != std::string::npos,
                  "a named source renders as path:line");
            check(named.find("satellite.return(") != std::string::npos,
                  "a named source still shows the source line");
        }
    }

    {
        // Two spaceships in one SourceMap, which is the case the file id
        // exists for. Both errors are on line 1 and they must render against
        // DIFFERENT text — that is precisely the lie §16 describes, and the
        // only way to catch it is to make the two sources disagree.
        SourceMap sources;
        const uint32_t first = sources.add("satellite.return(\n", "first.satl");
        const uint32_t second = sources.add("x = \n", "second.satl");

        ParseResult a = parse("satellite.return(\n", first);
        ParseResult b = parse("x = \n", second);
        check(!a.ok() && !b.ok(), "both spaceships fail to parse");

        if (!a.errors.empty() && !b.errors.empty()) {
            const std::string ra = format_error(a.errors[0], sources);
            const std::string rb = format_error(b.errors[0], sources);

            check(ra.find("first.satl:1") != std::string::npos &&
                      rb.find("second.satl:1") != std::string::npos,
                  "each error names its own spaceship");
            check(ra.find("satellite.return(") != std::string::npos &&
                      ra.find("x = ") == std::string::npos,
                  "the first error shows only the first spaceship's text");
            check(rb.find("x = ") != std::string::npos &&
                      rb.find("satellite.return(") == std::string::npos,
                  "the second error shows only the second spaceship's text");
        }
    }

    {
        // A span whose file id names a source that cannot hold it. Impossible
        // while format_error was handed the one true text, and reachable the
        // moment a span picks its own — so the caret has to stay inside the
        // line rather than padding out to the offset it was given.
        ParseResult r = parse("satellite.variable.number x = 1\n"
                              "satellite.return(\n");
        check(!r.ok(), "the long source fails to parse");

        if (!r.errors.empty()) {
            const std::string rendered =
                format_error(r.errors[0], SourceMap("x\n", "short.satl"));
            check(rendered.find("short.satl:2") != std::string::npos,
                  "a mismatched source still names where it thinks it is");
            check(rendered.size() < 120,
                  "a span past the end of its source does not pad the caret out "
                  "to the offset");
        }
    }
}

void parser_test_error_recovery()
{
    {
        // Recovery: a bad statement must not swallow the ones after it.
        ParseResult r = parse("satellite.capsule a()\n{\n}\n"
                              "satellite.variable.time\n"
                              "satellite.capsule b()\n{\n}\n");
        check(!r.ok(), "the malformed declaration is reported");
        size_t capsules = 0;
        for (const TopLevel &item : r.program.items)
            if (std::holds_alternative<Capsule>(item))
                capsules++;
        check(capsules == 2, "both good capsules survive the bad line between");
    }
}
