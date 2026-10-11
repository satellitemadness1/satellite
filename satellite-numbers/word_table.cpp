// satellite-numbers/word_table.cpp -- THE TABLE THAT CALLS A FUNCTION FOR EACH NUMBER (the author,
// 2026-10-07). One row a word, in the order of the numbers; word_table.hpp says the rest.

#include "word_table.hpp"

namespace satellite004 {

using namespace built_in;

const BuiltInWord kBuiltInWords[] = {
    {"1 4 2 1", "satellite.container.list.append", describe_1_4_2_1},
    {"1 4 2 2", "satellite.container.list.size", describe_1_4_2_2},
    {"1 4 2 3", "satellite.container.list.sort()", describe_1_4_2_3},
    {"1 4 2 8", "satellite.container.list.contains(x)", describe_1_4_2_8},
    {"1 4 2 9", "satellite.container.list.index_of(x)", describe_1_4_2_9},
    {"1 4 2 10", "satellite.container.list.empty", describe_1_4_2_10},
    {"1 4 2 11", "satellite.container.list.clear", describe_1_4_2_11},
    {"1 4 2 12", "satellite.container.list.first", describe_1_4_2_12},
    {"1 4 2 13", "satellite.container.list.last", describe_1_4_2_13},
    {"1 4 2 14", "satellite.container.list.truncate(n)", describe_1_4_2_14},
    {"1 4 2 15", "satellite.container.list.reserve(n)", describe_1_4_2_15},
    {"1 4 2 16", "satellite.container.list.remove_first()", describe_1_4_2_16},
    {"1 4 2 17", "satellite.container.list.remove_last()", describe_1_4_2_17},
    {"1 4 2 18", "satellite.container.list.remove_at(n)", describe_1_4_2_18},
    {"1 4 2 19", "satellite.container.list.remove(x)", describe_1_4_2_19},
    {"1 4 2 20", "satellite.container.list.insert(n, x)", describe_1_4_2_20},
    {"1 4 2 21", "satellite.container.list.join(separator)", describe_1_4_2_21},
    {"1 4 2 22", "satellite.container.list.reverse", describe_1_4_2_22},
    {"1 4 2 23", "satellite.container.list.sum", describe_1_4_2_23},
    {"1 4 2 24", "satellite.container.list.max", describe_1_4_2_24},
    {"1 4 2 25", "satellite.container.list.min", describe_1_4_2_25},
    {"1 4 2 26", "satellite.container.list.search(pattern)", describe_1_4_2_26},
    {"1 4 2 27", "satellite.container.list.by_name", describe_1_4_2_27},
    {"1 4 2 28", "satellite.container.list.by_value", describe_1_4_2_28},
    {"1 5 1", "satellite.console.display", describe_1_5_1},
    {"1 6 1 0", "satellite.variable.string()", describe_1_6_1_0},
    {"1 6 1 1", "satellite.variable.string.size", describe_1_6_1_1},
    {"1 6 1 2", "satellite.variable.string.empty", describe_1_6_1_2},
    {"1 6 1 3", "satellite.variable.string.find(x)", describe_1_6_1_3},
    {"1 6 1 4", "satellite.variable.string.contains(x)", describe_1_6_1_4},
    {"1 6 1 5", "satellite.variable.string.substring(start, end)", describe_1_6_1_5},
    {"1 6 1 6", "satellite.variable.string.starts_with(x)", describe_1_6_1_6},
    {"1 6 1 7", "satellite.variable.string.ends_with(x)", describe_1_6_1_7},
    {"1 6 1 8", "satellite.variable.string.lower", describe_1_6_1_8},
    {"1 6 1 9", "satellite.variable.string.upper", describe_1_6_1_9},
    {"1 6 1 10", "satellite.variable.string.split(separator)", describe_1_6_1_10},
    {"1 6 1 11", "satellite.variable.string.trim", describe_1_6_1_11},
    {"1 6 1 12", "satellite.variable.string.replace(a, b)", describe_1_6_1_12},
    {"1 6 1 13", "satellite.variable.string.to_number", describe_1_6_1_13},
    {"1 6 1 14", "satellite.variable.string.append(x)", describe_1_6_1_14},
    {"1 6 1 15", "satellite.variable.string.clear", describe_1_6_1_15},
    {"1 6 1 16", "satellite.variable.string.at(n)", describe_1_6_1_16},
    {"1 6 1 17", "satellite.variable.string.resolved", describe_1_6_1_17},
    {"1 6 1 18", "satellite.variable.string(x)", describe_1_6_1_18},
    {"1 6 1 19", "satellite.variable.string.string", describe_1_6_1_19},
    {"1 6 1 20", "satellite.variable.string.number", describe_1_6_1_20},
    {"1 6 1 21", "satellite.variable.string.binary", describe_1_6_1_21},
    {"1 6 1 22", "satellite.variable.string.hex", describe_1_6_1_22},
    {"1 6 1 23", "satellite.variable.string.reverse", describe_1_6_1_23},
    {"1 6 1 24", "satellite.variable.string.foreground(color)", describe_1_6_1_24},
    {"1 6 1 25", "satellite.variable.string.background(color)", describe_1_6_1_25},
    {"1 6 1 26", "satellite.variable.string.add(x)", describe_1_6_1_26},
    {"1 18 1", "satellite.directory.change(d)", describe_1_18_1},
    {"1 18 4", "satellite.directory.list()", describe_1_18_4},
    {"1 18 5", "satellite.directory.list(d)", describe_1_18_5},
    {"1 18 6", "satellite.directory.system()", describe_1_18_6},
    {"1 25", "satellite.feedback", describe_1_25},
    {"1 25 1", "satellite.feedback(x)", describe_1_25_1},
};

const std::size_t kBuiltInWordCount = sizeof kBuiltInWords / sizeof kBuiltInWords[0];

} // namespace satellite004
