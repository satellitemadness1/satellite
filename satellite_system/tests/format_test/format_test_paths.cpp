// The arity table -- find_path() keyed by the whole four-word quadruple, and the
// variadic marker that lets a row say "0 or 1" without meaning "0". Part of the
// format_test binary; the harness is declared in format_test.hpp.
//
// Both sections are here rather than in files of their own because they are one
// subject read twice: test_paths() asks whether a quadruple finds the row it
// should, and test_variadic() asks what the arity column of that row is allowed
// to say. Splitting them would put satellite.time.now's zero arity in one file
// and the proof that a zero arity is not a variadic marker in another, which is
// the pair the old encoding could not tell apart and the reason the marker
// exists at all.

#include "format_test.hpp"

using namespace satellite::format;

// --- the arity table --------------------------------------------------------

void test_paths()
{
    // DESIGN.md §17.3 dumps satellite.console.display("hello, world!") in full,
    // and word 0 of its name code is quoted as 0x...0001 with word1 = 6 and
    // word2 = 7. This is that claim, checked against the table.
    const uint64_t display[4] = {1, 6, 7, 0};
    const Path *p = find_path(display);
    check(p != nullptr, "satellite.console.display has an arity row");
    if (p) {
        check(p->arity == 1, "display takes one operand unit");
        check_str(p->ident, "P_DISPLAY", "the display row is P_DISPLAY");
    }

    // Built from the enum rather than from literals, so renaming a word cannot
    // leave this passing against a stale number.
    const uint64_t built[4] = {static_cast<uint64_t>(Word::SATELLITE),
                               static_cast<uint64_t>(Word::CONSOLE),
                               static_cast<uint64_t>(Word::DISPLAY), 0};
    check(find_path(built) == p, "the enum and the hex dump agree");

    // Zero-argument paths exist and must not be confused with absent rows.
    const uint64_t now[4] = {1, 24, 30, 0};
    const Path *n = find_path(now);
    check(n != nullptr && n->arity == 0, "satellite.time.now takes no operands");

    // A path nobody has recorded is a stream that cannot be decoded, and
    // find_path says so rather than guessing.
    const uint64_t bogus[4] = {1, 6, 99, 0};
    check(find_path(bogus) == nullptr, "an unrecorded path has no arity");

    // Prefixes are distinct from the paths that extend them: the space is flat,
    // so only the whole quadruple identifies a row.
    const uint64_t prefix[4] = {1, 6, 0, 0};
    check(find_path(prefix) == nullptr, "satellite.console alone is not a path");

    // Four segments, which no path used until §18. The name code has been four
    // words since §17 and this is the first row that fills the fourth, so it is
    // also the first proof that find_path keys on the whole quadruple rather
    // than on the three that happened to be populated.
    const uint64_t ranged[4] = {1, 80, 83, 84};
    const Path *r = find_path(ranged);
    check(r != nullptr && r->arity == 2,
          "satellite.random.ultra.range takes two operands");

    // The same tier without .range is a DIFFERENT path with a different arity.
    // §18 spells the second form as a fourth segment rather than overloading the
    // third; that was once forced by the table holding one arity per path, and
    // since SAT_VARIADIC it is a choice about the surface. Either way the two
    // rows are distinct and neither is variadic.
    const uint64_t bare[4] = {1, 80, 83, 0};
    const Path *b = find_path(bare);
    check(b != nullptr && b->arity == 1,
          "satellite.random.ultra takes one operand");

    // satellite.system.delete(x). One argument whichever alternative the value
    // holds -- a string path or an open file -- because the arity column counts
    // operand UNITS and a value is one unit. That is why accepting two types
    // did not need a second row, and why it is not variadic.
    const uint64_t erase[4] = {1, 88, 99, 0};
    const Path *e = find_path(erase);
    check(e != nullptr && !is_variadic(*e) && e->arity == 1,
          "satellite.system.delete takes exactly one operand");

    // satellite.file.new(path[, mode]). Variadic, and it is the second row of a
    // pair that share their first three segments with nothing: {1,25,31,0} is
    // open and {1,25,100,0} is new, which is what lets one word mean `open` in
    // both a path and a selector without either becoming ambiguous.
    const uint64_t made[4] = {1, 25, 100, 0};
    const Path *m = find_path(made);
    check(m != nullptr && is_variadic(*m),
          "satellite.file.new is variadic — 1 or 2 arguments");

    check(kPathCount == 29, "twenty-nine paths carry an arity");
}

// --- the variadic marker ----------------------------------------------------

void test_variadic()
{
    // 0 is a real arity and 255 is not one, which is the entire distinction the
    // marker rests on.
    check(kVariadic != 0, "the sentinel is not a legal count");

    const uint64_t help[4] = {1, 29, 0, 0};
    const Path *h = find_path(help);
    check(h != nullptr, "satellite.help has an arity row");
    if (h) {
        check(is_variadic(*h), "satellite.help is variadic — 0 or 1 arguments");
        check_str(h->ident, "P_HELP", "the help row is P_HELP");

        // The regression this marker exists to prevent. `arity == 0` was the
        // recorded value for three commits and it meant "the overview, and the
        // other form is unwritable" rather than "no operands".
        check(h->arity != 0,
              "satellite.help no longer records 0 and loses satellite.help(x)");
    }

    // A zero-arity path is NOT variadic, which is the pair the old encoding
    // could not tell apart.
    const uint64_t now[4] = {1, 24, 30, 0};
    const Path *n = find_path(now);
    check(n != nullptr && !is_variadic(*n) && n->arity == 0,
          "satellite.time.now takes no operands and says so from the table");

    // Two variadic rows today, and the second one arriving is what this count
    // was written for: P_DIR_LIST joined P_HELP when satellite.directory.list
    // grew its no-argument form. Still a count and not a whitelist -- a third
    // path growing a second shape should land here deliberately, and a row
    // going variadic by a typo in the arity column should not be silent.
    size_t variadic = 0;
    for (size_t i = 0; i < kPathCount; i++)
        if (is_variadic(kPaths[i]))
            variadic++;
    check(variadic == 8,
          "help, directory.list, file.new and the five memory quantities are variadic");

    // Every other row is a count a decoder can consume directly, and small
    // enough to be one.
    unsigned widest = 0;
    for (size_t i = 0; i < kPathCount; i++)
        if (!is_variadic(kPaths[i]) && kPaths[i].arity > widest)
            widest = kPaths[i].arity;
    check(widest == 2, "the widest fixed path takes two operand units");

    // The count unit is read against the immediate space. §17.5 answers method
    // arity the same way, and this is the same answer for the same reason.
    check(static_cast<uint64_t>(Kind::IMMEDIATE) == 1,
          "a variadic path's count arrives as a kind-1 immediate");
}
