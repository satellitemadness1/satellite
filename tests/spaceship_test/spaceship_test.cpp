// spaceship_test -- PLAN M25's include half, `satellite.include(spaceship)` and
// `satellite.capsule.launch`, built 2026-09-13.
//
// THE REAL satl AND NOTHING ELSE. Loading a second file is programs/
// spaceships.cpp's, which no other suite links -- eval_test compiles one file
// with no loader in front of it, and says so where it refuses an include. So
// every clause here writes real files into a fresh directory, runs the `satl`
// this tree just built on one of them, and asserts on what a person would see:
// stdout, the sentence on stderr, and the exit status.
//
// EVERY EXPECTED LINE WAS PREDICTED BEFORE THE PROGRAM FIRST RAN, and the three
// that were not what was predicted were bugs rather than expectations: a
// spacesuit from another file compiled against the wrong tree, an include of the
// file satl was given read as "nothing loaded", and a global below an include
// was still nothing when that include's launch read it. Each has a clause.

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what.c_str());
        failures++;
    }
}

struct Ran {
    std::string out; // stdout and stderr together, in the order they arrived
    int status = -1;
};

std::string satl_path;
std::string root;

void write(const std::string &name, const std::string &text)
{
    std::ofstream(root + "/" + name) << text;
}

// `satl <args>` from `root`, with no window to hand over to and nothing on
// stdin. Output goes through a pipe, never /dev/null -- satl hands itself to
// satl-term when its output is /dev/null, and a sweep that did that would
// report a pass from a program that never ran here.
Ran run(const std::string &args)
{
    Ran ran;
    const std::string command = "cd '" + root + "' && SATL_NO_WINDOW=1 '" +
                                satl_path + "' " + args + " 2>&1 </dev/null";
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
        return ran;
    char buffer[4096];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof buffer, pipe)) > 0)
        ran.out.append(buffer, got);
    const int raw = pclose(pipe);
    ran.status = WIFEXITED(raw) ? WEXITSTATUS(raw) : -1;
    return ran;
}

bool holds(const std::string &text, const std::string &needle)
{
    return text.find(needle) != std::string::npos;
}

size_t count_of(const std::string &text, const std::string &needle)
{
    size_t count = 0;
    for (size_t at = text.find(needle); at != std::string::npos;
         at = text.find(needle, at + needle.size()))
        count++;
    return count;
}

// `satl --repl` from `root`, fed `typed` on stdin, with HOME in `root` so the
// session's history lands in the scratch directory and not in anyone's home.
Ran repl(const std::string &typed)
{
    write("typed.txt", typed);
    Ran ran;
    const std::string command = "cd '" + root + "' && HOME='" + root +
                                "' SATL_NO_WINDOW=1 '" + satl_path +
                                "' --repl 2>&1 <typed.txt";
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == nullptr)
        return ran;
    char buffer[4096];
    size_t got = 0;
    while ((got = std::fread(buffer, 1, sizeof buffer, pipe)) > 0)
        ran.out.append(buffer, got);
    const int raw = pclose(pipe);
    ran.status = WIFEXITED(raw) ? WEXITSTATUS(raw) : -1;
    return ran;
}

const char *kMain =
    "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n";

// --- 1: the demonstration, run whole ------------------------------------------

void section_demonstration(const std::string &example)
{
    const Ran ran = run("'" + example + "/launch.satl'");
    check(ran.status == 0, "example/spaceships/launch.satl exits 0: " + ran.out);
    check(ran.out == "cargo: docked with nothing to load\n"
                     "launch.satl: ready\n"
                     "cargo: loading fuel\n"
                     "cargo: loading 2 of fuel\n"
                     "loads so far: 3\n"
                     "three crates weigh 120\n"
                     "a crate of apples\n",
          "the demonstration prints its seven lines in order -- the include's "
          "launch, the file's own, both includes inside main, a global, a "
          "capsule and a spacesuit from cargo.satl: " + ran.out);

    const Ran alone = run("'" + example + "/cargo.satl'");
    check(alone.status == 0 &&
              alone.out == "cargo: docked with nothing to load\n"
                           "cargo.satl run on its own\n",
          "a spaceship run on its own runs its no-argument launch and its own "
          "main: " + alone.out);

    const Ran checked = run("--check '" + example + "/launch.satl'");
    check(checked.status == 0 && checked.out.empty(),
          "--check loads the spaceship and finds nothing: " + checked.out);
}

