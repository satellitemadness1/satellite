#pragma once
// satellite/satellite_variable_string/string_overwrite.hpp -- size a string and
// write it through a pointer, without first filling it with zeros.
//
// WHY: std::basic_string::resize writes a zero into every new character, so a
// decoder that is about to write every one of them pays twice -- and on a fresh
// allocation the zeros also fault in every page, used or not (measured
// 2026-09-15: 162 ms against 147 ms encoding 100 MB, the same loop). C++23's
// resize_and_overwrite skips the zeros. libstdc++, g++'s, gives C++20 the same thing as
// __resize_and_overwrite; libc++ -- clang's library here since 2026-10-22 -- gives it to
// C++23 alone, and satl stays C++20 for now (make_support/010-compiler.mk says why), so
// under libc++ it falls back to resize, as anywhere else: correct, only slower.
//
// USED BY satellite_string.cpp AND by plain_utf16.hpp, the race's plain C++, so
// the race never compares a trick against code denied it.
//
// write(pointer, most) may write up to `most` characters and answers how many
// it wrote; that becomes the string's size. The characters the string already
// held, up to `most`, are still there when write starts. write must not touch
// the string any other way.

#include <cstddef>
#include <string>
#include <version>

namespace satellite004 {

template <typename Character, typename Write>
void overwrite_string(std::basic_string<Character> &text, std::size_t most, Write write)
{
#if defined(__cpp_lib_string_resize_and_overwrite)
    text.resize_and_overwrite(most, [&](Character *pointer, std::size_t room) { return write(pointer, room); });
#elif defined(__GLIBCXX__) && _GLIBCXX_USE_CXX11_ABI
    text.__resize_and_overwrite(most, [&](Character *pointer, std::size_t room) { return write(pointer, room); });
#else
    text.resize(most);
    text.resize(write(text.data(), most));
#endif
}

} // namespace satellite004
