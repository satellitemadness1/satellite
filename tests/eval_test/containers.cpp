// M16's rows: the two containers, their thirty-six methods, the subscripts,
// and the ten-level search ladder with its dial.
//
// WHAT THIS SECTION IS FOR, in the order the milestone's done-when asks it:
// that a container can be CONSTRUCTED at all (the bare call shapes, which is
// the author's 2026-09-05 decision and the thing DESIGN §8.7 made necessary),
// that every numbered row answers, that a subscript means what the receiver
// and the subscript together say it means, and that the search power's ten
// levels are ten levels rather than a ladder with a rung missing.
//
// THE LADDER IS CHECKED ONE RUNG AT A TIME AND THAT IS THE POINT. search.hpp
// promises each level is a STRICT SUPERSET of the one below it, which is what
// makes the dial a dial; a fixture that only checked "the search finds
// things" would pass with two rungs welded together. So each rung has a pair
// that matches THERE and not tighter.
//
// THE TABLE IS PROCESS-WIDE, SO THIS SECTION PUTS IT BACK EMPTY -- the
// contract every section here keeps.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_scalars/handlers.hpp"

#include <string>

namespace eval_test {

namespace {

std::string capsule(const std::string &body)
{
    return "satellite.capsule it()\n{\n" + body + "}\n";
}

std::string answers(const std::string &body)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

bool refused_with(const std::string &body, satellite::errors::Code code)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return false;
    call(run, "it", {});
    return ran_into(code);
}

// A list of three numbers, built the way a program has to build one now: a
// declaration holds nothing, the constructor makes the value, and `append`
// mutates through the slot.
const char *kThree =
    "    satellite.container.list<satellite.variable.number> l = "
    "satellite.container.list()\n"
    "    l.append(3)\n"
    "    l.append(1)\n"
    "    l.append(2)\n";

// A map of two entries, the same way.
const char *kTwo =
    "    satellite.container.map<satellite.variable.string, "
    "satellite.variable.number> m = satellite.container.map()\n"
    "    m.set(\"bolt\", 7)\n"
    "    m.set(\"nut\", 9)\n";

} // namespace