// --- 2: launches, fitted and ordered -------------------------------------------

void section_launches()
{
    write("engine.satl",
          "satellite.include(satellite)\n"
          "satellite.library.starts = 0\n"
          "satellite.capsule settings()\n{\n"
          "    satellite.variable.number speed = 9\n}\n"
          "satellite.capsule.launch start()\n{\n"
          "    satellite.library.starts = satellite.library.starts + 1\n"
          "    satellite.console.display(\"start\")\n}\n"
          "satellite.capsule.launch start_at(satellite.variable.number n)\n{\n"
          "    satellite.library.starts = satellite.library.starts + 1\n"
          "    satellite.console.display(\"start_at\")\n}\n"
          "satellite.capsule.launch named(satellite.variable.string s)\n{\n"
          "    satellite.console.display(\"named \" + s)\n}\n"
          "satellite.capsule.launch float_too(satellite.variable.float f)\n{\n"
          "    satellite.console.display(\"float_too\")\n}\n");
    write("twice.satl",
          std::string("satellite.include(satellite)\n"
                      "satellite.include(engine)\n"
                      "satellite.include(engine(5))\n"
                      "satellite.include(engine.satl(\"x\"))\n") +
              kMain +
              "{\n"
              "    satellite.variable.number speed = satellite.library.engine.settings.speed\n"
              "    satellite.variable.number starts = satellite.library.engine.starts\n"
              "    satellite.variable.string a = speed.to_string()\n"
              "    satellite.variable.string b = starts.to_string()\n"
              "    satellite.console.display(a + \" \" + b)\n"
              "    satellite.return(satellite)\n}\n");

    const Ran ran = run("twice.satl");
    check(ran.status == 0, "three includes of one file run: " + ran.out);
    check(ran.out == "start\nstart_at\nfloat_too\nnamed x\n9 2\n",
          "each include runs every launch its arguments fit, in file order -- "
          "a number fits a number AND a float parameter -- the globals are set "
          "up once, and satellite.library.engine.settings.speed reads a "
          "capsule's declared variable across the file: " + ran.out);

    write("nofit.satl", std::string("satellite.include(satellite)\n") + kMain +
                            "{\n    satellite.include(engine(1, 2))\n"
                            "    satellite.return(satellite)\n}\n");
    const Ran refused = run("nofit.satl");
    check(refused.status == 1 && holds(refused.out, "S0734") &&
              holds(refused.out, "takes (number, number)") &&
              holds(refused.out, "(number), (string) and (float)") &&
              !holds(refused.out, "start"),
          "arguments that fit no launch are refused before any launch runs, "
          "naming what was handed over and what each launch takes: " +
              refused.out);
}

// --- 3: two files that include each other ---------------------------------------

