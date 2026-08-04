# Embedding the C++ standard library headers in the binary.
#
# A satellite.cxx block does #include <iostream>, and clang has to find it.
# Reading it off the machine means satellite is only as portable as the
# toolchain installed there -- so the headers travel inside the executable.
#
# Three steps, all at build time:
#   1. ask clang which files a broad set of standard headers actually reaches
#   2. pack exactly those into one blob
#   3. turn the blob into an object file and link it in
#
# Step 1 is what keeps this small.  /usr/include is 351 MB on this machine,
# almost all of it dev packages a C++ block will never touch; the reachable
# closure of 46 common headers is about 7 MB.  We embed what is reachable, not
# what is installed.

function(satellite_build_header_blob out_object)
  set(gen "${CMAKE_CURRENT_BINARY_DIR}/headerblob")
  file(MAKE_DIRECTORY "${gen}")

  # ---- 1. the probe -------------------------------------------------------
  # Every header worth having available without the user asking.  Anything not
  # listed still works when the real filesystem has it; it just is not embedded.
  set(probe "${gen}/probe.cpp")
  set(HEADERS
      algorithm any array atomic bitset charconv chrono cmath codecvt complex
      condition_variable cstdio cstdlib cstring deque exception filesystem
      format forward_list fstream functional future initializer_list iomanip
      ios iosfwd iostream istream iterator limits list locale map memory
      mutex new numeric optional ostream queue random ranges ratio regex set
      shared_mutex span sstream stack stdexcept streambuf string string_view
      system_error thread tuple type_traits typeindex typeinfo unordered_map
      unordered_set utility valarray variant vector)
  set(probe_text "// generated -- see cmake/HeaderBlob.cmake\n")
  foreach(h ${HEADERS})
    string(APPEND probe_text "#include <${h}>\n")
  endforeach()
  # satellite's own headers are what a block needs to speak Container at all.
  string(APPEND probe_text "#include \"satellite/cxx.hpp\"\n")
  file(WRITE "${probe}" "${probe_text}")

  # ---- 2. the reachable set ----------------------------------------------
  # Asked of the SAME clang, with the SAME flags, that will compile blocks --
  # a list gathered from a different toolchain would embed headers that do not
  # match the ones the JIT's driver goes looking for.
  set(deps "${gen}/deps.txt")
  add_custom_command(
    OUTPUT "${deps}"
    COMMAND "${SATELLITE_LLVM_ROOT}/bin/clang++"
            -std=c++20 -M -MF "${gen}/raw.d"
            -resource-dir "${CLANG_RESOURCE_DIR}"
            -I "${SATELLITE_RUNTIME_INCLUDE_DIR}"
            "${probe}"
    COMMAND ${CMAKE_COMMAND}
            -DRAW=${gen}/raw.d -DOUT=${deps}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DepsToList.cmake"
    DEPENDS "${probe}"
    COMMENT "Scanning which headers a satellite.cxx block can reach")

  # ---- 3. pack, and make it an object -------------------------------------
  set(blob "${gen}/headers.blob")
  add_custom_command(
    OUTPUT "${blob}"
    COMMAND pack_headers "${deps}" "${blob}"
    DEPENDS "${deps}" pack_headers
    COMMENT "Packing the embedded header blob")

  # ld names the symbols after the path it is GIVEN, so this runs in the blob's
  # own directory with a bare filename.  Anything else and the symbols come out
  # as _binary_home_jsadlier_..._headers_blob_start.
  set(obj "${gen}/headers_blob.o")
  add_custom_command(
    OUTPUT "${obj}"
    COMMAND ${CMAKE_LINKER} -r -b binary -o "${obj}" "headers.blob"
    WORKING_DIRECTORY "${gen}"
    DEPENDS "${blob}"
    COMMENT "Linking the header blob into an object file")

  add_custom_target(satellite_header_blob DEPENDS "${obj}")
  set(${out_object} "${obj}" PARENT_SCOPE)
endfunction()