void section_containers()
{
    using namespace satellite;

    scalars::install_handlers();
    containers::install_handlers();
    check(eval::Handlers::table().installed() == 74 + 40,
          "M16's 40 rows -- 2 constructors, 25 list methods, 9 map methods, "
          "2 minted `search(pattern)` rows and the 2 threshold dials -- on "
          "top of M11 and M12's 37, M19.5's 11, M20's 1 and M21's 1, and the "
          "count is here so a row dropped from an install loop cannot vanish "
          "quietly");

    // --- construction, which is where DESIGN §8.7 put the work -------------

    check(answers("    satellite.container.list<satellite.variable.number> l\n"
                  "    satellite.return(l)\n") == "nothing",
          "a bare declaration holds NOTHING, of a container exactly as of "
          "every other type -- DESIGN §8.7 says the state is universal, and "
          "M16 declined v1's default-construction to keep that sentence true");

    check(answers("    satellite.container.list<satellite.variable.number> l = "
                  "satellite.container.list()\n"
                  "    satellite.return(l)\n") == "[]",
          "`satellite.container.list()` `1 4 2 0` constructs the empty list, "
          "which is what a bare declaration no longer does");

    check(answers("    satellite.container.map<satellite.variable.string, "
                  "satellite.variable.number> m = satellite.container.map()\n"
                  "    satellite.return(m)\n") == "{}",
          "`satellite.container.map()` `1 4 1 0` constructs the empty map");

    // --- the list's rows ---------------------------------------------------

    check(answers(std::string(kThree) + "    satellite.return(l)\n") ==
              "[3, 1, 2]",
          "append writes back through the slot -- DESIGN §6.4's storage-slot "
          "rule, and the printer's one-line form");

    check(answers(std::string(kThree) + "    satellite.return(l.size())\n") == "3",
          "`size` `1 4 2 2`");
    check(answers(std::string(kThree) + "    satellite.return(l.first())\n") == "3",
          "`first` `1 4 2 12`");
    check(answers(std::string(kThree) + "    satellite.return(l.last())\n") == "2",
          "`last` `1 4 2 13`");
    check(answers(std::string(kThree) +
                  "    satellite.return(l.contains(1))\n") == "true",
          "`contains(x)` `1 4 2 8`, by the language's own equality");
    check(answers(std::string(kThree) +
                  "    satellite.return(l.index_of(1))\n") == "1",
          "`index_of(x)` `1 4 2 9`, counting from 0");
    check(answers(std::string(kThree) + "    satellite.return(l.empty())\n") ==
              "false",
          "`empty` `1 4 2 10`");
    check(answers(std::string(kThree) +
                  "    l.clear()\n    satellite.return(l)\n") == "[]",
          "`clear` `1 4 2 11` mutates, like the string's");
    check(answers(std::string(kThree) +
                  "    l.reverse()\n    satellite.return(l)\n") == "[2, 1, 3]",
          "`reverse` `1 4 2 22`");
    check(answers(std::string(kThree) + "    satellite.return(l.sum())\n") == "6",
          "`sum` `1 4 2 23`");
    check(answers(std::string(kThree) + "    satellite.return(l.max())\n") == "3",
          "`max` `1 4 2 24`");
    check(answers(std::string(kThree) + "    satellite.return(l.min())\n") == "1",
          "`min` `1 4 2 25`");
    check(answers(std::string(kThree) +
                  "    l.truncate(2)\n    satellite.return(l)\n") == "[3, 1]",
          "`truncate(n)` `1 4 2 14`");
    check(answers(std::string(kThree) +
                  "    l.remove_first()\n    satellite.return(l)\n") == "[1, 2]",
          "`remove_first()` `1 4 2 16`");
    check(answers(std::string(kThree) +
                  "    l.remove_last()\n    satellite.return(l)\n") == "[3, 1]",
          "`remove_last()` `1 4 2 17`");
    check(answers(std::string(kThree) +
                  "    l.remove_at(1)\n    satellite.return(l)\n") == "[3, 2]",
          "`remove_at(n)` `1 4 2 18`");
    check(answers(std::string(kThree) +
                  "    l.remove(3)\n    satellite.return(l)\n") == "[1, 2]",
          "`remove(x)` `1 4 2 19`, by value and not by position");
    check(answers(std::string(kThree) +
                  "    l.insert(1, 9)\n    satellite.return(l)\n") ==
              "[3, 9, 1, 2]",
          "`insert(n, x)` `1 4 2 20` puts x AT n");
    check(answers(std::string(kThree) +
                  "    l.insert(3, 9)\n    satellite.return(l)\n") ==
              "[3, 1, 2, 9]",
          "`insert` at the size is the append position, which is why it is "
          "not out of range");
    check(answers(std::string(kThree) +
                  "    satellite.return(l.join(\"-\"))\n") == "3-1-2",
          "`join(separator)` `1 4 2 21`");

    // --- the sort family, which is not the search power --------------------

    check(answers(std::string(kThree) +
                  "    l.sort()\n    satellite.return(l)\n") == "[1, 2, 3]",
          "`sort()` `1 4 2 3` -- ascending, no key, and it mutates");
    check(answers(std::string(kThree) +
                  "    l.sort_down()\n    satellite.return(l)\n") == "[3, 2, 1]",
          "`sort_down()` `1 4 2 5`");

    // THE FOLD, WHICH M16 IS THE FIRST MILESTONE TO REACH. M7 built it and
    // nothing in the language could exercise it -- `sort` is the only word
    // with options and there were no lists. Both halves are checked: the
    // ordinary fold onto a spelled row, and the author's 2026-09-05 rule that
    // a fold may land on the bare word it was spelled from.
    check(answers(std::string(kThree) +
                  "    l.sort(\"down\")\n    satellite.return(l)\n") ==
              "[3, 2, 1]",
          "`sort(\"down\")` folds to `sort_down()` `1 4 2 5` at resolve, and "
          "the literal is dropped from the call because it is part of the "
          "NUMBER -- WORD_NUMBERS §1.5");
    check(answers(std::string(kThree) +
                  "    l.sort(\"up\")\n    satellite.return(l)\n") ==
              "[1, 2, 3]",
          "`sort(\"up\")` folds onto the BARE `sort()` `1 4 2 3`, which is "
          "the author's decision of 2026-09-05 and closes MILESTONES/M7.md "
          "§6 item 1");

    check(answers("    satellite.container.list<satellite.variable.string> l = "
                  "satellite.container.list()\n"
                  "    l.append(\"pear\")\n"
                  "    l.append(\"apple\")\n"
                  "    l.sort()\n"
                  "    satellite.return(l)\n") == "[apple, pear]",
          "strings order too, which is what `satellite.directory.list()` will "
          "need at M19 and is why the sort is not numbers-only");

    check(refused_with("    satellite.container.list<satellite.variable.number> l = "
                       "satellite.container.list()\n"
                       "    l.append(1)\n"
                       "    l.append(\"a\")\n"
                       "    l.sort()\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "a MIXED list is refused rather than ordered by a rule nobody "
          "chose");

    check(answers("    satellite.container.list<satellite.container.map> l = "
                  "satellite.container.list()\n"
                  "    satellite.container.map<satellite.variable.string, "
                  "satellite.variable.number> a = satellite.container.map()\n"
                  "    a.set(\"n\", 2)\n"
                  "    satellite.container.map<satellite.variable.string, "
                  "satellite.variable.number> b = satellite.container.map()\n"
                  "    b.set(\"n\", 7)\n"
                  "    l.append(a)\n"
                  "    l.append(b)\n"
                  "    l.sort_down(\"n\")\n"
                  "    satellite.container.map<satellite.variable.string, "
                  "satellite.variable.number> top = l.first()\n"
                  "    satellite.return(top.get(\"n\"))\n") == "7",
          "`sort_down(key)` `1 4 2 6` -- DESIGN §12's primitive, the one all "
          "seven of QUAD's comparators are");

    // --- the map's rows ----------------------------------------------------

    check(answers(std::string(kTwo) + "    satellite.return(m)\n") ==
              "{bolt: 7, nut: 9}",
          "a map prints in INSERTION order, which is what makes `.keys` a "
          "walk a program can rely on");
    check(answers(std::string(kTwo) + "    satellite.return(m.get(\"bolt\"))\n") ==
              "7",
          "`get(k)` `1 4 1 2`");
    check(answers(std::string(kTwo) + "    satellite.return(m.has(\"nut\"))\n") ==
              "true",
          "`has(k)` `1 4 1 3`");
    check(answers(std::string(kTwo) + "    satellite.return(m.size())\n") == "2",
          "`size` `1 4 1 4`");
    check(answers(std::string(kTwo) + "    satellite.return(m.empty())\n") ==
              "false",
          "`empty` `1 4 1 5`");
    check(answers(std::string(kTwo) +
                  "    m.clear()\n    satellite.return(m)\n") == "{}",
          "`clear` `1 4 1 6` mutates");
    check(answers(std::string(kTwo) +
                  "    m.remove(\"bolt\")\n    satellite.return(m)\n") ==
              "{nut: 9}",
          "`remove(k)` `1 4 1 7` mutates, and the index is rebuilt");
    check(answers(std::string(kTwo) + "    satellite.return(m.keys())\n") ==
              "[bolt, nut]",
          "`keys` `1 4 1 8`, in insertion order");
    check(answers(std::string(kTwo) + "    satellite.return(m.values())\n") ==
              "[7, 9]",
          "`values` `1 4 1 9`");
    check(answers(std::string(kTwo) +
                  "    m.set(\"bolt\", 1)\n    satellite.return(m)\n") ==
              "{bolt: 1, nut: 9}",
          "an existing key KEEPS ITS POSITION -- updating a value is not "
          "re-inserting it");

    check(refused_with(std::string(kTwo) + "    m.get(\"washer\")\n",
                       errors::Code::EVAL_NO_SUCH_KEY),
          "a miss is an ERROR and not nothing, because `nothing` is a value "
          "an entry can legitimately hold -- and `has(k)` is the question "
          "that answers false instead");

    check(answers(std::string(kTwo) +
                  "    satellite.return(m.has(1))\n") == "false",
          "`has` answers a key it could never hold rather than refusing, "
          "because it is the guard `get`'s own sentence sends people to");

    // --- the subscripts ----------------------------------------------------

    check(answers(std::string(kThree) + "    satellite.return(l[0])\n") == "3",
          "`l[0]` is positional, and stays positional whatever the receiver "
          "turns out to be -- v1's DECISION 6a");
    check(answers(std::string(kThree) + "    satellite.return(l[-1])\n") == "2",
          "a negative subscript counts from the end");
    check(answers(std::string(kTwo) + "    satellite.return(m[\"bolt\"])\n") == "7",
          "`m[k]` is a key lookup, asked BEFORE the number rule so a string "
          "subscript never reports 'index must be a whole number'");
    check(answers("    satellite.variable.string s = \"hello\"\n"
                  "    satellite.return(s[1])\n") == "e",
          "a string subscript selects satellite characters, the same rule "
          "`at(n)` has answered since M11");
    check(answers(std::string(kThree) + "    satellite.return(l[0:2])\n") ==
              "[3, 1]",
          "a slice takes a run");
    check(answers(std::string(kThree) + "    satellite.return(l[1:])\n") ==
              "[1, 2]",
          "an absent bound is the end, and is not compiled as a constant");
    check(answers(std::string(kThree) + "    satellite.return(l[0:99])\n") ==
              "[3, 1, 2]",
          "A SLICE CLAMPS AND AN INDEX DOES NOT, which is deliberate on both "
          "sides");
    check(refused_with(std::string(kThree) + "    satellite.return(l[9])\n",
                       errors::Code::EVAL_OUTSIDE_THE_LIST),
          "an out-of-range INDEX refuses, naming both numbers");

    check(answers(std::string(kThree) +
                  "    l[0] = 8\n    satellite.return(l)\n") == "[8, 1, 2]",
          "`l[i] = v` writes the whole container back through the slot");
    check(answers(std::string(kTwo) +
                  "    m[\"washer\"] = 4\n    satellite.return(m)\n") ==
              "{bolt: 7, nut: 9, washer: 4}",
          "`m[k] = v` too -- DESIGN §12 struck them as ONE entry, so a "
          "milestone that built one half would leave that entry half-struck");
    check(refused_with(std::string(kThree) + "    l[9] = 1\n",
                       errors::Code::EVAL_OUTSIDE_THE_LIST),
          "an index assignment out of range is an error and NOT a grow: "
          "`l[5] = x` on an empty list must not create four elements holding "
          "nothing");

    // --- the search power, one rung at a time ------------------------------

    // The dial is read back, so a program can ask what it is set to.
    check(answers("    satellite.return(satellite.system.threshold())\n") == "1",
          "`satellite.system.threshold()` `1 22 5` starts at 1 -- the exact "
          "match every program written before the power existed assumed");

    // A one-element list holding the hay, searched for the needle, at the
    // dial named. The answer is the count of hits, so a rung that stopped
    // matching shows as 0 and a rung that matched too early shows at the
    // level below it.
    //
    // THE RESULT IS NAMED BEFORE IT IS ASKED ANYTHING, which is not fixture
    // style but WORD_NUMBERS §1.5's one hop: a selector folds only through a
    // declared name, so `l[p].size()` does not resolve and `found.size()`
    // does. §5 of MILESTONES/M16.md records that this is what a search result
    // costs a program today.
    const auto finds = [&](const std::string &hay, const std::string &needle,
                           int level) {
        return answers("    satellite.system.threshold(" +
                       std::to_string(level) +
                       ")\n"
                       "    satellite.container.list<satellite.variable.string> l = "
                       "satellite.container.list()\n"
                       "    l.append(" + hay + ")\n"
                       "    satellite.container.list<satellite.variable.string> found = "
                       "l[" + needle + "]\n"
                       "    satellite.return(found.size())\n");
    };

    check(finds("\"bolt\"", "\"bolt\"", 1) == "1", "level 1 -- EXACT");
    check(finds("\"bolt\"", "\"Bolt\"", 1) == "0" &&
              finds("\"bolt\"", "\"Bolt\"", 2) == "1",
          "level 2 -- CASE, and it is NOT reachable at 1, which is what makes "
          "the ladder a ladder");
    check(finds("12", "\"12\"", 2) == "0" && finds("12", "\"12\"", 3) == "1",
          "level 3 -- CROSS-TYPE, the type tag dropped on request");
    check(finds("\" bolt \"", "\"bolt\"", 3) == "0" &&
              finds("\" bolt \"", "\"bolt\"", 4) == "1",
          "level 4 -- TRIMMED");
    check(finds("\"boltzmann\"", "\"bolt\"", 4) == "0" &&
              finds("\"boltzmann\"", "\"bolt\"", 5) == "1",
          "level 5 -- PREFIX, either direction");
    check(finds("\"a bolt here\"", "\"bolt\"", 5) == "0" &&
              finds("\"a bolt here\"", "\"bolt\"", 6) == "1",
          "level 6 -- SUBSTRING, either direction");
    check(finds("\"bolt\"", "\"bolp\"", 7) == "0" &&
              finds("\"bolt\"", "\"bolp\"", 8) == "1",
          "level 8 -- ONE TYPO, edit distance 1");
    check(finds("\"bolt\"", "\"bopp\"", 8) == "0" &&
              finds("\"bolt\"", "\"bopp\"", 9) == "1",
          "level 9 -- TWO TYPOS");
    check(finds("\"b-o-l-t\"", "\"bolt\"", 9) == "0" &&
              finds("\"b-o-l-t\"", "\"bolt\"", 10) == "1",
          "level 10 -- SUBSEQUENCE, the fuzzy-finder rule");

    check(refused_with("    satellite.system.threshold(11)\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "OUT OF RANGE IS AN ERROR, NOT A CLAMP -- v1's DECISION 5b: "
          "threshold(11) silently meaning 10 would never tell the program it "
          "had asked for something the language does not have");

    // A MAP'S STRING SUBSCRIPT IS A LOOKUP AND NEVER A SEARCH, which is v1's
    // rule kept deliberately: "a lookup that quietly became a search would
    // change what `m[k]` MEANS for every program that has ever read a map".
    // So a miss is S0726 even with the dial wide open, and `.search(pattern)`
    // is how a map is asked the loose question.
    check(refused_with("    satellite.system.threshold(10)\n" + std::string(kTwo) +
                       "    satellite.return(m[\"olt\"])\n",
                       errors::Code::EVAL_NO_SUCH_KEY),
          "a key that misses is a MISS at every threshold -- the dial moves "
          "how loose a SEARCH is, and a map subscript is a lookup");

    check(answers("    satellite.system.threshold(6)\n" + std::string(kTwo) +
                  "    satellite.container.list<satellite.container.map> hits = "
                  "m.search(\"olt\")\n"
                  "    satellite.container.map<satellite.variable.string, "
                  "satellite.variable.string> first = hits.first()\n"
                  "    satellite.return(first.get(\"key\"))\n") == "bolt",
          "`search(pattern)` `1 4 1 10` is the RICH spelling -- a map per hit "
          "with the value, the key, the path and the score, and its number "
          "was minted for this milestone (WORD_NUMBERS §2.6)");

    check(answers(std::string(kTwo) +
                  "    satellite.container.list<satellite.container.map> hits = "
                  "m.search(\"washer\")\n"
                  "    satellite.return(hits.size())\n") == "0",
          "a search that finds nothing answers an EMPTY list, which is the "
          "one place it parts company with `m[k]`: asking whether a structure "
          "contains something is the whole point, and a question that errored "
          "when the answer is 'no' could not be asked");

    // A LIST'S STRING SUBSCRIPT IS THE SEARCH, because a string is not a
    // position and a list has no keys -- the one rule decides both.
    check(answers("    satellite.system.threshold(6)\n"
                  "    satellite.container.list<satellite.variable.string> l = "
                  "satellite.container.list()\n"
                  "    l.append(\"bolt\")\n"
                  "    l.append(\"nut\")\n"
                  "    satellite.return(l[\"olt\"])\n") == "[bolt]",
          "a subscript that is not a position is a PATTERN -- the short "
          "spelling, answering the values that matched");

    // A NESTED WALK, which is what makes this a structural search rather
    // than a scan: the hit is two levels down and its path says where.
    check(answers("    satellite.container.list<satellite.container.list> outer = "
                  "satellite.container.list()\n"
                  "    satellite.container.list<satellite.variable.string> inner = "
                  "satellite.container.list()\n"
                  "    inner.append(\"bolt\")\n"
                  "    outer.append(inner)\n"
                  "    satellite.return(outer[\"bolt\"])\n") == "[bolt]",
          "the walker DESCENDS -- the hit is two levels down -- while the "
          "comparator never does, which is what stops a nested hit being "
          "counted twice");

    // AND A NUMBER SUBSCRIPT STAYS POSITIONAL EVEN WHERE A SEARCH WOULD HAVE
    // ANSWERED, which is DECISION 6a at its sharpest: `outer[41]` on a
    // one-element list is out of range and NOT a search for 41, because
    // `l[0]` is positional in every program ever written in this language.
    check(refused_with("    satellite.container.list<satellite.variable.number> l = "
                       "satellite.container.list()\n"
                       "    l.append(41)\n"
                       "    satellite.return(l[41])\n",
                       errors::Code::EVAL_OUTSIDE_THE_LIST),
          "a whole number indexes and never searches, whatever the list "
          "happens to hold");

    // --- split, the row that was waiting for this milestone ----------------

    check(answers("    satellite.variable.string s = \"a,b,c\"\n"
                  "    satellite.return(s.split(\",\"))\n") == "[a, b, c]",
          "`split(separator)` `1 6 1 10` answered S0720 naming M16 from M11 "
          "until the containers existed");
    check(answers("    satellite.variable.string s = \"a,,b\"\n"
                  "    satellite.container.list<satellite.variable.string> parts = "
                  "s.split(\",\")\n"
                  "    satellite.return(parts.size())\n") == "3",
          "an empty piece is a piece -- a split that dropped them would lose "
          "a blank column in every CSV line that has one");

    eval::Handlers::table().clear();
}

} // namespace eval_test