void section_cycle()
{
    write("alpha.satl",
          std::string("satellite.include(satellite)\n"
                      "satellite.include(beta(\"from alpha\"))\n"
                      "satellite.library.greeting = \"set\"\n"
                      "satellite.capsule.launch hello()\n{\n"
                      "    satellite.console.display(\"alpha hello\")\n}\n"
                      "satellite.capsule.launch hello_with(satellite.variable.string who)\n{\n"
                      "    satellite.console.display(\"alpha hello \" + who)\n}\n"
                      "satellite.capsule shout()\n{\n"
                      "    satellite.console.display(\"alpha shout\")\n}\n") +
              kMain + "{\n    beta.ping()\n    satellite.return(satellite)\n}\n");
    write("beta.satl",
          "satellite.include(satellite)\n"
          "satellite.include(alpha(\"from beta\"))\n"
          "satellite.capsule.launch arrived(satellite.variable.string how)\n{\n"
          "    satellite.variable.string g = satellite.library.alpha.greeting\n"
          "    satellite.console.display(\"beta \" + how + \" \" + g)\n"
          "    alpha.shout()\n}\n"
          "satellite.capsule ping()\n{\n"
          "    satellite.console.display(\"beta ping\")\n}\n");

    const Ran ran = run("alpha.satl");
    check(ran.status == 0 &&
              ran.out == "alpha hello from beta\n"
                         "beta from alpha set\n"
                         "alpha shout\n"
                         "alpha hello\n"
                         "beta ping\n",
          "a spaceship that includes the file satl was given reaches file 0 -- "
          "every global is set before any include runs, beta's include of "
          "alpha runs alpha's launch and not alpha's includes again, and each "
          "calls the other: " + ran.out);
}

// --- 4: refusals ------------------------------------------------------------------

void section_refusals()
{
    write("ship.satl",
          "satellite.include(satellite)\n"
          "satellite.library.total = 7\n"
          "satellite.capsule divide(satellite.variable.number n)\n{\n"
          "    satellite.variable.number zero = 0\n"
          "    satellite.return(n / zero)\n}\n");

    const auto program = [](const std::string &top, const std::string &body) {
        return "satellite.include(satellite)\n" + top + kMain + "{\n" + body +
               "    satellite.return(satellite)\n}\n";
    };

    write("missing.satl", program("satellite.include(nowhere)\n", ""));
    Ran ran = run("missing.satl");
    check(ran.status == 1 && holds(ran.out, "S1601") &&
              holds(ran.out, "looked for nowhere.satl"),
          "a spaceship that is not there is S1601, naming where it looked: " +
              ran.out);

    write("taken.satl", program("satellite.include(ship)\n"
                                "satellite.capsule ship()\n{\n}\n", ""));
    ran = run("taken.satl");
    check(ran.status == 1 && holds(ran.out, "S1602") && holds(ran.out, "capsule"),
          "a spaceship named like the program's own capsule is S1602: " + ran.out);

    write("string.satl", program("satellite.include(\"ship.satl\")\n", ""));
    ran = run("string.satl");
    check(ran.status == 1 && holds(ran.out, "S1604"),
          "a spaceship named by a string is S1604: " + ran.out);

    write("members.satl", program("satellite.include(ship)\n",
                                  "    ship.nothing_here()\n"
                                  "    satellite.variable.number t = ship.total\n"));
    ran = run("members.satl");
    check(ran.status == 1 && holds(ran.out, "S0529") && holds(ran.out, "S0530") &&
              holds(ran.out, "satellite.library.ship.total"),
          "a name the spaceship does not declare is S0529, and its global "
          "reached bare is S0530 naming the road: " + ran.out);

    write("inship.satl", program("satellite.include(ship)\n",
                                 "    satellite.variable.number x = ship.divide(4)\n"));
    ran = run("inship.satl");
    check(ran.status == 1 && holds(ran.out, "satl: ship.satl:6:") &&
              holds(ran.out, "S0601") &&
              holds(ran.out, "called at line 5 of inship.satl"),
          "an error inside a spaceship is rendered against the spaceship's own "
          "file, and the call stack names the file the call was made in: " +
              ran.out);

    write("placement.satl",
          program("satellite.spacesuit s()\n{\n    satellite.public\n    {\n"
                  "        satellite.capsule.launch nope()\n        {\n        }\n"
                  "    }\n}\n",
                  ""));
    ran = run("placement.satl");
    check(ran.status == 1 && holds(ran.out, "S0246") && !holds(ran.out, "S0207") &&
              !holds(ran.out, "S0204"),
          "a launch inside a spacesuit is S0246, said once, with nothing "
          "cascading after it: " + ran.out);

    write("returns.satl",
          program("satellite.capsule.launch go() satellite.returns(satellite.variable.number)\n"
                  "{\n    satellite.return(1)\n}\n",
                  ""));
    ran = run("returns.satl");
    check(ran.status == 1 && holds(ran.out, "S0528"),
          "a launch that declares a return type is S0528: " + ran.out);

    write("brokenship.satl", "satellite.include(satellite)\nsatellite.capsule broken(\n");
    write("parsebad.satl", program("satellite.include(brokenship)\n", ""));
    ran = run("parsebad.satl");
    check(ran.status == 1 && holds(ran.out, "satl: brokenship.satl:"),
          "a spaceship that does not parse says so against its own file: " +
              ran.out);
}

