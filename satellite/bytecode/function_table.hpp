#pragma once
// satellite/bytecode/function_table.hpp -- a word's 16-bit code straight to the
// library that runs it.
//
// (the author, 2026-09-16) "We have to match the 16-bit codes starting at 4096
// with the correct function call."
//
// THE CODE IS THE INDEX, and that is what the 4096 range is for. `table[code]`
// is one subtraction and one load: no string compare, no hash, no search.
// NumberIndex::find() walks 364 rows comparing names, which is right at start-up
// and wrong once a program is running -- call_number.hpp says so itself: found
// "before a program runs -- never while it runs". This is how that promise is
// kept now that a word is a code.
//
// 4096 pointers is 32 KB, filled once at start-up and read-only afterwards, so
// any number of threads may dispatch through it at once without a lock.
//
// A NULL ROW IS NOT AN ERROR HERE. A word can be numbered and have no library
// built yet -- PROGRESS §1 lists several -- and answering nullptr lets the
// caller say `not_built_yet` (machine code 14) with the word's own name, which
// is what 003 did and what a person can act on.

#include "../machine/machine_state.hpp"
#include "../../satellite-numbers/call_number.hpp"
#include "token_codes.hpp"
#include "word_codes.hpp"

#include <vector>

namespace satellite004 {

class FunctionTable {
public:
    // Fills the table from the loaded index. Answers success; reports, in debug
    // mode, how many words have a library and how many are still owed one.
    signed long long int build(const NumberIndex &index, MachineState &state);

    // The library for a word code, or nullptr -- for a code outside the word
    // range, a word this build has no library for, or before build() ran.
    //
    // A WORD CALLED BY ITS NAME NEVER SEES A LIST'S METHOD (2026-10-10). A list's method
    // has a number and a row since that day, holding only its list_call -- which runs on a
    // list, through run_list_number and method() below, and never by the word's name. So
    // `satellite.container.list.append(3)` written as a call still finds nothing here, and is
    // refused before anything runs as "has no library built for it yet", as it was before the
    // row existed (the list step's review). Every type numbered after the list is the same
    // (its rows hold a method_call). A string's methods had rows before that day, and keep the
    // answer they had.
    const NumberRow *operator[](token::Code code) const
    {
        if (!word::is_word_code(code)) return nullptr;
        const unsigned int slot = static_cast<unsigned int>(code) - word::kBase;
        return slot < rows_.size() ? rows_[slot] : nullptr;
    }

    // THE ROW A METHOD'S NUMBER RUNS, whatever it holds -- what run_string_number,
    // run_list_number and every other type's run_<type>_number call through. One subtraction
    // and one load, as above.
    const NumberRow *method(token::Code code) const
    {
        if (!word::is_word_code(code)) return nullptr;
        const unsigned int slot = static_cast<unsigned int>(code) - word::kBase;
        return slot < every_row_.size() ? every_row_[slot] : nullptr;
    }

    unsigned long long int filled() const { return filled_; }

private:
    std::vector<const NumberRow *> rows_;        // a word called by its name
    std::vector<const NumberRow *> every_row_;   // every row, every type's methods' included
    unsigned long long int filled_ = 0;
};

} // namespace satellite004
