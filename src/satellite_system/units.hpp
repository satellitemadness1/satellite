#pragma once

// The unit a memory answer is reported in -- "b", "kb", "mb", "gb", "tb".
//
// A FILE OF ITS OWN BECAUSE THREE SUBJECTS READ IT. The machine's memory, the
// swap file and this process's own stack all report in a unit, they are written
// in different files, and a unit table copied into each of them is three places
// for "gb" to come to mean different things. v1 had it as a lambda inside
// call_module and its own comment says the lambda had already been moved once
// for the same reason.
//
// EVERY DIVISOR IS A POWER OF 1024, AND THAT IS WHY M20 NEEDS NO FLOAT. 1024 is
// 2^10, 2 divides 10, so bytes divided by any of these is a decimal that
// TERMINATES -- there is no repeating tail for a digit count to cut off and
// nothing for M15's rounding rule to decide. Measured 2026-09-11 rather than
// asserted: `1 / 1099511627776` answers 9.094947017729282379150390625e-13 in
// satellite, which is Python's exact Decimal to the last digit, with
// `division_digits` sitting at its default 34 -- because that dial is read by
// "a division that does not end" and none of these ever is.
//
// THE UNIT IS A STRING AND NOT A WORD, which is v1's decision kept whole: a
// bare `tb` is a name the USER owns (DESIGN §1), so `memory.used(tb)` asks for
// a variable called `tb` and is rightly told there is none. Making it mean a
// unit instead would cost a second reserved word, and not spending those is
// most of what this language is. `satellite.file.open` settled the shape with
// "read" and "write": the word says at the call site what it is.

#include <string_view>

namespace satellite::system {

// How many bytes the unit names, or 0 when it names nothing.
//
// 0 IS THE REFUSAL AND NOT A UNIT, which is safe because no unit is zero bytes
// wide -- so a caller writes `if (!divisor) refuse` and needs no second
// out-parameter to tell "b" from "no such word".
//
// CASE IS NOT SIGNIFICANT: "KB", "Kb" and "kb" are one word. A unit is a unit
// however it is typed, and refusing "MB" would be refusing a spelling rather
// than a mistake.
unsigned long long unit_divisor(std::string_view unit);

// The unit a call uses when it is given none.
//
// MEGABYTES, WHICH IS v1'S DEFAULT AND THE UNIT PEOPLE ACTUALLY SPEAK IN about
// a machine's memory. It is named here rather than spelled "mb" at each call
// site so that the handlers and the S1301 message cannot disagree about it.
inline constexpr std::string_view kDefaultUnit = "mb";

} // namespace satellite::system