// --- 5: across a thread, and back through the printer ----------------------------

void section_threads_and_printing()
{
    write("worker.satl",
          "satellite.include(satellite)\n"
          "satellite.spacesuit thing()\n{\n    satellite.public\n    {\n"
          "        satellite.capsule get() satellite.returns(satellite.variable.number)\n"
          "        {\n            satellite.return(3)\n        }\n    }\n}\n"
          "satellite.capsule count_to(satellite.variable.number n)\n{\n"
          "    satellite.variable.number i = 0\n"
          "    satellite.statement.while (i < n)\n    {\n        i = i + 1\n    }\n"
          "    satellite.return(i)\n}\n");
    write("threads.satl",
          std::string("satellite.include(satellite)\nsatellite.include(worker)\n") +
              kMain +
              "{\n"
              "    satellite.variable.thread t = satellite.thread.new(worker.count_to(1000))\n"
              "    t.start()\n"
              "    satellite.variable.number got = t.join()\n"
              "    satellite.variable.string shown = got.to_string()\n"
              "    satellite.console.display(shown)\n"
              "    satellite.return(satellite)\n}\n");
    Ran ran = run("threads.satl");
    check(ran.status == 0 && ran.out == "1000\n",
          "a thread runs a capsule another file declares: " + ran.out);

    const std::string printed =
        "satellite.include(satellite)\n\n"
        "satellite.include(worker.satl)\n\n"
        "satellite.capsule.launch start_up(worker.thing t)\n{\n}\n\n"
        "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n"
        "{\n"
        "    satellite.include(worker(1, \"two\"))\n"
        "    satellite.container.list<worker.thing> things = satellite.container.list()\n"
        "    worker.thing one\n"
        "    satellite.return(satellite)\n"
        "}\n";
    write("printed.satl", printed);
    ran = run("--unparse printed.satl");
    check(ran.status == 0 && ran.out == printed,
          "--unparse prints a launch, a qualified spacesuit type and an include "
          "inside a capsule back exactly as written: " + ran.out);
}

// --- 6: what the review found ------------------------------------------------
//
// 27 findings from four independent readers on 2026-09-13, fourteen defects
// once the duplicates were folded. One clause each, written against the
// reviewer's own reproduction, so the defect cannot come back quietly.

