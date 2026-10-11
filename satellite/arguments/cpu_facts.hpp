#pragma once
// satellite/arguments/cpu_facts.hpp -- what the processor can run: two rows of the
// arguments, gathered at start-up (arguments.cpp).
//
//     arguments.cpu.architecture   "haswell"
//     arguments.cpu.features       {"mmx", "sse", ... "avx", "avx2", "fma", ... "x86-64-v3"}
//
// The author, 2026-09-23: "there's no argument that has "haswell" like,
// arguments.cpu.architecture = "haswell" then there's no argument that displays what
// features are on this machine, like arguments.cpu.features = AVX, AVX2, 512-bit stuff,
// all that in a single list ... we should keep a list of features, as we enable all
// haswell features".
//
// THE ARCHITECTURE IS 003's WORD FOR IT (old_versions/second_satellite/src/programs/
// cpu_level.cpp): "haswell" when the processor runs the whole x86-64-v3 set -- AVX, AVX2,
// BMI1, BMI2, F16C, FMA, LZCNT, MOVBE, the level Haswell brought in 2013 and every later
// Intel and every Zen has -- and "baseline" when it does not, or is not x86 at all. It is
// the name of the satl build this machine can run, which is the question 003's
// satl-cpu-level answered, and not a guess at the chip's code name.
//
// THE FEATURES ARE ASKED OF THE PROCESSOR AND THE KERNEL TOGETHER. __builtin_cpu_supports
// answers yes for AVX or AVX-512 only when the kernel has switched its registers on, so a
// feature on this list is one a program can actually use; /proc/cpuinfo's flags line
// (105 words here) also names what the kernel turned off, and a hundred things that are
// not instructions (fpu, vme, tsc ...). In the order they arrived, then the x86-64 levels
// the machine reaches. Measured 2026-09-23 on the Xeon E5-2670 v3 (Haswell-EP): 23
// features, through x86-64-v3, no AVX-512.
//
// Every name here compiles with clang 24 and g++ 17, the two compilers the tree is built
// with (checked one by one, 2026-09-23); a name a compiler does not know is a build error,
// never a wrong answer.

#include <string>
#include <vector>

namespace satellite004 {

inline std::vector<std::string> cpu_features()
{
    std::vector<std::string> has;
#if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    // __builtin_cpu_supports takes a string LITERAL, so the table is spelled out as calls.
#define SATL_CPU_FEATURE(name) if (__builtin_cpu_supports(name)) has.emplace_back(name)
    SATL_CPU_FEATURE("cmov"); SATL_CPU_FEATURE("mmx"); SATL_CPU_FEATURE("sse"); SATL_CPU_FEATURE("sse2");
    SATL_CPU_FEATURE("sse3"); SATL_CPU_FEATURE("ssse3"); SATL_CPU_FEATURE("sse4.1"); SATL_CPU_FEATURE("sse4.2");
    SATL_CPU_FEATURE("sse4a"); SATL_CPU_FEATURE("popcnt"); SATL_CPU_FEATURE("aes"); SATL_CPU_FEATURE("pclmul");
    SATL_CPU_FEATURE("avx"); SATL_CPU_FEATURE("f16c"); SATL_CPU_FEATURE("rdrnd"); SATL_CPU_FEATURE("fma");
    SATL_CPU_FEATURE("fma4"); SATL_CPU_FEATURE("xop"); SATL_CPU_FEATURE("bmi"); SATL_CPU_FEATURE("bmi2");
    SATL_CPU_FEATURE("avx2"); SATL_CPU_FEATURE("lzcnt"); SATL_CPU_FEATURE("movbe"); SATL_CPU_FEATURE("adx");
    SATL_CPU_FEATURE("rdseed"); SATL_CPU_FEATURE("sha");
    SATL_CPU_FEATURE("avx512f"); SATL_CPU_FEATURE("avx512dq"); SATL_CPU_FEATURE("avx512cd");
    SATL_CPU_FEATURE("avx512bw"); SATL_CPU_FEATURE("avx512vl"); SATL_CPU_FEATURE("avx512ifma");
    SATL_CPU_FEATURE("avx512vbmi"); SATL_CPU_FEATURE("avx512vnni"); SATL_CPU_FEATURE("avx512vbmi2");
    SATL_CPU_FEATURE("avx512bitalg"); SATL_CPU_FEATURE("avx512vpopcntdq"); SATL_CPU_FEATURE("avx512bf16");
    SATL_CPU_FEATURE("avx512fp16"); SATL_CPU_FEATURE("avx512vp2intersect");
    SATL_CPU_FEATURE("vaes"); SATL_CPU_FEATURE("vpclmulqdq"); SATL_CPU_FEATURE("gfni"); SATL_CPU_FEATURE("avxvnni");
    SATL_CPU_FEATURE("amx-tile"); SATL_CPU_FEATURE("amx-int8"); SATL_CPU_FEATURE("amx-bf16");
    SATL_CPU_FEATURE("x86-64"); SATL_CPU_FEATURE("x86-64-v2"); SATL_CPU_FEATURE("x86-64-v3");
    SATL_CPU_FEATURE("x86-64-v4");
#undef SATL_CPU_FEATURE
#endif
    return has;
}

inline std::string cpu_architecture()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    return __builtin_cpu_supports("x86-64-v3") ? "haswell" : "baseline";
#else
    return "baseline";
#endif
}

} // namespace satellite004
