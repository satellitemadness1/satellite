// satellite/cpu_level/cpu_level.cpp -- satl-cpu-level: which of satl's builds this
// machine can run, and the best of them.
//
//     satl-cpu-level              one word: the build to run here, or "baseline"
//     satl-cpu-level --explain    every build, and what each needs that this machine lacks
//     satl-cpu-level <folder>     the builds in <folder>, not in cpu/ beside this program
//     satl-cpu-level --runs <needs file>   exit 0 when this machine has everything the list
//                                 names, 1 (and what it lacks) when not -- make asks this
//
// 003's satl-cpu-level (old_versions/second_satellite/src/programs/cpu_level.cpp) chose
// between two builds, baseline and haswell. The author, 2026-09-23: "if there are 137
// targets, we could technically just build all targets, and alter satl-cpu-level to check
// for them". make cpus builds 53 (make_support/055-cpus.mk says which and why), each into
// cpu/<processor>/ with a file `needs`: the compiler's own list of every instruction set
// -march=<processor> let it use, as the macros it defined (__AVX2__, __AVX512F__ ...).
//
// A BUILD IS CHOSEN BY WHAT IT NEEDS, NEVER BY THE PROCESSOR'S NAME. 003's rule, kept:
// "check what you compiled for; compile for what you check". A machine is asked about
// every instruction set on a build's list, and a build is runnable only when every one is
// there. A name would lie exactly where it matters: a virtual machine that hides AVX-512
// from a Sapphire Rapids guest still calls it a Sapphire Rapids.
//
// A NEED THIS PROGRAM CANNOT ASK ABOUT MAKES A BUILD UNRUNNABLE, which is the safe way to
// be wrong: a compiler newer than this table is a build not chosen, never a crash.
//
// EVERY QUESTION COMPILES WITH THE OLDEST COMPILER THE TREE FALLS BACK TO -- g++ 14, the
// system's, which 010-compiler.mk uses where clang-current is missing -- and with clang 21,
// 24 and g++ 17 (the review, 2026-09-23: g++ 14 refused seven names and the ordinary make
// failed there). And every answer is one g++ 14's runtime can give: clang links libgcc's
// __cpu_indicator_init, and a name newer than that runtime is answered no forever. The
// newer ones are asked of the processor itself (CPUID), below.
//
// THIS PROGRAM IS COMPILED AT THE BASELINE, as 003's was: it runs before anything is
// known about the machine, so it may not assume anything about it.

#include <algorithm>
#include <cpuid.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

struct Askable {
    const char *macro;
    bool (*has)();
};

// THE STATE THE KERNEL SAVES FOR US: XCR0, read only when the processor says the kernel
// turned XSAVE on (CPUID 1, ECX bit 27) -- xgetbv would fault otherwise.
bool kernel_saves(unsigned long long wanted)
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid(1, &a, &b, &c, &d) == 0 || (c & (1u << 27)) == 0) return false;
    unsigned low = 0, high = 0;
    __asm__ volatile("xgetbv" : "=a"(low), "=d"(high) : "c"(0));
    return ((static_cast<unsigned long long>(high) << 32 | low) & wanted) == wanted;
}

// AVX10's VERSION, 0 for none: CPUID 7.1 EDX bit 19 says AVX10 is there, leaf 0x24 EBX's
// low byte which version, and the kernel must save the vector, mask and 512-bit state
// (XCR0 bits 1, 2, 5, 6, 7).
unsigned avx10_version()
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid_count(7, 1, &a, &b, &c, &d) == 0 || (d & (1u << 19)) == 0) return 0;
    if (!kernel_saves(0xE6)) return 0;
    if (__get_cpuid_count(0x24, 0, &a, &b, &c, &d) == 0) return 0;
    return b & 0xFF;
}