void section_review()
{
    const auto program = [](const std::string &top, const std::string &body) {
        return "satellite.include(satellite)\n" + top + kMain + "{\n" + body +
               "    satellite.return(satellite)\n}\n";
    };

    // A spaceship reaches only its own `satellite.library`.
    write("snoop.satl", "satellite.include(satellite)\n"
                        "satellite.capsule peek()\n{\n"
                        "    satellite.library.secret = \"overwritten\"\n}\n");
    write("leak.satl", program("satellite.library.secret = \"mine\"\n"
                               "satellite.include(snoop)\n",
                               "    snoop.peek()\n"));
    Ran ran = run("leak.satl");
    check(ran.status == 1 && holds(ran.out, "snoop.satl:4:") && holds(ran.out, "S0521"),
          "a spaceship cannot write its includer's global through its own bare "
          "satellite.library.secret: " + ran.out);

    // The file satl was given, included back, is reached by its name.
    write("back.satl", "satellite.include(satellite)\n"
                       "satellite.include(front)\n"
                       "satellite.capsule read()\n{\n"
                       "    satellite.variable.string s = satellite.library.front.word\n"
                       "    satellite.console.display(s)\n}\n");
    write("front.satl", program("satellite.library.word = \"front's\"\n"
                                "satellite.include(back)\n",
                                "    back.read()\n"));
    ran = run("front.satl");
    check(ran.status == 0 && ran.out == "front's\n",
          "satellite.library.<file 0's name>.global reads it from a spaceship: " +
              ran.out);

    // A spaceship calling satellite.main is refused, not handed the program's.
    write("mainer.satl", "satellite.include(satellite)\n"
                         "satellite.capsule call()\n{\n"
                         "    satellite.container.list<satellite.variable.string> a = satellite.container.list()\n"
                         "    satellite.main(a)\n}\n");
    write("callsmain.satl", program("satellite.include(mainer)\n",
                                    "    satellite.console.display(\"once\")\n"
                                    "    mainer.call()\n"));
    ran = run("callsmain.satl");
    check(ran.status == 1 && ran.out.find("once") == ran.out.rfind("once") &&
              holds(ran.out, "S0720") && holds(ran.out, "inside a spaceship"),
          "satellite.main called inside a spaceship is refused: " + ran.out);

    // S1602 in a nested file, and a name the language owns.
    write("leaf.satl", "satellite.include(satellite)\n"
                       "satellite.capsule ping()\n{\n}\n");
    write("twig.satl", "satellite.include(satellite)\n"
                       "satellite.include(leaf)\n"
                       "satellite.capsule leaf()\n{\n}\n");
    write("trunk.satl", program("satellite.include(twig)\n", ""));
    ran = run("trunk.satl");
    check(ran.status == 1 && holds(ran.out, "twig.satl:2:") && holds(ran.out, "S1602"),
          "a nested file naming a spaceship like its own capsule is S1602 at that "
          "file's include: " + ran.out);
    write("system.satl", "satellite.include(satellite)\n");
    write("owned.satl", program("satellite.include(system)\n", ""));
    ran = run("owned.satl");
    check(ran.status == 1 && holds(ran.out, "S0241"),
          "a spaceship named like a language word under satellite.library is "
          "S0241: " + ran.out);

    // A subclass object fits a launch typed with its parent, and the launch's
    // frame names the include as its call site.
    write("zoo.satl",
          "satellite.include(satellite)\n"
          "satellite.spacesuit animal()\n{\n    satellite.public\n    {\n"
          "        satellite.capsule noise()\n        {\n        }\n    }\n}\n"
          "satellite.spacesuit dog(animal)\n{\n}\n"
          "satellite.capsule.launch take(animal a)\n{\n"
          "    satellite.console.display(\"took it\")\n}\n"
          "satellite.capsule.launch fail(satellite.variable.number n)\n{\n"
          "    satellite.variable.number zero = 0\n"
          "    satellite.variable.number q = n / zero\n}\n");
    write("keeper.satl", program("satellite.include(zoo)\n",
                                 "    zoo.dog d\n"
                                 "    satellite.include(zoo(d))\n"
                                 "    satellite.include(zoo(5))\n"));
    ran = run("keeper.satl");
    check(ran.status == 1 && holds(ran.out, "took it") &&
              holds(ran.out, "in fail, called at line 7 of keeper.satl"),
          "a dog fits an animal parameter, and a refusal inside a launch names "
          "the include that ran it: " + ran.out);

    // One-file arms see a spaceship's name and say nothing about its insides;
    // --check analyses every file.
    const Ran resolved = run("--resolve front.satl");
    check(resolved.status == 0 && !holds(resolved.out, "S0521"),
          "--resolve on a file whose spaceship it has not loaded refuses "
          "nothing through it: " + resolved.out);
    write("ring.satl", "satellite.include(satellite)\n"
                       "satellite.spacesuit link()\n{\n    satellite.protected\n    {\n"
                       "        link next\n    }\n}\n");
    write("ringed.satl", program("satellite.include(ring)\n", ""));
    ran = run("--check ringed.satl");
    check(holds(ran.out, "ring.satl:6:") && holds(ran.out, "S1501"),
          "--check reports a ring of spacesuits in an included file, against "
          "that file: " + ran.out);
}

