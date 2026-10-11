// The vector number index -- the author's call_number.satellite.cpp.
//
// Their first version built the vector with 1,000,000 push_backs of empty
// maps "to hold enough space". reserve() gives the same room without making a
// million empty maps; here it reserves exactly the number of words there are.
//
// EVERY WORD IS BUILT INTO satl (the author, 2026-10-07: "We were supposed to have built the
// satellite-numbers directly into the interpreter, so they are not external"): the 33 arguments
// words through satellite/arguments/argument_words.hpp since 2026-10-03, and since this day the
// thirty that were .so files beside the binary, through satellite-numbers/word_table.hpp -- a
// table that calls one function a number, each holding the code its library held. Nothing is
// opened: no folder is read, nothing is dlopened, and satl alone is the whole language.

#include "call_number.hpp"
#include "word_table.hpp"

#include "../satellite/arguments/argument_words.hpp"
#include "../satellite/bytecode/word_codes.hpp"
#include "../satellite/machine/machine_codes.hpp"
#include "../satellite/machine/machine_state.hpp"

#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

namespace satellite004 {

std::string numbers_text(const std::vector<unsigned long long int> &numbers)
{
    std::string text;
    for (size_t i = 0; i < numbers.size(); i++) {
        if (i > 0)
            text += ' ';
        text += std::to_string(numbers[i]);
    }
    return text;
}

namespace {

// THE NUMBERS words/words.tsv GIVES A SPELLING, or empty when it has no such word.
std::vector<unsigned long long int> numbers_in_the_word_table(const std::string &spelling)
{
    const token::Code code = word::code_of_spelling(spelling);
    unsigned int depth = 0;
    const int *numbers = code != 0 ? word::numbers_of(code, depth) : nullptr;
    if (numbers == nullptr || depth == 0)
        return {};
    return std::vector<unsigned long long int>(numbers, numbers + depth);
}

} // namespace

signed long long int NumberIndex::load(MachineState &state)
{
    state.set("vector.number.index(loading)", success);

    rows_.clear();
    rows_.reserve(std::size(argument_words::kPlaces) * std::size(argument_words::kWords) + kBuiltInWordCount);

    // THE ARGUMENTS WORDS FIRST (the author, 2026-10-03: "build each of the 33 arguments .so
    // file's into the interpreter"; satellite/arguments/argument_words.hpp). Each is filed once
    // under each place the table names, with its numbers read from the word table -- so a row
    // cannot disagree with words.tsv.
    for (const char *place : argument_words::kPlaces) {
        for (const argument_words::BuiltInWord &built : argument_words::kWords) {
            const std::string spelling = std::string(place) + "." + built.key;
            NumberRow row;
            row.name = spelling;
            row.numbers = numbers_in_the_word_table(spelling);
            if (row.numbers.empty())
                return report_error("vector.number.index(error): " + spelling +
                                        " is built into satl, and words/words.tsv has no such word",
                                    vector_loading_error);
            row.scenarios = built.scenarios;
            row.file = "(built into satl)";
            rows_.push_back(std::move(row));
            state.set("vector.number.index(built in " + rows_.back().name + " " +
                          numbers_text(rows_.back().numbers) + ")",
                      success);
        }
    }

    // THE TABLE'S WORDS (word_table.hpp): each function is called to fill a LibraryRow, exactly as
    // a library's one export was called once it was dlopened -- and what it says is held against
    // the table's own row and against words/words.tsv, so a file, the table and the word list
    // cannot quietly disagree: a satl built from sources that do refuses to start, by name.
    for (std::size_t i = 0; i < kBuiltInWordCount; i++) {
        const BuiltInWord &word = kBuiltInWords[i];
        LibraryRow described;
        if (word.describe == nullptr || word.describe(&described) != success || described.name == nullptr ||
            described.depth == 0 || described.depth > kMaxDepth)
            return report_error(std::string("vector.number.index(error): ") + word.name + " (" + word.numbers +
                                    ") described itself wrongly",
                                vector_loading_error);

        NumberRow row;
        row.name = described.name;
        row.numbers.assign(described.numbers, described.numbers + described.depth);
        row.scenarios = described.scenarios;
        row.file = "(built into satl)";

        if (row.name != word.name || numbers_text(row.numbers) != word.numbers)
            return report_error("vector.number.index(error): word_table.cpp says " + std::string(word.name) + " " +
                                    word.numbers + ", and its function describes " + row.name + " " +
                                    numbers_text(row.numbers),
                                vector_loading_error);
        const std::vector<unsigned long long int> listed = numbers_in_the_word_table(row.name);
        if (listed != row.numbers)
            return report_error("vector.number.index(error): " + row.name + " is built into satl as " +
                                    numbers_text(row.numbers) + ", and words/words.tsv " +
                                    (listed.empty() ? std::string("has no such word")
                                                    : "numbers it " + numbers_text(listed)),
                                vector_loading_error);
        if (find(row.name) != nullptr)
            return report_error("vector.number.index(error): " + row.name + " is in the table twice",
                                vector_loading_error);
        for (const NumberRow &other : rows_)
            if (other.numbers == row.numbers)
                return report_error("vector.number.index(error): " + row.name + " and " + other.name +
                                        " both claim " + numbers_text(row.numbers),
                                    vector_loading_error);

        rows_.push_back(std::move(row));
        state.set("vector.number.index(built in " + rows_.back().name + " " +
                      numbers_text(rows_.back().numbers) + ")",
                  success);
    }

    return state.set("vector.number.index(defined)", number_vector_defined);
}

const NumberRow *NumberIndex::find(const std::string &name) const
{
    for (const NumberRow &row : rows_)
        if (row.name == name)
            return &row;
    return nullptr;
}

} // namespace satellite004