// Every macro a -march of clang 24 defines beyond the baseline that __builtin_cpu_supports
// can take -- 81, each compiled with -Werror under g++ 14, clang 21, clang 24 and g++ 17
// (2026-09-23). Generated from those compilers' answers, not typed.
const Askable kAskable[] = {
    {"__SSE3__", [] { return __builtin_cpu_supports("sse3") != 0; }},
    {"__SSSE3__", [] { return __builtin_cpu_supports("ssse3") != 0; }},
    {"__POPCNT__", [] { return __builtin_cpu_supports("popcnt") != 0; }},
    {"__SSE4_1__", [] { return __builtin_cpu_supports("sse4.1") != 0; }},
    {"__SSE4_2__", [] { return __builtin_cpu_supports("sse4.2") != 0; }},
    {"__PRFCHW__", [] { return __builtin_cpu_supports("prfchw") != 0; }},
    {"__PCLMUL__", [] { return __builtin_cpu_supports("pclmul") != 0; }},
    {"__XSAVE__", [] { return __builtin_cpu_supports("xsave") != 0; }},
    {"__MOVBE__", [] { return __builtin_cpu_supports("movbe") != 0; }},
    {"__AVX__", [] { return __builtin_cpu_supports("avx") != 0; }},
    {"__XSAVEOPT__", [] { return __builtin_cpu_supports("xsaveopt") != 0; }},
    {"__LZCNT__", [] { return __builtin_cpu_supports("lzcnt") != 0; }},
    {"__RDRND__", [] { return __builtin_cpu_supports("rdrnd") != 0; }},
    {"__F16C__", [] { return __builtin_cpu_supports("f16c") != 0; }},
    {"__FSGSBASE__", [] { return __builtin_cpu_supports("fsgsbase") != 0; }},
    {"__BMI__", [] { return __builtin_cpu_supports("bmi") != 0; }},
    {"__FMA__", [] { return __builtin_cpu_supports("fma") != 0; }},
    {"__AES__", [] { return __builtin_cpu_supports("aes") != 0; }},
    {"__BMI2__", [] { return __builtin_cpu_supports("bmi2") != 0; }},
    {"__AVX2__", [] { return __builtin_cpu_supports("avx2") != 0; }},
    {"__RDSEED__", [] { return __builtin_cpu_supports("rdseed") != 0; }},
    {"__XSAVES__", [] { return __builtin_cpu_supports("xsaves") != 0; }},
    {"__XSAVEC__", [] { return __builtin_cpu_supports("xsavec") != 0; }},
    {"__CLFLUSHOPT__", [] { return __builtin_cpu_supports("clflushopt") != 0; }},
    {"__ADX__", [] { return __builtin_cpu_supports("adx") != 0; }},
    {"__SHA__", [] { return __builtin_cpu_supports("sha") != 0; }},
    {"__CLWB__", [] { return __builtin_cpu_supports("clwb") != 0; }},
    {"__PKU__", [] { return __builtin_cpu_supports("pku") != 0; }},
    {"__RDPID__", [] { return __builtin_cpu_supports("rdpid") != 0; }},
    {"__VPCLMULQDQ__", [] { return __builtin_cpu_supports("vpclmulqdq") != 0; }},
    {"__VAES__", [] { return __builtin_cpu_supports("vaes") != 0; }},
    {"__GFNI__", [] { return __builtin_cpu_supports("gfni") != 0; }},
    {"__SGX__", [] { return __builtin_cpu_supports("sgx") != 0; }},
    {"__SHSTK__", [] { return __builtin_cpu_supports("shstk") != 0; }},
    {"__AVX512F__", [] { return __builtin_cpu_supports("avx512f") != 0; }},
    {"__AVX512CD__", [] { return __builtin_cpu_supports("avx512cd") != 0; }},
    {"__MOVDIRI__", [] { return __builtin_cpu_supports("movdiri") != 0; }},
    {"__MOVDIR64B__", [] { return __builtin_cpu_supports("movdir64b") != 0; }},
    {"__AVX512VL__", [] { return __builtin_cpu_supports("avx512vl") != 0; }},
    {"__AVX512DQ__", [] { return __builtin_cpu_supports("avx512dq") != 0; }},
    {"__AVX512BW__", [] { return __builtin_cpu_supports("avx512bw") != 0; }},
    {"__PTWRITE__", [] { return __builtin_cpu_supports("ptwrite") != 0; }},
    {"__AVXVNNI__", [] { return __builtin_cpu_supports("avxvnni") != 0; }},
    {"__PCONFIG__", [] { return __builtin_cpu_supports("pconfig") != 0; }},
    {"__WAITPKG__", [] { return __builtin_cpu_supports("waitpkg") != 0; }},
    {"__SSE4A__", [] { return __builtin_cpu_supports("sse4a") != 0; }},
    {"__SERIALIZE__", [] { return __builtin_cpu_supports("serialize") != 0; }},
    {"__AVX512VNNI__", [] { return __builtin_cpu_supports("avx512vnni") != 0; }},
    {"__AVX512VPOPCNTDQ__", [] { return __builtin_cpu_supports("avx512vpopcntdq") != 0; }},
    {"__AVX512VBMI__", [] { return __builtin_cpu_supports("avx512vbmi") != 0; }},
    {"__AVX512IFMA__", [] { return __builtin_cpu_supports("avx512ifma") != 0; }},
    {"__AVX512VBMI2__", [] { return __builtin_cpu_supports("avx512vbmi2") != 0; }},
    {"__AVX512BITALG__", [] { return __builtin_cpu_supports("avx512bitalg") != 0; }},
    {"__UINTR__", [] { return __builtin_cpu_supports("uintr") != 0; }},
    {"__ENQCMD__", [] { return __builtin_cpu_supports("enqcmd") != 0; }},
    {"__WBNOINVD__", [] { return __builtin_cpu_supports("wbnoinvd") != 0; }},
    {"__HRESET__", [] { return __builtin_cpu_supports("hreset") != 0; }},
    {"__AVX512BF16__", [] { return __builtin_cpu_supports("avx512bf16") != 0; }},
    {"__MWAITX__", [] { return __builtin_cpu_supports("mwaitx") != 0; }},
    {"__AVXVNNIINT8__", [] { return __builtin_cpu_supports("avxvnniint8") != 0; }},
    {"__AVXNECONVERT__", [] { return __builtin_cpu_supports("avxneconvert") != 0; }},
    {"__AVXIFMA__", [] { return __builtin_cpu_supports("avxifma") != 0; }},
    {"__WIDEKL__", [] { return __builtin_cpu_supports("widekl") != 0; }},
    {"__KL__", [] { return __builtin_cpu_supports("kl") != 0; }},
    {"__CMPCCXADD__", [] { return __builtin_cpu_supports("cmpccxadd") != 0; }},
    {"__CLZERO__", [] { return __builtin_cpu_supports("clzero") != 0; }},
    {"__CLDEMOTE__", [] { return __builtin_cpu_supports("cldemote") != 0; }},
    {"__SM4__", [] { return __builtin_cpu_supports("sm4") != 0; }},
    {"__SM3__", [] { return __builtin_cpu_supports("sm3") != 0; }},
    {"__SHA512__", [] { return __builtin_cpu_supports("sha512") != 0; }},
    {"__PREFETCHI__", [] { return __builtin_cpu_supports("prefetchi") != 0; }},
    {"__AVXVNNIINT16__", [] { return __builtin_cpu_supports("avxvnniint16") != 0; }},
    {"__AVX512FP16__", [] { return __builtin_cpu_supports("avx512fp16") != 0; }},
    {"__TSXLDTRK__", [] { return __builtin_cpu_supports("tsxldtrk") != 0; }},
    {"__XOP__", [] { return __builtin_cpu_supports("xop") != 0; }},
    {"__LWP__", [] { return __builtin_cpu_supports("lwp") != 0; }},
    {"__FMA4__", [] { return __builtin_cpu_supports("fma4") != 0; }},
    {"__TBM__", [] { return __builtin_cpu_supports("tbm") != 0; }},
    {"__AVX512VP2INTERSECT__", [] { return __builtin_cpu_supports("avx512vp2intersect") != 0; }},
    {"__APX_F__", [] { return __builtin_cpu_supports("apxf") != 0; }},
    {"__USERMSR__", [] { return __builtin_cpu_supports("usermsr") != 0; }},
};

