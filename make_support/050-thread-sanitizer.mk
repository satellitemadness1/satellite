# satellite -- ThreadSanitizer, and finding a compiler that can actually link it.
#
# TSAN_TEST names a path under $(TESTS), which 070-directories.mk sets below
# this file. That is safe and deliberate: it is a recursively expanded `=`, so
# nothing here is measured until a rule asks for it.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# ThreadSanitizer is the one part of the suite that is not portable: it is a
# 64-bit-only runtime, and Debian builds libtsan2 for amd64, arm64, mips64el,
# ppc64el, riscv64 and s390x and for nothing else, so on i386 or armhf
# -fsanitize=thread does not compile at all. TSAN=0 drops that one binary and
# still runs the other ten, which is what a package build on such an
# architecture needs: both binary packages are Architecture: any, and that is a
# promise the package builds everywhere (Debian Policy 5.6.8).
TSAN      ?= 1
TSAN_TEST  = $(if $(filter-out 0,$(TSAN)),$(TESTS)/library_test/library_test_tsan)

# ThreadSanitizer needs a runtime the COMPILER supplies, and $(CXX) is not
# guaranteed to have one. The clang-24 build -- which $(LLVM_BIN) named until
# 2026-08-24 -- is built WITHOUT compiler-rt: `clang++ -print-runtime-dir`
# answers "(runtime dir is not present)" and the link dies on a missing
# libclang_rt.tsan.a, which took the whole of `make test` down with it. That is
# a property of that build of clang and not of this machine -- /usr/bin/clang++
# (21.1.8) ships the runtime, and so does g++ as libtsan.
#
# $(LLVM_BIN) now names clang-24-2, which DOES ship it, so on this machine the
# fallback no longer fires and TSAN_CXX resolves to $(CXX). None of the
# machinery below changes, and it must not: it is not scaffolding for one
# broken install, it is what keeps `make test` alive on any machine whose
# compiler cannot supply the runtime -- which this one could not until today,
# and a packaging machine still may not be able to.
#
# So the sanitized binary gets its own compiler. The default asks $(CXX)
# whether it can supply either runtime and uses it if it can; otherwise it
# falls back, clang first because the instrumentation and the runtime then come
# from the same project. Every source in this binary is compiled by the SAME
# compiler as the runtime it links, which is the one thing that must not be
# mixed.
#
# Recursively expanded and referenced only in the library_test_tsan recipe, so
# the two `-print-file-name` subprocesses run when that binary is built and
# never on a `make satl`. `origin` rather than ?= so a command-line or
# environment TSAN_CXX still wins.
#
# The test is a LINK and not a file lookup, and the first attempt at this got it
# wrong in a way worth recording: asking clang for -print-file-name=libtsan.so
# answers /usr/lib/gcc/x86_64-redhat-linux/14/libtsan.so, because clang searches
# GCC's install directories -- a 38-byte linker script for a runtime clang would
# never link against, since clang's -fsanitize=thread wants libclang_rt.tsan.a.
# The lookup therefore said yes for the one compiler that cannot do it. Linking
# an empty main is the only probe that answers the question actually being asked.
#
# **Verified** on this machine on 2026-08-24, by running the probe below by hand
# against all four candidates: yes for $(LLVM_BIN)/clang++ (clang-24-2), NO for
# the old clang-24 build, yes for /usr/bin/clang++ and yes for c++. So TSAN_CXX
# resolves to $(CXX) after the move and resolved to /usr/bin/clang++ before it,
# and the one thing that must not be mixed -- instrumentation and runtime from
# one project -- now holds without the fallback having to arrange it.
#
# The binary the /usr/bin/clang++ fallback built PASSED: 160,000 increments
# intact, 403,921 lock-free reads, 165 variables in satellite.library.
ifeq ($(origin TSAN_CXX),undefined)
  TSAN_OK   = $(shell printf 'int main(){}' | $(1) -fsanitize=thread -x c++ - \
                          -o /dev/null >/dev/null 2>&1 && echo ok)
  TSAN_CXX  = $(strip $(if $(call TSAN_OK,$(CXX)),$(CXX),\
                  $(if $(call TSAN_OK,/usr/bin/clang++),/usr/bin/clang++,c++)))
endif
