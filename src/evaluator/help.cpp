// satellite.help — the whole language on one screen.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"
#include "system_facts/version.hpp"

namespace satellite {

std::string help_overview()
{
    // Not a literal: the number comes from the same place --version reads it,
    // so the banner and the flag cannot drift into disagreeing. It said 0.1
    // until 2026-08-24, long after the language had stopped being 0.1.
    return "satellite " + version_line() +
" -- the whole language\n"
"\n"
"  declare    satellite.variable.number x = 1        bool number string time file\n"
"             satellite.container.list<satellite.variable.number> l\n"
"             satellite.container.map<satellite.variable.string, ...> m\n"
"             my_class thing            a spacesuit; thing(\"arg\") to construct\n"
"  values     satellite.bool.true  satellite.bool.false  satellite (the runtime)\n"
"  operators  + - * / %   == != < <= > >=   !   -x    l[i]  l[a:b]  l[:b]  l[a:]\n"
"\n"
"  capsule    satellite.capsule name(satellite.variable.number n)\n"
"                 satellite.returns(satellite.variable.number) { ... }\n"
"  spacesuit  satellite.spacesuit my_class(superclass) {\n"
"                 satellite.protected { ...fields... }\n"
"                 satellite.public    { my_class(args) {...}  ...capsules... } }\n"
"  control    satellite.statement.if (c) {} satellite.statement.else {}\n"
"             satellite.statement.while (c) {}   satellite.statement.for (i;c;s) {}\n"
"  return     satellite.return(value)\n"
"\n"
"  console    satellite.console.display(value)\n"
"                 display(100ms)              pause 100 ms between lines\n"
"  time       satellite.time.now()                  .minus(t) .nanoseconds()\n"
"  file       satellite.file.open(path, \"read\"|\"write\"|\"append\"|\"read_append\")\n"
"                 satellite.file.new(path[, mode])   refuses to clobber\n"
"                 .ok() .read() .write(s) .clear() .open() .close()\n"
"                 .error() .path()\n"
"  directory  satellite.directory.current()  .change(dir)  .exists(dir)\n"
"                 .list()  .list(dir)        the names in it, sorted\n"
"  random     satellite.random.ultra(40)      fast normal ultra; 40 digits\n"
"                 .range(low, high)           inclusive at both ends\n"
"                 satellite.help(random)      and (ultra) (random.ultra)\n"
"  system     satellite.system.home()               $HOME\n"
"                 .delete(path)  .delete(file)  a file, or an empty dir\n"
"  analyze    satellite.analyze(\"file.satl\")        what a spaceship holds\n"
"  globals    satellite.library.<namespace>.<name>\n"
"\n"
"  help       satellite.help          this text (the () is optional)\n"
"             satellite.help(value)   the methods that value answers to\n"
"             satellite.directory     what one module answers to\n"
"             satellite.help(topic)   a topic below, bare or in quotes\n"
"\n"
"  topics     arguments       what satellite.main is handed, and how any\n"
"                             capsule's arguments are passed. Also: args\n"
"                             arg argz argument argumentz, and each of\n"
"                             those with \"system \" in front.\n"
"             random          the three tiers, and what none of them is\n"
"             fast normal ultra          one tier each, with its cost\n"
"             random.ultra    the same, written the way the path is\n"
"             wide            numbers past the 100,000 digit ceiling\n";
}

// What ONE module answers to.
//
// `satellite.directory` is a path the language owns, so asking it what it can
// do should not have to start with remembering which of its commands you
// wanted -- the same argument that makes satellite.help reachable without
// parentheses, one level down the tree. An empty answer means the name is not
// a module, which is how both callers tell.
std::string help_for_module(const std::string &module)
{
    if (module == "console")
        return "satellite.console\n"
               "  .display(value)        print it, one line\n"
               "  .display(100ms)        pause that long between lines, from here on\n";
    if (module == "time")
        return "satellite.time\n"
               "  .now()                 an instant, read off the clock\n"
               "                         .minus(t) and .nanoseconds() on what it hands back\n";
    if (module == "file")
        return "satellite.file\n"
               "  .open(path, \"read\")    also \"write\", \"append\", \"read_append\"\n"
               "  .new(path)             make one that is NOT there; read_append\n"
               "  .new(path, mode)       the same, in the mode you name\n"
               "                         a path that exists is .ok() false, not an error\n"
               "                         there is no \"text\" or \"binary\": a string is bytes\n"
               "                         .ok() .read() .write(s) .clear() .open() .close()\n"
               "                         .error() .path()\n";
    if (module == "directory")
        return "satellite.directory\n"
               "  .current()             where you are\n"
               "  .change(dir)           go there; true or false, never an error\n"
               "  .exists(dir)           is there a directory there\n"
               "  .list()  .list(dir)    the names in it, sorted\n";
    if (module == "system")
        return "satellite.system\n"
               "  .home()                        this user's home directory\n"
               "  .delete(path)                  a file, or an EMPTY directory\n"
               "  .delete(file)                  the path that handle opened\n"
               "                                 true or false, never an error\n"
               "                                 does not recurse; walk .list()\n"
               "  .memory.used([unit])           and .free .total .swap .main\n"
               "  .memory.swap.used()            swap in use\n"
"  .memory.this.used()            this thread's stack; .free .available\n"
               "  .memory.frequency()  .bit()    from SMBIOS; 0 unless root\n"
               "  unit is \"b\" \"kb\" \"mb\" \"gb\" \"tb\"; mb by default\n";
    if (module == "random")
        return "satellite.random\n"
               "  .fast(digits)          also .normal(digits) and .ultra(digits)\n"
               "  .fast.range(low, high) inclusive at both ends; same three tiers\n";
    return std::string();
}

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

// What a particular value can do. The type table below is the same one
// call_method dispatches on, so a method that exists is a method that is
// listed -- a help text that drifts from the code is worse than none.
//
// That contract is why case 5 exists at all. An instance used to fall to the
// default and be told "nil has no methods", which was survivable while every
// method it had was its spacesuit's own; §8.7 gave it a built-in .size(), and
// the sentence became false about a language-owned method. .size() is on every
// line below for the same reason -- it answers for every receiver that has
// methods at all, so it belongs in every list, not in a footnote.
std::string help_for(const Value &value)
{
    switch (value.index()) {
    case 1:  return "satellite.variable.bool\n"
                    "  .negate()  .and(b)  .or(b)  .to_string()  .size()\n";
    case 2:  return "satellite.variable.number\n"
                    "  .plus(n) .minus(n) .times(n) .divided_by(n) .modulo(n)\n"
                    "  .abs() .floor() .ceil() .round() .to_string()\n"
                    "  .digits() -> decimal digits   .size() -> bytes\n"
                    "  no .length(): a number holds no items\n";
    case 3:  return "satellite.variable.string\n"
                    "  .length() .concat(s) .contains(s) .starts_with(s)\n"
                    "  .ends_with(s) .to_string() .size()  s[i]  s[a:b]\n";
    case 4:  return "satellite.container.list\n"
                    "  .length() .append(v) .first() .last() .contains(v)\n"
                    "  .to_string() .size()                l[i]  l[a:b]\n";
    case 5:  return "a spacesuit instance\n"
                    "  its own satellite.public capsules, which answer first\n"
                    "  .size() -> bytes, unless the spacesuit defines size()\n";
    case 6:  return "satellite.variable.time\n"
                    "  .minus(t) -> nanoseconds   .nanoseconds()  .to_string()\n"
                    "  .size()\n";
    case 9:  return "satellite.variable.binary / satellite.variable.hex\n"
                    "  .digits() -> how many digits, leading zeros included\n"
                    "  .bytes()  -> the packed size, rounded up to a byte\n"
                    "  .to_number() .to_hex() .to_binary() .to_string()\n"
                    "  .concat(v) -> same radix only          .size()\n"
                    "  the width is part of the value: x0009 != x9\n";
    case 7:  return "satellite.variable.file\n"
                    "  .ok() .read() .write(s) .clear() .close() .error() .path()\n"
                    "  .read() is the whole file; .write(s) adds no newline\n"
                    "  .open() reopens after a close; a live handle is left alone\n"
                    "  .size() -> the value's bytes, not the file's\n";
    case 8:  return "satellite.container.map\n"
                    "  .get(k) .set(k, v) .has(k) .remove(k) .length()\n"
                    "  .keys() .values() .to_string() .size()  m[k]\n"
                    "  a key is a string or a number; a missing key is an error\n";
    case 10: return "the arguments object -- satellite.main's parameter\n"
                    "  a.name          a bare selector: a.cxx_compiler\n"
                    "  a[\"name\"] .get(\"name\") .has(\"name\") .names()\n"
                    "  .length() a[i] .first() .last() .contains(v)\n"
                    "                  the COMMAND LINE only, as always\n"
                    "  .count()        every entry, command line and machine\n"
                    "  .to_string()    one entry per line, names beside data\n"
                    "  satellite.help(arguments)   the whole topic\n";
    default: break;
    }
    return "nil has no methods\n";
}

// A language-owned VALUE reached by a module path, as against a language-owned
// FUNCTION. §5 anticipated the shape when it warned not to make a trailing '('
// mandatory after a module path, "or module constants like satellite.math.pi
// become errors".
//
// satellite.bool.true and satellite.bool.false are the two satellite.variable.
// bool values, and they are the only way to write either one. The spelling
// costs no parser rule, no lexer keyword and no second reserved word: segment-1
// dispatch already sends `bool` down the module path like any other name.
//
// It cannot be satellite.variable.bool.true, however much that reads like the
// type it belongs to, because §4 reserves satellite.variable.* as a TYPE
// namespace in which no path is ever a value expression — and that reservation
// is what keeps '<' unambiguous. `bool` as a module segment and `bool` as a
// type name are different things, and keeping them apart is the point.
//
// The obvious alternative, bare `true` and `false`, would be the language's
// second and third reserved words. §1 has exactly one.

} // namespace satellite