// --- 7: a launch runs when its include is reached, and then only ---------------
//
// The author, 2026-09-14: "so when the interpreter hits satellite.include
// (filename) it runs the launch inside of filename then and only then". A file
// always did; the prompt, which rebuilds its program for every line typed, ran
// every kept include again on every later line -- calling one of the file's
// capsules on the next line printed the launch first.
void section_prompt()
{
    write("pod.satl", "satellite.include(satellite)\n"
                      "satellite.include(inner)\n"
                      "satellite.capsule.launch hello()\n{\n"
                      "    satellite.console.display(\"pod launch\")\n}\n"
                      "satellite.capsule.launch hello_n(satellite.variable.number n)\n{\n"
                      "    satellite.console.display(\"pod launch with a number\")\n}\n"
                      "satellite.capsule ping()\n{\n"
                      "    satellite.console.display(\"pod ping\")\n}\n");
    write("inner.satl", "satellite.include(satellite)\n"
                        "satellite.capsule.launch arrive()\n{\n"
                        "    satellite.console.display(\"inner launch\")\n}\n");

    Ran ran = repl("satellite.include(pod)\n"
                   "pod.ping()\n"
                   "satellite.console.display(\"unrelated\")\n"
                   "pod.ping()\n");
    check(count_of(ran.out, "pod launch\n") == 1 &&
              count_of(ran.out, "inner launch") == 1 &&
              count_of(ran.out, "pod ping") == 2 && holds(ran.out, "unrelated"),
          "at the prompt, an include's launches run on the line that includes "
          "and on no later line -- capsule calls and other lines run none: " +
              ran.out);

    ran = repl("satellite.include(pod)\n"
               "satellite.variable.number five = 5\n"
               "satellite.include(pod(five))\n"
               "pod.ping()\n");
    check(count_of(ran.out, "pod launch\n") == 1 &&
              count_of(ran.out, "pod launch with a number") == 1 &&
              count_of(ran.out, "inner launch") == 1 &&
              count_of(ran.out, "pod ping") == 1,
          "a second include of a file on a later line runs that include's "
          "launches, once, and not the includes at the file's top again: " +
              ran.out);

    write("podhost.satl", std::string("satellite.include(satellite)\n"
                                      "satellite.include(pod)\n") +
                              kMain + "{\n    pod.ping()\n    pod.ping()\n"
                                      "    satellite.return(satellite)\n}\n");
    const Ran file = run("podhost.satl");
    check(file.status == 0 &&
              file.out == "inner launch\npod launch\npod ping\npod ping\n",
          "in a file the same include runs its nested include's launch and its "
          "own once, and calling its capsules runs neither again: " + file.out);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: spaceship_test <satl> <example/spaceships>\n");
        return 2;
    }
    char resolved[4096];
    satl_path = realpath(argv[1], resolved) ? resolved : argv[1];
    const std::string example = realpath(argv[2], resolved) ? resolved : argv[2];

    const char *tmp = std::getenv("TMPDIR");
    std::string pattern = std::string(tmp ? tmp : "/tmp") + "/spaceship_test.XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    if (mkdtemp(buffer.data()) == nullptr) {
        std::fprintf(stderr, "spaceship_test: no temporary directory\n");
        return 2;
    }
    root = buffer.data();

    section_demonstration(example);
    section_launches();
    section_cycle();
    section_refusals();
    section_threads_and_printing();
    section_review();
    section_prompt();

    std::system(("rm -rf '" + root + "'").c_str());
    if (failures != 0) {
        std::printf("spaceship_test: %d failed\n", failures);
        return 1;
    }
    std::printf("spaceship_test: ok\n");
    return 0;
}
