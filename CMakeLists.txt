cmake_minimum_required(VERSION 3.20)
project(satellite CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_library(satellite_core src/container.cpp src/lexer.cpp)
target_include_directories(satellite_core PUBLIC include)

enable_testing()

add_executable(test_container tests/test_container.cpp)
target_link_libraries(test_container satellite_core)
add_test(NAME container COMMAND test_container)

add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer satellite_core)
add_test(NAME lexer COMMAND test_lexer)

add_executable(bench_dispatch bench/bench_dispatch.cpp)
target_link_libraries(bench_dispatch satellite_core)