// ASKED OF THE PROCESSOR ITSELF, because no compiler's builtin can take them everywhere.
const Askable kAskedDirectly[] = {
    // "lahf_lm" is taken by clang with only a warning, and answered no: CPUID 0x80000001, ECX bit 0.
    {"__LAHF_SAHF__", [] { unsigned a = 0, b = 0, c = 0, d = 0;
                           return __get_cpuid(0x80000001u, &a, &b, &c, &d) != 0 && (c & 1u) != 0; }},
    // CMPXCHG16B: the compiler says so as __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16, which is not
    // shaped like the others and was missed until the review. CPUID 1, ECX bit 13.
    {"__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16", [] { unsigned a = 0, b = 0, c = 0, d = 0;
                                                 return __get_cpuid(1, &a, &b, &c, &d) != 0 && (c & (1u << 13)) != 0; }},
    {"__AVX10_1__", [] { return avx10_version() >= 1; }},
    {"__AVX10_2__", [] { return avx10_version() >= 2; }},
};

// IMPLIED BY ONE ABOVE, and not askable by themselves: CRC32 is SSE4.2's; APX's eight
// parts come together as apxf; AVX10's 512-bit forms are AVX10 itself; g++'s EVEX
// markers say only that AVX-512's encoding may be used, which AVX-512F is.
const char *const kImplied[][2] = {
    {"__CRC32__", "__SSE4_2__"},     {"__ZU__", "__APX_F__"},       {"__PUSH2POP2__", "__APX_F__"},
    {"__PPX__", "__APX_F__"},        {"__NF__", "__APX_F__"},       {"__NDD__", "__APX_F__"},
    {"__JMPABS__", "__APX_F__"},     {"__EGPR__", "__APX_F__"},     {"__CCMP__", "__APX_F__"},
    {"__AVX10_1_512__", "__AVX10_1__"}, {"__AVX10_2_512__", "__AVX10_2__"},
    {"__EVEX256__", "__AVX512F__"},  {"__EVEX512__", "__AVX512F__"},
};

