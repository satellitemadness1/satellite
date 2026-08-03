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
  src/container.cpp
  src/ops.cpp
  src/source.cpp
  src/lexer.cpp
  src/machine.cpp
  src/cxx.cpp
  src/interpret.cpp)
target_compile_definitions(satellite_core PRIVATE
  SATELLITE_INCLUDE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/include"
  SATELLITE_CORE_LIB="${CMAKE_CURRENT_BINARY_DIR}/libsatellite_core.so")
target_link_libraries(satellite_core PUBLIC dl)
target_include_directories(satellite_core PUBLIC include)

add_executable(satellite src/main.cpp)
target_link_libraries(satellite satellite_core)

# The GUI console is optional: GTK4 is not required to build satellite itself,
# so a machine without it still gets the library, the CLI and the tests.
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(GTK4 gtk4)
endif()
if(GTK4_FOUND)
  add_executable(satellite-gui src/gui.cpp)
  target_link_libraries(satellite-gui satellite_core ${GTK4_LIBRARIES})
  target_include_directories(satellite-gui PRIVATE ${GTK4_INCLUDE_DIRS})
  target_link_directories(satellite-gui PRIVATE ${GTK4_LIBRARY_DIRS})
  target_compile_options(satellite-gui PRIVATE ${GTK4_CFLAGS_OTHER})
  message(STATUS "GTK4 ${GTK4_VERSION} found -- building satellite-gui")
else()
  message(STATUS "GTK4 not found -- skipping satellite-gui")
endif()

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

add_executable(bench_source bench/bench_source.cpp)

add_executable(bench_dispatch bench/bench_dispatch.cpp)
target_link_libraries(bench_dispatch satellite_core)
