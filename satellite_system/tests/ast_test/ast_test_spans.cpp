// Spans and the SourceMap: the byte offsets an error message points at, the
// file id §16 added so a span knows which spaceship it came from, and the
// table that turns that id back into a path and a line.
//
// Part of the ast_test binary, split out of a 413-line ast_test.cpp. These
// checks build no trees, so the only thing they take from ast_test.hpp is the
// failure counter they report into, which ast_test.cpp defines.

#include "ast_test.hpp"

void ast_test_spans()
{
    // ---- spans ------------------------------------------------------------
    {
        std::vector<Token> t = lex("satellite.variable.time my_time");
        Span s = span_of(t[5]);
        check(s.start == 24 && s.end == 31 && s.line == 1,
              "a span carries byte offsets straight from the token");
        check(s.file == 0, "a span with no spaceship named defaults to file 0");
        Span joined = span_join(span_of(t[0]), span_of(t[5]));
        check(joined.start == 0 && joined.end == 31,
              "joining spans covers the whole range");

        // §16: the id has to survive both span-making paths, or an error from
        // a joined span reports the right line against the wrong spaceship.
        Span stamped = span_of(t[5], 7);
        check(stamped.file == 7, "a span carries the file id it was given");
        check(span_join(span_of(t[0], 7), stamped).file == 7,
              "joining spans keeps the file id");
    }
}

void ast_test_source_map()
{
    // ---- the SourceMap ----------------------------------------------------
    {
        SourceMap sources;
        check(sources.size() == 0, "a fresh SourceMap holds nothing");

        const uint32_t a = sources.add("first", "a.satl");
        const uint32_t b = sources.add("second", "b.satl");
        check(a == 0 && b == 1, "ids are dense and handed out in load order");
        check(sources.text(a) == "first" && sources.text(b) == "second",
              "each id answers its own text");
        check(sources.path(b) == "b.satl", "each id answers its own path");

        // The REPL's case: a source that came from no file at all.
        SourceMap bare("x = 1");
        check(bare.path(0).empty(), "a source with no path says so");

        // A Span pointing at nothing must degrade, not read off the end.
        check(sources.text(99).empty() && sources.path(99).empty(),
              "an unknown file id answers empty rather than reading past the end");

        Span at_b{0, 1, 4, b};
        check(span_location(at_b, sources) == "b.satl:4",
              "a located span renders as path:line");
        Span at_bare{0, 1, 4, 0};
        check(span_location(at_bare, bare) == "line 4",
              "an unnamed span renders as a bare line number");
    }
}
