#pragma once
// satellite-numbers/word_table.hpp -- THE WORDS BUILT INTO satl, BY NUMBER (the author, 2026-10-07:
// "We were supposed to have built the satellite-numbers directly into the interpreter, so they are
// not external ... make a table that calls a certain function for each number, and for each number,
// just copy over that source code into the function that corresponds to each number").
//
// EVERY WORD THAT WAS A .so UNDER satellite-numbers/<word>/ IS A FILE UNDER satellite-numbers/words/,
// named by its numbers (1.6.1.5.cpp), compiled into satl like any other source, and ends in ONE
// function named by the same numbers: describe_1_6_1_5. The table below is what call_number.
// satellite.cpp walks at start-up, calling each function to fill a LibraryRow exactly as
// dlsym("satellite_number_describe") was called on each library until this day -- so nothing is
// opened, nothing is found beside the binary, and satl alone is the whole language. The 33
// arguments words went the same way on 2026-10-03 (satellite/arguments/argument_words.hpp).
//
// A NEW WORD IS A NEW FILE AND A NEW ROW HERE -- and one more name in WORD_SOURCES
// (make_support/040-sources.mk), which names every file one by one. The row's name and numbers are
// checked against words/words.tsv as satl starts, so a row cannot quietly disagree with the table.

#include "number_row.hpp"

#include <cstddef>

namespace satellite004 {

namespace built_in {

signed long long int describe_1_4_2_1(LibraryRow *row);   // satellite.container.list.append
signed long long int describe_1_4_2_2(LibraryRow *row);   // satellite.container.list.size
signed long long int describe_1_4_2_3(LibraryRow *row);   // satellite.container.list.sort()
signed long long int describe_1_4_2_8(LibraryRow *row);   // satellite.container.list.contains(x)
signed long long int describe_1_4_2_9(LibraryRow *row);   // satellite.container.list.index_of(x)
signed long long int describe_1_4_2_10(LibraryRow *row);   // satellite.container.list.empty
signed long long int describe_1_4_2_11(LibraryRow *row);   // satellite.container.list.clear
signed long long int describe_1_4_2_12(LibraryRow *row);   // satellite.container.list.first
signed long long int describe_1_4_2_13(LibraryRow *row);   // satellite.container.list.last
signed long long int describe_1_4_2_14(LibraryRow *row);   // satellite.container.list.truncate(n)
signed long long int describe_1_4_2_15(LibraryRow *row);   // satellite.container.list.reserve(n)
signed long long int describe_1_4_2_16(LibraryRow *row);   // satellite.container.list.remove_first()
signed long long int describe_1_4_2_17(LibraryRow *row);   // satellite.container.list.remove_last()
signed long long int describe_1_4_2_18(LibraryRow *row);   // satellite.container.list.remove_at(n)
signed long long int describe_1_4_2_19(LibraryRow *row);   // satellite.container.list.remove(x)
signed long long int describe_1_4_2_20(LibraryRow *row);   // satellite.container.list.insert(n, x)
signed long long int describe_1_4_2_21(LibraryRow *row);   // satellite.container.list.join(separator)
signed long long int describe_1_4_2_22(LibraryRow *row);   // satellite.container.list.reverse
signed long long int describe_1_4_2_23(LibraryRow *row);   // satellite.container.list.sum
signed long long int describe_1_4_2_24(LibraryRow *row);   // satellite.container.list.max
signed long long int describe_1_4_2_25(LibraryRow *row);   // satellite.container.list.min
signed long long int describe_1_4_2_26(LibraryRow *row);   // satellite.container.list.search(pattern)
signed long long int describe_1_4_2_27(LibraryRow *row);   // satellite.container.list.by_name
signed long long int describe_1_4_2_28(LibraryRow *row);   // satellite.container.list.by_value
signed long long int describe_1_5_1(LibraryRow *row);   // satellite.console.display
signed long long int describe_1_6_1_0(LibraryRow *row);   // satellite.variable.string()
signed long long int describe_1_6_1_1(LibraryRow *row);   // satellite.variable.string.size
signed long long int describe_1_6_1_2(LibraryRow *row);   // satellite.variable.string.empty
signed long long int describe_1_6_1_3(LibraryRow *row);   // satellite.variable.string.find(x)
signed long long int describe_1_6_1_4(LibraryRow *row);   // satellite.variable.string.contains(x)
signed long long int describe_1_6_1_5(LibraryRow *row);   // satellite.variable.string.substring(start, end)
signed long long int describe_1_6_1_6(LibraryRow *row);   // satellite.variable.string.starts_with(x)
signed long long int describe_1_6_1_7(LibraryRow *row);   // satellite.variable.string.ends_with(x)
signed long long int describe_1_6_1_8(LibraryRow *row);   // satellite.variable.string.lower
signed long long int describe_1_6_1_9(LibraryRow *row);   // satellite.variable.string.upper
signed long long int describe_1_6_1_10(LibraryRow *row);   // satellite.variable.string.split(separator)
signed long long int describe_1_6_1_11(LibraryRow *row);   // satellite.variable.string.trim
signed long long int describe_1_6_1_12(LibraryRow *row);   // satellite.variable.string.replace(a, b)
signed long long int describe_1_6_1_13(LibraryRow *row);   // satellite.variable.string.to_number
signed long long int describe_1_6_1_14(LibraryRow *row);   // satellite.variable.string.append(x)
signed long long int describe_1_6_1_15(LibraryRow *row);   // satellite.variable.string.clear
signed long long int describe_1_6_1_16(LibraryRow *row);   // satellite.variable.string.at(n)
signed long long int describe_1_6_1_17(LibraryRow *row);   // satellite.variable.string.resolved
signed long long int describe_1_6_1_18(LibraryRow *row);   // satellite.variable.string(x)
signed long long int describe_1_6_1_19(LibraryRow *row);   // satellite.variable.string.string
signed long long int describe_1_6_1_20(LibraryRow *row);   // satellite.variable.string.number
signed long long int describe_1_6_1_21(LibraryRow *row);   // satellite.variable.string.binary
signed long long int describe_1_6_1_22(LibraryRow *row);   // satellite.variable.string.hex
signed long long int describe_1_6_1_23(LibraryRow *row);   // satellite.variable.string.reverse
signed long long int describe_1_6_1_24(LibraryRow *row);   // satellite.variable.string.foreground(color)
signed long long int describe_1_6_1_25(LibraryRow *row);   // satellite.variable.string.background(color)
signed long long int describe_1_6_1_26(LibraryRow *row);   // satellite.variable.string.add(x)
signed long long int describe_1_18_1(LibraryRow *row);   // satellite.directory.change(d)
signed long long int describe_1_18_4(LibraryRow *row);   // satellite.directory.list()
signed long long int describe_1_18_5(LibraryRow *row);   // satellite.directory.list(d)
signed long long int describe_1_18_6(LibraryRow *row);   // satellite.directory.system()
signed long long int describe_1_25(LibraryRow *row);   // satellite.feedback
signed long long int describe_1_25_1(LibraryRow *row);   // satellite.feedback(x)

} // namespace built_in

struct BuiltInWord {
    const char *numbers;         // "1 6 1 5", as words/words.tsv writes it
    const char *name;            // "satellite.variable.string.substring(start, end)"
    DescribeFunction describe;   // fills the row, as the library's one export did
};

extern const BuiltInWord kBuiltInWords[];
extern const std::size_t kBuiltInWordCount;

} // namespace satellite004
