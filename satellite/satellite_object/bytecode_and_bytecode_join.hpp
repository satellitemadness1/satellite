#pragma once
// satellite/satellite_object/bytecode_and_bytecode_join.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// bytecode + bytecode -> bytecode. ONE OF THE FAST PATHS THAT ONLY EXISTS BECAUSE
// THE BYTECODE ARM IS IN THE VARIANT (the author, 2026-09-16: 'keep the bytecode
// in... it gives you more options for fast path functions'). He was right: with
// bytecode outside the object there is nowhere for this to live.
//
// WHAT IT IS FOR. QUAD writes satellite. A program that builds a program joins
// runs of codes, and doing it here -- as codes -- means never going back out to
// text and re-lexing. CANNOT REFUSE.

#include "satellite_bytecode.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int bytecode_and_bytecode_join(const satellite_bytecode &left,
                                                       const satellite_bytecode &right,
                                                       satellite_bytecode &out)
{
    out.codes.clear();
    out.codes.reserve(left.size() + right.size());
    out.codes.insert(out.codes.end(), left.codes.begin(), left.codes.end());
    out.codes.insert(out.codes.end(), right.codes.begin(), right.codes.end());
    return success;
}

} // namespace satellite004
