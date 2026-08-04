cmake_minimum_required(VERSION 3.20)
project(satellite CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# SHARED so that satellite.cxx modules can link against it -- a static host
# would need -rdynamic and would still export symbols unpredictably.
add_library(satellite_core SHARED
  src/charmap.cpp
  src/string.cpp
  src/bignum.cpp
  src/bigdiv.cpp
  src/container.cpp
  src/ops.cpp
  src/source.cpp
  src/library.cpp
  src/capsule.cpp
  src/lexer.cpp
  src/machine.cpp
  src/cxx.cpp
  src/cxx_args.cpp
  src/interpret.cpp)
# satellite.cxx blocks are compiled at runtime, so the headers and the core .so
# have to be findable *then*, not just at build time. These default to the source
# and build trees (what a developer wants); a redistributable bundle overrides
# them to point inside itself. See .ship/001/verify.sh.
set(SATELLITE_RUNTIME_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE STRING "Directory holding satellite/*.hpp when a cxx block is compiled")
set(SATELLITE_RUNTIME_CORE_LIB "${CMAKE_CURRENT_BINARY_DIR}/libsatellite_core.so"
    CACHE STRING "libsatellite_core.so that cxx modules link against at runtime")

target_compile_definitions(satellite_core PRIVATE
  SATELLITE_INCLUDE_DIR="${SATELLITE_RUNTIME_INCLUDE_DIR}"
  SATELLITE_CORE_LIB="${SATELLITE_RUNTIME_CORE_LIB}")
find_package(Threads REQUIRED)
target_link_libraries(satellite_core PUBLIC dl Threads::Threads)
target_include_directories(satellite_core PUBLIC include)

# The GUI console is optional: GTK4 is not required to build satellite itself,
# so a machine without it still gets the library, the CLI and the tests.
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(GTK4 gtk4)
endif()
if(GTK4_FOUND)
  message(STATUS "GTK4 ${GTK4_VERSION} found -- satellite includes the console")
else()
  message(STATUS "GTK4 not found -- satellite is command-line only")
endif()

# ---------------------------------------------------------------------------
# The embedded C++ compiler.
#
# satellite.cxx blocks currently fork g++ and dlopen the result.  With clang and
# LLVM linked in, that becomes an in-process JIT: no fork, no temp file.  This
# needs STATIC LLVM archives -- the AlmaLinux clang-devel RPM ships only
# libclang-cpp.so and no libclang*.a, so a distro install cannot produce a
# single file.  Point SATELLITE_LLVM_ROOT at a from-source build to get one.
#
# Without it everything still builds; src/jit.cpp compiles to a stub that
# reports the compiler unavailable.  See docs/SINGLE_FILE.md.
# ---------------------------------------------------------------------------
set(SATELLITE_LLVM_ROOT "$ENV{HOME}/source-opt/llvm_build"
    CACHE PATH "A from-source LLVM/clang build with static archives")

set(SATELLITE_HAVE_CLANG OFF)
if(EXISTS "${SATELLITE_LLVM_ROOT}/bin/llvm-config"
   AND EXISTS "${SATELLITE_LLVM_ROOT}/lib/libclangCodeGen.a")
  set(SATELLITE_HAVE_CLANG ON)
  execute_process(COMMAND "${SATELLITE_LLVM_ROOT}/bin/llvm-config" --includedir
                  OUTPUT_VARIABLE LLVM_INCDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "${SATELLITE_LLVM_ROOT}/bin/llvm-config" --libdir
                  OUTPUT_VARIABLE LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "${SATELLITE_LLVM_ROOT}/bin/llvm-config" --libs all
                  OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND "${SATELLITE_LLVM_ROOT}/bin/llvm-config" --system-libs
                  OUTPUT_VARIABLE LLVM_SYSLIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_LIBS    UNIX_COMMAND "${LLVM_LIBS}")
  separate_arguments(LLVM_SYSLIBS UNIX_COMMAND "${LLVM_SYSLIBS}")
  # Every clang archive, in one --start-group so their mutual references resolve
  # regardless of order.  Naming them individually is a losing game: LLVM splits
  # libraries out between releases (clangOptions and clangAnalysisLifetimeSafety
  # both appeared this way), and each one costs a full static link to discover.
  file(GLOB CLANG_ARCHIVES "${LLVM_LIBDIR}/libclang*.a")
  list(LENGTH CLANG_ARCHIVES CLANG_ARCHIVE_COUNT)
  message(STATUS "Embedded clang: ${SATELLITE_LLVM_ROOT} "
                 "(${CLANG_ARCHIVE_COUNT} clang archives)")
else()
  message(STATUS "No static LLVM at ${SATELLITE_LLVM_ROOT} -- "
                 "building without an embedded compiler")
endif()

# ---------------------------------------------------------------------------
# satellite -- one binary, both faces.  No arguments opens the GTK4 console;
# `satellite check foo.satl` stays a command-line tool.  That is what keeps
# .ship/001/verify.sh and every README example working.
# ---------------------------------------------------------------------------
if(GTK4_FOUND)
  add_executable(satellite src/main.cpp src/gui.cpp src/jit.cpp)
  target_link_libraries(satellite ${GTK4_LIBRARIES})
  target_include_directories(satellite PRIVATE ${GTK4_INCLUDE_DIRS})
  target_link_directories(satellite PRIVATE ${GTK4_LIBRARY_DIRS})
  target_compile_options(satellite PRIVATE ${GTK4_CFLAGS_OTHER})
else()
  add_executable(satellite src/main.cpp src/gui_none.cpp src/jit.cpp)
endif()
target_link_libraries(satellite satellite_core)

if(SATELLITE_HAVE_CLANG)
  # jit.cpp is the only file that sees clang's headers, and it needs -fno-rtti
  # to match how the LLVM archives were built.  Confining both to one source
  # file is why include/satellite/jit.hpp deliberately leaks no LLVM types.
  # clang finds its builtin headers (stddef.h, stdarg.h) relative to its own
  # executable.  Embedded, that executable is satellite, so the path has to be
  # baked in.  A single-file build will serve these from the header blob instead.
  execute_process(COMMAND "${SATELLITE_LLVM_ROOT}/bin/clang" -print-resource-dir
                  OUTPUT_VARIABLE CLANG_RESOURCE_DIR
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  set_source_files_properties(src/jit.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-rtti"
    INCLUDE_DIRECTORIES "${LLVM_INCDIR}"
    COMPILE_DEFINITIONS "SATELLITE_HAVE_CLANG;SATELLITE_CLANG_RESOURCE_DIR=\"${CLANG_RESOURCE_DIR}\"")
  # The -L must come first and must point at the from-source build.  Without
  # it the linker happily resolves half these symbols against the system
  # LLVM 21 .so and fails on the half that changed between versions -- which
  # reads like a missing library rather than the version mismatch it is.
  target_link_directories(satellite BEFORE PRIVATE "${LLVM_LIBDIR}")
  # clang and LLVM reference each other in both directions, so they go in ONE
  # group rather than two.  A group per project only resolves one way.
  target_link_libraries(satellite
    -Wl,--start-group ${CLANG_ARCHIVES} ${LLVM_LIBS} -Wl,--end-group
    ${LLVM_SYSLIBS} z zstd)
endif()

# The shipped tree always carries the binary that was just built, so it can
# never quietly go stale against the source it claims to match.
set(SATELLITE_SHIP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.ship/001/bin")
add_custom_command(TARGET satellite POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SATELLITE_SHIP_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:satellite>" "${SATELLITE_SHIP_DIR}/satellite"
  COMMENT "Copying satellite to .ship/001/bin/satellite")

enable_testing()

add_executable(test_container tests/test_container.cpp)
target_link_libraries(test_container satellite_core)
add_test(NAME container COMMAND test_container)

add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer satellite_core)
add_test(NAME lexer COMMAND test_lexer)

add_executable(test_machine tests/test_machine.cpp)
target_link_libraries(test_machine satellite_core)
add_test(NAME machine COMMAND test_machine)

add_executable(test_cxx tests/test_cxx.cpp)
target_link_libraries(test_cxx satellite_core)
add_test(NAME cxx COMMAND test_cxx)

add_executable(test_interpret tests/test_interpret.cpp)
target_link_libraries(test_interpret satellite_core)
add_test(NAME interpret COMMAND test_interpret)

add_executable(test_source tests/test_source.cpp)
target_link_libraries(test_source satellite_core)
add_test(NAME source COMMAND test_source)

add_executable(test_library tests/test_library.cpp)
target_link_libraries(test_library satellite_core)
add_test(NAME library COMMAND test_library)

add_executable(test_capsule tests/test_capsule.cpp)
target_link_libraries(test_capsule satellite_core)
add_test(NAME capsule COMMAND test_capsule)

add_executable(test_capsule_value tests/test_capsule_value.cpp)
target_link_libraries(test_capsule_value satellite_core)
add_test(NAME capsule_value COMMAND test_capsule_value)

add_executable(bench_source bench/bench_source.cpp)

add_executable(bench_dispatch bench/bench_dispatch.cpp)
target_link_libraries(bench_dispatch satellite_core)

add_executable(test_bignum tests/test_bignum.cpp)
target_link_libraries(test_bignum satellite_core)
add_test(NAME bignum COMMAND test_bignum)
