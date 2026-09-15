#pragma once

// Reading a gzip stream, and the only place in satellite that names zlib.
//
// AN OPAQUE HANDLE AND NOT A `gzFile`, WHICH IS THE WHOLE REASON THIS FILE
// EXISTS. If `satellite_file/file_handle.hpp` declared a `gzFile` it would have
// to include <zlib.h>, and then every translation unit that touches a file
// handle -- the methods, the reading, the handlers, and every test binary that
// links any of them -- would need `-isystem vendor/zlib-develop` on its compile
// line. One wrapper costs one explicit rule in 060-compile.mk instead of a
// vendored header spreading through the tree. satellite_random/random.cpp draws
// the same line around PCG for the same reason.
//
// READING ONLY, TODAY. Writing a `.gz` is `gzwrite` and is the obvious next
// twin; it is absent because nothing asks for it yet, and a wrapper with an
// unused half is a wrapper whose unused half is wrong by the time somebody
// needs it.

#include <cstddef>

namespace satellite::file::gzip {

// Is what this descriptor holds a gzip stream? Answers from the CONTENT and
// never from the name.
//
// THE MAGIC TWO BYTES, WHICH IS WHAT ZLIB ITSELF LOOKS AT -- zlib.h:1388 says
// gzopen detects the format "by looking for the magic two-byte gzip header".
// So the question is answered the same way the library would answer it, and
// asking it here rather than letting zlib decide is what keeps a plain file on
// the pread path it has had since M19.
//
// AND NOT THE FILE EXTENSION, WHICH IS THE WHOLE POINT. A `.gz` that is not
// gzip would be inflated into nonsense and a gzip stream not named `.gz` would
// be read as bytes -- and Common Crawl's segments are full of the second. A
// name is a claim; the first two bytes are the fact.
//
// READS WITHOUT MOVING ANYTHING. pread(2) at offset 0, so the shared file
// offset the write side owns is untouched -- file_reading.cpp's cursor note is
// the same argument.
bool looks_gzipped(int descriptor);

// Take over a descriptor and read it as gzip. Answers nullptr if zlib could
// not, in which case the descriptor is still the caller's to close.
//
// THE DESCRIPTOR IS ADOPTED ON SUCCESS -- `gzdopen`'s contract, and it is the
// thing most worth stating here because it decides who calls close(2). After a
// non-null answer the descriptor belongs to zlib and `close_stream` releases
// it; closing it as well would close a number the OS had already reissued,
// which is the race file_handle.hpp's atomics exist to prevent one level up.
void *open_for_reading(int descriptor);

// Bytes into `into`, 0 at the end of the stream, -1 on a decompression error.
//
// MULTI-MEMBER GZIP IS HANDLED AND IT IS NOT A DETAIL. A Common Crawl WARC is
// one gzip member PER RECORD, concatenated -- so a reader that stopped at the
// first member's trailer would decompress one record of a 900MB file, report
// success, and be believed. zlib's reader crosses member boundaries on its own;
// this note is here because the failure looks exactly like working.
long read_some(void *stream, char *into, std::size_t many);

// What went wrong, in zlib's words, or nullptr. Used to fill the sentence a
// refusal prints rather than leaving errno to describe a decompression fault it
// knows nothing about.
const char *error_of(void *stream);

// Release the stream and the descriptor it adopted.
void close_stream(void *stream);

} // namespace satellite::file::gzip