// NOT NEEDED TO RUN: system instructions no compiler writes from ordinary code -- they
// exist to be called by name, by an operating system or a program that asks for them --
// and which firmware and hypervisors are the ones most often to switch off (SGX, TSX, key
// locker, protection keys, CET). They only break a tie (best_of).
const char *const kSystemOnly[] = {
    "__SGX__", "__PCONFIG__", "__TSXLDTRK__", "__KL__", "__WIDEKL__", "__UINTR__", "__ENQCMD__",
    "__HRESET__", "__PKU__", "__SHSTK__", "__PTWRITE__", "__WAITPKG__", "__MOVDIRI__", "__MOVDIR64B__",
    "__SERIALIZE__", "__CLDEMOTE__", "__WBNOINVD__", "__INVPCID__", "__RDPRU__", "__LWP__", "__MWAITX__",
    "__CLZERO__", "__USERMSR__", "__USER_MSR__", "__PREFETCHI__",
};

// NOT NEEDED TO RUN EITHER: AMX's tile instructions and MOVRS, which a compiler writes only
// when a program calls them by name -- and satl's source calls none: no <immintrin.h>, no
// _mm_ or __builtin_ia32_ anywhere in satellite/, satellite-numbers/ or strings/ (checked
// 2026-09-23). The newest of them are also ones no compiler before 2026 can ask about.
const char *const kByNameOnly[] = {
    "__AMX_TILE__", "__AMX_INT8__", "__AMX_BF16__", "__AMX_FP16__", "__AMX_COMPLEX__", "__AMX_FP8__",
    "__AMX_MOVRS__", "__AMX_AVX512__", "__MOVRS__",
};

enum class Answer { has, lacks, unaskable, not_needed };

const Askable *question_for(const std::string &macro)
{
    for (const Askable &feature : kAskedDirectly)
        if (macro == feature.macro) return &feature;
    for (const Askable &feature : kAskable)
        if (macro == feature.macro) return &feature;
    return nullptr;
}

bool one_of(const std::string &macro, const char *const *list, std::size_t count)
{
    for (std::size_t n = 0; n < count; ++n)
        if (macro == list[n]) return true;
    return false;
}

Answer ask(const std::string &macro)
{
    if (one_of(macro, kSystemOnly, sizeof kSystemOnly / sizeof *kSystemOnly) ||
        one_of(macro, kByNameOnly, sizeof kByNameOnly / sizeof *kByNameOnly))
        return Answer::not_needed;
    std::string asked = macro;
    for (const auto &pair : kImplied)
        if (macro == pair[0]) asked = pair[1];
    const Askable *question = question_for(asked);
    if (question == nullptr) return Answer::unaskable;
    return question->has() ? Answer::has : Answer::lacks;
}

struct Build {
    std::string name;
    std::size_t needs = 0;           // every line of its list
    std::size_t asked = 0;           // the ones that decide whether it runs, all there
    std::size_t system_missing = 0;  // system-only ones this machine lacks: a worse match
    std::size_t system_present = 0;
    std::vector<std::string> missing;
    bool has_satl = true;
};

// A LIST, READ AND ASKED. An empty one is refused: no processor's build needs nothing --
// that is the ordinary build -- so an empty list is one that was never written whole
// (the review, 2026-09-23: a compiler that failed left one, and it ran "anywhere").
Build read_needs(const std::string &path, const std::string &name)
{
    Build build;
    build.name = name;
    std::ifstream list(path);
    for (std::string macro; list >> macro; ++build.needs) {
        const Answer answer = ask(macro);
        if (answer == Answer::has) ++build.asked;
        else if (answer == Answer::lacks) build.missing.push_back(macro);
        else if (answer == Answer::unaskable) build.missing.push_back(macro + " (satl-cpu-level cannot ask about it)");
        else if (const Askable *question = question_for(macro))
            ++(question->has() ? build.system_present : build.system_missing);
    }
    if (build.needs == 0) build.missing.push_back("an empty list (it was never written whole)");
    return build;
}

