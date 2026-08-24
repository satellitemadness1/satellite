// satellite.help's TOPIC pages — arguments, random, the three tiers, wide.
//
// Split out of src/evaluator/help.cpp, which had reached 492 lines. The seam is
// one the file already had: help.cpp answers WHAT IS THERE -- the whole language
// on one screen, a module's functions, a value's methods -- and the pages below
// answer HOW IT WORKS, at a length no listing has room for. The six topic texts
// are two thirds of the original file on their own.
//
// help_overview() stayed behind, and this file must not grow anything that
// wants it: the banner reads version_line(), and the Makefile hands
// VERSION_DEFS to help.o and to no other object in the tree.
//
// Moved verbatim. Not one line of topic text below was reworded, and the
// measured numbers in it are still the numbers that were measured.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// A TOPIC, as against a value or a module.
//
// satellite.help already answered two questions -- "what can this VALUE do"
// and "what does this MODULE answer to" -- and neither of them is the one
// somebody has when they type satellite.help(ultra). That question is about a
// thing the language does, not about a receiver or a namespace, and the honest
// answers to it are longer than a method list and carry a warning a method list
// has nowhere to put.
//
// The topic name is matched BARE (satellite.help(ultra)) and QUOTED
// (satellite.help("ultra")) and dotted (satellite.help(random.ultra)), and all
// three arrive here as the same string. §1 is not weakened by the bare form:
// see the note in src/environment/names.cpp, which is what lets a bare word
// reach this table only after every scope the user owns has said no.
//
// An empty answer means the name is not a topic, which is how the callers tell.
//
// EVERY NUMBER BELOW WAS MEASURED on the machine this was written on, not
// reasoned about. The tier timings come from timing one call of each; the
// concatenation figures from building a million digits and counting them.
std::string help_for_topic(const std::string &topic)
{
    // ONE topic, however it is spelled. `ultra`, `random.ultra` and
    // `satellite.random.ultra` are the same question, and so are `if` and
    // `satellite.statement.if`. The long form is how the language actually
    // writes the path; the short form is what somebody types once they know
    // which one they mean. Answering one spelling and not the other would make
    // the unanswered one look like it named nothing.
    //
    // Stripped HERE, in one place, so no caller has to know: the quoted
    // spelling in src/evaluator/modules.cpp and the bare one in
    // src/evaluator/expr.cpp both arrive with whatever the user wrote.
    //
    // One asymmetry, and it is a property of the grammar rather than an
    // oversight: the control-flow topics can only be reached QUOTED.
    // satellite.help(satellite.statement.if) does not parse at all, because §5
    // dispatches on segment 1 and `statement` has a parse rule of its own -- so
    // the parser sees the beginning of an if STATEMENT where an argument
    // should be. satellite.help("satellite.statement.if") has no such trouble,
    // for §19.1's reason: a string literal was never a name, so it can carry a
    // spelling without that spelling having to parse as one.
    static const std::string kPrefixes[] = {
        "satellite.statement.",
        "satellite.random.",
        "random.",
    };

    std::string key = topic;
    for (const std::string &prefix : kPrefixes) {
        if (key.rfind(prefix, 0) == 0) {
            key = key.substr(prefix.size());
            break;
        }
    }

    // THE ARGUMENTS TOPIC, twelve spellings onto one entry, as two rules
    // rather than twelve table rows.
    //
    // Rule one strips a leading `system`, in either of the two ways somebody
    // would type it: `system args` reads like English and `system.args` reads
    // like a path, and refusing one of them would be refusing it for no reason
    // a user could guess.
    //
    // Rule two maps the six arg spellings onto `arguments`. They exist because
    // the parameter can be NAMED any of them (plans/arguments.txt, DECISION 1)
    // and somebody who called theirs `argz` will ask about `argz`.
    //
    // The two with a space are reachable only QUOTED --
    // satellite.help("system args") -- because a bare `system args` is two
    // tokens and no change to §3 is worth making it one. The topic text says
    // so in its own first line rather than leaving it to be discovered.
    {
        static const std::string kSystem[] = {"system ", "system."};
        for (const std::string &prefix : kSystem) {
            if (key.rfind(prefix, 0) == 0) {
                key = key.substr(prefix.size());
                break;
            }
        }
        if (key == "arg" || key == "args" || key == "argz" ||
            key == "argument" || key == "argumentz")
            key = "arguments";
    }

    if (key == "arguments")
        return
"the arguments object, and passing arguments generally\n"
"\n"
"  Spellings that reach this page: arguments, argument, argumentz, args,\n"
"  arg, argz -- each bare or quoted, and each with `system ` or `system.`\n"
"  in front. The two with a space need the quotes: satellite.help(\"system\n"
"  args\"), because a bare `system args` is two tokens and not a name.\n"
"\n"
"-- PART ONE: THE OBJECT satellite.main IS HANDED ---------------------------\n"
"\n"
"  satellite.capsule satellite.main(\n"
"      satellite.container.list<satellite.variable.string> arguments)\n"
"\n"
"  THE NAME IS YOURS. arguments, args, argz, argument, argumentz, arg -- or\n"
"  banana. It is an ordinary parameter in an ordinary frame slot, and §1 says\n"
"  a bare identifier names something you own, so the language does not read\n"
"  your spelling and behave differently. The six above are what the docs use.\n"
"\n"
"  THE COMMAND LINE, exactly as it has always been:\n"
"    a.length()      how many command-line arguments, a[0] included\n"
"    a[0]            the script, as argv[0] in C\n"
"    a[1] .. a[n-1]  what the user typed, in order\n"
"    a.first() .last() .contains(v)   over those, and nothing else\n"
"\n"
"  .length() COUNTS THE COMMAND LINE AND NOTHING ELSE, on purpose. Every\n"
"  program that takes arguments writes\n"
"      satellite.statement.for (satellite.variable.number i = 1;\n"
"                               i < a.length(); i = i + 1)\n"
"  and if .length() counted the entries below, that loop would walk off the\n"
"  user's arguments and start reading the kernel release as though it had\n"
"  been typed. So the rest is reached BY NAME.\n"
"\n"
"  BY NAME -- three spellings, all the same lookup:\n"
"    a.cxx_compiler          a bare selector. The short one.\n"
"    a[\"cxx_compiler\"]       when the name is computed\n"
"    a.get(\"cxx_compiler\")   the same, spelled as a call\n"
"    a.has(\"cxx_compiler\")   ask without failing\n"
"    a.names()               every name, in order, as a list<string>\n"
"    a.count()               every entry -- command line AND machine\n"
"\n"
"  A NAME THAT IS NOT THERE IS AN ERROR, never \"\", and the message lists\n"
"  every name that is. A typo that quietly returns the empty string is a bug\n"
"  that surfaces three capsules away. .has() is how to ask without failing.\n"
"\n"
"  satellite.console.display(a) prints every entry, one per line, with the\n"
"  names beside the data. That is what .to_string() gives back.\n"
"\n"
"  WHAT IS IN IT\n"
"    the command line   program  argument_1..n  argument_count\n"
"    where you are      current_directory  home_directory  username\n"
"                       hostname\n"
"    the system         operating_system  kernel_release  kernel_version\n"
"                       architecture  distribution  distribution_id\n"
"                       distribution_version\n"
"    what built satl    cxx_compiler  cxx_compiler_version  cxx_standard\n"
"                       cxx_flags  make_version  standard_library\n"
"                       c_library  built\n"
"    the interpreter    interpreter  interpreter_version  library_path\n"
"                       library_path_source\n"
"    the process        process_id  parent_process_id  thread_count\n"
"                       page_size  pointer_bits  byte_order\n"
"    the session        shell  terminal  language\n"
"\n"
"  A fact this machine will not give up is PRESENT and says so in words:\n"
"  `unrecorded` when the build never passed it, `unavailable` when the system\n"
"  declined. An entry is never silently missing, so .has() can tell \"no such\n"
"  name\" from \"no such answer here\".\n"
"\n"
"  $PATH is deliberately not among them: unbounded, and the likeliest of all\n"
"  of them to hold something private in an object people paste into reports.\n"
"\n"
"  The object is READ-ONLY and a program cannot build one. There is no\n"
"  satellite.variable.arguments to declare. satellite.library is where a\n"
"  program records something of its own.\n"
"\n"
"-- PART TWO: PASSING ARGUMENTS, IN GENERAL ---------------------------------\n"
"\n"
"  satellite.capsule show(satellite.variable.string label,\n"
"                         satellite.variable.number n) { ... }\n"
"  show(\"count\", 41)\n"
"\n"
"  POSITIONAL, always. There is no calling by name, no default argument and\n"
"  no overloading (§12) -- one capsule, one name, one parameter list. Too few\n"
"  or too many is an error at the call, naming both counts.\n"
"\n"
"  A PARAMETER IS A FRAME SLOT (§6). Each activation gets its own, so a\n"
"  recursive call's parameters never overwrite its caller's, and a local\n"
"  declared in the body shadows nothing outside it. A capsule is lexically\n"
"  closed: it cannot see a top-level variable by bare name, only through\n"
"  satellite.library.<namespace>.<name>.\n"
"\n"
"  WHAT THE CALLEE CAN CHANGE, which is the part that catches people out:\n"
"    number string bool time    VALUES. The callee has a copy. Nothing it\n"
"                               does is visible to the caller.\n"
"    list map                   the callee sees the caller's items, but\n"
"                               .append() and .set() publish through the\n"
"                               storage slot they were called on -- so a\n"
"                               callee mutating a list parameter changes its\n"
"                               own slot, not yours.\n"
"    spacesuit                  a REFERENCE (§14). A method that writes a\n"
"                               field IS visible to the caller. Two names for\n"
"                               one object are two names for one object.\n"
"    file                       a reference to an open file description, so a\n"
"                               .close() in a callee closes yours too.\n"
"\n"
"  RETURNING\n"
"    satellite.return(v)        hands v back and ends the capsule\n"
"    satellite.return(satellite) succeeds without a value\n"
"    satellite.returns(T)       is PARSED AND NOT ENFORCED at run time. It\n"
"                               documents intent today; it does not check.\n"
"\n"
"  satellite.main takes AT MOST ONE parameter, and it must be spelled\n"
"  satellite.container.list<satellite.variable.string>. A main with no\n"
"  parameter at all is legal and gets nothing.\n";

    if (key == "random")
        return
"satellite.random -- three tiers, and what none of them is\n"
"\n"
"  .fast(digits)          50-100 ms of spin per call\n"
"  .normal(digits)        250-300 ms\n"
"  .ultra(digits)         2000-3000 ms\n"
"  .<tier>.range(lo, hi)  inclusive at BOTH ends\n"
"\n"
"  ultra(40) is uniform over [0, 10^40) -- zero through forty nines. About\n"
"  one draw in ten prints 39 digits or fewer, because a leading zero is not\n"
"  printed and a uniform draw has one a tenth of the time. That is what\n"
"  uniform means; a draw that always printed 40 digits would not be one.\n"
"\n"
"  NOT SECURE, and no tier is. PCG makes no cryptographic claim and its\n"
"  state is recoverable from its output. A longer spin buys a wider spread\n"
"  of possible seeds and nothing else -- roughly 30 bits, measured, whatever\n"
"  the tier. Do not key anything on this.\n"
"\n"
"  satellite.help(ultra)  one tier in full\n"
"  satellite.help(wide)   numbers past the 100,000 digit ceiling\n";

    if (key == "fast")
        return
"satellite.random.fast(digits) -- the short tier\n"
"\n"
"  50-100 ms of throwaway spin per call, then the draw.\n"
"  Uniform over [0, 10^digits). digits is at most 100000.\n"
"  .range(low, high) is inclusive at both ends.\n"
"\n"
"  The spin is a fixed cost per CALL and not per digit.\n"
"  Not secure -- see satellite.help(random).\n";

    if (key == "normal")
        return
"satellite.random.normal(digits) -- the middle tier\n"
"\n"
"  250-300 ms of throwaway spin per call, then the draw.\n"
"  Uniform over [0, 10^digits). digits is at most 100000.\n"
"  .range(low, high) is inclusive at both ends.\n"
"\n"
"  The spin is a fixed cost per CALL and not per digit.\n"
"  Not secure -- see satellite.help(random).\n";

    if (key == "ultra")
        return
"satellite.random.ultra(digits) -- the long tier\n"
"\n"
"  2000-3000 ms of throwaway spin per call, then the draw.\n"
"  Uniform over [0, 10^digits). digits is at most 100000; asking for more\n"
"  is REFUSED and never clamped, because a program that asks for a million\n"
"  digits has made a mistake and a quiet hundred thousand would hide it.\n"
"\n"
"  THE SPIN IS PER CALL, NOT PER DIGIT. Measured here:\n"
"      ultra(1)        2.40 s  ->      1 digit\n"
"      ultra(1000)     2.12 s  ->   1000 digits\n"
"      ultra(100000)   2.13 s  -> 100000 digits\n"
"  One digit costs what a hundred thousand costs. Chunking a wider number\n"
"  out of several calls therefore MULTIPLIES the spin: ten calls is about\n"
"  21 s of spin before any arithmetic. See satellite.help(wide).\n"
"\n"
"  ULTRA IS NOT A SECURITY TIER, and the name is about how long it takes.\n"
"  Every tier seeds from the kernel and then funnels that seed through a\n"
"  32-bit accumulator, keeping about 30 bits of spread -- measured, and the\n"
"  same 30 bits however long the spin runs, because the fold is a moving\n"
"  average and not a sum. A longer spin is a statistical character, not\n"
"  more entropy. PCG state is recoverable from PCG output. Do not key\n"
"  anything on this. The route to a tier that could carry the word is\n"
"  getrandom(2), and it is not taken.\n";

    if (key == "wide")
        return
"random numbers past the 100,000 digit ceiling\n"
"\n"
"  CONCATENATE draws, never add them. Addition carries, it does not append:\n"
"  ten 40-digit draws ADDED make a 41-digit number, not a 400-digit one, and\n"
"  the sum is bell-shaped rather than uniform. Concatenation is\n"
"\n"
"      acc = acc * 10^digits + satellite.random.fast(digits)\n"
"\n"
"  and it IS uniform: each chunk is uniform on [0, 10^digits), so the chunks\n"
"  are the base-10^digits digits of the answer, each one uniform and\n"
"  independent. Verified at a million digits: every digit within 0.023%% of\n"
"  10%%, chi-square 2.62 against 16.9 at p=0.05.\n"
"\n"
"  Cost, measured, ten chunks of 100000 digits:\n"
"      fast    about 18 s     ultra   about 43 s\n"
"  The arithmetic is quadratic in the total width, so widening is not free.\n"
"\n"
"  LENGTH IS NOT STRENGTH. Ten chunks carry about 300 bits of\n"
"  unpredictability however many digits come out. See satellite.help(ultra).\n";

    return std::string();
}

} // namespace satellite
