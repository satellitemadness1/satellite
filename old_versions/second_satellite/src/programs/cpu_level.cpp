// satl-cpu-level -- which satl this machine can run.
//
// Prints ONE WORD on stdout, and that word is the name of a build:
//
//     haswell     this machine has the whole x86-64-v3 instruction set
//     baseline    it does not, so it gets the build that assumes nothing
//
// One word because install.sh reads it with `$(...)` and picks a file with it.
// Anything friendlier would have to be parsed, and a parser between two
// programs in the same tree is a place for them to disagree. --explain is the
// friendly form, for a person who wants to know why they got what they got.
//
// THIS PROGRAM IS COMPILED AT THE BASELINE AND MUST STAY THERE. It is the one
// binary that runs before we know what the machine can do, so an -march that
// let the compiler emit a single AVX2 instruction here would turn "you get the
// portable build" into SIGILL on the machine that needed to be told that. See
// make_support/045-microarchitecture.mk, which compiles it with no -march at
// all and says the same thing from the build's side.
//
// It is also NOT INSTALLED. It answers one question, once, during an install,
// and the answer is then visible forever in `satl --version`, which prints the
// flags its own objects were compiled with. A machine that wants to re-ask the
// question can re-run install.sh.

#include <cstdio>
#include <cstring>

namespace {

// x86-64-v3 is the microarchitecture level Haswell introduced in 2013: AVX,
// AVX2, BMI1, BMI2, F16C, FMA, LZCNT, MOVBE and XSAVE on top of v2. AMD reaches
// it at Excavator (2015) and every Zen. So "haswell or newer" and "v3 or newer"
// name the same set of machines, and the second one is the one a compiler and a
// CPUID leaf can both be asked about exactly.
//
// __builtin_cpu_supports("x86-64-v3") IS THAT EXACT QUESTION, and asking it is
// the whole reason the haswell build is compiled with -march=x86-64-v3 rather
// than -march=haswell. -march=haswell additionally licenses AES, RDRND, PCLMUL
// and INVPCID, which every real Haswell has and which are NOT part of v3 -- so
// with that flag the set of instructions the compiler may emit would be a
// strict superset of the set this check verifies, and the gap between them is
// exactly where a wrong answer lives. Check what you compiled for; compile for
// what you check.
//
// Measured here 2026-08-26: Intel Xeon E5-2670 v3 (Haswell-EP) answers
// v2=1 v3=1 v4=0.
//
// __builtin_cpu_init() before the first query. It is redundant in main(), where
// the runtime has already run its own initialiser, and it is correct anyway --
// the documented rule is that a caller reaching __builtin_cpu_supports before
// that initialiser must call it, and "this call site is not one of those" is a
// fact about link order rather than about this file.
const char *detect()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    return __builtin_cpu_supports("x86-64-v3") ? "haswell" : "baseline";
#else
    // Not x86, so the levels do not exist and neither does a second build:
    // 045-microarchitecture.mk asks the COMPILER what it targets and builds one
    // satl on anything that is not x86-64. Answering "baseline" here is not a
    // guess about the machine -- it is the name of the only build there is.
    return "baseline";
#endif
}

// The same answer as a sentence, for somebody who ran this by hand because the
// install picked something they did not expect.
//
// It reports the LEVELS AROUND the one that decides, because the useful reply
// to "why did I get the portable build" is which rung the machine actually
// stands on, not a repeat of the word it already saw.
void explain()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    const int v2 = __builtin_cpu_supports("x86-64-v2");
    const int v3 = __builtin_cpu_supports("x86-64-v3");
    const int v4 = __builtin_cpu_supports("x86-64-v4");

    printf("This machine supports:  x86-64-v2 %s   x86-64-v3 %s   x86-64-v4 %s\n",
           v2 ? "yes" : "no ", v3 ? "yes" : "no ", v4 ? "yes" : "no ");
    printf("\n");

    if (v3) {
        printf("v3 is the level Haswell introduced in 2013 -- AVX2, BMI1, BMI2,\n"
               "FMA and the rest -- so this machine gets the haswell build.\n");
    } else {
        printf("v3 is the level Haswell introduced in 2013 -- AVX2, BMI1, BMI2,\n"
               "FMA and the rest -- and this machine does not have all of it, so\n"
               "it gets the baseline build, which assumes nothing beyond x86-64\n"
               "itself and runs anywhere.\n");
    }
    if (v4) {
        printf("\nv4 (AVX-512) is present and satellite does not build for it "
               "yet.\nThere are two builds, not three.\n");
    }
#else
    printf("This is not an x86 machine, so the x86-64 microarchitecture levels\n"
           "do not apply and satellite builds one satl rather than two.\n");
#endif
    printf("\nThe build that was installed says so itself: run `satl --version`\n"
           "and read the flags line.\n");
}

} // namespace

int main(int argc, char **argv)
{
    if (argc > 1) {
        if (strcmp(argv[1], "--explain") == 0) {
            explain();
            return 0;
        }
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("usage: satl-cpu-level              print haswell or baseline\n"
                   "       satl-cpu-level --explain    say which levels this "
                   "machine has\n"
                   "\n"
                   "Which satl build this machine can run. install.sh runs this\n"
                   "to choose one; it is not installed alongside satl.\n");
            return 0;
        }
        fprintf(stderr, "satl-cpu-level: unknown option %s\n", argv[1]);
        return 2;
    }

    // No newline handling to get wrong on the reading side: `$(...)` strips
    // trailing newlines, so install.sh sees the bare word either way, and a
    // person running this in a terminal gets their prompt back on its own line.
    printf("%s\n", detect());
    return 0;
}