std::vector<Build> builds_in(const std::string &folder)
{
    std::vector<Build> found;
    DIR *listing = opendir(folder.c_str());
    if (listing == nullptr) return found;
    while (const dirent *entry = readdir(listing)) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        const std::string needs = folder + "/" + name + "/needs";
        if (access(needs.c_str(), R_OK) != 0) continue;     // not a finished build: no list
        Build build = read_needs(needs, name);
        build.has_satl = access((folder + "/" + name + "/satl").c_str(), X_OK) == 0;
        found.push_back(build);
    }
    closedir(listing);
    std::sort(found.begin(), found.end(), [](const Build &a, const Build &b) { return a.name < b.name; });
    return found;
}

// THE BEST RUNNABLE BUILD: the one whose deciding list is longest -- the richest
// instruction set the machine allows. Two alike are told apart by the system instructions
// they were built for: fewer this machine lacks, then more it has -- so a Panther Lake gets
// the pantherlake build and not clearwaterforest's, which differs from it only by three it
// lacks (the review, 2026-09-23). Then by name, so a folder's order never decides.
const Build *best_of(const std::vector<Build> &builds)
{
    const Build *best = nullptr;
    for (const Build &build : builds) {
        if (!build.missing.empty() || !build.has_satl) continue;
        if (best == nullptr || build.asked > best->asked ||
            (build.asked == best->asked && (build.system_missing < best->system_missing ||
                                            (build.system_missing == best->system_missing &&
                                             build.system_present > best->system_present))))
            best = &build;
    }
    return best;
}

std::string beside_this_program()
{
    char path[4096];
    const ssize_t length = readlink("/proc/self/exe", path, sizeof path - 1);
    if (length <= 0) return "cpu";
    const std::string self(path, static_cast<std::size_t>(length));
    return self.substr(0, self.rfind('/')) + "/cpu";
}

} // namespace

int main(int argc, char **argv)
{
    __builtin_cpu_init();
    bool explain = false;
    std::string folder, runs;
    for (int at = 1; at < argc; ++at) {
        if (std::strcmp(argv[at], "--explain") == 0) explain = true;
        else if (std::strcmp(argv[at], "--runs") == 0 && at + 1 < argc) runs = argv[++at];
        else if (argv[at][0] == '-') {
            std::fprintf(stderr, "satl-cpu-level: %s is not an option -- --explain, --runs <needs file>, "
                                 "or a folder of builds\n", argv[at]);
            return 2;
        } else folder = argv[at];
    }

    if (!runs.empty()) {
        if (access(runs.c_str(), R_OK) != 0) {
            std::fprintf(stderr, "satl-cpu-level: %s cannot be read\n", runs.c_str());
            return 2;
        }
        const Build build = read_needs(runs, runs);
        for (const std::string &why : build.missing) std::printf("%s\n", why.c_str());
        return build.missing.empty() ? 0 : 1;
    }

    if (folder.empty()) folder = beside_this_program();
    const std::vector<Build> builds = builds_in(folder);
    const Build *best = best_of(builds);
    if (!explain) {
        std::printf("%s\n", best != nullptr ? best->name.c_str() : "baseline");
        return 0;
    }
    std::printf("%zu builds in %s\n\n", builds.size(), folder.c_str());
    for (const Build &build : builds) {
        std::printf("  %-18s needs %3zu  ", build.name.c_str(), build.needs);
        if (!build.has_satl) std::printf("no satl beside its list\n");
        else if (build.missing.empty()) std::printf("runs here%s\n", &build == best ? "   <- the best" : "");
        else {
            std::printf("lacks %zu:", build.missing.size());
            for (std::size_t n = 0; n < build.missing.size() && n < 4; ++n) std::printf(" %s", build.missing[n].c_str());
            std::printf("%s\n", build.missing.size() > 4 ? " ..." : "");
        }
    }
    std::printf("\n%s\n", best != nullptr ? ("this machine runs " + best->name + " best").c_str()
                                          : "no build here runs on this machine: the baseline satl does");
    return 0;
}
