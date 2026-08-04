# Turn `clang -M` output into one absolute path per line.
#
# A .d file is a make rule: a target, a colon, then the prerequisites separated
# by spaces with backslash-newline continuations.  This strips it back to the
# file list that pack_headers wants.
#
# Run with -DRAW=<the .d file> -DOUT=<the list to write>.

file(READ "${RAW}" text)

# Line continuations first, or a path at the end of a line keeps its backslash.
string(REGEX REPLACE "\\\\\r?\n" " " text "${text}")
string(REGEX REPLACE "\r?\n" " " text "${text}")

# Drop everything up to and including the first colon -- that is the target.
string(FIND "${text}" ":" colon)
if(colon GREATER -1)
  math(EXPR after "${colon} + 1")
  string(SUBSTRING "${text}" ${after} -1 text)
endif()

string(REPLACE " " ";" items "${text}")
set(paths "")
foreach(p ${items})
  string(STRIP "${p}" p)
  # Absolute paths only.  The probe itself is relative and is not a header.
  if(p MATCHES "^/")
    list(APPEND paths "${p}")
  endif()
endforeach()

list(REMOVE_DUPLICATES paths)
list(SORT paths)
string(REPLACE ";" "\n" out "${paths}")
file(WRITE "${OUT}" "${out}\n")

list(LENGTH paths n)
message(STATUS "Header closure: ${n} files")
