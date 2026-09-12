// The four top-level forms, and M2's name allocator getting its first caller.
// See tests/parser_test/parser_test.hpp.
//
// THE NUMBERS IN THIS FILE ARE THE ONES PLAN §8.1 AND WORD_NUMBERS §3 WRITE
// OUT, and they are checked here rather than described because they are the
// only claim in the milestone that a reader cannot verify by reading the
// parser: `satellite.library.main` is 1 14 1 and `satellite.library.system` is
// 1 14 2, so a user's first name under `library` is 1 14 3. PLAN §8.1 says
// exactly that sentence. If words.def ever gains a third child of `library`,
// this test says 4 and the sentence in PLAN has to be corrected -- which is the
// behaviour wanted, because the sentence would then be wrong.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"
#include "satellite_words/words.hpp"

#include <string>

using satellite::NodeKind;
using satellite::words::NodeId;

namespace parser_test {

void section_declarations()
{
    // -- A user capsule takes the next free number under `library` -----------

    {
        const Program program = run("satellite.capsule helper()\n{\n}\n");
        check(program.ok(), "a bare capsule parses: " + program.first_error());

        const satellite::NodeIndex node = first_of(program.ast(), NodeKind::Capsule);
        const satellite::words::PathId path = program.ast()[node].a;
        check(path != satellite::words::kNoPath, "and its name was given a number");
        check(!satellite::words::is_language_word(path),
              "a number on the USER's side of the boundary -- above kNodeCount, "
              "which words_nodes.hpp calls the predicate anything about to write "
              "a PathId down has to ask first");
        check(program.words.parent_of(path) ==
                  static_cast<satellite::words::PathId>(NodeId::LIBRARY),
              "under satellite.library, which DESIGN §7.2 reserves for shared and "
              "global state");
        check(program.words.number_of(path) == 3,
              "AND THE NUMBER IS 3, because main is 1 14 1 and system is 1 14 2 "
              "-- PLAN §8.1's worked example, checked");
        check(program.words.name_of(path) == "helper",
              "and the NAME is recorded, because WORD_NUMBERS §3 says the number "
              "is valid inside one run only");
        check(program.words.defined() == 1, "one name met, one name numbered");
    }

    {
        const Program program =
            run("satellite.capsule one()\n{\n}\n\nsatellite.capsule two()\n{\n}\n");
        check(program.ok(), "two capsules parse: " + program.first_error());
        check(program.words.defined() == 2 &&
                  program.words.number_of(satellite::words::kNodeCount + 1) == 3 &&
                  program.words.number_of(satellite::words::kNodeCount + 2) == 4,
              "and take 3 and 4 in the order they were MET -- WORD_NUMBERS §1.1's "
              "rule, applied to the user's half");
    }

    // -- The reserved capsule name -------------------------------------------

    {
        const Program program =
            run("satellite.capsule satellite.main()\n{\n}\n");
        check(program.ok(), "satellite.main parses: " + program.first_error());

        const satellite::NodeIndex node = first_of(program.ast(), NodeKind::Capsule);
        const satellite::words::PathId path = program.ast()[node].a;
        check(satellite::words::is_language_word(path),
              "THE RESERVED NAME IS LOOKED UP AND NOT DEFINED. `capsule_name := "
              "IDENT | \"satellite\" \".\" IDENT` and the second arm names a "
              "capsule the language already has a number for");
        check(path == static_cast<satellite::words::PathId>(NodeId::MAIN),
              "and it is satellite.main, 1 3 -- words.def calls it 'the capsule "
              "satl calls'");
        check(program.words.defined() == 0,
              "so nothing was allocated for it");
    }

    check(!run("satellite.capsule satellite.nonsense()\n{\n}\n").ok(),
          "and a reserved name the language does NOT own is refused rather than "
          "given a number -- the arm defines nothing, so it has to find it");

    // -- The collision policy PLAN M4 sets -----------------------------------

    {
        const Program program = run("satellite.capsule main()\n{\n}\n");
        check(!program.ok(),
              "A NAME THE LANGUAGE ALREADY OWNS UNDER THAT PARENT IS REFUSED "
              "RATHER THAN RENUMBERED -- PLAN M4's policy. `main` is 1 14 1, so "
              "a user capsule cannot be called `main` bare");
        check(program.words.defined() == 0, "and no number was handed out");
    }

    {
        const Program program =
            run("satellite.capsule f()\n{\n}\n\nsatellite.capsule f()\n{\n}\n");
        check(!program.ok(), "and a name defined twice in one run is refused too");
        check(program.words.defined() == 1, "with the first definition standing");
    }

    // -- A spacesuit's header, in every form that means one thing ------------
    //
    // DESIGN §6: `IDENT [ "(" [ IDENT ] ")" ] [ ":" ]`. FIVE SPELLINGS AND ONE
    // MEANING EACH -- the empty pair and no pair at all are both "no
    // superclass", and the trailing colon is dropped. Until 2026-09-09 only two
    // of the five parsed; the author overruled that after writing `test_class()`
    // and `my_superclass():` in two files days apart, which is DESIGN §7.7's
    // "people type what they type" one level up from a spelling.
    {
        struct Form {
            const char *header;
            bool has_super;
            const char *what;
        };
        const Form forms[] = {
            {"suit",              false, "no parentheses"},
            {"suit()",            false, "an empty pair"},
            {"suit():",           false, "an empty pair and a colon"},
            {"suit(base)",        true,  "a superclass"},
            {"suit(base):",       true,  "a superclass and a colon"},
        };
        for (const Form &form : forms) {
            const Program program =
                run(std::string("satellite.spacesuit ") + form.header +
                    "\n{\n    satellite.public\n    {\n    }\n}\n");
            check(program.ok(), std::string("a spacesuit header with ") +
                                    form.what + " parses: " +
                                    program.first_error());
            check(count_of(program.ast(), NodeKind::Spacesuit) == 1,
                  std::string("and ") + form.what + " builds one Spacesuit");
            // The superclass is a Name node under the suit, so its presence is
            // the whole difference between the two halves of the table -- an
            // empty pair must not leave one behind.
            check((count_of(program.ast(), NodeKind::Name) > 0) == form.has_super,
                  std::string("and ") + form.what +
                      (form.has_super ? " keeps the superclass"
                                      : " leaves NO superclass node"));
        }
    }

    // -- and the header forms that are still refused -------------------------
    //
    // ACCEPTING WHAT IS UNAMBIGUOUS IS NOT ACCEPTING ANYTHING. Each of these
    // means nothing definite, so each still stops at the parser where §4.6's
    // suggester can help, rather than becoming a silent misreading later.
    {
        const char *const refused[] = {
            "suit(",        // never closed
            "suit(a b)",    // two names, no separator
            "suit(a, b)",   // multiple inheritance -- DESIGN §13 defers super(...)
            "(base)",       // no name for the suit itself
        };
        for (const char *header : refused) {
            const Program program =
                run(std::string("satellite.spacesuit ") + header +
                    "\n{\n    satellite.public\n    {\n    }\n}\n");
            check(!program.ok(), std::string("`satellite.spacesuit ") + header +
                                     "` is still refused, and means nothing "
                                     "definite -- that is why");
        }
    }

    // -- A spacesuit ---------------------------------------------------------

    {
        const Program program = run(
            "satellite.spacesuit box(container)\n"
            "{\n"
            "    satellite.protected\n"
            "    {\n"
            "        satellite.variable.number held\n"
            "    }\n"
            "\n"
            "    satellite.public\n"
            "    {\n"
            "        satellite.capsule get()\n"
            "        {\n"
            "        }\n"
            "    }\n"
            "}\n");
        check(program.ok(), "a spacesuit with both sections parses: " +
                                program.first_error());
        check(count_of(program.ast(), NodeKind::Spacesuit) == 1 &&
                  count_of(program.ast(), NodeKind::Section) == 2,
              "one suit, two sections");
        check(count_of(program.ast(), NodeKind::Capsule) == 1 &&
                  count_of(program.ast(), NodeKind::VarDecl) == 1,
              "A SECTION HOLDS MEMBERS AND NOT STATEMENTS. DESIGN §6 wrote "
              "`suit_section := ... block` and `block := { statement }`, and a "
              "capsule declaration is not a statement -- so as written the rule "
              "could not parse example/class_test.satl. Corrected 2026-08-30 to "
              "cite suit_block, which is what this check pins");

        const satellite::NodeIndex suit = first_of(program.ast(), NodeKind::Spacesuit);
        check(program.words.parent_of(program.ast()[suit].a) ==
                  static_cast<satellite::words::PathId>(NodeId::LIBRARY),
              "the suit's own name is numbered under library, beside the capsules");
        check(program.ast()[suit].c != satellite::kNoNode,
              "and its superclass is kept");

        // THE LIMIT M4 FOUND IN M2 BY BEING ITS FIRST CALLER, AND M26 CLOSED.
        // This block asserted the opposite of what it asserts now, on purpose:
        // words::Words seeded one counter per node of the FROZEN table, so a
        // user's PathId could not be a parent -- and a capsule inside a
        // spacesuit has exactly that -- so a method got kNoPath and the test
        // PINNED that, because a limit nobody wrote down is a limit somebody
        // rediscovers. MILESTONES/M4.md §6 named the fix and the milestone:
        // "a growable counter vector and a user-side child list -- small, and
        // M26's, because M26 is where a spacesuit's members have to resolve."
        //
        // THE CHECK IS KEPT AND TURNED OVER rather than deleted, because the
        // interesting fact is not that a method has a number -- it is that this
        // is the first name in the language whose PARENT is a user's name, and
        // that is what the two lines below say.
        const satellite::NodeIndex method = first_of(program.ast(), NodeKind::Capsule);
        check(program.ast()[method].a != satellite::words::kNoPath,
              "a capsule inside a spacesuit IS numbered since M26");
        check(program.words.parent_of(program.ast()[method].a) ==
                  program.ast()[suit].a,
              "and it hangs under THE SPACESUIT -- the first name in the "
              "language whose parent is itself a user's name, which is what M4 "
              "recorded as impossible and M26 built");
        check(program.words.number_of(program.ast()[method].a) == 1,
              "numbered from 1 under its suit, because 0 is the bare shape "
              "(WORD_NUMBERS §1.3) and a member is not one");
        // TWO AND NOT THREE, AND THE ONE THAT IS MISSING SAYS SOMETHING. The
        // suit and its capsule are numbered; the FIELD is not, because a field
        // is a slot rather than a path -- DESIGN §7.2's storage, reached by
        // index and never by name at run time. That is the same distinction
        // `satellite.library` draws one level up, and it is why M26's resolve
        // gives fields slots and methods numbers.
        check(program.words.defined() == 2,
              "two names met: the suit and its capsule. A field is a SLOT and "
              "not a path, so it takes no number");
    }

    // -- A global ------------------------------------------------------------

    {
        const Program program = run("satellite.library.counter = 0\n");
        check(program.ok(), "a global with an initialiser parses: " +
                                program.first_error());
        const satellite::NodeIndex node = first_of(program.ast(), NodeKind::Global);
        check(node != satellite::kNoNode && program.ast()[node].b != satellite::kNoNode,
              "as one Global with a value");
        check(program.words.number_of(program.ast()[node].a) == 3,
              "and it takes the next free number under library, from the same "
              "counter a capsule takes one from");
    }

    check(run("satellite.library.name\n").ok(),
          "and the initialiser is optional -- DESIGN §6's `[ \"=\" expression ]`");

    // -- include, and the returns clause -------------------------------------

    check(run("satellite.include(satellite)\n").ok(), "satellite.include parses");
    check(count_of(run("satellite.include(satellite)\n").ast(), NodeKind::Include) == 1,
          "as one Include over an expression, which is what makes "
          "satellite.include(spaceship) need no new rule at M25");

    {
        // `satellite.include()` `1 1 0` -- M17, and the form WORD_NUMBERS §1.3
        // teaches the numbering with. Until then this was S0231, "expected an
        // expression", against a §1.3 that says "a trailing 0 is written only
        // where a program can actually write the bare form". DESIGN §6's
        // `include_decl` now reads `"(" [ expression ] ")"`, which is the
        // grammar bending to the authority over the numbering rather than the
        // other way round.
        const Program program = run("satellite.include()\n");
        check(program.ok(),
              "satellite.include() parses: " + program.first_error());
        const satellite::NodeIndex node =
            first_of(program.ast(), NodeKind::Include);
        check(node != satellite::kNoNode, "as an Include node of its own");
        check(program.ast()[node].a == satellite::kNoNode,
              "with NO argument, which is how the tree spells §1.3's \"0 means "
              "nothing in that position\" -- an include's argument is a node "
              "and not a list, so an empty one is kNoNode");
        check(satellite::unparse(program.ast()) == "satellite.include()\n",
              "and it prints back as itself rather than as brackets round "
              "nothing -- unparse(ast) and not unparse(ast, node), because the "
              "second is unparse.cpp's INLINE form and a declaration is not "
              "one");
    }

    {
        const Program program = run(
            "satellite.capsule f() satellite.returns(satellite.variable.number)\n"
            "{\n}\n");
        check(program.ok(), "a returns clause parses: " + program.first_error());
        check(program.ast()[first_of(program.ast(), NodeKind::Capsule)].c !=
                  satellite::kNoNode,
              "DESIGN §13 records the return type as DECIDED and §6's grammar "
              "already has the rule -- 1 21, and it is optional, which is what "
              "leaves hello world byte-identical");
    }

    // -- What a file holds ---------------------------------------------------

    {
        const Program program = run("satellite.console.display(\"hi\")\n");
        check(!program.ok(),
              "DESIGN §6's `top_level` is four forms and a statement is not one "
              "of them -- a program with no capsule has nothing to run");
    }
}

} // namespace parser_test
